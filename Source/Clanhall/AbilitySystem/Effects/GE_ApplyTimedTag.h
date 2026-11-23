// Универсальный "навесь тег на N секунд". Раньше это был GE_ApplyMark — переименован,
// когда оказалось, что временные состояния устроены точно так же (тег на
// время), просто другой GameplayTag и другая длительность. Какой тег и насколько — решает
// вызывающий код: DynamicGrantedTags для тега, SetByCaller для длительности.
// Используется UGA_Dodge и UGA_Duck (State.EvadeRecovery, по длине анимации), сменой оружия
// (State.WeaponSwap), контром (State.Stunned) и насмешкой-парированием. Метки этим эффектом не
// пользуются: их время держит UClanhallMarkComponent. Перезарядку приёмов тоже держит свой
// компонент (UClanhallSkillCooldownComponent), а не этот эффект.

#pragma once

#include "GameplayEffect.h"
#include "GE_ApplyTimedTag.generated.h"

UCLASS()
class CLANHALL_API UGE_ApplyTimedTag : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_ApplyTimedTag();
};
