// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"
#include "ClanhallCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UAbilitySystemComponent;
class UClanhallAttributeSet;
class UClanhallMarkComponent;
class UClanhallParryComponent;
class UAbilityData;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AClanhallCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	// --- GAS: ASC и AttributeSet живут прямо на Character (не на PlayerState) — см. technical_context.md ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallMarkComponent> MarkComponent;

	/** Раздел 5: состояние окна парирования — читается GA_EnemyWASDSeries, пишется обработчиками WASD. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallParryComponent> ParryComponent;

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
	FGameplayAbilitySpecHandle AttackOverheadHandle;
	FGameplayAbilitySpecHandle AttackRightSlashHandle;
	FGameplayAbilitySpecHandle AttackLeftSlashHandle;
	FGameplayAbilitySpecHandle AttackLowSweepHandle;

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

	/** DataAsset'ы Knight привязываются в Blueprint-наследнике персонажа (Content/...) */
	UPROPERTY(EditAnywhere, Category = "Combat|Knight")
	TObjectPtr<UAbilityData> KnightSkillQ_ShieldSlam;

	UPROPERTY(EditAnywhere, Category = "Combat|Knight")
	TObjectPtr<UAbilityData> KnightSkillE_PowerStrike;

	UPROPERTY(EditAnywhere, Category = "Combat|Knight")
	TObjectPtr<UAbilityData> KnightSkillR_ShieldCharge;

	UPROPERTY(EditAnywhere, Category = "Combat|Knight")
	TObjectPtr<UAbilityData> KnightSkillF_Retribution;

	FGameplayAbilitySpecHandle ActiveSkillQHandle;
	FGameplayAbilitySpecHandle ActiveSkillEHandle;
	FGameplayAbilitySpecHandle ActiveSkillRHandle;
	FGameplayAbilitySpecHandle ActiveSkillFHandle;

	// --- Раздел 6: контрнавык (LMB+Ctrl+<Q/E/R/F>) ---

	/** Ctrl — переключатель режима контрнавыка. Удерживается одновременно с LMB (стойкой). */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* CounterModeAction;

	/** Раздел 2 placeholder: настоящий выбор оружия появится в Разделе 10. Переключает тег Weapon.Type.STR/DEX на ASC. */
	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bStartWithSTRWeapon = true;

public:

	/** Constructor */
	AClanhallCharacter();

	// ~begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	// ~end IAbilitySystemInterface

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Initializes the ASC actor info and grants starting attribute values (hardcoded placeholders, see combat_system.md) */
	virtual void BeginPlay() override;

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

	void OnCounterModePressed();
	void OnCounterModeReleased();

	/** Ищет ближайшего врага в радиусе ближнего боя.
	 *  Если у него открыто State.CounterWindow → отменяет его активный Ability.Skill.* навык.
	 *  Возвращает true при успешном прерывании (окно открыто и навык отменён). */
	bool TryCounterNearestEnemy();

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

private:

	/** Раздел 6: Ctrl зажат → режим контрнавыка активен. Q/E/R/F пытается прервать врага. */
	bool bCounterModeActive = false;
};

