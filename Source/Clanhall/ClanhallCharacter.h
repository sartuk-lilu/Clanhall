// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "ClanhallHumanoidBase.h"
// Полное определение, не forward-declare: EClanhallEvadeDirection - тип параметра TriggerEvade.
#include "ClanhallCombatTypes.h"
#include "ClanhallCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UClanhallTargetingComponent;
class UClanhallBossSensorComponent;
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

	// --- Shift: бег (`CombatSystem.md`) ---

	/** Shift - бег. */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* SprintAction;

	// --- Пробел: прыжок / рывок (`CombatSystem.md`) ---

	/** Пробел - прыжок, либо (с зажатым Shift) длинный рывок вперёд. */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* SpaceAction;

	// --- Ctrl: присед (`CombatSystem.md`) ---

	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* DuckAction;

public:

	/** Constructor */
	AClanhallCharacter();

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Initializes the ASC actor info and grants starting attribute values (hardcoded placeholders, see CombatSystem.md).
	 *  Здесь же выставляется ротация и базовая скорость общей локомоции (`LocomotionStructure.md`) -
	 *  один раз, не в конструкторе: TurnRate это UPROPERTY, в конструкторе ещё не перезаписан
	 *  значением из BP. */
	virtual void BeginPlay() override;

	/** Считает доворот корпуса - см. TickBodyTurn. */
	virtual void Tick(float DeltaSeconds) override;

	/** Прыжок запрещён, пока доигрывает начатая активка (State.SkillCommitted). */
	virtual bool CanJumpInternal_Implementation() const override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Q/E/R/F - активка своего слота. Ввод «клавиша выбирает, кнопка запускает»
	 *  (`Controls.md`) приходит в P5. */
	void OnActiveSkillQ();
	void OnActiveSkillE();

	void OnActiveSkillR();
	void OnActiveSkillF();

	/** Shift нажат - запускает бег. */
	void OnSprintPressed();

	/** Shift отпущен - гасит бег, если он был активен. */
	void OnSprintReleased();

	/** Пробел нажат: на бегу - длинный рывок вперёд (TriggerEvade), иначе прыжок. */
	void OnSpacePressed();

	/** Пробел отпущен - StopJumping(). */
	void OnSpaceReleased();

	/** Ctrl нажат - активирует UGA_Duck. */
	void OnDuckPressed();

	void StartSprint();
	void StopSprint();

	/** Считает YawDelta между камерой и корпусом и по нему включает/выключает движковый
	 *  bUseControllerDesiredRotation (доворот) на CharacterMovementComponent. Доворот - общее
	 *  правило локомоции (`LocomotionStructure.md`). На бегу ротацией
	 *  заведует bOrientRotationToMovement, сюда тик не заходит. Стоя - по гистерезису
	 *  TurnThreshold/TurnSettleAngle, со взводом bTurningInPlace для ABP (подшаг); двигаясь
	 *  (не бегом) доворот идёт постоянно, без порога. */
	void TickBodyTurn();

public:

	/** Активирует DodgeAbilityHandle событием, с направлением в EventMagnitude - см.
	 *  UGA_Dodge::ActivateAbility (`CombatSystem.md`). */
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
};
