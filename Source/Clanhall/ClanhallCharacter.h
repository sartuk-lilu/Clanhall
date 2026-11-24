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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClanhallInputLayerChanged, EClanhallInputLayer, Layer);

/** Клавиша слота и её позиция (`Controls.md`, «Ввод»). Клавиша - позиция, слой - смысл: под ЛКМ
 *  она запускает приём этой позиции, под ПКМ вводит слог. Клавиша - что угодно из IMC: вторая
 *  раскладка - другие клавиши у тех же действий. */
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

	// --- Ввод удержанием кнопки мыши (`Controls.md`, «Ввод») ---

	/** Клавиши слотов: Q W E R / A S D F → Slot.Q/W/E/R/A/S/D/F. Что клавиша делает, решает слой. */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	TArray<FClanhallSlotInput> SlotInputs;

	/** ЛКМ (удержание) - слой тела: клавиши слотов запускают приёмы, появляется прицел. */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	TObjectPtr<UInputAction> BodyLayerAction;

	/** ПКМ (удержание) - слой рта: клавиши слотов вводят слоги, Пробел - выпуск, Ctrl - сброс. */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	TObjectPtr<UInputAction> MouthLayerAction;

	/** Клавиши оружия: убрать то, что в руках, и выбрать новое (`WeaponSystem.md`, «Свап оружия»). */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	TArray<FClanhallWeaponInput> WeaponInputs;

	/** Индекс в WeaponInputs и время последнего нажатия клавиши оружия - для двойного нажатия. */
	int32 LastWeaponInputIndex = INDEX_NONE;
	double LastWeaponInputTime = -1.0;

	/** Зажатые кнопки мыши в порядке нажатия; активный слой - последний (главная - последняя
	 *  зажатая). */
	TArray<EClanhallInputLayer> HeldLayers;

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

	// --- Пробел: уклонение «Пробел + направление» (`CombatSystem.md`, «Уклонение») ---

	/** Пробел - уклонение в сторону зажатого направления; без направления ничего. */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* SpaceAction;

	/** Прыжок - своя клавиша, не Пробел (`Controls.md`: уклонение на важной клавише, прыжок - нет). */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	TObjectPtr<UInputAction> JumpAction;

	/** Насмешка - общее действие на своей клавише (`CombatSystem.md`, «Насмешка»). */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	TObjectPtr<UInputAction> TauntAction;

	/** Применение кристалла - общее действие на своей клавише (`EconomySystem.md`, «Кристалл»). */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	TObjectPtr<UInputAction> CrystalAction;

	/** Вектор движения этого кадра (X - вправо, Y - вперёд) - из Move, обнуляется на отпускании.
	 *  Уклонение читает его: GetLastMovementInputVector обнуляется после потребления. */
	FVector2D CurrentMoveInput = FVector2D::ZeroVector;

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

	/** Переназначенные клавиши приёмов - в сохранение HUD (`Controls.md`, «Панель и клавиши приёмов»). */
	virtual void OnSkillSlotsAssigned() override;

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

	/** Клавиша слота нажата: под ЛКМ - приём этой позиции сразу, под ПКМ - слог, без слоя -
	 *  ничего (W A S D при этом двигают через Move). */
	void OnSlotKeyPressed(FGameplayTag Slot);

	/** Кнопка мыши зажата / отпущена - стек слоёв. */
	void OnLayerPressed(EClanhallInputLayer Layer);
	void OnLayerReleased(EClanhallInputLayer Layer);

	/** Активный слой сменился - вещать OnInputLayerChanged; вход в слой останавливает движение. */
	void HandleInputLayerChange(EClanhallInputLayer Before);

	/** Слог под ПКМ - заглушка до магии (P6): сообщение в LogClanhallCombat. */
	void OnSyllableKey(FGameplayTag Slot);

	/** ПКМ + Пробел - выпуск фразы, ПКМ + Ctrl - сброс фразы: заглушки до магии (P6). */
	void OnReleasePressed();
	void OnPhraseResetPressed();

	/** Клавиша оружия нажата - выбор оружия лоадаута, двойное нажатие - DoubleTapLoadoutIndex. */
	void OnWeaponKeyPressed(int32 WeaponInputIndex);


	/** Shift нажат - запускает бег. */
	void OnSprintPressed();

	/** Shift отпущен - гасит бег, если он был активен. */
	void OnSprintReleased();

	/** Пробел нажат: уклонение по доминирующей оси зажатого направления (W/S/A/D). */
	void OnSpacePressed();

	/** Движение отпущено - вектор ввода обнулён, Пробел без направления ничего не делает. */
	void OnMoveReleased();

	void OnJumpPressed();
	void OnJumpReleased();

	/** Насмешка - UGA_Taunt через хэндл гуманоида. */
	void OnTauntPressed();

	/** Кристалл - UGA_UseCrystal через хэндл базы. */
	void OnCrystalPressed();

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

	/** Активный слой ввода - для прицела (ЛКМ) и подсветки панели приёмов или слогов. */
	UFUNCTION(BlueprintPure, Category = "Input")
	EClanhallInputLayer GetInputLayer() const { return HeldLayers.Num() > 0 ? HeldLayers.Last() : EClanhallInputLayer::None; }

	UPROPERTY(BlueprintAssignable, Category = "Input")
	FOnClanhallInputLayerChanged OnInputLayerChanged;

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
