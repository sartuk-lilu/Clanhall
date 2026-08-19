// Чем этим оружием можно драться (`weapon_system.md`, «Тип оружия ↔ экземпляр»). Бывший
// UClassKitData: ComboData переезжает сюда без изменений, доход и потолок серии — прежде
// свойства класса/ранга — стали полями типа оружия (`economy_system.md`, «Заряды: доход»,
// «Длина серии»). Header-only, по образцу UAbilityData.
//
// Дефолты SeriesLength/ChargeIncome ниже совпадают с фолбэком на отсутствие данных
// (ClanhallWeaponDefaults) — так новый ассет сразу играбелен, а не мёртв.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Fragments/WeaponFragment.h"
#include "WeaponTypeData.generated.h"

class UComboData;
class UAbilityData;

/** Именованные фолбэки на отсутствие данных в цепочке CharacterSheet -> Weapon -> Type — не
 *  баланс, страховка от нуля. SeriesLength = 0 означает «драться нельзя», ChargeIncome = 0 —
 *  «этим оружием заряды не заработать», оба состояния в дизайне отсутствуют. Живут здесь, а не
 *  разъезжаются по трём файлам. */
namespace ClanhallWeaponDefaults
{
	constexpr int32 ChargeIncome = 1;
	constexpr int32 SeriesLength = 2;
	constexpr float ArmorPenetration = 0.0f;
	constexpr float StanceSpeedMultiplier = 1.0f;
}

UCLASS()
class CLANHALL_API UWeaponTypeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Дерево ходов и профиль урона WASD-серии — общий на тип оружия. Переезжает
	 *  из UClassKitData без изменений. */
	UPROPERTY(EditAnywhere, Category = "WeaponType")
	TObjectPtr<UComboData> ComboData;

	/** Потолок длины WASD-серии, ВСЕГО ударов, а не «плюс один». Дефолт совпадает
	 *  с ClanhallWeaponDefaults::SeriesLength — пустой ассет сразу играбелен. */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (ClampMin = "1"))
	int32 SeriesLength = ClanhallWeaponDefaults::SeriesLength;

	/** Доход в Charges за подтверждённый WASD-удар, начиная со второго в серии
	 *  (`economy_system.md`, «Заряды: доход»). Дефолт совпадает с ClanhallWeaponDefaults::ChargeIncome. */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (ClampMin = "1"))
	int32 ChargeIncome = ClanhallWeaponDefaults::ChargeIncome;

	/** Множитель к базовой скорости бойца в боевой стойке, не см/с — абсолютное число
	 *  непереносимо между бойцами с разной базой (`weapon_system.md`, «Оружие как актор»).
	 *  1.0 — как обычно. ClampMin 0.1, не 0: ноль означал бы «в стойке не двигается вовсе»,
	 *  такого решения нет. Потребитель появится в этапе 4 — сейчас никто не читает. */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (ClampMin = "0.1"))
	float StanceSpeedMultiplier = 1.0f;

	/** База плоского пробития DT. Потребитель — этап 6. */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (ClampMin = "0.0"))
	float ArmorPenetration = ClanhallWeaponDefaults::ArmorPenetration;

	UPROPERTY(EditAnywhere, Instanced, Category = "WeaponType")
	TArray<TObjectPtr<UWeaponFragment>> Fragments;

	/** Активные навыки. Ключ — Slot.* (Q/E/R/F и далее по канону восьми
	 *  слотов, `ability_system.md`, «Слоты активных навыков»; `Combatant Hierarchy.md`,
	 *  «Ключ по слоту, а не по имени навыка»), а не имя скилла: слот один и тот же для всех оружий,
	 *  а именованные поля-на-скилл зашивали бы имя класса в поле, которое обязано
	 *  обслужить все классы, и их пришлось бы переписывать на восемь.
	 *  Миграция с Cooldown.Slot.* (`combat_system.md`, «Боевая стойка и переключение режимов») завершена в коде —
	 *  старых тегов больше не существует. Существующие ассеты, если ключи ещё не перенесены
	 *  вручную в редакторе, ссылаются на несуществующий тег — грант молча не срабатывает.
	 *  Переехало с UCharacterSheetData: набор активок принадлежит оружию, а не персонажу
	 *  (`ability_system.md`, «Физические активные навыки»). */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (Categories = "Slot"))
	TMap<FGameplayTag, TObjectPtr<UAbilityData>> Skills;

	/** Теги владения этим типом оружия по рангам: индекс 0 — ранг 1, индекс 3 — ранг 4.
	 *  Массив, а не сборка тега из имени в рантайме: строковая сборка ломается молча при первом
	 *  переименовании, а теги в проекте залочены именно от этого (`weapon_system.md`,
	 *  «Владение оружием»). */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (Categories = "Perk.Proficiency"))
	TArray<FGameplayTag> ProficiencyTagsByRank;

	/** Возвращает первый фрагмент типа T, или nullptr если у этого типа оружия такого нет. */
	template <typename T>
	T* FindFragment() const
	{
		for (const TObjectPtr<UWeaponFragment>& Fragment : Fragments)
		{
			if (T* Match = Cast<T>(Fragment.Get()))
			{
				return Match;
			}
		}
		return nullptr;
	}

	/** Ранг владения, открывающий этот слот: Q/E -> 1, R/F -> 2, Z/X -> 3, C/V -> 4.
	 *  0 — слот не опознан (`weapon_system.md`, «Владение оружием»). Правило едино для всех
	 *  оружий, поэтому живёт в коде, а не дублируется на каждом ассете. */
	static int32 GetRequiredProficiencyRank(FGameplayTag SlotTag);

	/** Открыт ли тир этого слота при данном наборе перков листа. Ранг вне границ
	 *  ProficiencyTagsByRank (недозаполненный ассет — законное состояние, не повод для краша)
	 *  считается закрытым. */
	bool IsSlotUnlocked(FGameplayTag SlotTag, const FGameplayTagContainer& Perks) const;
};
