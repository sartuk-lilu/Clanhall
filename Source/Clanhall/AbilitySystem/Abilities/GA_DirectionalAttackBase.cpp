#include "GA_DirectionalAttackBase.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
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

	// Раздел 6.5 / Порция C: сообщаем HitboxComponent направление до открытия зоны (придёт
	// из AnimNotifyState_Hitbox::NotifyBegin во время монтажа) — читается только зонами
	// с bParryable == true. Оставлено строго здесь, до открытия зоны нотифаем.
	if (ACharacter* Char = Cast<ACharacter>(Avatar))
	{
		if (UClanhallHitboxComponent* HitboxComp = Char->FindComponentByClass<UClanhallHitboxComponent>())
		{
			HitboxComp->SetCurrentDirection(GetDirection());
		}
	}

	PendingBaseDamage = TriggerEventData ? TriggerEventData->EventMagnitude : 0.0f;

	const UAnimMontage* StepMontage = TriggerEventData ? Cast<UAnimMontage>(TriggerEventData->OptionalObject.Get()) : nullptr;
	const bool bResolveOnContact = UAnimNotifyState_Hitbox::MontageHasHitbox(StepMontage);

	if (!bResolveOnContact)
	{
		// У шага нет монтажа или на монтаже не расставлены зоны — резолвим мгновенно сферой,
		// как до Порции D. Это осознанный фолбэк, а не деградация: он и есть то, что позволяет
		// проверять дерево комбо до нарезки анимаций (инвариант Раздела 7).
		ResolveHitOn(FindMeleeTarget(Avatar));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Контактный путь: EndAbility НЕ вызывается здесь — ждём Event.Hitbox.Hit/Closed.
	// Попутный эффект (combat_system.md §3): если зону так и не открыли (отпустили ЛКМ в фазе
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
		// combat_system.md §4: STR-удар → MP +10, Balance +5..+15; DEX-удар → MP +5, Balance -5..-15.
		const bool bIsSTR = SourceASC->HasMatchingGameplayTag(ClanhallGameplayTags::Weapon_Type_STR.GetTag());
		const float MPGain = bIsSTR ? 10.0f : 5.0f;
		const float BalanceShift = GetBalanceSign(SourceASC) * FMath::FRandRange(5.0f, 15.0f);

		ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, SourceASC, UGE_ModifyMP::StaticClass(), MPGain);
		ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, SourceASC, UGE_ModifyBalance::StaticClass(), BalanceShift);

#if !UE_BUILD_SHIPPING
		if (const UClanhallAttributeSet* SelfAttributes = SourceASC->GetSet<UClanhallAttributeSet>())
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, FString::Printf(
				TEXT("WASD hit | self AP %.0f/%.0f  MP %.0f/%.0f  Balance %.1f"),
				SelfAttributes->GetAP(), SelfAttributes->GetMaxAP(),
				SelfAttributes->GetMP(), SelfAttributes->GetMaxMP(), SelfAttributes->GetBalance()));
		}
#endif
	}
}
