// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClanhallCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Clanhall.h"
#include "ClanhallCombatTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystem/Abilities/GA_CombatStance.h"
#include "AbilitySystem/Abilities/GA_Dodge.h"
#include "AbilitySystem/Fragments/ComboData.h"
#include "AbilitySystem/ClanhallComboComponent.h"
#include "AbilitySystem/ClanhallTargetingComponent.h"
#include "AbilitySystem/ClanhallBossSensorComponent.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

AClanhallCharacter::AClanhallCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// HUD: camera line trace, мягкая цель под удар/метку (Enemy Frame больше не водит).
	TargetingComponent = CreateDefaultSubobject<UClanhallTargetingComponent>(TEXT("TargetingComponent"));
	// HUD: радиус + Unit.Role.Boss — драйвер Enemy Frame (`HUD.md`).
	BossSensorComponent = CreateDefaultSubobject<UClanhallBossSensorComponent>(TEXT("BossSensorComponent"));

	// WASD-классы дефолтятся в AClanhallHumanoidCombatant — общий конструктор для игрока
	// и AClanhallHumanoidBoss (`Combatant Hierarchy.md`, «Три слоя»). Здесь их больше нет намеренно:
	// у пустого конструктора AClanhallHumanoidBoss эти поля оставались бы nullptr.
}

void AClanhallCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Стартовые значения ресурсов инициализирует AClanhallCombatantBase::BeginPlay (Default*
	// поля) — общий путь для игрока и AI (`combat_system.md`, «Ресурсы персонажа»).

	if (AbilitySystemComponent)
	{
		// Плейсхолдер категорий STR/DEX (тег Weapon.Type.*) снесён вместе с остатком шкалы
		// Balance (CLAUDE.md, «Идёт переработка боевой системы») — доход зарядов и потолок
		// серии теперь читаются с UWeaponTypeData через CharacterSheet -> Weapon -> Type
		// (`weapon_system.md`).

		// Грант способности боевой стойки (`combat_system.md`, «Боевая стойка и переключение режимов»). WASD-удары и активки Q/E/R/F
		// гранятся выше по иерархии — см. AClanhallHumanoidCombatant::BeginPlay.
		StanceAbilityHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGA_CombatStance::StaticClass(), 1, INDEX_NONE, this));

		// Грант отскока (`combat_system.md`, «Отскок») — рядом со стойкой. У противников
		// отскока пока нет: стойки как способности у них тоже нет.
		DodgeAbilityHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGA_Dodge::StaticClass(), 1, INDEX_NONE, this));
	}
}

bool AClanhallCharacter::CanJumpInternal_Implementation() const
{
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(ClanhallGameplayTags::State_InStance.GetTag()))
	{
		return false;
	}

	// Fullbody-активка занимает всё тело, включая ноги — та же причина, что в DoMove:
	// иначе стойка отпущена, прыжок разрешён, персонаж подпрыгивает посреди рывка.
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(ClanhallGameplayTags::State_SkillCommitted.GetTag()))
	{
		return false;
	}

	return Super::CanJumpInternal_Implementation();
}

void AClanhallCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AClanhallCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AClanhallCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AClanhallCharacter::Look);

		// Combat stance (`combat_system.md`, «Боевая стойка и переключение режимов»): ЛКМ зажат/отпущен
		EnhancedInputComponent->BindAction(StanceAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnStancePressed);
		// Ретрай: Started может прийти во время State.ComboRecovery и быть отклонён. Triggered
		// повторяет попытку каждый кадр удержания, поэтому по истечении лока персонаж войдёт в
		// стойку сам, без повторного клика. Повторный вызов при уже активной стойке — дешёвый
		// отказ по ActivationBlockedTags(State.InStance).
		EnhancedInputComponent->BindAction(StanceAction, ETriggerEvent::Triggered, this, &AClanhallCharacter::OnStancePressed);
		EnhancedInputComponent->BindAction(StanceAction, ETriggerEvent::Completed, this, &AClanhallCharacter::OnStanceReleased);
		EnhancedInputComponent->BindAction(StanceAction, ETriggerEvent::Canceled, this, &AClanhallCharacter::OnStanceReleased);

		// Shift + WASD в стойке = перемещение (`combat_system.md`, «Боевая стойка и переключение режимов»).
		EnhancedInputComponent->BindAction(StanceMoveModifierAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnStanceMoveModifierPressed);
		EnhancedInputComponent->BindAction(StanceMoveModifierAction, ETriggerEvent::Completed, this, &AClanhallCharacter::OnStanceMoveModifierReleased);
		EnhancedInputComponent->BindAction(StanceMoveModifierAction, ETriggerEvent::Canceled, this, &AClanhallCharacter::OnStanceMoveModifierReleased);

		// Directional WASD-attacks (`combat_system.md`, «Направления атаки (WASD)») — те же клавиши, что и Move,
		// но отдельные дискретные действия: срабатывают один раз на нажатие, а не каждый кадр.
		// GA_DirectionalAttackBase сам отказывает, если игрок не в стойке (ActivationRequiredTags).
		EnhancedInputComponent->BindAction(AttackOverheadAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnAttackOverhead);
		EnhancedInputComponent->BindAction(AttackRightSlashAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnAttackRightSlash);
		EnhancedInputComponent->BindAction(AttackLeftSlashAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnAttackLeftSlash);
		EnhancedInputComponent->BindAction(AttackLowSweepAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnAttackLowSweep);

		// Активные навыки Knight (GA_PhysicalSkill) — Q/E/R/F.
		EnhancedInputComponent->BindAction(ActiveSkillQAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnActiveSkillQ);
		EnhancedInputComponent->BindAction(ActiveSkillEAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnActiveSkillE);
		EnhancedInputComponent->BindAction(ActiveSkillRAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnActiveSkillR);
		EnhancedInputComponent->BindAction(ActiveSkillFAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnActiveSkillF);

		// Пробел: тап/двойной тап/удержание разводятся в C++ на этом классе, не тремя триггерами
		// Enhanced Input на одну клавишу (`combat_system.md`, «Отскок»). Заменяет старый JumpAction.
		EnhancedInputComponent->BindAction(SpaceAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnSpacePressed);
		EnhancedInputComponent->BindAction(SpaceAction, ETriggerEvent::Completed, this, &AClanhallCharacter::OnSpaceReleased);
		EnhancedInputComponent->BindAction(SpaceAction, ETriggerEvent::Canceled, this, &AClanhallCharacter::OnSpaceReleased);
	}
	else
	{
		UE_LOG(LogClanhall, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AClanhallCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AClanhallCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AClanhallCharacter::DoMove(float Right, float Forward)
{
	// В боевой стойке WASD = направление удара, а не движение (`combat_system.md`, «Боевая стойка и переключение режимов», «Направления атаки (WASD)»).
	// Стойка наземная: тег State.InStance может висеть и в воздухе (держим ЛКМ при прыжке/падении),
	// поэтому здесь дополнительно спрашиваем IsFalling() — тем же предикатом, что гейтит позу
	// стойки в ABP. В воздухе air control не режем; тег сам "включит" стойку в кадре приземления.
	// Shift + WASD в стойке — исключение: перемещение, не удары (`combat_system.md`,
	// «Боевая стойка и переключение режимов», «Локомоция в стойке»).
	if (AbilitySystemComponent
		&& AbilitySystemComponent->HasMatchingGameplayTag(ClanhallGameplayTags::State_InStance.GetTag())
		&& !bStanceMoveHeld
		&& GetCharacterMovement() && !GetCharacterMovement()->IsFalling())
	{
		return;
	}

	// Активка занимает слот fullbody целиком — свободных ног нет, в отличие от upperbody
	// (WASD-удары, Recovery-хвосты), откуда убежать посреди доигрывания законно. State.SkillCommitted
	// висит весь каст-монтаж (и рывок) и снимается только в EndAbility — не гейтить по
	// State.ComboRecovery, тот про upperbody-хвост и сюда не относится.
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(ClanhallGameplayTags::State_SkillCommitted.GetTag()))
	{
		return;
	}

	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AClanhallCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AClanhallCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AClanhallCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AClanhallCharacter::OnStancePressed()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(StanceAbilityHandle);
	}
}

