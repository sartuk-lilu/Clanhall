// Единственный источник истины для чейна WASD-ударов (combo_system.md). Ворота ввода, не буфер:
// до открытия окна чтения ввод отбрасывается целиком, ничего не копится; в открытом окне действует
// "последнее нажатие решает". Сам решает, когда активировать GA_DirectionalAttack_* (инверсия
// потока — активация идёт через этот валидатор, невалидный ввод не доходит до урона/MP/Balance) и
// сам проигрывает монтаж конкретного шага — GA_DirectionalAttackBase собственного монтажа больше
// не играет. Живёт на AClanhallHumanoidCombatant (игрок и AI-боец, main_dev_plan.md §8).
//
// Резолв данных (combo_system.md): модель пар, не путей. Ход определяется только
// парой «предыдущее направление -> новое» (UComboData::FindOpenerMontage/FindTransitionMontage) —
// история серии до предыдущего шага не участвует, LastDirection хранит только последний шаг.
// Урон берётся из UComboData::FindDamageByDirection (4 именованных поля профиля) по направлению
// шага и передаётся в GA_DirectionalAttackBase через FGameplayEventData
// (TriggerAbilityFromGameplayEvent) — Handle-активация сохраняется, тег события служебный.
// Потолок длины серии = AClanhallHumanoidCombatant::ClassRank + 1 (ранг 0 -> 1 удар, ранг 4 -> 5),
// а не поле ассета.
//
// State.ComboRecovery (уточнение намерения): блокирует НОВЫЕ атаки (WASD и физактивки Q/E/R/F —
// см. UGA_ClanhallAbilityBase) и вход в стойку (UGA_CombatStance) ровно на время проигрывания
// Recovery-анимации — никаких долей и добавок. Выход из стойки (ЛКМ вверх, OnStanceExit) остаётся
// бесплатным всегда — тег гасит только возможность бить дальше и войти в стойку, не возможность
// убежать. Тег вешается в EndSequenceWithRecovery, когда Recovery реально стартует, и только тогда —
// нет Recovery-анимации или Montage_Play не стартовал -> лока нет вообще.

#pragma once

#include "Components/ActorComponent.h"
#include "ClanhallCombatTypes.h"
#include "Misc/Optional.h"
#include "UObject/WeakObjectPtr.h"
#include "ClanhallComboComponent.generated.h"

class UComboData;
class UAbilitySystemComponent;
class UAnimMontage;
class UAnimInstance;
class AActor;

