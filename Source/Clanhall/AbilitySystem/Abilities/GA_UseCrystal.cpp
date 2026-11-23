#include "GA_UseCrystal.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallCrystalComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "ClanhallCombatSettings.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"

UGA_UseCrystal::UGA_UseCrystal()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(ClanhallGameplayTags::Ability_Common_Crystal.GetTag());
	SetAssetTags(AssetTags);

	// Пока идёт применение, боец открыт.
	ActivationOwnedTags.AddTag(ClanhallGameplayTags::State_UsingCrystal.GetTag());

	// Общее действие: не поверх приёма, уклонения, смены оружия, насмешки, стана, парирования и смерти.
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_SkillCommitted.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_EvadeRecovery.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_WeaponSwap.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_Taunting.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_Stunned.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_TauntParried.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_TauntParrying.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_Dead.GetTag());
}

bool UGA_UseCrystal::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const UClanhallCrystalComponent* Crystal = Avatar ? Avatar->FindComponentByClass<UClanhallCrystalComponent>() : nullptr;
	return Crystal && Crystal->HasWholeCrystal();
}

void UGA_UseCrystal::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float DurationSeconds = ClanhallBeats::ToSeconds(UClanhallCombatSettings::Get().CrystalUseBeats);

	if (UseMontage)
	{
		if (ACharacter* Character = Cast<ACharacter>(Avatar))
		{
			if (UAnimInstance* AnimInst = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
			{
				const float ClipLength = UseMontage->GetPlayLength();
				const float PlayRate = (ClipLength > 0.0f && DurationSeconds > 0.0f) ? ClipLength / DurationSeconds : 1.0f;
				if (AnimInst->Montage_Play(UseMontage, PlayRate) > 0.0f)
				{
					PlayedMontage = UseMontage;
				}
			}
		}
	}

	// Стан прерывает применение. Смерть отменяет все способности сама (HandleOutOfHealth).
	UAbilityTask_WaitGameplayTagAdded* StunTask = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(this, ClanhallGameplayTags::State_Stunned.GetTag());
	StunTask->Added.AddDynamic(this, &UGA_UseCrystal::OnInterrupted);
	StunTask->ReadyForActivation();

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, DurationSeconds);
	DelayTask->OnFinish.AddDynamic(this, &UGA_UseCrystal::OnUseElapsed);
	DelayTask->ReadyForActivation();
}

void UGA_UseCrystal::OnUseElapsed()
{
	AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;
	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	UClanhallCrystalComponent* Crystal = Avatar ? Avatar->FindComponentByClass<UClanhallCrystalComponent>() : nullptr;

	// Списание и восполнение - вместе, в конце.
	if (ASC && Crystal && Crystal->SpendWhole())
	{
		const float MaxAP = ASC->GetNumericAttribute(UClanhallAttributeSet::GetMaxAPAttribute());
		ClanhallGameplayEffects::ApplyModifyEffect(ASC, ASC, UGE_ModifyAP::StaticClass(), MaxAP * UClanhallCombatSettings::Get().CrystalAPRestore);
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_UseCrystal::OnInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_UseCrystal::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
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
