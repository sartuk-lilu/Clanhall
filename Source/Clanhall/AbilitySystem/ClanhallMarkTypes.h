// Форма синергии метки — намеренно совпадает с FMarkSynergy из main_dev_plan.md
// (заготовка для UMarkTriggerFragment в Разделе 4). mark_system.md §2, Правило 2:
// метка сгорает → бафф на атакующего ИЛИ дебафф на цель (никогда оба) → +ChargeGain Charges.

#pragma once

#include "GameplayTagContainer.h"
#include "ClanhallMarkTypes.generated.h"

class UGameplayEffect;

USTRUCT()
struct FMarkSynergy
{
	GENERATED_BODY()

	/** Метка на цели, которую этот навык умеет активировать. */
	UPROPERTY(EditDefaultsOnly, Category = "Mark", meta = (Categories = "Mark"))
	FGameplayTag RequiredMark;

	/** Дебафф на цель. Заполняется ИЛИ это, ИЛИ EffectOnSelf — никогда оба. */
	UPROPERTY(EditDefaultsOnly, Category = "Mark")
	TSubclassOf<UGameplayEffect> EffectOnTarget;

	/** Бафф на атакующего. Заполняется ИЛИ это, ИЛИ EffectOnTarget — никогда оба. */
	UPROPERTY(EditDefaultsOnly, Category = "Mark")
	TSubclassOf<UGameplayEffect> EffectOnSelf;

	/** Сколько Charges даёт срабатывание этой синергии (mark_system.md §2 Правило 2, §4).
	 *  Канон — 2; поле существует, чтобы число жило в данных, а не в коде. 0 = синергия
	 *  срабатывает, но зарядов не даёт (легально). */
	UPROPERTY(EditDefaultsOnly, Category = "Mark", meta = (ClampMin = "0"))
	int32 ChargeGain = 2;
};
