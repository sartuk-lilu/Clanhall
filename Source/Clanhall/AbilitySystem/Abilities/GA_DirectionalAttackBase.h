// Базовый класс для 4 направленных WASD-ударов. Канон: combat_system.md §4.
// Раздел 2: без анимации и без DataAsset/Fragments (те приходят в Разделе 4/6.5) —
// удар резолвится мгновенно при активации: сферой ищем цель перед персонажем,
// снимаем её AP (+ 50% возврат себе), переполнение урона идёт в HP, плюс MP/Balance
// по типу оружия (Weapon.Type.STR/DEX, тег на ASC). WASD не накладывает метки.

#pragma once

#include "GA_ClanhallAbilityBase.h"
#include "GA_DirectionalAttackBase.generated.h"

class UAbilitySystemComponent;

UENUM(BlueprintType)
enum class EClanhallAttackDirection : uint8
{
	Overhead,	// W
	RightSlash,	// D
	LeftSlash,	// A
	LowSweep	// S
};

UCLASS(Abstract)
class CLANHALL_API UGA_DirectionalAttackBase : public UGA_ClanhallAbilityBase
{
	GENERATED_BODY()

public:
	UGA_DirectionalAttackBase();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** W/A/S/D — пока используется только для будущей анимации (Раздел 6.5) и парирования; на формулу удара не влияет. */
	virtual EClanhallAttackDirection GetDirection() const PURE_VIRTUAL(UGA_DirectionalAttackBase::GetDirection, return EClanhallAttackDirection::Overhead;);

protected:
	/** Плейсхолдер урона WASD-удара — канон фиксирует числа только для активных навыков (physical_abilities.md), не для базовых ударов. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float RawDamage = 30.0f;
};
