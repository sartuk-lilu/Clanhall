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
#include "AbilitySystemInterface.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystem/Abilities/GA_CombatStance.h"
#include "AbilitySystem/Abilities/GA_DirectionalAttacks.h"
#include "AbilitySystem/Abilities/GA_PhysicalSkill.h"
#include "AbilitySystem/AbilityData.h"
#include "AbilitySystem/Effects/GE_BalanceDrift.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/ClanhallParryComponent.h"
#include "AbilitySystem/ClanhallCounterComponent.h"
#include "AbilitySystem/ClanhallComboComponent.h"
#include "AbilitySystem/ClanhallWeaponTraceComponent.h"
#include "AbilitySystem/ClanhallTargetingComponent.h"
#include "AbilitySystem/ClanhallBossSensorComponent.h"
#include "Engine/Engine.h"

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

	// GAS: ASC живёт прямо на Character, владелец и аватар — один и тот же актор (см. technical_context.md)
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UClanhallAttributeSet>(TEXT("AttributeSet"));

	// Метка персонажа — независимый трек (mark_system.md §3/§5), не связан с тем, что игрок
	// сам кладёт на врагов. См. UClanhallMarkComponent.
	MarkComponent = CreateDefaultSubobject<UClanhallMarkComponent>(TEXT("MarkComponent"));
	ParryComponent = CreateDefaultSubobject<UClanhallParryComponent>(TEXT("ParryComponent"));
	// Раздел 6 (переработан): симметричный компонент окна контрнавыка, тот же класс на враге.
	CounterComponent = CreateDefaultSubobject<UClanhallCounterComponent>(TEXT("CounterComponent"));
	// Раздел 6.5 (combo_system_redesign.md): ворота ввода + владелец активации WASD-ударов.
	ComboComponent = CreateDefaultSubobject<UClanhallComboComponent>(TEXT("ComboComponent"));
	// Раздел 6.5: weapon trace для парирования и будущей damage-on-hit логики.
	WeaponTraceComponent = CreateDefaultSubobject<UClanhallWeaponTraceComponent>(TEXT("WeaponTraceComponent"));
	// HUD: camera line trace, мягкая цель под удар/метку (Enemy Frame больше не водит).
	TargetingComponent = CreateDefaultSubobject<UClanhallTargetingComponent>(TEXT("TargetingComponent"));
	// HUD: радиус + Unit.Role.Boss — драйвер Enemy Frame (changelog_enemyframe_unitroles.md §3).
	BossSensorComponent = CreateDefaultSubobject<UClanhallBossSensorComponent>(TEXT("BossSensorComponent"));

	// WASD-классы дефолтно равны C++ классам; Blueprint персонажа может переопределить их
	// на BP-наследников (см. GA_DirectionalAttackBase.h). Монтажи — per-move в UComboData
	// (Moves/Chains).
	AttackOverheadClass   = UGA_DirectionalAttack_Overhead::StaticClass();
	AttackRightSlashClass = UGA_DirectionalAttack_RightSlash::StaticClass();
	AttackLeftSlashClass  = UGA_DirectionalAttack_LeftSlash::StaticClass();
	AttackLowSweepClass   = UGA_DirectionalAttack_LowSweep::StaticClass();
}

