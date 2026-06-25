#include "GA_EnemyActiveSkill.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

UGA_EnemyActiveSkill::UGA_EnemyActiveSkill()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	// Не требует State.InStance (навык врага, не игрока).
	ActivationRequiredTags.RemoveTag(ClanhallGameplayTags::State_InStance.GetTag());

	// Заблокирован оглушением и лок-аутом после серии.
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_Stunned.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_ComboRecovery.GetTag());
}

void UGA_EnemyActiveSkill::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* SelfASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!SelfASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

#if !UE_BUILD_SHIPPING
	GEngine->AddOnScreenDebugMessage(-1, CounterWindowDuration + 0.2f, FColor::Orange,
		FString::Printf(TEXT("⚡ ENEMY SKILL — Ctrl+E чтобы прервать! (%.1f сек)"), CounterWindowDuration));
#endif

	// Открыть окно контрнавыка: тег на ASC врага — игрок его детектирует.
	ClanhallGameplayEffects::ApplyTimedTag(SelfASC, ClanhallGameplayTags::State_CounterWindow.GetTag(), CounterWindowDuration);

	// Таймер удара. Если CancelAbility вызван извне — WaitDelay-задача снимается,
	// OnHitDelayExpired не вызовется, урон не применяется.
	UAbilityTask_WaitDelay* HitTask = UAbilityTask_WaitDelay::WaitDelay(this, CounterWindowDuration);
	HitTask->OnFinish.AddDynamic(this, &UGA_EnemyActiveSkill::OnHitDelayExpired);
	HitTask->ReadyForActivation();
}

void UGA_EnemyActiveSkill::OnHitDelayExpired()
{
	// Этот метод срабатывает только если контрнавык НЕ прервал способность.
	UAbilitySystemComponent* SelfASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn || !SelfASC)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	IAbilitySystemInterface* PlayerInterface = Cast<IAbilitySystemInterface>(PlayerPawn);
	UAbilitySystemComponent* PlayerASC = PlayerInterface ? PlayerInterface->GetAbilitySystemComponent() : nullptr;

	if (PlayerASC)
	{
		ResolveStandardDamage(SelfASC, PlayerASC, HitDamage);

		if (HitMarkTag.IsValid())
		{
			if (UClanhallMarkComponent* MarkComp = PlayerPawn->FindComponentByClass<UClanhallMarkComponent>())
			{
				MarkComp->ApplyMark(HitMarkTag);
			}
		}
	}

#if !UE_BUILD_SHIPPING
	GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, TEXT("✗ Навык врага ПОПАЛ (не прерван)"));
#endif

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

// ---------------------------------------------------------------------------

UGA_Enemy_PowerStrike::UGA_Enemy_PowerStrike()
{
	// Тот же тег, что у Knight E (Power Strike) — контрнавык находит через CancelAbilities(Ability.Skill.*).
	AbilityTags.AddTag(ClanhallGameplayTags::Ability_Skill_Knight_PowerStrike.GetTag());

	CounterWindowDuration = 1.2f;
	HitDamage = 40.0f;
	// HitMarkTag = Mark.BrokenGuard будет добавлен в Разделе 7 вместе с полным Часовым.
}
