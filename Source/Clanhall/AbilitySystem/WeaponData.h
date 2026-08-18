// Что конкретно в руках — «Клинок Рассвета» (`weapon_system.md`, «Тип оружия ↔ экземпляр»).
// Экземпляр ссылается на тип объектом, не тегом: тип задаёт форму, экземпляр — величину.

#pragma once

#include "Engine/DataAsset.h"
#include "Fragments/WeaponFragment.h"
#include "WeaponData.generated.h"

class UWeaponTypeData;
class UStaticMesh;

UCLASS()
class CLANHALL_API UWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Чем этим экземпляром можно драться — прямая ссылка на объект, не тег. */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TObjectPtr<UWeaponTypeData> Type;

	/** Меш в основной руке. Крепит Blueprint — потребителя в C++ нет. */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TObjectPtr<UStaticMesh> Mesh;

	/** Плоская надбавка к профилю урона типа. Потребителя в C++ пока не имеет — урон резолвится
	 *  через UComboData::FindDamageByDirection, DT-атрибута в UClanhallAttributeSet нет вовсе.
	 *  Поле заводится сейчас, потому что без него экземпляр оружия — пустая обёртка вокруг
	 *  ссылки на тип, и смысл расщепления не проверяется. */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float DamageBonus = 0.0f;

	/** Надбавка к базе пробития типа. Тот же статус, что у DamageBonus — потребителя пока нет. */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float ArmorPenetrationBonus = 0.0f;

	/** Сюда кладётся Offhand у оружий с левой рукой занятой. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Weapon")
	TArray<TObjectPtr<UWeaponFragment>> Fragments;

	/** ChargeIncome и SeriesLength здесь НЕ переопределяются, и полей под них нет.
	 *  Легендарный меч, дающий больше зарядов за тот же удар, — тот самый арбитраж, ради
	 *  убийства которого написан инвариант экономики (`economy_system.md`, «Инвариант экономики
	 *  оружия»). Не заводить поле-переопределение «на всякий случай». */

	/** Возвращает первый фрагмент типа T, или nullptr если у этого экземпляра такого нет. */
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

	/** BlueprintCallable-обёртка над FindFragment<T> — шаблон из Blueprint не позвать,
	 *  а оффхенд крепит Blueprint. */
	UFUNCTION(BlueprintCallable, Category = "Weapon", meta = (DeterminesOutputType = "FragmentClass"))
	UWeaponFragment* FindWeaponFragment(TSubclassOf<UWeaponFragment> FragmentClass) const;
};
