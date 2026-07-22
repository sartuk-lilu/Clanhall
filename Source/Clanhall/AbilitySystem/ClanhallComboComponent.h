// Единственный источник истины для чейна WASD-ударов (combo_transition_model_task.md, ранее
// combo_fragments_redesign_task.md, combo_system_redesign.md). Ворота ввода, не буфер: до открытия
// окна чтения ввод отбрасывается целиком, ничего не копится; в открытом окне действует "последнее
// нажатие решает". Сам решает, когда активировать GA_DirectionalAttack_* (Часть B1: инверсия
// потока — активация идёт через этот валидатор, невалидный ввод не доходит до урона/MP/Balance) и
// сам проигрывает монтаж конкретного шага — GA_DirectionalAttackBase собственного монтажа больше
// не играет. Живёт на AClanhallCharacter.
//
// Резолв данных (combo_transition_model_task.md): модель пар, не путей. Ход определяется только
// парой «предыдущее направление -> новое» (UComboData::FindOpenerMontage/FindTransitionMontage) —
// история серии до предыдущего шага не участвует, LastDirection хранит только последний шаг.
// Урон берётся из UComboData::FindDamageByDirection (4 именованных поля профиля) по направлению
// шага и передаётся в GA_DirectionalAttackBase через FGameplayEventData
// (TriggerAbilityFromGameplayEvent) — Handle-активация сохраняется, тег события служебный.
// Потолок длины серии = AClanhallCharacter::ClassRank, а не поле ассета.

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

UCLASS(ClassGroup="Clanhall", meta=(BlueprintSpawnableComponent))
class CLANHALL_API UClanhallComboComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Длительность лок-аута после максимальной длины комбо (State.ComboRecovery). Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	float ComboRecoveryDuration = 1.0f;

	/** Blend-out монтажа комбо при выходе из стойки (отпуск ЛКМ), сек. combo_system_redesign.md:
	 *  "порядка 0.15-0.2 с, чтобы верх плавно ушёл в локомоцию". */
	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	float StanceExitBlendOutTime = 0.18f;

	/** Slot node в AnimGraph персонажа, через который Recovery (Animation Sequence, не Montage —
	 *  см. FComboRecoveryAnimations) проигрывается как динамический монтаж
	 *  (PlaySlotAnimationAsDynamicMontage). Должен совпадать с именем Slot-ноды в ABP; поменять
	 *  здесь, если ABP использует другое имя, чем "DefaultSlot". */
	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	FName RecoverySlotName = "DefaultSlot";

	/** Вызывается из WASD-обработчика ввода (OnAttackX в ClanhallCharacter). Решает: опенер,
	 *  запись в окне чтения (продолжение) или мусор вне окна — сама активирует направленный
	 *  удар, когда решение валидно. */
	void HandleAttackInput(EClanhallAttackDirection Direction);

	/** Зовутся из UAnimNotifyState_ComboWindow, расставленного на монтаже текущего удара. */
	void OnComboWindowOpen();
	void OnComboWindowClose();

	/** Зовётся из обработчика отпуска ЛКМ — работает в любой фазе, вне ворот. Останавливает
	 *  активный монтаж комбо с blend-out и сбрасывает последовательность. */
	void OnStanceExit();

	/** Внешнее прерывание серии чужим монтажом (активка Q/E/R/F в общей slot-группе,
	 *  notify_state_migration_task.md Фаза 4). БЕЗ Recovery-анимации и БЕЗ State.ComboRecovery:
	 *  чужой монтаж уже занимает слот, Recovery дрался бы с ним за него; наказания за прерывание
	 *  нет — тот же принцип, что у невалидного продолжения. Зовётся ДО Montage_Play активки. */
	void CancelSequenceForExternalMontage();

private:
	/** Последнее сыгранное направление серии. Не задано = нейтраль. Определяет и следующий переход,
	 *  и Recovery на завершении — история до него не хранится (модель пар). */
	TOptional<EClanhallAttackDirection> LastDirection;

	/** Длина текущей серии. 0 = нейтраль. Сверяется с ClassRank как потолок. */
	int32 StepCount = 0;

	bool bReadWindowOpen = false;
	TOptional<EClanhallAttackDirection> LatestInWindow;
	TWeakObjectPtr<UAnimMontage> LastPlayedMontage;

	/** Нейтраль + валидный опенер по данным -> активировать и стартовать серию. */
	void TryStartSequence(EClanhallAttackDirection Direction);

	/** Активирует GA_DirectionalAttack_* для Direction через ASC, передавая BaseDamage профиля
	 *  (по Direction, UComboData::FindDamageByDirection) в FGameplayEventData::EventMagnitude
	 *  (формулы урона/MP/Balance в GA не тронуты, только источник числа). На успехе играет Montage.
	 *  Возвращает успех активации — вызывающий код фиксирует состояние только если true. */
	bool ActivateStep(EClanhallAttackDirection Direction, UAnimMontage* Montage);

	void PlayMontage(UAnimMontage* Montage);
	void ResetCombo();

	/** Страховка от залипшего weapon trace (notify_state_migration_task.md §2.2): комбо-компонент
	 *  владеет жизненным циклом удар-монтажа, поэтому страховка идёт сюда, а не в GA (тот
	 *  InstancedPerExecution и заканчивается синхронно до начала монтажа). Двойной вызов
	 *  EndTrace() (нотифай + страховка) безвреден. */
	void ForceEndWeaponTrace();

	/** Вешает State.ComboRecovery (лок-аут ввода) при достижении потолка ClassRank. Состояние
	 *  (LastDirection/StepCount) НЕ трогает — сброс и Recovery-анимация делает EndSequenceWithRecovery
	 *  по завершении терминального удар-монтажа; тег живёт своим таймером параллельно. */
	void ApplyComboRecovery();

	/** Делегат конца УДАР-монтажа. Подстраховка от залипания состояния, если на монтаже
	 *  нет/не сработал AnimNotifyState_ComboWindow. bInterrupted==false (доиграл сам) ->
	 *  EndSequenceWithRecovery(); bInterrupted==true (чейн прервал новым Montage_Play, либо
	 *  OnStanceExit) -> ничего не делать, чейн/стойку уже отработал соответствующий путь. */
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Единственная точка завершения серии — вызывается и из fall-through OnComboWindowClose
	 *  (терминальный удар без продолжения), и из OnAttackMontageEnded (страховка без окна). Guard
	 *  от повторного входа (StepCount уже 0) не даёт сыграть Recovery дважды на одну серию —
	 *  не путать Recovery-анимацию (играет всегда после терминального удара, по LastDirection) с
	 *  State.ComboRecovery (лок-аут только по ClassRank). */
	void EndSequenceWithRecovery();

	const UComboData* GetComboData() const;
	int32 GetClassRank() const;
	UAbilitySystemComponent* GetASC() const;
	UAnimInstance* GetAnimInstance() const;
};
