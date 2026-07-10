#include "ComboFragment.h"

const FComboSequence* UComboFragment::FindExact(const TArray<EClanhallAttackDirection>& Path) const
{
	for (const FComboSequence& Entry : Sequences)
	{
		if (Entry.Sequence == Path)
		{
			return &Entry;
		}
	}
	return nullptr;
}