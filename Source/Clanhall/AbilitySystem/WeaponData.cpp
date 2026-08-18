#include "WeaponData.h"

UWeaponFragment* UWeaponData::FindWeaponFragment(TSubclassOf<UWeaponFragment> FragmentClass) const
{
	for (const TObjectPtr<UWeaponFragment>& Fragment : Fragments)
	{
		if (Fragment && Fragment->IsA(FragmentClass))
		{
			return Fragment;
		}
	}
	return nullptr;
}
