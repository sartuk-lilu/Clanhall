#include "WeaponTypeData.h"

bool UWeaponTypeData::IsTierUnlocked(int32 Tier, const FGameplayTagContainer& Perks) const
{
	if (Tier <= 0 || Tier > ProficiencyTagsByRank.Num())
	{
		return false;
	}

	return Perks.HasTag(ProficiencyTagsByRank[Tier - 1]);
}
