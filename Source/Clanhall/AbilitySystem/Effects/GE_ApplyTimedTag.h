// Универсальный "навесь тег на N секунд". Раньше это был GE_ApplyMark — переименован
// в Разделе 4, когда оказалось, что временные состояния устроены точно так же (тег на
// время), просто другой GameplayTag и другая длительность. Какой тег и насколько — решает
// вызывающий код: DynamicGrantedTags для тега, SetByCaller для длительности.
// Используется UClanhallMarkComponent (метки, фикс. 5 сек) и UClanhallComboComponent
// (State.ComboRecovery, по длине анимации). КД навыков в проекте не осталось нигде
// (task_skill_economy_loops.md) — этот эффект для них больше не применяется.

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