/** main_dev_plan.md §8, Блок B: единственное расширение публичного API под AI-водителя.
 *  Открытие/закрытие окна чтения — момент, когда AI обязан подать направление в
 *  HandleAttackInput; вне окна ввод отбрасывается так же, как и у игрока (ворота, не буфер). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FClanhallOnComboWindowOpened);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FClanhallOnComboWindowClosed);

UCLASS(ClassGroup="Clanhall", meta=(BlueprintSpawnableComponent))
class CLANHALL_API UClanhallComboComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Blend-out монтажа комбо при выходе из стойки (отпуск ЛКМ), сек. combo_system.md:
	 *  "порядка 0.15-0.2 с, чтобы верх плавно ушёл в локомоцию". */
	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	float StanceExitBlendOutTime = 0.18f;

	/** main_dev_plan.md §8, Блок D: оглушение владельца серии при полном парировании ВСЕХ
	 *  её шагов (канон combat_system.md §5). Было GA_EnemyWASDSeries::StunDuration. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Parry")
	float FullParryStunDuration = 2.0f;

	/** Снижение КД парировавшего при полном парировании серии. Было GA_EnemyWASDSeries::CDReduction. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Parry")
	float FullParryCooldownReduction = 5.0f;

	/** main_dev_plan.md §8, Блок B: единственный публичный сигнал для AI/BT о том, что окно
	 *  чтения ввода открылось/закрылось — по нему водитель узнаёт момент подать направление
	 *  в HandleAttackInput. Решений компонент не принимает и не подсказывает — какое именно
	 *  направление подать, решает вызывающий (BT/исполнитель, Раздел 8 Блок G). */
	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FClanhallOnComboWindowOpened OnComboWindowOpened;

	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FClanhallOnComboWindowClosed OnComboWindowClosed;

	/** Вызывается из WASD-обработчика ввода (OnAttackX в ClanhallCharacter) — и, с Блока B,
	 *  из AI-водителя (main_dev_plan.md §8): точка входа одна и та же и стороне-нейтральна,
	 *  второй не заводим. Решает: опенер, запись в окне чтения (продолжение) или мусор вне
	 *  окна — сама активирует направленный удар, когда решение валидно. BlueprintCallable —
	 *  единственный способ прогнать серию боссу до появления BT-исполнителя (Блок G):
	 *  временный отладочный вызов кнопкой/консолью из Блока B. */
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void HandleAttackInput(EClanhallAttackDirection Direction);

	/** Зовутся из UAnimNotifyState_ComboWindow, расставленного на монтаже текущего удара. */
	void OnComboWindowOpen();
	void OnComboWindowClose();

	/** Зовётся из обработчика отпуска ЛКМ — работает в любой фазе, вне ворот. Останавливает
	 *  активный монтаж комбо с blend-out и сбрасывает последовательность. */
	void OnStanceExit();

	/** Внешнее прерывание серии чужим монтажом (активка Q/E/R/F в общей slot-группе,
	 *  combo_system.md). БЕЗ Recovery-анимации и БЕЗ State.ComboRecovery:
	 *  чужой монтаж уже занимает слот, Recovery дрался бы с ним за него; наказания за прерывание
	 *  нет — тот же принцип, что у невалидного продолжения. Зовётся ДО Montage_Play активки. */
	void CancelSequenceForExternalMontage();

	/** main_dev_plan.md §8, Блок B: публичный запрос состояния серии для AI — активна,
	 *  текущая длина и потолок по ClassRank. Ничего из этого не решает за вызывающего:
	 *  сравнивать со StepCount/ClassRank и выбирать следующее направление — дело водителя. */
	bool IsSequenceActive() const { return StepCount > 0; }
	int32 GetStepCount() const { return StepCount; }
	int32 GetClassRank() const;

	/** main_dev_plan.md §8, Блок D: зовёт UClanhallParryComponent::TryParry владельца ЦЕЛИ —
	 *  чужой counter-swing успешно распарировал ТЕКУЩИЙ шаг ЭТОЙ серии. Симметрично: роль
	 *  (игрок/AI) в резолве не участвует, только факт «мой шаг только что отпарировали». */
	void NotifyStepParried(AActor* ParrierActor);

