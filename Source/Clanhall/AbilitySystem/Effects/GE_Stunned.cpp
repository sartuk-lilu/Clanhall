#include "GE_Stunned.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Stunned::UGE_Stunned()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	// Дефолт для native CDO; Blueprint-потомок правит через штатный Class Defaults (см. .h).
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(2.0f));

	UTargetTagsGameplayEffectComponent& TagComponent = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagChanges;
	TagChanges.AddTag(ClanhallGameplayTags::State_Stunned.GetTag());
	TagComponent.SetAndApplyTargetTagChanges(TagChanges);
}
