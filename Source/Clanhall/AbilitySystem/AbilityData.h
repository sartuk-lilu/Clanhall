// Центральный паттерн проекта (CLAUDE.md: "Главный паттерн: DataAsset + Fragments").
// Заголовок — поля физического навыка (ChargeCost, ManaGain, CounterTag), Fragments —
// только то, что нужно конкретной. Логика GameplayAbility
// не меняется при правке этого ассета — меняется только сам DataAsset (`Architecture.md`, «Почему Fragments»).
// Кулдауна у навыков в проекте не осталось нигде (`economy_system.md`, «Почему кулдаунов нет»)
// — единственный гейт применения активки Charges. UAbilityData описывает данные
// ФИЗИЧЕСКОГО навыка — у заклинаний игрока свой ресурс, только MP (CLAUDE.md, `magic_system.md`),
// им понадобится свой ассет.

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

	/** Идентичность навыка — чем я контрю (Ability.Knight.PowerStrike и т.д.), а не кого я контрю.
	 *  Уходит в TryResolveCounter как IncomingCounterTag; матчится против набора CounteredBy на
	 *  защищающемся (`ability_system.md`, «Контрнавык»). */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Ability"))
	FGameplayTag CounterTag;

	/** Чем ЭТОТ навык можно сбить. Контейнер, не один тег: один навык врага может контриться
	 *  навыками нескольких классов. Проверка идёт через HasTag, поэтому запись ветки
	 *  (Ability.Lancer) матчит любой навык Ланцера. Пусто = навык не контрится.
	 *  Парный к CounterTag: тот — идентичность («чем я контрю»), этот — уязвимость. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Ability"))
	FGameplayTagContainer CounteredBy;

	/** Стоимость в Charges. Канон: Q/E=2, R/F=4, Z/X=6, C/V=8 (`combat_system.md`, «Ресурсы персонажа») — бесплатных
	 *  активок не остаётся, включая утилиту. Дефолт 2 (не 0) — самый дешёвый платный тир, чтобы
	 *  новый ассет не выглядел бесплатным по умолчанию; конкретное значение всё равно проставляется
	 *  на каждом ассете вручную по тиру слота. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (ClampMin = "0"))
	int32 ChargeCost = 2;

	/** Мана за подтверждённое попадание, раз за применение независимо от числа задетых целей
	 *  (`ability_system.md`, «Физические активные навыки»). Курс нелинеен по тиру: Q/E=4, R/F=10, Z/X=18, C/V=28. 0 законен —
	 *  утилите можно не давать маны независимо от тира. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (ClampMin = "0.0"))
	float ManaGain = 0.0f;

	/** Монтаж навыка. Слот — fullbody, для ВСЕХ активок без исключения: активка отыгрывается
	 *  целым телом, разбиение «лёгкие на верх, тяжёлые целиком» отменено (`locomotion_structure.md`, «Слоты: создание и назначение»).
	 *  Типовая ошибка — монтаж остался в DefaultSlot: логика работает, анимации не видно.
	 *  nullptr законен: механика навыка работает без монтажа, резолв уходит в мгновенный фолбэк. */
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
