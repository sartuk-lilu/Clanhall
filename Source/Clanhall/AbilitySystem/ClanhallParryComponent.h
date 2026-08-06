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

	/** Пауза без единого события, кормящего шкалу, после которой Stagger начинает распадаться
	 *  (task_stagger_control_code.md §3). */
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Parry")
	float StaggerDecayDelay = 10.0f;

	/** Сколько секунд идёт слив ПОЛНОЙ шкалы (не деления!) — интервал между тиками считается как
	 *  StaggerDrainDuration / MaxStagger, так скорость слива фиксирована и не зависит от потолка
	 *  конкретного бойца (task_stagger_control_code.md §3). */
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Parry")
	float StaggerDrainDuration = 5.0f;

	/** Вызывается UClanhallComboComponent::ActivateStep перед каждым новым шагом владельца —
	 *  дедуп многофазной зоны ВНУТРИ шага. Не путать с ResetStaggerSeries() (счётчик серии). */
	void ResetParry();

	/** Сбрасывает счётчик отпарированных шагов серии (ParriedStepsThisSeries) — зовётся
	 *  UClanhallComboComponent::TryStartSequence в момент, когда реально стартует новая серия
	 *  ВЛАДЕЛЬЦА (task_stagger_control_code.md §2.2). Отличается от ResetParry(): тот дедуп-guard
	 *  на шаге, этот — счётчик дохода на серии. */
	void ResetStaggerSeries() { ParriedStepsThisSeries = 0; }

	/** Зовётся из UClanhallHitboxComponent::CheckAndHandleParry на ЗОНЕ АТАКУЮЩЕГО (владельца
	 *  этого компонента) — только для зон с bParryable == true (ability_system.md §2: активки
	 *  в клэше не участвуют). HitTarget — актор, которого задела ЭТА зона, уже проверен
	 *  вызывающим на State.Parrying; MyDirection — направление СВОЕГО удара. Успех — MyDirection
	 *  обратно направлению, которое HitTarget повесил на СЕБЯ тегом Attack.Direction.*
	 *  (UClanhallComboComponent::ActivateStep). При успехе: владелец (атакующий) получает
	 *  Stagger (со второго отпарированного шага серии) + хитстоп, HitTarget (парировавший) —
	 *  Charge, его собственная зона подавляется. */
	bool TryParry(AActor* HitTarget, EClanhallAttackDirection MyDirection, FVector HitLocation);

	/** Единственная точка входа в шкалу Stagger владельца (task_stagger_control_code.md §1).
	 *  Прямых записей в атрибут Stagger в проекте больше нет — только через этот метод. Порядок
	 *  внутри строгий: прервать активный слив -> прибавить Amount (если > 0) -> обработать
	 *  потолок (Mark.Staggered, не стан) -> перезапустить отсчёт паузы перед распадом.
	 *  AddStagger(0) легален и обязателен для первого отпарированного шага серии: контр-действие
	 *  было, выплаты нет, но слив всё равно рвётся и пауза перезапускается. No-op целиком, включая
	 *  таймеры, если у противника владельца нет навыка, обналичивающего Mark.Staggered (§7).
	 *  Третий кормилец (после парирования и контрнавыка) появится в Разделе 9 — антимагия,
	 *  Amount = число слов перехваченного заклинания. */
	void AddStagger(float Amount);

private:
	/** Сколько шагов ТЕКУЩЕЙ серии владельца уже отпарировано — со второго Stagger растёт на 1
	 *  (task_stagger_control_code.md §2). Приватно и без геттера: это счётчик ДОХОДА, а не признак
	 *  "подряд идущих" парирований, который понадобится BT для решения об отступлении (Блок G) —
	 *  разная семантика (см. пример D->A->D в задании), общий счётчик под оба назначения не подходит.
	 *  Сбрасывается ИСКЛЮЧИТЕЛЬНО через ResetStaggerSeries(), из UClanhallComboComponent::
	 *  TryStartSequence — не здесь и не в ResetParry(). */
	int32 ParriedStepsThisSeries = 0;

	/** task_stagger_control_code.md §7: подсистема Stagger владельца включена, только если у его
	 *  противника есть навык с синергией на Mark.Staggered — иначе AddStagger no-op целиком.
	 *  Считается один раз в BeginPlay (см. .cpp) — пересчёт при смене оружия в бэклог, оружие в
	 *  бою прототипа не меняется. */
	bool bStaggerGateOpen = false;

	virtual void BeginPlay() override;

	UAbilitySystemComponent* GetASC() const;

	/** (Пере)запускает отсчёт паузы перед распадом — зовётся из AddStagger при каждом вызове. */
	void ScheduleStaggerDecay();
	/** Пауза истекла без новых событий — запускает повторяющийся тик распада с интервалом
	 *  StaggerDrainDuration / MaxStagger (текущий потолок владельца). */
	void OnStaggerDecayDelayElapsed();
	/** Один тик распада: −1 Stagger; останавливает себя, когда Stagger дошёл до 0. */
	void DecayStaggerStep();

	/** Общий хендл: сначала держит одноразовую паузу (StaggerDecayDelay), затем сам себя
	 *  перезапускает как повторяющийся тик (StaggerDrainDuration / MaxStagger) — SetTimer с тем же
	 *  хендлом полностью заменяет предыдущее расписание. */
	FTimerHandle StaggerDecayTimer;
};
