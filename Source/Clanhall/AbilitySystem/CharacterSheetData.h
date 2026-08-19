// Что я умею и чем владею — шаблон стартового состояния бойца. Заменяет UClassKitData:
// понятия класса больше нет, ClassTag никуда не переезжает (`weapon_system.md`,
// «Тип оружия ↔ лист персонажа»).
//
// Живой лист (компонент, который мутирует в бою) появится вместе с прогрессией — сейчас
// строится только шаблон, мутировать физически нечем: статов, перков и изучения навыков
// в игре нет (`weapon_system.md`, «Шаблон и живой лист»).

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
	 *  бойца (`weapon_system.md`, «Что в лист входит, а что нет»): боец свапает оружие в бою,
	 *  шаблон описывает только то, с чем он вышел. У игрока это слоты 1–6, у противника —
	 *  набор, между которыми ходит AI. */
	UPROPERTY(EditAnywhere, Category = "CharacterSheet")
	TArray<TObjectPtr<UWeaponData>> Loadout;

	/** Стартовые значения статов. Атрибутов STR/DEX в UClanhallAttributeSet нет, скейла урона
	 *  от них тоже — числа лежат здесь, потребитель появится вместе со скейлом
	 *  (`weapon_system.md`, «Что отменено и почему»). */
	UPROPERTY(EditAnywhere, Category = "CharacterSheet|Stats", meta = (ClampMin = "0"))
	int32 STR = 0;

	UPROPERTY(EditAnywhere, Category = "CharacterSheet|Stats", meta = (ClampMin = "0"))
	int32 DEX = 0;

	/** Перки, включая ранги владения оружием (Perk.Proficiency.*.RankN). Один контейнер
	 *  на всё: ранги накапливаются, повторная выдача низкого ранга ничего не меняет —
	 *  тег уже на месте, клэмп не нужен (`weapon_system.md`, «Владение оружием»). */
	UPROPERTY(EditAnywhere, Category = "CharacterSheet", meta = (Categories = "Perk"))
	FGameplayTagContainer Perks;

	/** Навыки, которые боец видел в бою. Симметрично SeenSyllables/SeenSpells магии
	 *  (`ability_system.md`, «Боевой журнал как источник «узнанного»»). Писателя пока нет —
	 *  заполняется вручную в редакторе; потребитель (условия выдачи ранга) не спроектирован. */
	UPROPERTY(EditAnywhere, Category = "CharacterSheet", meta = (Categories = "Ability"))
	FGameplayTagContainer SeenSkills;

	/** Навыки, которые боец выучил. Второй из двух гейтов гранта активки — первый ранг владения
	 *  (`weapon_system.md`, «Владение оружием»). Ключ — идентичность навыка, UAbilityData::CounterTag. */
	UPROPERTY(EditAnywhere, Category = "CharacterSheet", meta = (Categories = "Ability"))
	FGameplayTagContainer LearnedSkills;
};
