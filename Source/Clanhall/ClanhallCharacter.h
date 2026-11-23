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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClanhallSelectedSlotChanged, FGameplayTag, Slot);

/** Клавиша выбора и слот, который она выбирает (`Controls.md`, «Ввод»). Слот - позиция панели,
 *  клавиша - что угодно из IMC: все клавиши переназначаются. */
USTRUCT(BlueprintType)
struct FClanhallSlotInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> Action;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (Categories = "Slot"))
	FGameplayTag Slot;
};

/** Клавиша оружия (как в песочнице симулятора: 1-4 - лоадаут, двойное 1 - парные мечи).
 *  Двойное нажатие той же клавиши в окне WeaponDoubleTapSeconds выбирает DoubleTapLoadoutIndex,
 *  если он валиден; -1 - двойного нажатия нет. */
USTRUCT(BlueprintType)
struct FClanhallWeaponInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> Action;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0"))
	int32 LoadoutIndex = 0;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "-1"))
	int32 DoubleTapLoadoutIndex = -1;
};

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

	// --- Ввод «клавиша выбирает, кнопка запускает» (`Controls.md`, «Ввод») ---

	/** Клавиши выбора: зажатая клавиша выбирает слот, ЛКМ запускает приём в нём, ПКМ - слог
	 *  (магия - P6). Раскладка по умолчанию - Q/E/R/F/1/2/3/4 → Slot.Q/E/R/F/Z/X/C/V. */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	TArray<FClanhallSlotInput> SlotInputs;

	/** ЛКМ - запуск приёма в выбранном слоте. Без выбранного слота ничего не делает. */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	TObjectPtr<UInputAction> LaunchAction;

	/** ПКМ - произнести слог выбранного слота. В P5 ничего не делает: магия - P6. */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	TObjectPtr<UInputAction> CastAction;

	/** Клавиши оружия: убрать то, что в руках, и выбрать новое (`WeaponSystem.md`, «Свап оружия»). */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	TArray<FClanhallWeaponInput> WeaponInputs;

	/** Индекс в WeaponInputs и время последнего нажатия клавиши оружия - для двойного нажатия. */
	int32 LastWeaponInputIndex = INDEX_NONE;
	double LastWeaponInputTime = -1.0;

	/** Зажатые сейчас слоты в порядке нажатия; выбран последний. */
	TArray<FGameplayTag> HeldSlots;

	/** Выбранный слот - последняя нажатая из зажатых клавиш выбора; невалиден - ничего не выбрано. */
	FGameplayTag SelectedSlot;

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

	/** Тело занято - движение и прыжок закрыты: начатый приём, потеря управления, смерть. */
	bool IsBodyBusy() const;

	/** Клавиша выбора нажата - её слот становится выбранным. */
	void OnSlotPressed(FGameplayTag Slot);

	/** Клавиша выбора отпущена - выбор переходит к последней из ещё зажатых, нет таких - выбора нет. */
	void OnSlotReleased(FGameplayTag Slot);

	/** ЛКМ - приём выбранного слота активного оружия; без выбора - ничего (`Controls.md`). */
	void OnLaunchPressed();

	/** ПКМ - слог; в P5 только Verbose-лог. */
	void OnCastPressed();

	/** Клавиша оружия нажата - выбор оружия лоадаута, двойное нажатие - DoubleTapLoadoutIndex. */
	void OnWeaponKeyPressed(int32 WeaponInputIndex);


	void SetSelectedSlot(FGameplayTag NewSlot);

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

	/** Скорость с учётом бега: на бегу база - SprintSpeed, иначе шаг. Оружие и метки - множители. */
	virtual void RefreshMovementSpeed() override;

	/** Слот, выбранный зажатой клавишей, - для подсветки на панели. */
	UFUNCTION(BlueprintPure, Category = "Input")
	FGameplayTag GetSelectedSlot() const { return SelectedSlot; }

	UPROPERTY(BlueprintAssignable, Category = "Input")
	FOnClanhallSelectedSlotChanged OnSelectedSlotChanged;

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
