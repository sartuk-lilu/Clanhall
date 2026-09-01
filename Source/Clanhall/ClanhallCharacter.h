// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "ClanhallHumanoidBase.h"
// Полное определение, не forward-declare: EClanhallInputMode - возвращаемый тип
// UFUNCTION(BlueprintPure) ниже, UHT для рефлексируемых типов требует видеть определение,
// не полагаться на то, что оно придёт транзитивно через ClanhallHumanoidBase.h.
#include "ClanhallCombatTypes.h"
#include "ClanhallCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UClanhallTargetingComponent;
class UClanhallBossSensorComponent;
class UAnimSequence;
class UGA_Dodge;
class UGA_Duck;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AClanhallCharacter : public AClanhallHumanoidBase
{
	GENERATED_BODY()

	/** HUD: camera-forward line trace 20 м. CurrentTarget → Enemy Frame виджета.
	 *  OnTargetChanged — делегат для биндинга в WBP_HUD. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallTargetingComponent> TargetingComponent;

	/** HUD: держит Unit.Role.Boss.* юнитов в радиусе игрока, вещает OnFrameUnitEntered/Exited
	 *  для мульти-контейнера Enemy Frame (`HUD.md`). Рамку водит этот
	 *  компонент, а не TargetingComponent — тот остаётся мягкой целью под удар/метку. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallBossSensorComponent> BossSensorComponent;

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

protected:

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	// --- Боевая стойка (`combat_system.md`): ЛКМ зажат = стойка, WASD = удары вместо движения ---

	/** ЛКМ — вход/выход из боевой стойки */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* StanceAction;

	/** W в стойке — Overhead */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* AttackOverheadAction;

	/** D в стойке — Right Slash */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* AttackRightSlashAction;

	/** A в стойке — Left Slash */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* AttackLeftSlashAction;

	/** S в стойке — Low Sweep */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* AttackLowSweepAction;

	FGameplayAbilitySpecHandle StanceAbilityHandle;

	// --- Активные навыки Knight (Q/E/R/F) через GA_PhysicalSkill + DataAsset ---

	/** Q — Shield Slam */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* ActiveSkillQAction;

	/** E — Power Strike */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* ActiveSkillEAction;

	/** R — Shield Charge */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* ActiveSkillRAction;

	/** F — Retribution */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* ActiveSkillFAction;

	/** Класс отскока/ухода, гранится в BeginPlay вместо UGA_Dodge::StaticClass() напрямую -
	 *  только так его EditDefaultsOnly-поля (дистанции, монтажи) открываются в редакторе: у
	 *  C++-класса без Blueprint-наследника их негде править. Дефолт - сам C++-класс, так что
	 *  без Blueprint-наследника всё продолжает работать на дефолтах кода. */
	UPROPERTY(EditDefaultsOnly, Category = "Input|Combat")
	TSubclassOf<UGA_Dodge> DodgeAbilityClass;

	FGameplayAbilitySpecHandle DodgeAbilityHandle;

	/** Класс приседа, тем же приёмом, что DodgeAbilityClass - см. комментарий там. */
	UPROPERTY(EditDefaultsOnly, Category = "Input|Combat")
	TSubclassOf<UGA_Duck> DuckAbilityClass;

	FGameplayAbilitySpecHandle DuckAbilityHandle;

	// --- Shift: бег (`combat_system.md`) ---

	/** Shift - бег в режиме защиты (ничего не зажато). В режиме атаки не делает ничего:
	 *  стойка статична. */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* SprintAction;

	/** Физическое состояние клавиши Shift, независимо от режима и State.Sprinting. Started
	 *  на SprintAction приходит один раз на нажатие - если Shift зажат ещё со входа в стойку
	 *  (CancelSprint снял тег на входе), при выходе из стойки новый Started не придёт, и без
	 *  этого флага игрок "залипал" бы на ходьбе до перенажатия Shift. OnStanceReleased
	 *  перепроверяет флаг и запускает бег сам. */
	bool bSprintKeyHeld = false;

	// --- Пробел: прыжок / рывок (`combat_system.md`) ---

	/** Пробел - в режиме защиты: прыжок, либо (с зажатым Shift) длинный рывок вперёд.
	 *  В режиме атаки не делает ничего - защиты в стойке нет. */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* SpaceAction;

	// --- Ctrl: присед (`combat_system.md`) ---

	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* DuckAction;

public:

