// Единственный источник истины для чейна WASD-ударов (combo_fragments_redesign_task.md, ранее
// combo_system_redesign.md). Ворота ввода, не буфер: до открытия окна чтения ввод отбрасывается
// целиком, ничего не копится; в открытом окне действует "последнее нажатие решает". Сам решает,
// когда активировать GA_DirectionalAttack_* (Часть B1: инверсия потока — активация идёт через
// этот валидатор, невалидный ввод не доходит до урона/MP/Balance) и сам проигрывает монтаж
// конкретного шага пути — GA_DirectionalAttackBase собственного монтажа больше не играет.
// Живёт на AClanhallCharacter.
//
// Резолв данных (combo_datatable_picker_task.md, combo_fragments_redesign_task.md, "Резолв шага"):
// направление ВАЛИДИРУЕТ путь (префиксный поиск по UComboData::Chains, направление шага =
// Move.Direction хода по Chain.Steps[i].MoveId), MoveId ВЫБИРАЕТ клип (строка UComboData::MovesTable).
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

struct FComboChain;
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

private:
	/** Пусто = нейтраль. Направления УЖЕ сыгранных шагов (направление = Move.Direction хода по
	 *  Chain.Steps[i] выбранной цепочки), не индексы/MoveId — резолв ищет по направлениям. */
	TArray<EClanhallAttackDirection> ActiveDirections;

	/** Цепочка, к которой резолвнулся текущий ActiveDirections (для RecoveryMontage на
	 *  завершении). Указывает внутрь UComboData::Chains — валиден, пока жив ComboData
	 *  персонажа (данные ассета не мутируются в рантайме). */
	const FComboChain* CurrentChain = nullptr;

	bool bReadWindowOpen = false;
	TOptional<EClanhallAttackDirection> LatestInWindow;
	TWeakObjectPtr<UAnimMontage> LastPlayedMontage;

	/** Нейтраль + валидный опенер по данным -> активировать и стартовать ActiveDirections. */
	void TryStartSequence(EClanhallAttackDirection Direction);

	/** Резолв шага (combo_fragments_redesign_task.md): префиксный поиск всех FComboChain, чья
	 *  последовательность направлений (через UComboData::Moves) совпадает с CandidateDirections.
	 *  Совпадения делятся на ExactMatches (цепочка длиной ровно N — терминируется на кандидате) и
	 *  PrefixMatches (цепочка длиннее — кандидат её промежуточный узел, дальше идёт ветвление).
	 *  Ветвление — не коллизия: если ExactMatches больше одной записи — это ошибка данных (разрешить
	 *  нечем), берётся первая + UE_LOG Warning вне shipping; если есть только PrefixMatches,
	 *  возвращается первая (без Warning — несколько длинных веток с общим префиксом это норма).
	 *  Явно не падает — nullptr, если совпадений нет вообще. */
	const FComboChain* ResolveChain(const TArray<EClanhallAttackDirection>& CandidateDirections) const;

	/** Активирует GA_DirectionalAttack_* для Direction через ASC, передавая BaseDamage профиля
	 *  (по направлению шага StepIndex цепочки Chain) в FGameplayEventData::EventMagnitude
	 *  (формулы урона/MP/Balance в GA не тронуты, только источник числа). На успехе играет клип
	 *  хода. Возвращает успех активации — вызывающий код фиксирует состояние только если true. */
	bool ActivateStep(EClanhallAttackDirection Direction, const FComboChain* Chain, int32 StepIndex);

	void PlayMontage(UAnimMontage* Montage);
	void ResetCombo();

	/** Страховка от залипшего weapon trace (notify_state_migration_task.md §2.2): комбо-компонент
	 *  владеет жизненным циклом удар-монтажа, поэтому страховка идёт сюда, а не в GA (тот
	 *  InstancedPerExecution и заканчивается синхронно до начала монтажа). Двойной вызов
	 *  EndTrace() (нотифай + страховка) безвреден. */
	void ForceEndWeaponTrace();

	/** Вешает State.ComboRecovery (лок-аут ввода) при достижении потолка ClassRank. Состояние
	 *  (ActiveDirections) НЕ трогает — сброс и Recovery-анимация делает EndSequenceWithRecovery по
	 *  завершении терминального удар-монтажа; тег живёт своим таймером параллельно. */
	void ApplyComboRecovery();

	/** Делегат конца УДАР-монтажа. Подстраховка от залипания ActiveDirections, если на монтаже
	 *  нет/не сработал AnimNotifyState_ComboWindow. bInterrupted==false (доиграл сам) ->
	 *  EndSequenceWithRecovery(); bInterrupted==true (чейн прервал новым Montage_Play, либо
	 *  OnStanceExit) -> ничего не делать, чейн/стойку уже отработал соответствующий путь. */
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Единственная точка завершения серии — вызывается и из fall-through OnComboWindowClose
	 *  (терминальный удар без чейна), и из OnAttackMontageEnded (страховка без окна). Guard от
	 *  повторного входа (ActiveDirections уже пуст) не даёт сыграть Recovery дважды на одну серию —
	 *  не путать Recovery-анимацию (играет всегда после терминального удара, из RecoveryMontage
	 *  завершившейся CurrentChain) с State.ComboRecovery (лок-аут только по ClassRank). */
	void EndSequenceWithRecovery();

	const UComboData* GetComboData() const;
	int32 GetClassRank() const;
	UAbilitySystemComponent* GetASC() const;
	UAnimInstance* GetAnimInstance() const;
};
