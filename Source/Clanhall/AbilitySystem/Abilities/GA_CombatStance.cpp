#include "GA_CombatStance.h"
#include "AbilitySystem/ClanhallGameplayTags.h"

UGA_CombatStance::UGA_CombatStance()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationOwnedTags.AddTag(ClanhallGameplayTags::State_InStance.GetTag());
	// Нельзя войти в стойку второй раз, пока уже в ней — TryActivateAbility просто откажет.
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_InStance.GetTag());
}