	/** Constructor */
	AClanhallCharacter();

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Initializes the ASC actor info and grants starting attribute values (hardcoded placeholders, see combat_system.md).
	 *  Здесь же выставляется ротация и базовая скорость общей локомоции (`locomotion_structure.md`) -
	 *  один раз, не в конструкторе: TurnRate это UPROPERTY, в конструкторе ещё не перезаписан
	 *  значением из BP. */
	virtual void BeginPlay() override;

	/** Считает доворот корпуса - см. TickBodyTurn. */
	virtual void Tick(float DeltaSeconds) override;

	/** Прыжок запрещён, пока игрок в боевой стойке (State.InStance) — см. GA_CombatStance. */
	virtual bool CanJumpInternal_Implementation() const override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** ЛКМ нажат — активировать GA_CombatStance */
	void OnStancePressed();

	/** ЛКМ отпущен - CancelAbilityHandle на GA_CombatStance (мгновенный выход, см. `combat_system.md`) */
	void OnStanceReleased();

	void OnAttackOverhead();
	void OnAttackRightSlash();
	void OnAttackLeftSlash();
	void OnAttackLowSweep();

	/** Q - вне стойки уход влево (TriggerEvade), в стойке активка слота Q. */
	void OnActiveSkillQ();

	/** E - вне стойки уход вправо (TriggerEvade), в стойке активка слота E. */
	void OnActiveSkillE();

	void OnActiveSkillR();
	void OnActiveSkillF();

	/** Shift нажат - в режиме защиты запускает бег. В режиме атаки не делает ничего:
	 *  стойка статична, бежать можно только отпустив ЛКМ (`combat_system.md`). */
	void OnSprintPressed();

	/** Shift отпущен - гасит бег, если он был активен. */
	void OnSprintReleased();

	/** Пробел нажат. В режиме защиты: на бегу - длинный рывок вперёд (TriggerEvade), иначе
	 *  прыжок. В режиме атаки не делает ничего - защиты в стойке нет (`combat_system.md`). */
	void OnSpacePressed();

	/** Пробел отпущен - StopJumping(). */
	void OnSpaceReleased();

	/** Ctrl нажат - активирует UGA_Duck. Только в режиме защиты. */
	void OnDuckPressed();

	void StartSprint();
	void StopSprint();

	/** Считает YawDelta между камерой и корпусом и по нему включает/выключает движковый
	 *  bUseControllerDesiredRotation (доворот) на CharacterMovementComponent. Доворот - общее
	 *  правило локомоции, не привилегия стойки (`locomotion_structure.md`). На бегу ротацией
	 *  заведует bOrientRotationToMovement, сюда тик не заходит. Стоя - по гистерезису
	 *  TurnThreshold/TurnSettleAngle, со взводом bTurningInPlace для ABP (подшаг); двигаясь
	 *  (не в стойке, не бегом) доворот идёт постоянно, без порога. */
	void TickBodyTurn();

public:

	/** Гасит бег - вызывается из UGA_CombatStance::ActivateAbility при входе в стойку с зажатым
	 *  Shift: скорость стойки обязана победить скорость бега. */
	void CancelSprint();

	/** Режим ввода текущего кадра - см. EClanhallInputMode. */
	UFUNCTION(BlueprintPure, Category = "Input")
	EClanhallInputMode GetInputMode() const;

	/** Активирует DodgeAbilityHandle событием, с направлением в EventMagnitude - см.
	 *  UGA_Dodge::ActivateAbility (`combat_system.md`). */
	void TriggerEvade(EClanhallEvadeDirection Direction);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Loop-поза боевой стойки текущего оружия (UComboData::StanceAnim). Статичная и берёт ACharacter,
	 *  а не член AClanhallCharacter: в Event Blueprint Update Animation обычно уже есть закэшированная
	 *  и провалидированная (IsValid) переменная Character как ACharacter — так не нужен второй Cast
	 *  To Clanhall Character поверх неё, каст на AClanhallHumanoidBase делается внутри. nullptr, если
	 *  Character не этого класса или ComboData не назначен. Оставлена именно на этом классе —
	 *  функция BlueprintPure читает ABP игрока по имени класса, перенос сломал бы ноду в графе. */
	UFUNCTION(BlueprintPure, Category = "Combat|WASD")
	static UAnimSequence* GetStanceAnim(const ACharacter* Character);
};
