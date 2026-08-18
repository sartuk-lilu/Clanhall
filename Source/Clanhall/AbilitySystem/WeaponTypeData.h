// Чем этим оружием можно драться (`weapon_system.md`, «Тип оружия ↔ экземпляр»). Бывший
// UClassKitData: ComboData переезжает сюда без изменений, доход и потолок серии — прежде
// свойства класса/ранга — стали полями типа оружия (`economy_system.md`, «Заряды: доход»,
// «Длина серии»). Header-only, по образцу UAbilityData.
//
// Дефолты SeriesLength/ChargeIncome ниже совпадают с фолбэком на отсутствие данных
// (ClanhallWeaponDefaults) — так новый ассет сразу играбелен, а не мёртв.

#pragma once

#include "Engine/DataAsset.h"
#include "Fragments/WeaponFragment.h"
#include "WeaponTypeData.generated.h"

class UComboData;

/** Именованные фолбэки на отсутствие данных в цепочке CharacterSheet -> Weapon -> Type — не
 *  баланс, страховка от нуля. SeriesLength = 0 означает «драться нельзя», ChargeIncome = 0 —
 *  «этим оружием заряды не заработать», оба состояния в дизайне отсутствуют. Живут здесь, а не
 *  разъезжаются по трём файлам. */
namespace ClanhallWeaponDefaults
{
	constexpr int32 ChargeIncome = 1;
	constexpr int32 SeriesLength = 2;
	constexpr float ArmorPenetration = 0.0f;
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

	/** См/с в боевой стойке. Потребитель появится в этапе 4 — сейчас никто не читает. */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (ClampMin = "0.0"))
	float StanceMoveSpeed = 0.0f;

	/** База плоского пробития DT. Потребитель — этап 6. */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (ClampMin = "0.0"))
	float ArmorPenetration = ClanhallWeaponDefaults::ArmorPenetration;

	UPROPERTY(EditAnywhere, Instanced, Category = "WeaponType")
	TArray<TObjectPtr<UWeaponFragment>> Fragments;

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
};
