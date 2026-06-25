// Считает, на сколько сдвинуть Balance к нулю за один период GE_BalanceDrift.
// Обычный Modifier не подходит: шаг должен идти в сторону, противоположную ТЕКУЩЕМУ
// знаку Balance, и не должен проскакивать через ноль — это требует прочитать текущее
// значение атрибута в момент исполнения, что умеет только ExecutionCalculation.

#pragma once

#include "GameplayEffectExecutionCalculation.h"
#include "ExecCalc_BalanceDrift.generated.h"

UCLASS()
class CLANHALL_API UExecCalc_BalanceDrift : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UExecCalc_BalanceDrift();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
