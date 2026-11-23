// Что я умею и чем владею — шаблон стартового состояния бойца. Заменяет UClassKitData:
// понятия класса больше нет, ClassTag никуда не переезжает (`WeaponSystem.md`,
// «Тип оружия ↔ лист персонажа»).
//
// Живой лист (компонент, который мутирует в бою) появится вместе с прогрессией — сейчас
// строится только шаблон, мутировать физически нечем: прогрессии в прототипе нет
// (`WeaponSystem.md`, «Шаблон и живой лист»).

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CharacterSheetData.generated.h"

class UWeaponData;

UCLASS()
class CLANHALL_API UCharacterSheetData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Стартовый набор оружия, не то, что в руках сейчас — активное оружие рантайм-состояние
	 *  бойца (`WeaponSystem.md`, «Что в лист входит, а что нет»): боец свапает оружие в бою,
	 *  шаблон описывает только то, с чем он вышел. У игрока это слоты 1–6, у противника —
	 *  набор, между которыми ходит AI. */
	UPROPERTY(EditAnywhere, Category = "CharacterSheet")
	TArray<TObjectPtr<UWeaponData>> Loadout;

	/** Навыки, которые боец видел в бою. Симметрично SeenSyllables/SeenSpells магии
	 *  (`AbilitySystem.md`, «Боевой журнал как источник «узнанного»»). Писателя пока нет —
	 *  заполняется вручную в редакторе; потребитель не спроектирован. */
	UPROPERTY(EditAnywhere, Category = "CharacterSheet", meta = (Categories = "Ability"))
	FGameplayTagContainer SeenSkills;

	/** Навыки, которые боец выучил — гейт гранта активки: на панели только выученные приёмы
	 *  класса оружия (`WeaponSystem.md`, «Класс ↔ лист персонажа»). Ключ — идентичность навыка,
	 *  UAbilityData::CounterTag. */
	UPROPERTY(EditAnywhere, Category = "CharacterSheet", meta = (Categories = "Ability"))
	FGameplayTagContainer LearnedSkills;
};