private:
	/** Последнее сыгранное направление серии. Не задано = нейтраль. Определяет и следующий переход,
	 *  и Recovery на завершении — история до него не хранится (модель пар). */
	TOptional<EClanhallAttackDirection> LastDirection;

	/** Длина текущей серии. 0 = нейтраль. Сверяется с ClassRank как потолок. */
	int32 StepCount = 0;

	bool bReadWindowOpen = false;
	TOptional<EClanhallAttackDirection> LatestInWindow;
	TWeakObjectPtr<UAnimMontage> LastPlayedMontage;

	/** main_dev_plan.md §8, Блок D: исход текущей серии — сколько её шагов отпарировано и
	 *  кем. Читаются в ResolveParryOutcome() перед ResetCombo(), который их обнуляет. */
	int32 ParriedStepsThisSeries = 0;
	TWeakObjectPtr<AActor> LastParrierActor;

	/** main_dev_plan.md §8, Блок D (хвост, ревью): индекс (StepCount) шага, за который уже
	 *  засчитан парированный — дедуп NotifyStepParried. Без него два разных актора, задевшие
	 *  один и тот же шаг в одном окне, дадут два инкремента ParriedStepsThisSeries, тот
	 *  перескочит StepCount, и ResolveParryOutcome() тихо не увидит полное парирование. 0 —
	 *  безопасный сброс: StepCount живого шага всегда ≥ 1. */
	int32 LastParriedStepIndex = 0;

	/** Нейтраль + валидный опенер по данным -> активировать и стартовать серию. */
	void TryStartSequence(EClanhallAttackDirection Direction);

	/** Закрывает зоны предыдущего шага (ForceEndHitboxes — иначе Event.Hitbox.Closed
	 *  прерванного монтажа прилетит уже новой способности), затем активирует GA_DirectionalAttack_*
	 *  для Direction через ASC, передавая BaseDamage профиля (по Direction,
	 *  UComboData::FindDamageByDirection) в FGameplayEventData::EventMagnitude и сам Montage в
	 *  EventData.OptionalObject (способность спрашивает по нему режим резолва — контакт или
	 *  мгновенный фолбэк, см. UAnimNotifyState_Hitbox::MontageHasHitbox). Формулы урона/MP/Balance
	 *  в GA не тронуты. На успехе играет Montage.
	 *  Возвращает успех активации — вызывающий код фиксирует состояние только если true. */
	bool ActivateStep(EClanhallAttackDirection Direction, UAnimMontage* Montage);

	void PlayMontage(UAnimMontage* Montage);
	void ResetCombo();

	/** Страховка от залипшей зоны поражения (main_dev_plan.md §7): комбо-компонент
	 *  владеет жизненным циклом удар-монтажа, поэтому страховка идёт сюда, а не в GA (тот
	 *  InstancedPerExecution и заканчивается синхронно до начала монтажа). Закрывает ВСЕ зоны
	 *  без разбора (UClanhallHitboxComponent::EndAllHitboxes) — двойной вызов безвреден. */
	void ForceEndHitboxes();

	/** Делегат конца УДАР-монтажа. Подстраховка от залипания состояния, если на монтаже
	 *  нет/не сработал AnimNotifyState_ComboWindow. bInterrupted==false (доиграл сам) ->
	 *  EndSequenceWithRecovery(); bInterrupted==true (чейн прервал новым Montage_Play, либо
	 *  OnStanceExit) -> ничего не делать, чейн/стойку уже отработал соответствующий путь. */
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Единственная точка завершения серии — вызывается и из fall-through OnComboWindowClose
	 *  (терминальный удар без продолжения), и из OnAttackMontageEnded (страховка без окна). Guard
	 *  от повторного входа (StepCount уже 0) не даёт сыграть Recovery дважды на одну серию.
	 *
	 *  Здесь же вешается State.ComboRecovery — ровно на длительность Recovery-анимации
	 *  (`GetPlayLength()`), и только если Recovery-монтаж реально нашёлся и стартовал. Нет
	 *  Recovery-анимации (пустой слот `Recovery.After*`) или `Montage_Play` не стартовал ->
	 *  лок-аута нет вообще, невидимого лока в системе не остаётся. Recovery-анимация должна
	 *  быть резолвлена ДО ResetCombo() — сброс очищает LastDirection. */
	void EndSequenceWithRecovery();

	/** main_dev_plan.md §8, Блок D: симметричный телеграф направления. Вешает/снимает
	 *  Parry.Incoming.<Direction> на СЕБЯ — State.Parrying по-прежнему на разметке монтажа
	 *  (AnimNotifyState_ParryWindow), но кому какое направление парировать, до сих пор решает
	 *  только код, т.к. в анимации направление не читается. TryParry читает оба тега с актора,
	 *  которого задели — сторона (игрок/AI) в резолве больше не участвует. */
	void ApplyIncomingParryTag(EClanhallAttackDirection Direction);

	/** Снимает тег предыдущего шага (LastDirection, ещё не перезаписан вызывающим) —
	 *  зовётся и в начале ActivateStep (переход между шагами), и в ResetCombo() (конец серии). */
	void ClearIncomingParryTag();

	/** main_dev_plan.md §8, Блок D: было GA_EnemyWASDSeries::FinalizeSeries. Все шаги серии
	 *  отпарированы -> владелец серии оглушён (FullParryStunDuration), парировавший получает
	 *  снижение КД (FullParryCooldownReduction). Зовётся из EndSequenceWithRecovery ДО
	 *  ResetCombo() — тот обнуляет ParriedStepsThisSeries/LastParrierActor. */
	void ResolveParryOutcome();

	/** Снижает остаток всех активных Cooldown.*-эффектов на ASC на ReductionSeconds: снимает
	 *  и перевешивает с укороченной длительностью те, что ещё не истекли. Было
	 *  GA_EnemyWASDSeries::ReducePlayerCooldowns, обобщено — теперь применяется к любому ASC,
	 *  не только к игроку. */
	void ReduceCooldowns(UAbilitySystemComponent* TargetASC, float ReductionSeconds) const;

	const UComboData* GetComboData() const;
	UAbilitySystemComponent* GetASC() const;
	UAnimInstance* GetAnimInstance() const;
};
