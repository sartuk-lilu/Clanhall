#include "AbilitySystem/AbilityData.h"
#include "AbilitySystem/Fragments/GameplayFragments.h"
#include "AbilitySystem/MarkData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "ClanhallAbilityData"

EDataValidationResult UAbilityData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	const USkillLevelsFragment* Levels = FindFragment<USkillLevelsFragment>();
	if (!Levels)
	{
		return Result;
	}

	TSet<int32> Seen;
	bool bHasLevelOne = false;
	FGameplayTag LevelOnePlaces;
	for (const FClanhallSkillLevel& Level : Levels->Levels)
	{
		if (Seen.Contains(Level.Level))
		{
			Context.AddError(FText::Format(LOCTEXT("DuplicateLevel", "Skill level {0} is listed twice"), Level.Level));
			Result = EDataValidationResult::Invalid;
		}
		Seen.Add(Level.Level);

		if (Level.Level <= 1)
		{
			bHasLevelOne = true;
			LevelOnePlaces = Level.Places ? Level.Places->MarkTag : FGameplayTag();
		}
		else if (!Level.Reads.IsValid())
		{
			// Уровень навыка читает ровно одну метку (`MarksSystem.md`, «Уровни навыка»).
			Context.AddError(FText::Format(LOCTEXT("MissingReads", "Skill level {0} has no mark to read"), Level.Level));
			Result = EDataValidationResult::Invalid;
		}
	}

	if (!bHasLevelOne)
	{
		Context.AddError(LOCTEXT("NoLevelOne", "Skill levels have no level 1: the skill has nothing to fall back to"));
		Result = EDataValidationResult::Invalid;
	}

	for (const FClanhallSkillLevel& Level : Levels->Levels)
	{
		if (Level.Level == 2 && LevelOnePlaces.IsValid() && Level.Reads == LevelOnePlaces)
		{
			Context.AddWarning(LOCTEXT("ReadsOwnMark", "Level 2 reads the mark this skill places on level 1 - open design question, the skill would climb on itself"));
		}
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE
#endif
