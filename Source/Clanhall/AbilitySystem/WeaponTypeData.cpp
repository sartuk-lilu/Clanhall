#include "AbilitySystem/WeaponTypeData.h"
#include "AbilitySystem/AbilityData.h"
#include "AbilitySystem/Fragments/GameplayFragments.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "ClanhallWeaponTypeData"

EDataValidationResult UWeaponTypeData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// Валидатор приёма не знает, чьё он оружие, - правило школы проверяет класс оружия.
	if (SchoolMechanic == EClanhallSchoolMechanic::Backstab)
	{
		for (const TPair<FGameplayTag, TObjectPtr<UAbilityData>>& Entry : Skills)
		{
			const UAbilityData* Skill = Entry.Value;
			if (!Skill || !Skill->FindFragment<UDamageFragment>())
			{
				continue;
			}
			if (!Skill->bBackstab)
			{
				Context.AddWarning(FText::Format(LOCTEXT("NoBackstab", "{0}: attacking skill of a backstab school has bBackstab off"),
					FText::FromString(Skill->GetName())));
			}
			if (Skill->LethalMode == EClanhallLethalMode::Always)
			{
				Context.AddWarning(FText::Format(LOCTEXT("LethalAlways", "{0}: backstab school skill is lethal from the front too (expected From Back Only)"),
					FText::FromString(Skill->GetName())));
			}
		}
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE
#endif
