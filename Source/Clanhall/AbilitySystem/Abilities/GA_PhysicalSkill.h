// Раздел 4: один навык-класс для всех физических активных способностей (Q/E/R/F/...).
// Канон CLAUDE.md: "Логика абилки не меняется при правке данных — меняется только DataAsset".
// Конкретное содержание (урон/метка/синергия/баланс/КД/стоимость) приходит из UAbilityData,
// который привязывается к каждому гранту через FGameplayAbilitySpec::SourceObject —
// то есть один и тот же класс граннтится 4 раза (Shield Slam/Power Strike/Shield Charge/
// Retribution), и каждый раз с другим SourceObject.
//
// Два режима резолва, выбираются на активации по Data->CastMontage (см.
// UAnimNotifyState_Hitbox::MontageHasHitbox). Монтажи Knight назначены и проигрываются —
// выбор режима зависит только от того, расставлен ли на конкретном монтаже
// AnimNotifyState_Hitbox (редакторная разметка, не код):
//   - Контактный (монтаж есть и на нём расставлен AnimNotifyState_Hitbox): способность ждёт
//     Event.Hitbox.Hit от UClanhallHitboxComponent и резолвит цель(и) в момент реального
//     касания зоны. Одна зона может задеть несколько целей за применение (Shield Charge —
//     капсула на пелвисе на весь рывок, main_dev_plan.md §7 п.11) — см. правило мультицели ниже.
//   - Мгновенный фолбэк (CastMontage == nullptr ИЛИ на нём не расставлена зона): резолв по
//     FindMeleeTarget на активации, как до перехода на контактный резолв.
//
// Перемещение навыка (Shield Charge и подобные) приходит из UDashFragment через Root Motion
// Source (StartDashIfNeeded), а не из root motion клипа — дистанция и длительность живут в
// данных, не в анимации (task_dash_and_counter_window.md, Задача 1). Способность живёт до
// позднейшего из двух терминаторов — конца каст-монтажа/фолбэка И конца рывка, см.
// TryFinishAbility.
//
// Инвариант «один резолвер на актора»: Event.Hitbox.Hit и Event.Hitbox.Closed летят на весь
// ASC владельца без адресации. Система корректна ровно потому, что в любой момент времени
// урон по контакту резолвит не более одной способности на актора. Это обеспечивается двумя
// правилами: (1) чейн WASD — ActivateStep закрывает зоны предыдущего шага ДО активации
// следующей способности; (2) активка — CancelSequenceForExternalMontage() гасит
// серию ДО того, как активка подпишется на события (см. ActivateAbility.cpp). Payload.EventMagnitude
// несёт хендл зоны и остаётся неиспользованным заделом: если инвариант когда-нибудь придётся
// ослабить (две зоны от разных владельцев одновременно), фильтр по хендлу — готовая точка
// расширения. Сейчас фильтр не добавлять — он создал бы вид защиты там, где защищает порядок
// вызовов.

#pragma once

#include "GA_ClanhallAbilityBase.h"
#include "GA_PhysicalSkill.generated.h"

class UAbilityData;
class UClanhallMarkComponent;
class UAnimMontage;
struct FMarkSynergy;

UCLASS()
class CLANHALL_API UGA_PhysicalSkill : public UGA_ClanhallAbilityBase
{
	GENERATED_BODY()

public:
	UGA_PhysicalSkill();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Единственная точка снятия State.SkillCommitted — переопределён именно ради этого,
	 *  снимает loose-тег независимо от того, как способность закончилась. */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UFUNCTION()
	void OnHitboxHitReceived(FGameplayEventData Payload);

	/** Терминатор контактного/фолбэк пути (первичный, см. bPrimaryTerminatorFired). Делегат
	 *  AnimInstance, не GameplayEvent — биндится через Montage_SetEndDelegate после
	 *  Montage_Play(Data->CastMontage), срабатывает и на нормальном завершении, и на
	 *  прерывании монтажа. */
	void OnCastMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Терминатор рывка (см. bDashPending). Дублирован на оба финальных делегата задачи —
	 *  оба означают конец Root Motion Source, различие (упёрся в стену/доехал) здесь не важно. */
	UFUNCTION()
	void OnDashFinished();

private:
	/** Запускает Root Motion Source рывка, если у навыка есть UDashFragment; иначе no-op.
	 *  Аватар не ACharacter (нет CharacterMovementComponent) → рывок пропускается с варном.
	 *  MontagePlayLength — длина каст-монтажа (0, если монтажа нет/не стартовал) для грубой
	 *  проверки Dash->Duration против неё (Задача 5, task_counterwindow_symmetry.md). */
	void StartDashIfNeeded(const UAbilityData* Data, AActor* Avatar, float MontagePlayLength);

	/** EndAbility вызывается только когда отработали ОБА терминатора — иначе фолбэк-путь без
	 *  рывка убил бы задачу рывка в тот же кадр, а контактный путь оборвал бы рывок на середине
	 *  (task_dash_and_counter_window.md, Задача 1.3). */
	void TryFinishAbility();
	/** SourceObject ищется через Handle, а не GetCurrentSourceObject() — у InstancedPerExecution
	 *  абилки на момент CanActivateAbility ещё нет персистентного инстанса (см. движок,
	 *  AbilitySystemComponent_Abilities.cpp: CanActivateAbility может вызываться на CDO). */
	const UAbilityData* GetAbilityData(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;

	void ResolveMarkLogic(const UAbilityData* Data, UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, UClanhallMarkComponent* TargetMarkComponent);

	/** Общая часть контактного и фолбэк-путей: урон + метки/синергия + Balance + debug.
	 *  Вызывается ОДИН раз на каждую задетую цель. */
	void ResolveHitOn(AActor* Target);

	/** Баф на себя из синергии (EffectOnSelf) уже применён за это применение навыка.
	 *  ChargeGain этим флагом НЕ гатится: заряды — ресурс и начисляются за каждую
	 *  задетую цель (mark_system.md §2, правило мультицели). */
	bool bSelfSynergySpent = false;

	/** Сдвиг Balance уже применён за это применение навыка. */
	bool bBalanceApplied = false;

	/** Первичный терминатор отработал: конец каст-монтажа (контактный путь) либо завершённый
	 *  мгновенный резолв (фолбэк). */
	bool bPrimaryTerminatorFired = false;

	/** Рывок запущен и ещё не финишировал. */
	bool bDashPending = false;

	/** Цели, по которым логика метки за это применение уже отработала. Урон идёт на КАЖДЫЙ
	 *  контакт (два хитбокса на монтаже = два урона), а метка/синергия/ChargeGain — один раз
	 *  на цель за применение: иначе вторая фаза увидит метку, которую положила первая, и
	 *  синергия с корневым RequiredMark (Retribution — «любая метка») сработает на собственной
	 *  метке, выдав заряды дважды. InstancedPerExecution — сбрасывать вручную не нужно. */
	TArray<TWeakObjectPtr<AActor>> MarkResolvedTargets;

	// Все поля выше — обычные, без сброса: InstancingPolicy == InstancedPerExecution,
	// на каждое применение создаётся свежий инстанс. Не превращать в статики и не сбрасывать
	// вручную — именно политика инстансирования делает их корректными.
};
