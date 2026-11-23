// Форма синергии метки — заготовка для UMarkTriggerFragment. (`MarksSystem.md`, «Активация синергии»):
// метка сгорает → бафф на атакующего ИЛИ дебафф на цель (никогда оба).

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
