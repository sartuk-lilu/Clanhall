#include "GA_DirectionalAttackBase.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "AbilitySystem/ClanhallComboComponent.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "Animation/AnimNotifyState_Hitbox.h"
#include "Animation/AnimMontage.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Engine/Engine.h"

UGA_DirectionalAttackBase::UGA_DirectionalAttackBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
}

void UGA_DirectionalAttackBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;

	if (!SourceASC || !Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Сообщаем HitboxComponent направление до открытия зоны (придёт из
	// AnimNotifyState_Hitbox::NotifyBegin во время монтажа) — читается только зонами
	// с bParryable == true. Оставлено строго здесь, до открытия зоны нотифаем.
	if (ACharacter* Char = Cast<ACharacter>(Avatar))
	{
		if (UClanhallHitboxComponent* HitboxComp = Char->FindComponentByClass<UClanhallHitboxComponent>())
		{
			HitboxComp->SetCurrentDirection(GetDirection());
		}
	}

	PendingBaseDamage = TriggerEventData ? TriggerEventData->EventMagnitude : 0.0f;

	// Снимок ДО инкремента UClanhallComboComponent::StepCount (см. комментарий у bChargeEligible
	// в заголовке) — ActivateStep вызывает TriggerAbilityFromGameplayEvent, которая синхронно
	// доходит и досюда, ДО собственного `++StepCount`/`StepCount = 1` в ComboComponent. Значение
	// StepCount в этот момент — число УЖЕ завершённых до этого удара шагов: 0 на первом ударе,
	// 1+ на втором и далее.
	if (const UClanhallComboComponent* Combo = Avatar->FindComponentByClass<UClanhallComboComponent>())
	{
		bChargeEligible = Combo->GetStepCount() >= 1;
	}

	const UAnimMontage* StepMontage = TriggerEventData ? Cast<UAnimMontage>(TriggerEventData->OptionalObject.Get()) : nullptr;
	const bool bResolveOnContact = UAnimNotifyState_Hitbox::MontageHasHitbox(StepMontage);

	if (!bResolveOnContact)
	{
		// У шага нет монтажа или на монтаже не расставлены зоны — резолвим мгновенно сферой.
		// Это осознанный фолбэк, а не деградация: он и есть то, что позволяет проверять дерево
		// комбо до нарезки анимаций (`Combat Stance and WASD Attacks.md`).
		ResolveHitOn(FindMeleeTarget(Avatar));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Контактный путь: EndAbility НЕ вызывается здесь — ждём Event.Hitbox.Hit/Closed.
	// Попутный эффект (`combat_system.md`, «Боевая стойка и переключение режимов»): если зону так и не открыли (отпустили ЛКМ в фазе
	// замаха -> OnStanceExit -> ForceEndHitboxes -> Event.Hitbox.Closed без единого
	// Event.Hitbox.Hit), способность заканчивается без единого ResolveHitOn — «прерванный удар
	// не засчитывается» становится правдой в коде, а не только в доке.
	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, ClanhallGameplayTags::Event_Hitbox_Hit.GetTag(), nullptr, /*OnlyTriggerOnce*/ false, /*OnlyMatchExact*/ true);
	HitTask->EventReceived.AddDynamic(this, &UGA_DirectionalAttackBase::OnHitboxHitReceived);
	HitTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* ClosedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, ClanhallGameplayTags::Event_Hitbox_Closed.GetTag(), nullptr, /*OnlyTriggerOnce*/ true, /*OnlyMatchExact*/ true);
	ClosedTask->EventReceived.AddDynamic(this, &UGA_DirectionalAttackBase::OnHitboxClosedReceived);
	ClosedTask->ReadyForActivation();
}

void UGA_DirectionalAttackBase::OnHitboxHitReceived(FGameplayEventData Payload)
{
	// Одна зона может задеть несколько целей за взмах — способность не заканчивается здесь,
	// ждём Event.Hitbox.Closed.
	ResolveHitOn(const_cast<AActor*>(Payload.Target.Get()));
}

void UGA_DirectionalAttackBase::OnHitboxClosedReceived(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_DirectionalAttackBase::ResolveHitOn(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!SourceASC)
	{
		return;
	}

	IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(Target);
	UAbilitySystemComponent* TargetASC = TargetInterface ? TargetInterface->GetAbilitySystemComponent() : nullptr;

	if (ResolveStandardDamage(SourceASC, TargetASC, PendingBaseDamage))
	{
		// Мана с WASD-ударов не капает — она переехала на подтверждённое попадание
		// физической активки (`ability_system.md`, «Физические активные навыки»).

		// Charges — доход начиная со ВТОРОГО удара серии (bChargeEligible, снятый в
		// ActivateAbility), раз за взмах, сколько бы целей ни задело (`combat_system.md`, «Ресурсы персонажа»).
		// Симметрично для игрока и врага — ResolveHitOn общий, проверок на роль нет.
		if (!bChargeApplied && bChargeEligible)
		{
			ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, SourceASC, UGE_ModifyCharges::StaticClass(), 1.0f);
			bChargeApplied = true;
		}

#if !UE_BUILD_SHIPPING
		if (const UClanhallAttributeSet* SelfAttributes = SourceASC->GetSet<UClanhallAttributeSet>())
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, FString::Printf(
				TEXT("WASD hit | self AP %.0f/%.0f  Charges %.0f/%.0f"),
				SelfAttributes->GetAP(), SelfAttributes->GetMaxAP(),
				SelfAttributes->GetCharges(), SelfAttributes->GetMaxCharges()));
		}
#endif
	}
}
