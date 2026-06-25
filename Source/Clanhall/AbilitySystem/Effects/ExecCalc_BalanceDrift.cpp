#include "ExecCalc_BalanceDrift.h"
#include "AbilitySystem/ClanhallAttributeSet.h"

namespace
{
	// Канон: combat_system.md §2 — "Пассивный дрейф: 2 единицы/сек к центру (0)".
	// GE_BalanceDrift тикает раз в секунду, поэтому шаг за один тик = 2.
	constexpr float DriftPerTick = 2.0f;

	struct FBalanceDriftCaptures
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(Balance);

		FBalanceDriftCaptures()
		{
			DEFINE_ATTRIBUTE_CAPTUREDEF(UClanhallAttributeSet, Balance, Target, false);
		}
	};

	const FBalanceDriftCaptures& GetBalanceDriftCaptures()
	{
		static FBalanceDriftCaptures Captures;
		return Captures;
	}
}

UExecCalc_BalanceDrift::UExecCalc_BalanceDrift()
{
	RelevantAttributesToCapture.Add(GetBalanceDriftCaptures().BalanceDef);
}

void UExecCalc_BalanceDrift::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FAggregatorEvaluateParameters EvalParams;

	float CurrentBalance = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetBalanceDriftCaptures().BalanceDef, EvalParams, CurrentBalance);

	if (FMath::IsNearlyZero(CurrentBalance))
	{
		return;
	}

	// Шаг к нулю не больше DriftPerTick и не дальше нуля (не проскакивает на другую сторону).
	const float Step = FMath::Clamp(FMath::Sign(CurrentBalance) * DriftPerTick, -FMath::Abs(CurrentBalance), FMath::Abs(CurrentBalance));

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(GetBalanceDriftCaptures().BalanceDef.AttributeToCapture, EGameplayModOp::Additive, -Step));
}
