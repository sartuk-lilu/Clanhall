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
class UBlendSpace;
class UGA_Dodge;

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

	// --- Боевая стойка (`combat_system.md`, «Боевая стойка и переключение режимов»): ЛКМ зажат = стойка, WASD = удары вместо движения ---

	/** ЛКМ — вход/выход из боевой стойки */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* StanceAction;

	/** Shift в стойке — переключает WASD с ударов на перемещение (`combat_system.md`,
	 *  «Боевая стойка и переключение режимов»). */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* StanceMoveModifierAction;

	/** Shift зажат — WASD в стойке перемещает, а не бьёт. Не сбрасывает живую серию
	 *  (`combat_system.md`: «Shift живую серию не сбрасывает») — гейтит только приём новых
	 *  ударных нажатий в OnAttack*, открытое окно ComboComponent не трогаем и не гасим. */
	bool bStanceMoveHeld = false;

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

	/** Класс отскока, гранится в BeginPlay вместо UGA_Dodge::StaticClass() напрямую — только так
	 *  его EditDefaultsOnly-поля (дистанции, монтажи) открываются в редакторе: у C++-класса без
	 *  Blueprint-наследника их негде править. Дефолт — сам C++-класс, так что без Blueprint-
	 *  наследника всё продолжает работать на дефолтах кода. */
	UPROPERTY(EditDefaultsOnly, Category = "Input|Combat")
	TSubclassOf<UGA_Dodge> DodgeAbilityClass;

	// --- Пробел: отскок / прыжок / бег (`combat_system.md`, «Отскок») ---
	// Разведение тапа/двойного тапа/удержания живёт в C++ на этом классе, не тремя триггерами
	// Enhanced Input на одну клавишу: те сработали бы независимо, и одиночный тап внутри
	// двойного выстрелил бы отскоком до прыжка.

	/** Пробел — единственный бинд на весь функционал ниже. Заменяет старый JumpAction:
	 *  прыжок теперь тоже вызывается из этой логики (двойной тап вне стойки). */
	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* SpaceAction;

	/** Вне стойки: удержание дольше этого — бег, короче — тап (ждёт второй на DoubleTapWindow).
	 *  Плейсхолдер. */
	UPROPERTY(EditAnywhere, Category = "Combat|Dodge")
	float HoldThreshold = 0.25f;

	/** Вне стойки: окно ожидания второго тапа после первого — пришёл вовремя, значит прыжок,
	 *  не пришёл — отскок. Единственное осознанное ожидание в системе, и оно живёт только вне
	 *  стойки: в стойке у Пробела нет альтернатив, отскок стреляет на Started без ожидания.
	 *  Плейсхолдер. */
	UPROPERTY(EditAnywhere, Category = "Combat|Dodge")
	float DoubleTapWindow = 0.25f;

	FGameplayAbilitySpecHandle DodgeAbilityHandle;

	/** Ждём второй Started в окне DoubleTapWindow — первый тап уже случился и не был удержанием. */
	bool bSpaceAwaitingDoubleTap = false;

	/** Второй Started двойного тапа уже вызвал Jump() — его парное Completed не должно
	 *  провалиться в ветку "это тап" и завести отскок через DoubleTapWindow. Снимается первым
	 *  делом в OnSpaceReleased. */
	bool bSpaceJumpConsumed = false;

	/** Бег активен удержанием Пробела вне стойки. */
	bool bSpaceSprinting = false;

	/** MaxWalkSpeed до начала бега — возвращается по Completed, не константой (та же причина,
	 *  что в UGA_CombatStance::EndAbility: MaxWalkSpeed вне стойки задаётся в BP-персонаже). */
	float SavedWalkSpeedBeforeSprint = 0.0f;

	FTimerHandle SpaceHoldTimerHandle;
	FTimerHandle SpaceDoubleTapTimerHandle;