UAbilitySystemComponent* AClanhallCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AClanhallCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		// OwnerActor == AvatarActor == this: ASC не на PlayerState, см. technical_context.md
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	if (AttributeSet)
	{
		// Хардкод стартовых значений — плейсхолдеры прототипа из combat_system.md.
		// Допустимо в Разделах 1-3 (см. development_plan.md); DataAsset/GameplayEffect
		// для инициализации атрибутов появится позже вместе с остальной системой данных.
		AttributeSet->InitMaxAP(300.0f);
		AttributeSet->InitAP(300.0f);
		AttributeSet->InitMaxHP(500.0f);
		AttributeSet->InitHP(500.0f);
		AttributeSet->InitMaxMP(200.0f);
		AttributeSet->InitMP(200.0f);
		AttributeSet->InitMaxCharges(4.0f);
		AttributeSet->InitCharges(4.0f);
		AttributeSet->InitBalance(0.0f);
	}

	if (AbilitySystemComponent)
	{
		// Раздел 2 placeholder: реального инвентаря оружия ещё нет (Раздел 10) — тег задаёт
		// STR/DEX-формулу WASD-удара (combat_system.md §4) и шкалу баланса.
		AbilitySystemComponent->AddLooseGameplayTag(bStartWithSTRWeapon
			? ClanhallGameplayTags::Weapon_Type_STR.GetTag()
			: ClanhallGameplayTags::Weapon_Type_DEX.GetTag());

		// Пассивный дрейф Balance к нулю — постоянно активен (combat_system.md §2).
		const FGameplayEffectContextHandle DriftContext = AbilitySystemComponent->MakeEffectContext();
		const FGameplayEffectSpecHandle DriftSpec = AbilitySystemComponent->MakeOutgoingSpec(UGE_BalanceDrift::StaticClass(), 1.0f, DriftContext);
		if (DriftSpec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*DriftSpec.Data.Get(), AbilitySystemComponent);
		}

		// Грант способностей боевой стойки и 4 направлений WASD-удара (combat_system.md §3-4).
		StanceAbilityHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGA_CombatStance::StaticClass(), 1, INDEX_NONE, this));
		AttackOverheadHandle   = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackOverheadClass,   1, INDEX_NONE, this));
		AttackRightSlashHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackRightSlashClass, 1, INDEX_NONE, this));
		AttackLeftSlashHandle  = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackLeftSlashClass,  1, INDEX_NONE, this));
		AttackLowSweepHandle   = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackLowSweepClass,   1, INDEX_NONE, this));

		// Раздел 4: кит Knight — один класс GA_PhysicalSkill гранится 4 раза,
		// каждый раз с разным UAbilityData как SourceObject (development_plan.md).
		// DataAsset'ы привязываются в Blueprint-наследнике в редакторе.
		if (KnightSkillQ_ShieldSlam)
		{
			ActiveSkillQHandle = AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(UGA_PhysicalSkill::StaticClass(), 1, INDEX_NONE, KnightSkillQ_ShieldSlam));
		}
		if (KnightSkillE_PowerStrike)
		{
			ActiveSkillEHandle = AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(UGA_PhysicalSkill::StaticClass(), 1, INDEX_NONE, KnightSkillE_PowerStrike));
		}
		if (KnightSkillR_ShieldCharge)
		{
			ActiveSkillRHandle = AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(UGA_PhysicalSkill::StaticClass(), 1, INDEX_NONE, KnightSkillR_ShieldCharge));
		}
		if (KnightSkillF_Retribution)
		{
			ActiveSkillFHandle = AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(UGA_PhysicalSkill::StaticClass(), 1, INDEX_NONE, KnightSkillF_Retribution));
		}
	}
}

bool AClanhallCharacter::CanJumpInternal_Implementation() const
{
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(ClanhallGameplayTags::State_InStance.GetTag()))
	{
		return false;
	}

	return Super::CanJumpInternal_Implementation();
}

void AClanhallCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AClanhallCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AClanhallCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AClanhallCharacter::Look);

		// Combat stance (combat_system.md §3): ЛКМ зажат/отпущен
		EnhancedInputComponent->BindAction(StanceAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnStancePressed);
		EnhancedInputComponent->BindAction(StanceAction, ETriggerEvent::Completed, this, &AClanhallCharacter::OnStanceReleased);
		EnhancedInputComponent->BindAction(StanceAction, ETriggerEvent::Canceled, this, &AClanhallCharacter::OnStanceReleased);

		// Directional WASD-attacks (combat_system.md §4) — те же клавиши, что и Move,
		// но отдельные дискретные действия: срабатывают один раз на нажатие, а не каждый кадр.
		// GA_DirectionalAttackBase сам отказывает, если игрок не в стойке (ActivationRequiredTags).
		EnhancedInputComponent->BindAction(AttackOverheadAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnAttackOverhead);
		EnhancedInputComponent->BindAction(AttackRightSlashAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnAttackRightSlash);
		EnhancedInputComponent->BindAction(AttackLeftSlashAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnAttackLeftSlash);
		EnhancedInputComponent->BindAction(AttackLowSweepAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnAttackLowSweep);

		// Раздел 4: активные навыки Knight (GA_PhysicalSkill) — Q/E/R/F.
		EnhancedInputComponent->BindAction(ActiveSkillQAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnActiveSkillQ);
		EnhancedInputComponent->BindAction(ActiveSkillEAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnActiveSkillE);
		EnhancedInputComponent->BindAction(ActiveSkillRAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnActiveSkillR);
		EnhancedInputComponent->BindAction(ActiveSkillFAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnActiveSkillF);
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
	// В боевой стойке WASD = направление удара, а не движение (combat_system.md §3-4).
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(ClanhallGameplayTags::State_InStance.GetTag()))
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
		// combat_system.md §3: "Отпустить LMB в любой момент = мгновенный выход из стойки".
		AbilitySystemComponent->CancelAbilityHandle(StanceAbilityHandle);
	}

	// combo_system_redesign.md: выход из стойки — всегда, вне ворот. Останавливает активный
	// монтаж комбо с blend-out и сбрасывает последовательность независимо от фазы.
	if (ComboComponent)
	{
		ComboComponent->OnStanceExit();
	}
}

// combo_system_redesign.md, Часть B1: WASD больше не активирует направленный удар напрямую —
// решение (опенер / продолжение по данным дерева / мусор вне окна) целиком у ComboComponent,
// он же сам вызывает TryActivateAbility, когда ввод валиден. Парирование обрабатывает
// UClanhallWeaponTraceComponent при хите врага (State.Parrying на ASC врага, не игрока).

void AClanhallCharacter::OnAttackOverhead()
{
	if (ComboComponent)
	{
		ComboComponent->HandleAttackInput(EClanhallAttackDirection::Overhead);
	}
}

void AClanhallCharacter::OnAttackRightSlash()
{
	if (ComboComponent)
	{
		ComboComponent->HandleAttackInput(EClanhallAttackDirection::RightSlash);
	}
}

void AClanhallCharacter::OnAttackLeftSlash()
{
	if (ComboComponent)
	{
		ComboComponent->HandleAttackInput(EClanhallAttackDirection::LeftSlash);
	}
}

void AClanhallCharacter::OnAttackLowSweep()
{
	if (ComboComponent)
	{
		ComboComponent->HandleAttackInput(EClanhallAttackDirection::LowSweep);
	}
}

FGameplayAbilitySpecHandle AClanhallCharacter::GetAttackHandle(EClanhallAttackDirection Direction) const
{
	switch (Direction)
	{
	case EClanhallAttackDirection::Overhead:   return AttackOverheadHandle;
	case EClanhallAttackDirection::RightSlash: return AttackRightSlashHandle;
	case EClanhallAttackDirection::LeftSlash:  return AttackLeftSlashHandle;
	case EClanhallAttackDirection::LowSweep:   return AttackLowSweepHandle;
	default:                                   return FGameplayAbilitySpecHandle();
	}
}

// ---------------------------------------------------------------------------
// Активные навыки (Q/E/R/F). Контрнавык (Раздел 6, переработан) больше не требует
// модификатора — распознаётся резолвером внутри GA_PhysicalSkill::ActivateAbility
// по совпадению CounterTag с открытым окном цели.
// ---------------------------------------------------------------------------

void AClanhallCharacter::OnActiveSkillQ()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(ActiveSkillQHandle);
	}
}

void AClanhallCharacter::OnActiveSkillE()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(ActiveSkillEHandle);
	}
}

void AClanhallCharacter::OnActiveSkillR()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(ActiveSkillRHandle);
	}
}

void AClanhallCharacter::OnActiveSkillF()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(ActiveSkillFHandle);
	}
}
