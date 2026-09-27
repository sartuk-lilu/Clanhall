// State.Stunned на фиксированную длительность — единственная выдача стана в проекте. Потеря
// управления, разрешена только на противнике асимметрией (`invariants.md`, «Разрешённые
// асимметрии»); на игроке не применяется. Источник — эффект синергии меток
// (FMarkSynergy::EffectOnTarget), обналичивающий требуемую метку конкретного навыка синергии.
// Применяется ClanhallGameplayEffects::ApplyEffect БЕЗ SetByCaller (в отличие от
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

	// FindOrAddComponent создаёт GE-компонент через NewObject(this, NAME_None, ...) — движок фатально
	// падает, если это происходит внутри собственного конструктора this (ObjectInitializer ловит
	// попытку создать субобъект без стабильного имени). PostInitProperties выполняется уже после
	// того, как конструктор отработал, поэтому безопасен для FindOrAddComponent.
	virtual void PostInitProperties() override;
};