public:

	/** Constructor */
	AClanhallCharacter();

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Initializes the ASC actor info and grants starting attribute values (hardcoded placeholders, see combat_system.md) */
	virtual void BeginPlay() override;

	/** Считает доворот корпуса в боевой стойке — см. TickStanceTurn. */
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

	/** ЛКМ отпущен — CancelAbilityHandle на GA_CombatStance (мгновенный выход, см. `combat_system.md`, «Боевая стойка и переключение режимов») */
	void OnStanceReleased();

	/** Shift нажат — WASD в стойке переключается на перемещение. */
	void OnStanceMoveModifierPressed();

	/** Shift отпущен — WASD в стойке снова бьёт. Открытое окно продолжения серии не трогаем:
	 *  оно дотикивает своим таймером и закрывается само (`combat_system.md`). */
	void OnStanceMoveModifierReleased();

	void OnAttackOverhead();
	void OnAttackRightSlash();
	void OnAttackLeftSlash();
	void OnAttackLowSweep();

	void OnActiveSkillQ();
	void OnActiveSkillE();
	void OnActiveSkillR();
	void OnActiveSkillF();

	/** Пробел нажат. В стойке — отскок сразу, без ожидания (`combat_system.md`: исключение
	 *  из приоритета отзывчивости существует только вне стойки). Вне стойки: второй тап
	 *  в открытом окне DoubleTapWindow — прыжок; иначе взводит таймер HoldThreshold. */
	void OnSpacePressed();

	/** Пробел отпущен. Если удержание уже перешло в бег — гасит бег. Если нет — это был тап:
	 *  открывает окно ожидания второго Started. */
	void OnSpaceReleased();

	/** HoldThreshold истёк без Completed — превращает удержание в бег. */
	void OnSpaceHoldThresholdReached();

	/** DoubleTapWindow истёк без второго Started — тап был одиночным, это дальний отскок. */
	void OnSpaceDoubleTapWindowExpired();

	void StartSprint();
	void StopSprint();

	/** Считает YawDelta между камерой и корпусом и по нему включает/выключает движковый
	 *  bUseControllerDesiredRotation (доворот) на CharacterMovementComponent, только пока висит
	 *  State.InStance (`locomotion_structure.md`, «Локомоция стойки»). В движении
	 *  (Shift + WASD) доворот идёт постоянно, без порога; стоя — по гистерезису
	 *  StanceTurnThreshold/StanceTurnSettleAngle, со взводом bStanceTurning для ABP (подшаг). */
	void TickStanceTurn();

public:

	/** Гасит бег и все таймеры/флаги Пробела — вызывается ровно один раз на реальный вход
	 *  в стойку, из UGA_CombatStance::ActivateAbility, а не из OnStancePressed: тот выполняется
	 *  каждый кадр удержания ЛКМ (ретрай на Triggered при State.ComboRecovery) и на активной
	 *  Recovery убивал бы таймер Пробела кадром позже, до входа в стойку (`task_stage4_code_fixes.md`,
	 *  п.5). Порядок «сначала погасить бег, потом прочитать MaxWalkSpeed» обязан сохраняться —
	 *  вызывать в самом начале ActivateAbility, до сохранения текущих значений движения. */
	void CancelSpaceHoldAndSprint();

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
	 *  Character не этого класса или ComboData не назначен. Оставлена именно на этом классе —
	 *  функция BlueprintPure читает ABP игрока по имени класса, перенос сломал бы ноду в графе. */
	UFUNCTION(BlueprintPure, Category = "Combat|WASD")
	static UAnimSequence* GetStanceAnim(const ACharacter* Character);

	/** BlendSpace локомоции стойки текущего оружия (UComboData::StanceLocomotion). Точная копия
	 *  шаблона GetStanceAnim — статичная, берёт ACharacter, каст внутри, по той же причине:
	 *  нода читается в ABP по имени класса, переносить нельзя. nullptr, если Character не этого
	 *  класса, ComboData не назначен, или StanceLocomotion не задан — тогда ABP играет
	 *  StanceAnim как раньше. */
	UFUNCTION(BlueprintPure, Category = "Combat|WASD")
	static UBlendSpace* GetStanceBlendSpace(const ACharacter* Character);
};
