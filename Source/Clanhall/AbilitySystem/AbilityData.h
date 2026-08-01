// Центральный паттерн проекта (CLAUDE.md: "Главный паттерн: DataAsset + Fragments").
// Заголовок — поля, которые есть у каждой абилки без исключения. Fragments — только то,
// что нужно конкретной. Логика GameplayAbility не меняется при правке этого ассета —
// меняется только сам DataAsset (main_dev_plan.md).
// UAbilityData описывает данные ФИЗИЧЕСКОГО навыка (ChargeCost, CooldownTag, CounterTag,
// BalanceShift) — у заклинаний игрока КД нет, только MP (CLAUDE.md, magic_system.md),
// им понадобится свой ассет (Раздел 9).

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Fragments/AbilityFragment.h"
#include "AbilityData.generated.h"

class UTexture2D;
class UAnimMontage;

UCLASS()
class CLANHALL_API UAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Ability")
	FText DisplayName;

	UPROPERTY(EditAnywhere, Category = "Ability")
	TObjectPtr<UTexture2D> Icon;

	/** КД в секундах. Канон (ability_system.md §3): по тиру навыка — для прототипа Knight Q/E/R/F → 10/10/20/20. */
	UPROPERTY(EditAnywhere, Category = "Ability")
	float Cooldown = 10.0f;

	/** Тег слота КД — Cooldown.Slot.Q/E/R/F/... (ability_system.md §3).
	 *  КД принадлежит СЛОТУ, общему для всех оружий. Не путать с Ability.Skill.* (идентичность навыка). */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Cooldown.Slot"))
	FGameplayTag CooldownTag;

	/** Тег требуемого класса — Ability.Class.Knight и т.д. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Ability.Class"))
	FGameplayTag RequiredClass;

	/** Идентичность навыка — чем я контрю (Ability.Skill.Knight.PowerStrike и т.д.), а не кого я контрю.
	 *  Уходит в TryResolveCounter как IncomingCounterTag; матчится против набора CounteredBy на
	 *  защищающемся (ability_system.md). */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Ability.Skill"))
	FGameplayTag CounterTag;

	/** Чем ЭТОТ навык можно сбить. Контейнер, не один тег: один навык врага может контриться
	 *  навыками нескольких классов. Проверка идёт через HasTag, поэтому запись ветки
	 *  (Ability.Skill.Lancer) матчит любой навык Ланцера. Пусто = навык не контрится.
	 *  Парный к CounterTag: тот — идентичность («чем я контрю»), этот — уязвимость. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Ability.Skill"))
	FGameplayTagContainer CounteredBy;

	/** Сколько шатает владельца ЭТОГО навыка, если его сбили, секунды. Свойство сбитого навыка,
	 *  а не контрящего: медленный тяжёлый замах шатает дольше быстрого. 0 = не шатает.
	 *  ОГРАНИЧЕНИЕ: должно покрывать время до кадра контакта контрящего монтажа, иначе враг
	 *  очнётся раньше, чем удар дойдёт. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (ClampMin = "0.0"))
	float CounterStunDuration = 0.5f;

	/** Стоимость в Charges. Канон: Q/E=0, R/F=2, Z/X=4, C/V=6 (combat_system.md §1). */
	UPROPERTY(EditAnywhere, Category = "Ability")
	int32 ChargeCost = 0;

	/** Монтаж навыка. Слот — fullbody, для ВСЕХ активок без исключения: активка отыгрывается
	 *  целым телом, разбиение «лёгкие на верх, тяжёлые целиком» отменено (locomotion_structure.md §3).
	 *  Типовая ошибка — монтаж остался в DefaultSlot: логика работает, анимации не видно.
	 *  nullptr законен: механика навыка работает без монтажа, резолв уходит в мгновенный фолбэк. */
	UPROPERTY(EditAnywhere, Category = "Ability")
	TObjectPtr<UAnimMontage> CastMontage;

	/** МОДУЛЬ сдвига шкалы DEX↔STR при подтверждённом попадании (combat_system.md §2: 5..15).
	 *  Знак здесь НЕ хранится — он определяется типом оружия (Weapon.Type.STR → вправо,
	 *  DEX → влево), см. UGA_ClanhallAbilityBase::GetBalanceSign. 0 = навык шкалу не двигает. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (ClampMin = "0.0"))
	float BalanceShift = 0.0f;

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
