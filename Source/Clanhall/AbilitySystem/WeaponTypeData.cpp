#include "WeaponTypeData.h"
#include "AbilitySystem/ClanhallGameplayTags.h"

int32 UWeaponTypeData::GetRequiredProficiencyRank(FGameplayTag SlotTag)
{
	if (SlotTag == ClanhallGameplayTags::Ability_Slot_Q.GetTag() || SlotTag == ClanhallGameplayTags::Ability_Slot_E.GetTag())
	{
		return 1;
	}
	if (SlotTag == ClanhallGameplayTags::Ability_Slot_R.GetTag() || SlotTag == ClanhallGameplayTags::Ability_Slot_F.GetTag())
	{
		return 2;
	}
	if (SlotTag == ClanhallGameplayTags::Ability_Slot_Z.GetTag() || SlotTag == ClanhallGameplayTags::Ability_Slot_X.GetTag())
	{
		return 3;
	}
	if (SlotTag == ClanhallGameplayTags::Ability_Slot_C.GetTag() || SlotTag == ClanhallGameplayTags::Ability_Slot_V.GetTag())
	{
		return 4;
	}
	return 0;
}

bool UWeaponTypeData::IsSlotUnlocked(FGameplayTag SlotTag, const FGameplayTagContainer& Perks) const
{
	const int32 RequiredRank = GetRequiredProficiencyRank(SlotTag);
	if (RequiredRank <= 0 || RequiredRank > ProficiencyTagsByRank.Num())
	{
		return false;
	}

	return Perks.HasTag(ProficiencyTagsByRank[RequiredRank - 1]);
}
