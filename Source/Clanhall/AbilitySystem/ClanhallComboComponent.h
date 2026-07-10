// Единственный источник истины для чейна WASD-ударов (combo_system_redesign.md). Ворота ввода,
// не буфер: до открытия окна чтения ввод отбрасывается целиком, ничего не копится; в открытом
// окне действует "последнее нажатие решает". Сам решает, когда активировать GA_DirectionalAttack_*
// (Часть B1: инверсия потока — активация идёт через этот валидатор, невалидный ввод не доходит
// до урона/MP/Balance) и сам проигрывает монтаж конкретного шага пути (per-path, UComboFragment) —
// GA_DirectionalAttackBase собственного монтажа больше не играет. Живёт на AClanhallCharacter.

#pragma once

#include "Components/ActorComponent.h"
#include "ClanhallCombatTypes.h"
#include "Misc/Optional.h"
#include "UObject/WeakObjectPtr.h"
#include "ClanhallComboComponent.generated.h"

class UComboFragment;
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
	/** Пусто = нейтраль. Полный путь направлений от опенера — "полная последовательность,
	 *  а не (шаг, направление)" (combo_system_redesign.md). */
	TArray<EClanhallAttackDirection> ActiveSequence;

	bool bReadWindowOpen = false;
	TOptional<EClanhallAttackDirection> LatestInWindow;
	TWeakObjectPtr<UAnimMontage> LastPlayedMontage;

	/** Нейтраль + валидный опенер по данным -> активировать и стартовать ActiveSequence. */
	void TryStartSequence(EClanhallAttackDirection Direction);

	/** Активирует GA_DirectionalAttack_* для Direction через ASC (формулы урона/MP/Balance не
	 *  тронуты, только точка вызова). На успехе играет Montage. Возвращает успех активации —
	 *  вызывающий код фиксирует состояние (ActiveSequence) только если true. */
	bool ActivateDirection(EClanhallAttackDirection Direction, UAnimMontage* Montage);

	void PlayMontage(UAnimMontage* Montage);
	void ResetCombo();

	/** Вешает State.ComboRecovery (лок-аут ввода) при достижении MaxComboLength. Состояние
	 *  (ActiveSequence) НЕ трогает — сброс и Recovery-анимация делает EndSequenceWithRecovery по
	 *  завершении терминального удар-монтажа; тег живёт своим таймером параллельно. */
	void ApplyComboRecovery();

	/** Делегат конца УДАР-монтажа. Подстраховка от залипания ActiveSequence, если на монтаже
	 *  нет/не сработал AnimNotifyState_ComboWindow. bInterrupted==false (доиграл сам) ->
	 *  EndSequenceWithRecovery(); bInterrupted==true (чейн прервал новым Montage_Play, либо
	 *  OnStanceExit) -> ничего не делать, чейн/стойку уже отработал соответствующий путь. */
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Единственная точка завершения серии — вызывается и из fall-through OnComboWindowClose
	 *  (терминальный удар без чейна), и из OnAttackMontageEnded (страховка без окна). Guard от
	 *  повторного входа (ActiveSequence уже пуст) не даёт сыграть Recovery дважды на одну серию —
	 *  не путать Recovery-анимацию (играет всегда после терминального удара) с State.ComboRecovery
	 *  (лок-аут только по MaxComboLength, вешается отдельно в ApplyComboRecovery). */
	void EndSequenceWithRecovery();

	const UComboFragment* GetActiveFragment() const;
	UAbilitySystemComponent* GetASC() const;
	UAnimInstance* GetAnimInstance() const;
};