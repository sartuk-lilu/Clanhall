// Базовый класс фрагментов оружия (CLAUDE.md, "Архитектура DataAsset + Fragments") — дословный
// аналог UAbilityFragment. Отдельный базовый класс, а не переиспользование UAbilityFragment:
// фрагмент навыка и фрагмент оружия попадают в разные массивы и не должны быть
// взаимозаменяемы в выпадающем списке редактора.

#pragma once

#include "UObject/Object.h"
#include "WeaponFragment.generated.h"

UCLASS(Abstract, DefaultToInstanced, EditInlineNew)
class CLANHALL_API UWeaponFragment : public UObject
{
	GENERATED_BODY()
};
