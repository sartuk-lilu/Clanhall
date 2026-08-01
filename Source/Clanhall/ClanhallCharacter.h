// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "ClanhallHumanoidCombatant.h"
#include "ClanhallCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UClanhallTargetingComponent;
class UClanhallBossSensorComponent;
class UAnimSequence;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AClanhallCharacter : public AClanhallHumanoidCombatant
{
	GENERATED_BODY()

	/** HUD: camera-forward line trace 20 м. CurrentTarget → Enemy Frame виджета.
	 *  OnTargetChanged — делегат для биндинга в WBP_HUD. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallTargetingComponent> TargetingComponent;

	/** HUD: держит Unit.Role.Boss.* юнитов в радиусе игрока, вещает OnFrameUnitEntered/Exited
	 *  для мульти-контейнера Enemy Frame (hud_dev_plan.md). Рамку водит этот
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

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	// --- Боевая стойка (combat_system.md §3): ЛКМ зажат = стойка, WASD = удары вместо движения ---

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

	// --- Раздел 4: активные навыки Knight (Q/E/R/F) через GA_PhysicalSkill + DataAsset ---

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

	/** Раздел 2 placeholder: настоящий выбор оружия появится в Разделе 10. Переключает тег Weapon.Type.STR/DEX на ASC. */
	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bStartWithSTRWeapon = true;

public:

	/** Constructor */
	AClanhallCharacter();

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Initializes the ASC actor info and grants starting attribute values (hardcoded placeholders, see combat_system.md) */
	virtual void BeginPlay() override;

	/** Прыжок запрещён, пока игрок в боевой стойке (State.InStance) — см. GA_CombatStance. */
	virtual bool CanJumpInternal_Implementation() const override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** ЛКМ нажат — активировать GA_CombatStance */
	void OnStancePressed();

	/** ЛКМ отпущен — CancelAbilityHandle на GA_CombatStance (мгновенный выход, см. combat_system.md §3) */
	void OnStanceReleased();

	void OnAttackOverhead();
	void OnAttackRightSlash();
	void OnAttackLeftSlash();
	void OnAttackLowSweep();

	void OnActiveSkillQ();
	void OnActiveSkillE();
	void OnActiveSkillR();
	void OnActiveSkillF();

public:

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
	 *  To Clanhall Character поверх неё, каст на AClanhallHumanoidCombatant делается внутри. nullptr, если
	 *  Character не этого класса или ComboData не назначен. Оставлена именно на этом классе (main_dev_plan.md
	 *  §8, Блок A) — функция BlueprintPure читает ABP игрока по имени класса, перенос сломал бы ноду в графе. */
	UFUNCTION(BlueprintPure, Category = "Combat|WASD")
	static UAnimSequence* GetStanceAnim(const ACharacter* Character);
};
