// Форма синергии метки — намеренно совпадает с FMarkSynergy из main_dev_plan.md
// (заготовка для UMarkTriggerFragment в Разделе 4). mark_system.md §2, Правило 2:
// метка сгорает → бафф на атакующего ИЛИ дебафф на цель (никогда оба). Заряды синергия
// не платит — ChargeGain удалён вместе с Правилом 4 (task_skill_economy_loops.md): заряды
// зарабатываются только ударом или парированием (combat_system.md §1).

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
};
