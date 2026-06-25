// Generic GameplayEffect-классы "прибавь X к атрибуту Y". Каждый несёт ровно один
// Additive-модификатор со SetByCaller-магнитудой — конкретное число решает вызывающий
// код в момент применения (см. ClanhallGameplayEffects::ApplyModifyEffect ниже).
// Знак магнитуды решает, урон это или восстановление: -30 к AP — урон, +15 к AP — лечение.

#pragma once

#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "ActiveGameplayEffectHandle.h"
#include "ClanhallGameplayEffects.generated.h"

class UAbilitySystemComponent;

UCLASS()
class CLANHALL_API UGE_ModifyAP : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_ModifyAP();
};

UCLASS()
class CLANHALL_API UGE_ModifyHP : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_ModifyHP();
};

UCLASS()
class CLANHALL_API UGE_ModifyMP : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_ModifyMP();
};

UCLASS()
class CLANHALL_API UGE_ModifyBalance : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_ModifyBalance();
};

UCLASS()
class CLANHALL_API UGE_ModifyCharges : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_ModifyCharges();
};

namespace ClanhallGameplayEffects
{
	/** Собирает FGameplayEffectSpec из EffectClass, выставляет его единственную SetByCaller-магнитуду
	 *  в Magnitude и применяет к TargetASC. SourceASC задаёт контекст эффекта (instigator), может быть
	 *  равен TargetASC при применении к себе. */
	CLANHALL_API void ApplyModifyEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, float Magnitude);

	/** Применяет произвольный GameplayEffect к TargetASC без SetByCaller — для эффектов синергии меток
	 *  (FMarkSynergy::EffectOnTarget/EffectOnSelf), у которых магнитуда уже зашита в самом классе эффекта. */
	CLANHALL_API void ApplyEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass);

	/** Навешивает Tag на ASC на DurationSeconds (источник == цель). Используется для меток и КД.
	 *  Возвращает handle, чтобы вызывающий мог снять эффект досрочно. */
	CLANHALL_API FActiveGameplayEffectHandle ApplyTimedTag(UAbilitySystemComponent* ASC, FGameplayTag Tag, float DurationSeconds);

	/** Навешивает Tag на TargetASC от имени SourceASC. Нужно когда источник и цель — разные акторы
	 *  (например, AI открывает окно парирования на игроке). */
	CLANHALL_API FActiveGameplayEffectHandle ApplyTimedTagToTarget(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, FGameplayTag Tag, float DurationSeconds);
}
