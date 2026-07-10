// Центральный паттерн проекта (CLAUDE.md: "Главный паттерн: DataAsset + Fragments").
// Заголовок — поля, которые есть у каждой абилки без исключения. Fragments — только то,
// что нужно конкретной. Логика GameplayAbility не меняется при правке этого ассета —
// меняется только сам DataAsset (development_plan.md).

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Fragments/AbilityFragment.h"
#include "AbilityData.generated.h"

class UTexture2D;

UCLASS()
class CLANHALL_API UAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Ability")
	FText DisplayName;

	UPROPERTY(EditAnywhere, Category = "Ability")
	TObjectPtr<UTexture2D> Icon;

	/** КД в секундах. Канон (development_plan.md): по тиру навыка — для прототипа Knight Q/E/R/F → 10/10/20/20. */
	UPROPERTY(EditAnywhere, Category = "Ability")
	float Cooldown = 10.0f;

	/** Тег слота КД — Cooldown.Slot.Q/E/R/F/... (ability_system.md §3).
	 *  КД принадлежит СЛОТУ, общему для всех оружий. Не путать с Ability.Skill.* (идентичность навыка). */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Cooldown.Slot"))
	FGameplayTag CooldownTag;

	/** Тег требуемого класса — Ability.Class.Knight и т.д. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Ability.Class"))
	FGameplayTag RequiredClass;

	/** Идентичность навыка для контрнавыка — Ability.Skill.Knight.PowerStrike и т.д.
	 *  Тот же тег на навыке врага = «тот же навык» → контр (clanhall_claude_code_counter.md). */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Ability.Skill"))
	FGameplayTag CounterTag;

	/** Стоимость в Charges. Канон: Q/E=0, R/F=2, Z/X=4, C/V=6 (combat_system.md §1). */
	UPROPERTY(EditAnywhere, Category = "Ability")
	int32 ChargeCost = 0;

	UPROPERTY(EditAnywhere, Instanced, Category = "Ability")
	TArray<TObjectPtr<UAbilityFragment>> Fragments;

	/** Возвращает первый фрагмент типа T, или nullptr если у этого навыка такого нет. */
	template <typename T>
	T* FindFragment() const
	{
		for (const TObjectPtr<UAbilityFragment>& Fragment : Fragments)
		{
			if (T* Match = Cast<T>(Fragment.Get()))
			{
				return Match;
			}
		}
		return nullptr;
	}
};
