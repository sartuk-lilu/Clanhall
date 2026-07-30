#include "GA_EnemyActiveSkill.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/ClanhallCounterComponent.h"
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
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UClanhallCounterComponent* CounterComp = Avatar ? Avatar->FindComponentByClass<UClanhallCounterComponent>() : nullptr;
	if (!SelfASC || !CounterComp)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

#if !UE_BUILD_SHIPPING
	GEngine->AddOnScreenDebugMessage(-1, CounterWindowDuration + 0.2f, FColor::Orange,
		FString::Printf(TEXT("⚡ ENEMY SKILL — сбить навыком из CounteredBy, попав в окно! (%.1f сек)"), CounterWindowDuration));
#endif

	// Открыть окно контрнавыка на своём UClanhallCounterComponent (interim: код, не AnimNotifyState —
	// реальных монтажей у врага пока нет). Игрок контрит, активировав любой навык из набора CounteredBy.
	CounterComp->OpenWindow(CounteredBy, Handle, CooldownTag, Cooldown, CounterStunDuration);

	// Таймер удара. Если ConsumeCounter вызвал CancelAbilityHandle извне — WaitDelay-задача снимается,
	// OnHitDelayExpired не вызовется, урон не применяется.
	UAbilityTask_WaitDelay* HitTask = UAbilityTask_WaitDelay::WaitDelay(this, CounterWindowDuration);
	HitTask->OnFinish.AddDynamic(this, &UGA_EnemyActiveSkill::OnHitDelayExpired);
	HitTask->ReadyForActivation();
}

void UGA_EnemyActiveSkill::OnHitDelayExpired()
{
	// Этот метод срабатывает только если контрнавык НЕ прервал способность.
	UAbilitySystemComponent* SelfASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	AActor* SelfAvatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;

	// Окно не было прервано контром — закрываем его сами (иначе State.CounterWindow повиснет навсегда).
	if (UClanhallCounterComponent* CounterComp = SelfAvatar ? SelfAvatar->FindComponentByClass<UClanhallCounterComponent>() : nullptr)
	{
		CounterComp->CloseWindow();
	}

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
				// SelfASC — источник врага. Метку, наложенную врагом, игрок снять не может
				// никак — только переждать 5 сек (mark_system.md §3).
				MarkComp->ApplyMark(HitMarkTag, SelfASC);
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
	// Тот же тег, что у Knight E (Power Strike) — как AbilityTags (идентичность способности для GAS),
	// так и запись в CounteredBy (набор контрящих навыков) намеренно совпадают — самоконтр.
	SetAssetTags(FGameplayTagContainer(ClanhallGameplayTags::Ability_Skill_Knight_PowerStrike.GetTag()));
	CounteredBy.AddTag(ClanhallGameplayTags::Ability_Skill_Knight_PowerStrike.GetTag());

	// Knight E — тир 10 сек (ability_system.md §3, CLAUDE.md "КД физнавыков").
	CooldownTag = ClanhallGameplayTags::Cooldown_Slot_E.GetTag();
	Cooldown = 10.0f;

	CounterWindowDuration = 1.2f;
	HitDamage = 40.0f;
	// HitMarkTag = Mark.BrokenGuard будет добавлен в Разделе 7 вместе с полным Часовым.
}
