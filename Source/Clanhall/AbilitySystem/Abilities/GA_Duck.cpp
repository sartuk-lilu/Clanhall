#include "GA_Duck.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallComboComponent.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"

UGA_Duck::UGA_Duck()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Общий с уходом/рывком тег хвоста восстановления — защищает класс действий от спама
	// (`combat_system.md`).
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_EvadeRecovery.GetTag());
}

bool UGA_Duck::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar)
	{
		return false;
	}

	// Удар коммитится (`combat_system.md`) — живой удар-монтаж блокирует присед, тот же
	// предикат, что у UGA_Dodge::CanActivateAbility.
	if (const UClanhallComboComponent* Combo = Avatar->FindComponentByClass<UClanhallComboComponent>())
	{
		if (Combo->IsSequenceActive())
		{
			return false;
		}
	}

	return true;
}

void UGA_Duck::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Косметика — механика не зависит от того, стартовал ли монтаж (`CLAUDE.md`, «Механика
	// работает без анимационных ассетов»).
	if (DuckMontage)
	{
		if (UAnimInstance* AnimInst = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInst->Montage_Play(DuckMontage);
		}
	}

	UAbilityTask_WaitDelay* WindupTask = UAbilityTask_WaitDelay::WaitDelay(this, DuckWindupTime);
	WindupTask->OnFinish.AddDynamic(this, &UGA_Duck::OnWindupFinished);
	WindupTask->ReadyForActivation();
}

void UGA_Duck::OnWindupFinished()
{
	// Задержка отыграна — теперь капсула обязана попасть в замах: опускаем её ровно на
	// DuckWindowDuration — присед - окно, а не удержание (`combat_system.md`).
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->Crouch();
	}

	UAbilityTask_WaitDelay* WindowTask = UAbilityTask_WaitDelay::WaitDelay(this, DuckWindowDuration);
	WindowTask->OnFinish.AddDynamic(this, &UGA_Duck::OnWindowFinished);
	WindowTask->ReadyForActivation();
}

void UGA_Duck::OnWindowFinished()
{
	if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->UnCrouch();
	}

	FinishWithRecovery();
}

void UGA_Duck::FinishWithRecovery()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAnimInstance* AnimInst = (Character && Character->GetMesh()) ? Character->GetMesh()->GetAnimInstance() : nullptr;
	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;

	// Монтаж не стартовал — лок не вешаем, невидимого лока в системе не бывает ни при каких
	// условиях (по образцу UClanhallComboComponent::EndSequenceWithRecovery).
	if (AnimInst && DuckRecoveryMontage && ASC)
	{
		const float PlayedDuration = AnimInst->Montage_Play(DuckRecoveryMontage);
		if (PlayedDuration > 0.0f)
		{
			ClanhallGameplayEffects::ApplyTimedTag(ASC, ClanhallGameplayTags::State_EvadeRecovery.GetTag(), DuckRecoveryMontage->GetPlayLength());
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
