// Компонент на бойце (игрок и AI, симметрично), который распознаёт клэш парирования и копит
// усталость (Stagger) от него.
//
// Модель (`Parrying.md`, «`UClanhallParryComponent`»): окно парирования размечается на монтаже защищающегося
// (AnimNotifyState_ParryWindow -> State.Parrying), резолв запускает КОНТАКТ КЛИНКА АТАКУЮЩЕГО —
// собственный хитбокс защищающегося в проверке не участвует вообще. TryParry вызывается на
// ParryComponent АТАКУЮЩЕГО (владельца зоны, задевшей цель); если цель держит State.Parrying и
// обратный тег направления (Attack.Direction.*, UClanhallComboComponent::ActivateStep) —
// атакующий оказывается ОТПАРИРОВАН, а цель — парировавшим. Кредит именно такой: клинок
// атакующего долетает первым, значит и реагирует на клэш он, а не тот, кто среагировал вовремя.
//
// Устарело: TryParry когда-то вызывался из обработчиков ввода WASD персонажа и записывал
// парировавшим владельца хитбокса — инверсия, исправлена. Сейчас: владелец хитбокса
// (атакующий) получает Stagger и хитстоп, цель (парировавший) — Charge и подавление
// собственной зоны.

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
	 *  (`Parrying.md`, «`UClanhallParryComponent`»). Сбрасывается ActivateStep перед новым своим шагом. */
	bool bStepParried = false;

	/** Звук столкновения оружий (воспроизводится TryParry при успехе). */
	UPROPERTY(EditAnywhere, Category="VFX")
	TObjectPtr<USoundBase> ClashSound;

	/** Пауза без единого события, кормящего шкалу, после которой Stagger начинает распадаться
	 *  (`combat_system.md`, «Stagger — усталость»). */
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Parry")
	float StaggerDecayDelay = 10.0f;

	/** Сколько секунд идёт слив ПОЛНОЙ шкалы (не деления!) — интервал между тиками считается как
	 *  StaggerDrainDuration / MaxStagger, так скорость слива фиксирована и не зависит от потолка
	 *  конкретного бойца (`combat_system.md`, «Stagger — усталость»). */
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Parry")
	float StaggerDrainDuration = 5.0f;

	/** Вызывается UClanhallComboComponent::ActivateStep перед каждым новым шагом владельца —
	 *  дедуп многофазной зоны ВНУТРИ шага. Не путать с ResetStaggerSeries() (счётчик серии). */
	void ResetParry();

	/** Сбрасывает счётчик отпарированных шагов серии (ParriedStepsThisSeries) — зовётся
	 *  UClanhallComboComponent::TryStartSequence в момент, когда реально стартует новая серия
	 *  ВЛАДЕЛЬЦА (`combat_system.md`, «Stagger — усталость»). Отличается от ResetParry(): тот дедуп-guard
	 *  на шаге, этот — счётчик дохода на серии. */
	void ResetStaggerSeries() { ParriedStepsThisSeries = 0; }

	/** Зовётся из UClanhallHitboxComponent::CheckAndHandleParry на ЗОНЕ АТАКУЮЩЕГО (владельца
	 *  этого компонента) — только для зон с bParryable == true (`ability_system.md`, «Контрнавык»: активки
	 *  в клэше не участвуют). HitTarget — актор, которого задела ЭТА зона, уже проверен
	 *  вызывающим на State.Parrying; MyDirection — направление СВОЕГО удара. Успех — MyDirection
	 *  обратно направлению, которое HitTarget повесил на СЕБЯ тегом Attack.Direction.*
	 *  (UClanhallComboComponent::ActivateStep). При успехе: владелец (атакующий) получает
	 *  Stagger (со второго отпарированного шага серии) + хитстоп, HitTarget (парировавший) —
	 *  Charge, его собственная зона подавляется. */
	bool TryParry(AActor* HitTarget, EClanhallAttackDirection MyDirection, FVector HitLocation);

	/** Единственная точка входа для КОРМИЛЬЦЕВ шкалы Stagger (`combat_system.md`, «Stagger — усталость») —
	 *  TryParry, контр, будущая антимагия. Прямых записей в атрибут в их коде больше нет.
	 *  DecayStaggerStep — легальное исключение: это не кормилец, а собственный внутренний
	 *  механизм ЭТОЙ ЖЕ подсистемы (распад), которому нечего добавить, кроме −1 за тик; гонять
	 *  −1 через AddStagger означало бы, что каждый тик распада сам себя прерывает и заново себя
	 *  планирует. Порядок внутри строгий: прервать активный слив -> прибавить Amount (если > 0) ->
	 *  обработать потолок (Mark.Staggered, не стан) -> перезапустить отсчёт паузы перед распадом.
	 *  AddStagger(0) легален и обязателен для первого отпарированного шага серии: контр-действие
	 *  было, выплаты нет, но слив всё равно рвётся и пауза перезапускается. No-op целиком, включая
	 *  таймеры, если у противника владельца нет навыка, обналичивающего Mark.Staggered.
	 *  Третий кормилец (после парирования и контрнавыка) — будущая антимагия,
	 *  Amount = число слов перехваченного заклинания. */
	void AddStagger(float Amount);

private:
	/** Сколько шагов ТЕКУЩЕЙ серии владельца уже отпарировано — со второго Stagger растёт на 1
	 *  (`combat_system.md`, «Stagger — усталость»). Приватно и без геттера: это счётчик ДОХОДА, а не признак
	 *  "подряд идущих" парирований, который понадобится BT для решения об отступлении —
	 *  разная семантика (пример D->A->D), общий счётчик под оба назначения не подходит.
	 *  Сбрасывается ИСКЛЮЧИТЕЛЬНО через ResetStaggerSeries(), из UClanhallComboComponent::
	 *  TryStartSequence — не здесь и не в ResetParry(). */
	int32 ParriedStepsThisSeries = 0;

	/** (`combat_system.md`, «Stagger — усталость»): подсистема Stagger владельца включена, только если у его
	 *  противника есть навык с синергией на Mark.Staggered — иначе AddStagger no-op целиком.
	 *  Считается один раз в BeginPlay, не «на входе в бой» — известное ограничение прототипа
	 *  (`Combatant Hierarchy.md`, «Открытые пункты»). */
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