void AClanhallCharacter::OnStanceReleased()
{
	if (AbilitySystemComponent)
	{
		// (`combat_system.md`, «Боевая стойка и переключение режимов»): "Отпустить LMB в любой момент = мгновенный выход из стойки".
		AbilitySystemComponent->CancelAbilityHandle(StanceAbilityHandle);
	}

	// Выход из стойки — всегда, вне ворот. Останавливает активный
	// монтаж комбо с blend-out и сбрасывает последовательность независимо от фазы.
	if (ComboComponent)
	{
		ComboComponent->OnStanceExit();
	}
}

// (`Combat Stance and WASD Attacks.md`): WASD больше не активирует направленный удар напрямую —
// решение (опенер / продолжение по данным дерева / мусор вне окна) целиком у ComboComponent,
// он же сам вызывает TryActivateAbility, когда ввод валиден. Парирование обрабатывает
// UClanhallHitboxComponent при хите врага зоной с bParryable == true (State.Parrying на ASC
// врага, не игрока).

void AClanhallCharacter::OnStanceMoveModifierPressed()
{
	bStanceMoveHeld = true;
}

void AClanhallCharacter::OnStanceMoveModifierReleased()
{
	bStanceMoveHeld = false;
}

void AClanhallCharacter::OnSpacePressed()
{
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(ClanhallGameplayTags::State_InStance.GetTag()))
	{
		// В стойке у Пробела нет альтернатив — прыжок в стойке запрещён, перемещение это
		// Shift+WASD. Короткий отскок стреляет прямо на Started, без ожидания второго тапа
		// (`combat_system.md`: исключение из приоритета отзывчивости существует только вне стойки).
		AbilitySystemComponent->TryActivateAbility(DodgeAbilityHandle);
		return;
	}

	if (bSpaceAwaitingDoubleTap)
	{
		// Второй Started в открытом окне — прыжок. Помечаем парное Completed как потраченное:
		// без этого оно провалилось бы в ветку "это тап" в OnSpaceReleased и через
		// DoubleTapWindow завело бы лишний отскок на каждый прыжок (`task_stage4_code_fixes.md`, п.1).
		GetWorldTimerManager().ClearTimer(SpaceDoubleTapTimerHandle);
		bSpaceAwaitingDoubleTap = false;
		bSpaceJumpConsumed = true;
		Jump();
		return;
	}

	// Первое нажатие вне стойки — не знаем ещё, тап это или начало удержания.
	GetWorldTimerManager().SetTimer(SpaceHoldTimerHandle, this, &AClanhallCharacter::OnSpaceHoldThresholdReached, HoldThreshold, false);
}

void AClanhallCharacter::OnSpaceReleased()
{
	if (bSpaceJumpConsumed)
	{
		// Это Completed — парное ко второму Started двойного тапа, прыжок уже случился
		// в OnSpacePressed. Гасим флаг и выходим первым делом, до любых других веток.
		bSpaceJumpConsumed = false;
		return;
	}

	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(ClanhallGameplayTags::State_InStance.GetTag()))
	{
		// Короткий отскок уже случился на Started — Completed в стойке ничего не делает.
		return;
	}

	if (bSpaceSprinting)
	{
		StopSprint();
		bSpaceSprinting = false;
		return;
	}

	// Отпустили раньше HoldThreshold — это тап: ждём второй Started в окне DoubleTapWindow.
	// Пришёл — прыжок (см. OnSpacePressed), не пришёл — дальний отскок (см. OnSpaceDoubleTapWindowExpired).
	GetWorldTimerManager().ClearTimer(SpaceHoldTimerHandle);
	bSpaceAwaitingDoubleTap = true;
	GetWorldTimerManager().SetTimer(SpaceDoubleTapTimerHandle, this, &AClanhallCharacter::OnSpaceDoubleTapWindowExpired, DoubleTapWindow, false);
}

void AClanhallCharacter::OnSpaceHoldThresholdReached()
{
	bSpaceSprinting = true;
	StartSprint();
}

void AClanhallCharacter::OnSpaceDoubleTapWindowExpired()
{
	bSpaceAwaitingDoubleTap = false;
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(DodgeAbilityHandle);
	}
}

