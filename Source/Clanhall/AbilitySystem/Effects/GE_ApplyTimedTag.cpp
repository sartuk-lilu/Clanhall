#include "GE_ApplyTimedTag.h"
#include "AbilitySystem/ClanhallGameplayTags.h"

UGE_ApplyTimedTag::UGE_ApplyTimedTag()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat DurationSetByCaller;
	DurationSetByCaller.DataTag = ClanhallGameplayTags::SetByCaller_Magnitude.GetTag();
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationSetByCaller);
}
