// State.Stunned на фиксированную длительность — единственная выдача стана в проекте после
// task_stagger_control_code.md: обналичивание Mark.Staggered синергией (FMarkSynergy::EffectOnTarget,
// mark_system.md §6.1). Применяется ClanhallGameplayEffects::ApplyEffect БЕЗ SetByCaller (в отличие от
// GE_ApplyTimedTag) — длительность целиком в данных, а не в вызывающем коде: сама точка вызова
// (GA_PhysicalSkill::ResolveMarkLogic) навыко-нейтральна и не может решать за конкретную синергию.
//
// Длительность НЕ вынесена в собственное поле (StunDuration): DurationMagnitude — уже штатный
// EditDefaultsOnly UPROPERTY на UGameplayEffect, правится напрямую в ассете/Blueprint-потомке.
// Если бы конструктор считал DurationMagnitude из отдельного поля, у Blueprint-потомка это поле
// приехало бы из архетипа ПОСЛЕ конструктора нативного класса — конструктор успел бы посчитать
// DurationMagnitude по ещё дефолтному значению поля, и правка в редакторе молча ничего не меняла
// бы. Нужен второй стан другой длительности — второй ассет (Blueprint-потомок с другим
// DurationMagnitude), а не параметр.

#pragma once

#include "GameplayEffect.h"
#include "GE_Stunned.generated.h"

UCLASS()
class CLANHALL_API UGE_Stunned : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Stunned();
};
