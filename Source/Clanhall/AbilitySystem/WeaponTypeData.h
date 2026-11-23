// Чем этим оружием можно драться (`WeaponSystem.md`, «Тип оружия ↔ экземпляр»): его приёмы,
// скорость и пробитие. Дефолты полей совпадают с фолбэком на отсутствие данных
// (ClanhallWeaponDefaults) — так новый ассет сразу играбелен, а не мёртв.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Fragments/WeaponFragment.h"
#include "WeaponTypeData.generated.h"

class UAbilityData;

/** Именованные фолбэки на отсутствие данных в цепочке CharacterSheet -> Weapon -> Type — не
 *  баланс, страховка от нуля. Живут здесь, а не разъезжаются по файлам. */
namespace ClanhallWeaponDefaults
{
	constexpr float ArmorPenetration = 0.0f;
	constexpr float WeaponSpeedMultiplier = 1.0f;
}

UCLASS()
class CLANHALL_API UWeaponTypeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Множитель к базовым скоростям бойца (JogSpeed/WalkSpeed/SprintSpeed), не см/с -
	 *  абсолютное число непереносимо между бойцами с разной базой (`WeaponSystem.md`,
	 *  «Оружие как актор»). Применяется при экипировке ко всем трём разом
	 *  (`AClanhallHumanoidBase::PostInitializeComponents`) - тяжёлое оружие медленное
	 *  всегда, а не в момент боевого контакта. 1.0 - как обычно, тяжёлое -
	 *  0.6-0.7. ClampMin 0.1, не 0: ноль означал бы «этим оружием не двигаться вовсе», такого
	 *  решения нет. */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (ClampMin = "0.1"))
	float WeaponSpeedMultiplier = 1.0f;

	/** База плоского пробития DT. Потребитель — этап 6. */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (ClampMin = "0.0"))
	float ArmorPenetration = ClanhallWeaponDefaults::ArmorPenetration;

	UPROPERTY(EditAnywhere, Instanced, Category = "WeaponType")
	TArray<TObjectPtr<UWeaponFragment>> Fragments;

	/** Активные навыки. Ключ — Slot.* (Q/E/R/F и далее по канону восьми
	 *  слотов, `AbilitySystem.md`, «Слоты активных навыков»; `CharacterHierarchy.md`,
	 *  «Ключ по слоту, а не по имени навыка»), а не имя скилла: слот один и тот же для всех оружий,
	 *  а именованные поля-на-скилл зашивали бы имя класса в поле, которое обязано
	 *  обслужить все классы, и их пришлось бы переписывать на восемь.
	 *  Переехало с UCharacterSheetData: набор активок принадлежит оружию, а не персонажу
	 *  (`AbilitySystem.md`, «Физические активные навыки»). */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (Categories = "Slot"))
	TMap<FGameplayTag, TObjectPtr<UAbilityData>> Skills;

	/** Теги владения этим типом оружия по рангам: индекс 0 — ранг 1, индекс 3 — ранг 4.
	 *  Массив, а не сборка тега из имени в рантайме: строковая сборка ломается молча при первом
	 *  переименовании, а теги в проекте залочены именно от этого (`WeaponSystem.md`,
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

	/** Открыт ли тир навыка (`UAbilityData::Tier`, 1-4) при данном наборе перков листа —
	 *  гейт больше не привязан к слоту панели, на который навык посажен (`WeaponSystem.md`,
	 *  «Владение оружием»). Tier вне границ ProficiencyTagsByRank (недозаполненный ассет —
	 *  законное состояние, не повод для краша) считается закрытым. */
	bool IsTierUnlocked(int32 Tier, const FGameplayTagContainer& Perks) const;
};
