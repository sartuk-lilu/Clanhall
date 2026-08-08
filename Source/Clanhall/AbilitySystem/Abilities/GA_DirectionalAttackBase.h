// Базовый класс для 4 направленных WASD-ударов. Канон: combat_system.md §4.
// Активация идёт только через UClanhallComboComponent (main_dev_plan.md §7 — валидатор
// комбо гейтит вызов активации, формулы урона/Charges/Balance ниже не тронуты).
// Величина урона (BaseDamage профиля по направлению шага) приходит в
// TriggerEventData->EventMagnitude — компонент резолвит её из UComboData::FindDamageByDirection
// ДО активации, эта абилка своего числа урона больше не хранит. Монтаж играет сам
// комбо-компонент — эта абилка своего монтажа не проигрывает.
//
// Два режима резолва, выбираются на активации по TriggerEventData->OptionalObject
// (сам Montage шага, см. UClanhallComboComponent::ActivateStep):
//   - Контактный (по умолчанию, монтаж есть и на нём расставлен AnimNotifyState_Hitbox):
//     способность ждёт Event.Hitbox.Hit/Event.Hitbox.Closed от UClanhallHitboxComponent и
//     применяет урон в момент реального касания зоны. Удар засчитывается зоне, а не нажатию.
//   - Мгновенный фолбэк (монтажа нет ИЛИ на нём не расставлена зона): резолв сферой
//     FindMeleeTarget/TraceRange/TraceRadius прямо на активации, как до перехода на контактный
//     резолв. Это то, что сохраняет инвариант «дерево комбо тестируется до нарезки анимаций»
//     (main_dev_plan.md §7).

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

	/** Общая часть контактного и фолбэк-путей: урон + Charges (ресурс, раз за взмах, со второго
	 *  удара серии) + Balance (состояние, один раз за взмах) + debug. Мана сюда больше не входит —
	 *  переехала на физактивки (ability_system.md §1, GA_PhysicalSkill::ResolveHitOn). */
	void ResolveHitOn(AActor* Target);

	/** Сдвиг Balance уже применён за этот взмах. InstancingPolicy == InstancedPerExecution —
	 *  инстанс свежий на каждую активацию, сбрасывать вручную не нужно (тот же паттерн, что
	 *  UGA_PhysicalSkill::bBalanceApplied). */
	bool bBalanceApplied = false;

	/** +1 Charge уже начислен за этот взмах — тем же приёмом, что bBalanceApplied. */
	bool bChargeApplied = false;

	/** Снимок «это как минимум второй удар серии», сделанный в ActivateAbility ДО того, как
	 *  UClanhallComboComponent инкрементирует свой StepCount. Не читать GetStepCount() напрямую
	 *  из ResolveHitOn: на контактном пути (async, через Event.Hitbox.Hit) StepCount к моменту
	 *  резолва уже увеличен на этот шаг, а на мгновенном фолбэке (синхронно внутри ActivateStep)
	 *  ещё нет — прямое чтение дало бы разные пороги в зависимости от режима резолва одного и
	 *  того же удара (combat_system.md §1: доход начиная со второго удара серии). */
	bool bChargeEligible = false;
};
