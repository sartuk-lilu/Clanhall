// Базовый класс для 4 направленных WASD-ударов. Канон: combat_system.md §4.
// Активация идёт только через UClanhallComboComponent (combo_fragments_redesign_task.md, Часть
// B1 — валидатор комбо гейтит вызов активации, формулы урона/MP/Balance ниже не тронуты).
// Величина урона (BaseDamage профиля по направлению шага) приходит в
// TriggerEventData->EventMagnitude — компонент резолвит её из UComboData::FindDamageByDirection
// ДО активации, эта абилка своего числа урона больше не хранит. Монтаж играет сам
// комбо-компонент — эта абилка своего монтажа не проигрывает.
//
// Порция D — два режима резолва, выбираются на активации по TriggerEventData->OptionalObject
// (сам Montage шага, см. UClanhallComboComponent::ActivateStep):
//   - Контактный (по умолчанию, монтаж есть и на нём расставлен AnimNotifyState_Hitbox):
//     способность ждёт Event.Hitbox.Hit/Event.Hitbox.Closed от UClanhallHitboxComponent и
//     применяет урон в момент реального касания зоны. Удар засчитывается зоне, а не нажатию.
//   - Мгновенный фолбэк (монтажа нет ИЛИ на нём не расставлена зона): резолв сферой
//     FindMeleeTarget/TraceRange/TraceRadius прямо на активации, как до Порции D. Это то, что
//     сохраняет инвариант «дерево комбо тестируется до нарезки анимаций» (main_dev_plan.md §7).

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

	/** W/A/S/D — направление удара. Используется HitboxComponent для проверки парирования. */
	virtual EClanhallAttackDirection GetDirection() const PURE_VIRTUAL(UGA_DirectionalAttackBase::GetDirection, return EClanhallAttackDirection::Overhead;);

protected:
	UFUNCTION()
	void OnHitboxHitReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnHitboxClosedReceived(FGameplayEventData Payload);

private:
	/** BaseDamage профиля шага, снятый с EventMagnitude на активации. Применяется в момент
	 *  контакта, а не активации. */
	float PendingBaseDamage = 0.0f;

	/** Общая часть контактного и фолбэк-путей: урон + MP/Balance + debug. */
	void ResolveHitOn(AActor* Target);
};
