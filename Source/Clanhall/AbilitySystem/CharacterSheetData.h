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
class UAbilityData;

UCLASS()
class CLANHALL_API UCharacterSheetData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Экземпляр оружия в руках. */
	UPROPERTY(EditAnywhere, Category = "CharacterSheet")
	TObjectPtr<UWeaponData> Weapon;

	/** Активные навыки. Ключ — Ability.Slot.* (Q/E/R/F и далее по канону восьми
	 *  слотов, `ability_system.md`, «Слоты активных навыков»; `Combatant Hierarchy.md`,
	 *  «Ключ по слоту, а не по имени навыка»), а не имя скилла: слот один и тот же для всех оружий,
	 *  а именованные поля-на-скилл зашивали бы имя класса в поле, которое обязано
	 *  обслужить все классы, и их пришлось бы переписывать на восемь.
	 *  Миграция с Cooldown.Slot.* (`combat_system.md`, «Боевая стойка и переключение режимов») завершена в коде —
	 *  старых тегов больше не существует. Существующие ассеты, если ключи ещё не перенесены
	 *  вручную в редакторе, ссылаются на несуществующий тег — грант молча не срабатывает. */
	UPROPERTY(EditAnywhere, Category = "CharacterSheet", meta = (Categories = "Ability.Slot"))
	TMap<FGameplayTag, TObjectPtr<UAbilityData>> Skills;
};
