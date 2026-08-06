#include "GE_Stunned.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Stunned::UGE_Stunned()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
}

void UGE_Stunned::PostInitProperties()
{
	Super::PostInitProperties();

	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(StunDuration));

	UTargetTagsGameplayEffectComponent& TagComponent = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(ClanhallGameplayTags::State_Stunned.GetTag());
	TagComponent.SetAndApplyTargetTagChanges(TagChanges);
}
