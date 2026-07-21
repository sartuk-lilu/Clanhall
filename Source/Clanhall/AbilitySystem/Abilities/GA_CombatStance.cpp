#include "GA_CombatStance.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "ClanhallCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_CombatStance::UGA_CombatStance()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationOwnedTags.AddTag(ClanhallGameplayTags::State_InStance.GetTag());
	// Нельзя войти в стойку второй раз, пока уже в ней — TryActivateAbility просто откажет.
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_InStance.GetTag());
}

void UGA_CombatStance::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (AClanhallCharacter* Character = Cast<AClanhallCharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
	}
}
