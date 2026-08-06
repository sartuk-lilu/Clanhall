// State.Stunned на фиксированную длительность — единственная выдача стана в проекте после
// task_stagger_control_code.md: обналичивание Mark.Staggered синергией (FMarkSynergy::EffectOnTarget,
// mark_system.md §6.1). Применяется ClanhallGameplayEffects::ApplyEffect БЕЗ SetByCaller (в отличие от
// GE_ApplyTimedTag) — длительность целиком в данных (StunDuration), а не в вызывающем коде: сама точка
// вызова (GA_PhysicalSkill::ResolveMarkLogic) навыко-нейтральна и не может решать за конкретную синергию.

#pragma once

#include "GameplayEffect.h"
#include "GE_Stunned.generated.h"

UCLASS()
class CLANHALL_API UGE_Stunned : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Stunned();

	/** Длительность стана, сек. EditDefaultsOnly — правится в данных без правки кода. */
	UPROPERTY(EditDefaultsOnly, Category = "Duration")
	float StunDuration = 2.0f;

protected:
	/** StunDuration -> DurationMagnitude и State.Stunned -> грант цели здесь, а не в конструкторе:
	 *  конструктор native-класса отрабатывает ДО того, как в CDO/Blueprint-потомок попадут
	 *  финальные значения EditDefaultsOnly-свойств, PostInitProperties — уже после. */
	virtual void PostInitProperties() override;
};
