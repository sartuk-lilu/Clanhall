// Базовый класс для 4 направленных WASD-ударов. Канон: combat_system.md §4.
// Урон резолвится мгновенно при активации (прямой sphere hit). Активация идёт только через
// UClanhallComboComponent (combo_fragments_redesign_task.md, Часть B1 — валидатор комбо гейтит
// вызов активации, формулы урона/MP/Balance ниже не тронуты). Величина урона (BaseDamage профиля
// по направлению шага) приходит в TriggerEventData->EventMagnitude — компонент резолвит её из
// UComboData::FindDamageByDirection ДО активации, эта абилка своего числа урона больше не хранит.
// Монтаж играет сам комбо-компонент per-move — эта абилка своего монтажа не проигрывает. Weapon
// trace (AnimNotifyState_WeaponTrace) в монтаже работает независимо и обеспечивает
// парирование (development_plan.md, Раздел 6.5).

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
};
