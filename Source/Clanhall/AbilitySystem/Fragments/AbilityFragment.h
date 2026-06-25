// Базовый класс фрагментов (development_plan.md, "Архитектура DataAsset + Fragments").
// DefaultToInstanced + EditInlineNew — стандартная пара спецификаторов для "массив
// полиморфных подобъектов, редактируемых прямо внутри родителя": каждый элемент массива
// Fragments в UAbilityData уникален (не shared CDO) и разворачивается в редакторе на месте,
// а не как отдельный ассет со своей ссылкой.

#pragma once

#include "UObject/Object.h"
#include "AbilityFragment.generated.h"

UCLASS(Abstract, DefaultToInstanced, EditInlineNew)
class CLANHALL_API UAbilityFragment : public UObject
{
	GENERATED_BODY()
};
