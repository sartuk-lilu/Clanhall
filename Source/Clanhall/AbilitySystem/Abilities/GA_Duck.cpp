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

	// Общий с уходом/рывком тег хвоста восстановления - защищает класс действий от спама
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

	// Удар коммитится (`combat_system.md`) - живой удар-монтаж блокирует присед, тот же
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

	// Косметика - механика не зависит от того, стартовал ли монтаж (`CLAUDE.md`, «Механика
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
	// Задержка отыграна - теперь капсула обязана попасть в замах: опускаем её ровно на
	// DuckWindowDuration (`combat_system.md`: присед - окно, а не удержание).
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
	FinishWithRecovery();
}

void UGA_Duck::FinishWithRecovery()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAnimInstance* AnimInst = (Character && Character->GetMesh()) ? Character->GetMesh()->GetAnimInstance() : nullptr;
	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;

	// Монтаж не стартовал - лок не вешаем, невидимого лока в системе не бывает ни при каких
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

void UGA_Duck::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Безусловный подъём капсулы - единственная гарантия против залипшего приседа. Windup ещё
	// не дошёл до Crouch() -> UnCrouch() безвреден (bWantsToCrouch и так false). Прервали между
	// Crouch() и UnCrouch() (смерть, отмена, CancelAbilities, смена уровня) -> без этой строки
	// капсула осталась бы опущенной до конца сессии - тот же класс залипшего состояния, что
	// ForceEndHitboxes лечит у зон поражения (`UClanhallComboComponent::OnAttackMontageEnded`).
	if (ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr)
	{
		Character->UnCrouch();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
