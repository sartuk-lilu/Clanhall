// Центральный паттерн проекта (CLAUDE.md: "Главный паттерн: DataAsset + Fragments").
// Заголовок — поля физического навыка (идентичность, длина в долях, монтаж), Fragments —
// только то, что нужно конкретной. Логика GameplayAbility
// не меняется при правке этого ассета — меняется только сам DataAsset (`Architecture.md`, «Почему Fragments»).
// Гейт применения — перезарядка приёма (`EconomySystem.md`); в коде её пока нет, приходит в P5.
// UAbilityData описывает данные ФИЗИЧЕСКОГО навыка — у заклинаний свой ассет (`MagicSystem.md`).

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Fragments/AbilityFragment.h"
#include "ClanhallCombatTypes.h"
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

	/** Идентичность навыка — чем я контрю (Ability.Knight.PowerStrike и т.д.), а не кого я контрю.
	 *  Уходит в TryResolveCounter как IncomingCounterTag; матчится против набора CounteredBy на
	 *  защищающемся (`AbilitySystem.md`, «Контрнавык»). */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Ability"))
	FGameplayTag CounterTag;

	/** Чем ЭТОТ навык можно сбить. Контейнер, не один тег: один навык врага может контриться
	 *  навыками нескольких классов. Проверка идёт через HasTag, поэтому запись ветки
	 *  (Ability.Lancer) матчит любой навык Ланцера. Пусто = навык не контрится.
	 *  Парный к CounterTag: тот — идентичность («чем я контрю»), этот — уязвимость. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Ability"))
	FGameplayTagContainer CounteredBy;

	/** Длина приёма в долях такта (`AbilitySystem.md`, «Приёмы оружия»): не всякий приём - одна
	 *  доля. Длину задают данные, а не клип: каст-монтаж подгоняется под неё play rate'ом, а без
	 *  монтажа приём держит State.SkillCommitted ровно столько же. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (ClampMin = "0.25"))
	float DurationBeats = 2.0f;

	/** Перезарядка приёма в долях (`EconomySystem.md`, «Перезарядка приёмов») - поле приёма, а не
	 *  клавиши; стартует на активации и идёт у всего оружия. Ноль законен только для отладки. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (ClampMin = "0.0"))
	float CooldownBeats = 0.0f;

	/** Удар в спину (`CombatSystem.md`, «Удар в спину»): из сектора спины цели урон идёт в HP мимо
	 *  AP и порога. Свойство приёма, а не вида оружия; спереди - обычный удар. */
	UPROPERTY(EditAnywhere, Category = "Ability|Damage")
	bool bBackstab = false;

	/** Может ли приём стать смертельным ударом (`CombatSystem.md`, «Случайность»). У атакующих
	 *  приёмов кинжала - только со спины. */
	UPROPERTY(EditAnywhere, Category = "Ability|Damage")
	EClanhallLethalMode LethalMode = EClanhallLethalMode::None;

	/** Монтаж навыка. Слот — fullbody, для ВСЕХ активок без исключения: активка отыгрывается
	 *  целым телом, разбиение «лёгкие на верх, тяжёлые целиком» отменено (`LocomotionStructure.md`, «Слоты: создание и назначение»).
	 *  Типовая ошибка — монтаж остался в DefaultSlot: логика работает, анимации не видно.
	 *  nullptr законен: механика навыка работает без монтажа, резолв уходит в мгновенный фолбэк.
	 *  Play rate монтажа считается из DurationBeats - длина клипа на длину приёма не влияет. */
	UPROPERTY(EditAnywhere, Category = "Ability")
	TObjectPtr<UAnimMontage> CastMontage;

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
