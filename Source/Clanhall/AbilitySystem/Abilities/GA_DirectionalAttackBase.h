// Базовый класс для 4 направленных WASD-ударов. Канон: combat_system.md §4.
// Урон резолвится мгновенно при активации (прямой sphere hit). Активация идёт только через
// UClanhallComboComponent (combo_system_redesign.md, Часть B1 — валидатор комбо гейтит вызов
// TryActivateAbility, формулы урона/MP/Balance ниже не тронуты). Монтаж играет сам комбо-компонент
// per-path (UComboFragment) — эта абилка своего монтажа не проигрывает. Weapon trace
// (AnimNotify_WeaponTraceStart/End) в монтаже работает независимо и обеспечивает парирование
// (development_plan.md, Раздел 6.5).

#pragma once

#include "GA_ClanhallAbilityBase.h"
#include "ClanhallCombatTypes.h"
#include "GA_DirectionalAttackBase.generated.h"

class UAbilitySystemComponent;

UCLASS(Abstract)
class CLANHALL_API UGA_DirectionalAttackBase : public UGA_ClanhallAbilityBase
{
	GENERATED_BODY()

public:
	UGA_DirectionalAttackBase();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** W/A/S/D — направление удара. Используется WeaponTraceComponent для проверки парирования. */
	virtual EClanhallAttackDirection GetDirection() const PURE_VIRTUAL(UGA_DirectionalAttackBase::GetDirection, return EClanhallAttackDirection::Overhead;);

protected:
	/** Плейсхолдер урона WASD-удара — канон фиксирует числа только для активных навыков. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float RawDamage = 10.0f;
};
