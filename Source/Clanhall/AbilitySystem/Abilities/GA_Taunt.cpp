#include "GA_Taunt.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallSkillCooldownComponent.h"
#include "ClanhallCombatSettings.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"

UGA_Taunt::UGA_Taunt()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Тег идентичности - по нему подтверждённое попадание срывает насмешку (CancelAbilities).
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(ClanhallGameplayTags::Ability_Common_Taunt.GetTag());
	SetAssetTags(AssetTags);

	// Пока идёт насмешка, боец открыт: State.Taunting видит и расчёт урона, и насмешка-парирование.
	ActivationOwnedTags.AddTag(ClanhallGameplayTags::State_Taunting.GetTag());

	// Общее действие: не поверх приёма, уклонения, смены оружия, стана, анимации парирования и смерти.
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_SkillCommitted.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_EvadeRecovery.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_WeaponSwap.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_Stunned.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_TauntParried.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_TauntParrying.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_UsingCrystal.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_Dead.GetTag());
}

bool UGA_Taunt::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const UClanhallSkillCooldownComponent* Cooldowns = Avatar ? Avatar->FindComponentByClass<UClanhallSkillCooldownComponent>() : nullptr;
	if (Cooldowns && Cooldowns->IsOnCooldown(ClanhallGameplayTags::Ability_Common_Taunt.GetTag()))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(ClanhallGameplayTags::Denied_Cooldown.GetTag());
		}
		return false;
	}
	return true;
}

void UGA_Taunt::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const UClanhallCombatSettings& Settings = UClanhallCombatSettings::Get();
	if (UClanhallSkillCooldownComponent* Cooldowns = Avatar->FindComponentByClass<UClanhallSkillCooldownComponent>())
	{
		Cooldowns->StartCooldown(ClanhallGameplayTags::Ability_Common_Taunt.GetTag(), Settings.TauntCooldownBeats);
	}

	const float DurationSeconds = ClanhallBeats::ToSeconds(Settings.TauntBeats);

	// Косметика - механика не зависит от того, стартовал ли монтаж.
	if (TauntMontage)
	{
		if (ACharacter* Character = Cast<ACharacter>(Avatar))
		{
			if (UAnimInstance* AnimInst = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
			{
				const float ClipLength = TauntMontage->GetPlayLength();
				const float PlayRate = (ClipLength > 0.0f && DurationSeconds > 0.0f) ? ClipLength / DurationSeconds : 1.0f;
				if (AnimInst->Montage_Play(TauntMontage, PlayRate) > 0.0f)
				{
					PlayedMontage = TauntMontage;
				}
			}
		}
	}

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, DurationSeconds);
	DelayTask->OnFinish.AddDynamic(this, &UGA_Taunt::OnTauntElapsed);
	DelayTask->ReadyForActivation();
}

void UGA_Taunt::OnTauntElapsed()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Taunt::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Сорванная насмешка обрывает и свою анимацию: награды нет, анимация не доигрывает.
	if (bWasCancelled && PlayedMontage.IsValid())
	{
		if (ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr)
		{
			if (UAnimInstance* AnimInst = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
			{
				AnimInst->Montage_Stop(0.2f, PlayedMontage.Get());
			}
		}
	}
	PlayedMontage.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
