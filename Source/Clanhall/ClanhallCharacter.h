// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UAbilitySystemComponent;
class UClanhallAttributeSet;
class UClanhallMarkComponent;
class UClanhallParryComponent;
class UClanhallCounterComponent;
class UClanhallComboComponent;
class UClanhallWeaponTraceComponent;
class UClanhallTargetingComponent;
class UClanhallBossSensorComponent;
class UAbilityData;
class UComboData;
class UGA_DirectionalAttackBase;

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

	/** Раздел 5: флаг bParrySuccessful — пишет ClanhallWeaponTraceComponent, читает GA_EnemyWASDSeries. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallParryComponent> ParryComponent;

	/** Раздел 6 (переработан): окно контрнавыка на самом игроке — держит State.CounterWindow,
	 *  когда враг (в будущем — с AI-контром) мог бы прервать навык игрока. Симметричный компонент,
	 *  тот же класс висит на враге (clanhall_claude_code_counter.md). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallCounterComponent> CounterComponent;

	/** Ворота ввода, не буфер (combo_system_redesign.md): до открытия окна чтения ввод
	 *  отбрасывается целиком, в окне — "последнее решает". Сам владеет активацией — решает,
	 *  когда вызвать TryActivateAbility на GA_DirectionalAttack_* (инверсия потока, Часть B1),
	 *  играет per-path монтаж и терминальный Recovery-хвост серии. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallComboComponent> ComboComponent;

	/** Раздел 6.5: sweep-трейс оружия, открывается AnimNotify_WeaponTraceStart/End.
	 *  При попадании во врага со State.Parrying вызывает ParryComponent->TryParry(). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallWeaponTraceComponent> WeaponTraceComponent;

	/** HUD: camera-forward line trace 20 м. CurrentTarget → Enemy Frame виджета.
	 *  OnTargetChanged — делегат для биндинга в WBP_HUD. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallTargetingComponent> TargetingComponent;

	/** HUD: держит Unit.Role.Boss.* юнитов в радиусе игрока, вещает OnFrameUnitEntered/Exited
	 *  для мульти-контейнера Enemy Frame (changelog_enemyframe_unitroles.md §3). Рамку водит этот
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

	/** Blueprint-наследники позволяют переопределить TraceRange/TraceRadius и т.п. в редакторе.
	 *  Монтажи и урон — per-move в UComboData (MovesTable/профиль урона 4 поля), не на этих классах.
	 *  По умолчанию — соответствующий C++ класс (работает без Blueprint). */
	UPROPERTY(EditAnywhere, Category="Combat|WASD")
	TSubclassOf<UGA_DirectionalAttackBase> AttackOverheadClass;

	UPROPERTY(EditAnywhere, Category="Combat|WASD")
	TSubclassOf<UGA_DirectionalAttackBase> AttackRightSlashClass;

	UPROPERTY(EditAnywhere, Category="Combat|WASD")
	TSubclassOf<UGA_DirectionalAttackBase> AttackLeftSlashClass;

	UPROPERTY(EditAnywhere, Category="Combat|WASD")
	TSubclassOf<UGA_DirectionalAttackBase> AttackLowSweepClass;

	/** «Данные оружия» для комбо (combo_fragments_redesign_task.md): профиль урона, база ходов и
	 *  дерево цепочек одним ассетом — без фрагментов-обёрток (состав комбо-данных фиксирован, в
	 *  отличие от опциональных Fragments у UAbilityData). Назначается в Blueprint-наследнике
	 *  персонажа, по образцу KnightSkill*. */
	UPROPERTY(EditAnywhere, Category = "Combat|WASD")
	TObjectPtr<UComboData> ComboData;

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

	/** Данные комбо текущего оружия — читает UClanhallComboComponent через GetComboData(). */
	FORCEINLINE const UComboData* GetComboData() const { return ComboData; }

	/** Хэндл направленного удара по W/A/S/D — UClanhallComboComponent сам решает, когда его
	 *  активировать (combo_system_redesign.md, Часть B1: инверсия потока активации). */
	FGameplayAbilitySpecHandle GetAttackHandle(EClanhallAttackDirection Direction) const;

	// TODO: вынести в общий предок с классом врага, когда враги поедут на общий исполнитель
	// комбо-дерева (сейчас враги используют отдельный GA_EnemyWASDSeries, в этом резолве не
	// участвуют — combo_fragments_redesign_task.md, ответ на вопрос 3).

	/** Тег класса персонажа (Ability.Class.Knight и т.д.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|WASD", meta = (Categories = "Ability.Class"))
	FGameplayTag ClassTag;

	/** Потолок длины серии WASD-комбо (0-4). Плейсхолдер до системы прокачки —
	 *  combo_fragments_redesign_task.md, "Ранг / потолок длины комбо". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|WASD", meta = (ClampMin = "0", ClampMax = "4"))
	int32 ClassRank = 1;
};