void AClanhallCharacter::StartSprint()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		// Сохранять и восстанавливать, а не писать константой — та же причина, что
		// в UGA_CombatStance::EndAbility: MaxWalkSpeed вне стойки задаётся в BP-персонаже.
		SavedWalkSpeedBeforeSprint = Movement->MaxWalkSpeed;
		Movement->MaxWalkSpeed = GetSprintSpeed();
	}
}

void AClanhallCharacter::StopSprint()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = SavedWalkSpeedBeforeSprint;
	}
}

void AClanhallCharacter::CancelSpaceHoldAndSprint()
{
	// Бег вообще не должен пережить вход в стойку — скорость стойки обязана победить скорость
	// бега (`combat_system.md`, «Отскок», блок «Бег»). Вызывается из
	// UGA_CombatStance::ActivateAbility ДО чтения текущего MaxWalkSpeed: та сохраняет его для
	// восстановления на выходе, и если не остановить бег первым, стойка на выходе вернула бы
	// скорость бега, а не настоящую базовую.
	if (bSpaceSprinting)
	{
		StopSprint();
		bSpaceSprinting = false;
	}
	GetWorldTimerManager().ClearTimer(SpaceHoldTimerHandle);
	GetWorldTimerManager().ClearTimer(SpaceDoubleTapTimerHandle);
	bSpaceAwaitingDoubleTap = false;
	bSpaceJumpConsumed = false;
}

void AClanhallCharacter::OnAttackOverhead()
{
	// Shift зажат — этот WASD-ввод перемещает (DoMove), а не бьёт. Гейт стоит здесь, а не
	// в UClanhallComboComponent::HandleAttackInput: Shift — ввод игрока, компонент
	// стороне-нейтрален и одинаково обслуживает игрока и AI, которому Shift не существует.
	if (bStanceMoveHeld)
	{
		return;
	}
	if (ComboComponent)
	{
		ComboComponent->HandleAttackInput(EClanhallAttackDirection::Overhead);
	}
}

void AClanhallCharacter::OnAttackRightSlash()
{
	if (bStanceMoveHeld)
	{
		return;
	}
	if (ComboComponent)
	{
		ComboComponent->HandleAttackInput(EClanhallAttackDirection::RightSlash);
	}
}

void AClanhallCharacter::OnAttackLeftSlash()
{
	if (bStanceMoveHeld)
	{
		return;
	}
	if (ComboComponent)
	{
		ComboComponent->HandleAttackInput(EClanhallAttackDirection::LeftSlash);
	}
}

void AClanhallCharacter::OnAttackLowSweep()
{
	if (bStanceMoveHeld)
	{
		return;
	}
	if (ComboComponent)
	{
		ComboComponent->HandleAttackInput(EClanhallAttackDirection::LowSweep);
	}
}

UAnimSequence* AClanhallCharacter::GetStanceAnim(const ACharacter* Character)
{
	const AClanhallHumanoidCombatant* Combatant = Cast<AClanhallHumanoidCombatant>(Character);
	const UComboData* Data = Combatant ? Combatant->GetComboData() : nullptr;
	return Data ? Data->StanceAnim : nullptr;
}

// ---------------------------------------------------------------------------
// Активные навыки (Q/E/R/F). Контрнавык (`ability_system.md`, «Контрнавык») больше не требует
// модификатора — распознаётся резолвером внутри GA_PhysicalSkill::ActivateAbility
// по совпадению CounterTag с открытым окном цели.
// ---------------------------------------------------------------------------

void AClanhallCharacter::OnActiveSkillQ()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(GetActiveSkillHandle(ClanhallGameplayTags::Ability_Slot_Q.GetTag()));
	}
}

void AClanhallCharacter::OnActiveSkillE()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(GetActiveSkillHandle(ClanhallGameplayTags::Ability_Slot_E.GetTag()));
	}
}

void AClanhallCharacter::OnActiveSkillR()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(GetActiveSkillHandle(ClanhallGameplayTags::Ability_Slot_R.GetTag()));
	}
}

void AClanhallCharacter::OnActiveSkillF()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(GetActiveSkillHandle(ClanhallGameplayTags::Ability_Slot_F.GetTag()));
	}
}
