#include "GE_BalanceDrift.h"
#include "ExecCalc_BalanceDrift.h"

UGE_BalanceDrift::UGE_BalanceDrift()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(1.0f);
	bExecutePeriodicEffectOnApplication = false;

	FGameplayEffectExecutionDefinition Execution;
	Execution.CalculationClass = UExecCalc_BalanceDrift::StaticClass();
	Executions.Add(Execution);
}
