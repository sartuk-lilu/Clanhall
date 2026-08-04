// Компонент на бойце (игрок и AI, симметрично), который распознаёт клэш парирования и копит
// усталость (Stagger) от него.
//
// Новая модель (task_parry_rework.md): окно парирования размечается на монтаже защищающегося
// (AnimNotifyState_ParryWindow -> State.Parrying), резолв запускает КОНТАКТ КЛИНКА АТАКУЮЩЕГО —
// собственный хитбокс защищающегося в проверке не участвует вообще. TryParry вызывается на
// ParryComponent АТАКУЮЩЕГО (владельца зоны, задевшей цель); если цель держит State.Parrying и
// обратный тег направления (Attack.Direction.*, UClanhallComboComponent::ActivateStep) —
// атакующий оказывается ОТПАРИРОВАН, а цель — парировавшим. Кредит именно такой: клинок
// атакующего долетает первым, значит и реагирует на клэш он, а не тот, кто среагировал вовремя.
//
// Раздел 5 (устарело): TryParry вызывался из обработчиков ввода WASD персонажа.
// Раздел 7 -> Блок D (устарело): TryParry записывал парировавшим владельца хитбокса —
//         инверсия, чинится task_parry_rework.md.
// task_parry_rework.md -> сейчас: владелец хитбокса (атакующий) получает Stagger и хитстоп,
//         цель (парировавший) — Charge и подавление собственной зоны.

#pragma once

#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallParryComponent.generated.h"

class USoundBase;
class UAbilitySystemComponent;

UCLASS(ClassGroup="Clanhall", meta=(BlueprintSpawnableComponent))
class CLANHALL_API UClanhallParryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** true, если ТЕКУЩИЙ шаг владельца (атакующего) уже засчитан отпарированным в этом окне —
	 *  дедуп-guard внутри TryParry: один замах не должен дать два Stagger, даже если его
	 *  многофазная зона задела нескольких парирующих. Было bParrySuccessful ("я парировал");
	 *  переименовано, т.к. смысл развернулся — теперь "мой текущий шаг уже отпарирован"
	 *  (task_parry_rework.md §1.1). Сбрасывается ActivateStep перед новым своим шагом. */
	bool bStepParried = false;

	/** Звук столкновения оружий (воспроизводится TryParry при успехе). */
	UPROPERTY(EditAnywhere, Category="VFX")
	TObjectPtr<USoundBase> ClashSound;

	/** Оглушение владельца серии при заполнении Stagger до потолка (task_parry_rework.md §1.3).
	 *  Было UClanhallComboComponent::FullParryStunDuration. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Parry")
	float FullParryStunDuration = 2.0f;

	/** Снижение КД парировавшего при заполнении Stagger владельца до потолка.
	 *  Было UClanhallComboComponent::FullParryCooldownReduction. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Parry")
	float FullParryCooldownReduction = 5.0f;

	/** Пауза без единого парирования, после которой Stagger начинает распадаться. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Parry")
	float StaggerDecayDelay = 3.0f;

	/** Интервал между тиками распада (−1 Stagger за тик), пока Stagger > 0. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Parry")
	float StaggerDecayInterval = 1.5f;

	/** Вызывается UClanhallComboComponent::ActivateStep перед каждым новым шагом владельца. */
	void ResetParry();

	/** Зовётся из UClanhallHitboxComponent::CheckAndHandleParry на ЗОНЕ АТАКУЮЩЕГО (владельца
	 *  этого компонента) — только для зон с bParryable == true (ability_system.md §2: активки
	 *  в клэше не участвуют). HitTarget — актор, которого задела ЭТА зона, уже проверен
	 *  вызывающим на State.Parrying; MyDirection — направление СВОЕГО удара. Успех — MyDirection
	 *  обратно направлению, которое HitTarget повесил на СЕБЯ тегом Attack.Direction.*
	 *  (UClanhallComboComponent::ActivateStep). При успехе: владелец (атакующий) получает
	 *  Stagger + хитстоп, HitTarget (парировавший) — Charge, его собственная зона подавляется. */
	bool TryParry(AActor* HitTarget, EClanhallAttackDirection MyDirection, FVector HitLocation);

private:
	/** Снижает остаток всех активных Cooldown.*-эффектов на ASC на ReductionSeconds — было
	 *  UClanhallComboComponent::ReduceCooldowns, переехало сюда вместе с выплатой при полном
	 *  заполнении Stagger (task_parry_rework.md §1.3). Умрёт в task_skill_economy_concept.md,
	 *  не здесь. */
	void ReduceCooldowns(UAbilitySystemComponent* TargetASC, float ReductionSeconds) const;

	UAbilitySystemComponent* GetASC() const;

	/** (Пере)запускает отсчёт паузы перед распадом — зовётся при каждом +1 Stagger. */
	void ScheduleStaggerDecay();
	/** Пауза истекла без новых парирований — запускает повторяющийся тик распада. */
	void OnStaggerDecayDelayElapsed();
	/** Один тик распада: −1 Stagger; останавливает себя, когда Stagger дошёл до 0. */
	void DecayStaggerStep();

	/** Общий хендл: сначала держит одноразовую паузу (StaggerDecayDelay), затем сам себя
	 *  перезапускает как повторяющийся тик (StaggerDecayInterval) — SetTimer с тем же хендлом
	 *  полностью заменяет предыдущее расписание. */
	FTimerHandle StaggerDecayTimer;
};
