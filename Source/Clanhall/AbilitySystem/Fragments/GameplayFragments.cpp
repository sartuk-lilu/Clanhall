#include "AbilitySystem/Fragments/GameplayFragments.h"
#include "AbilitySystem/ClanhallMarkComponent.h"

int32 USkillLevelsFragment::ChooseLevelIndex(const UClanhallMarkComponent* Holder) const
{
	int32 Best = INDEX_NONE;
	int32 BestLevel = 0;
	for (int32 Index = 0; Index < Levels.Num(); ++Index)
	{
		const FClanhallSkillLevel& Candidate = Levels[Index];
		bool bConditionMet = false;
		if (Candidate.Level <= 1)
		{
			bConditionMet = true;
		}
		else if (Holder && Candidate.Reads.IsValid())
		{
			// Уровень N читает метку уровня N-1: уровни лежат отдельно, у каждого свой слот.
			bConditionMet = Holder->HasMark(Candidate.Level - 1, Candidate.Reads);
		}

		if (bConditionMet && Candidate.Level > BestLevel)
		{
			Best = Index;
			BestLevel = Candidate.Level;
		}
	}
	return Best;
}
