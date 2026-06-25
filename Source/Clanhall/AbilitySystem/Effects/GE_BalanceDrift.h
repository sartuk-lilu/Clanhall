// Infinite Duration + Period 1с, исполняет UExecCalc_BalanceDrift каждый тик.
// Применяется один раз персонажу в BeginPlay и работает всё время — см. AClanhallCharacter.

#pragma once

#include "GameplayEffect.h"
#include "GE_BalanceDrift.generated.h"

UCLASS()
class CLANHALL_API UGE_BalanceDrift : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_BalanceDrift();
};
