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
#include "AbilitySystem/ClanhallWeaponTraceComponent.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"

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
	// Раздел 6.5: weapon trace для парирования и будущей damage-on-hit логики.
	WeaponTraceComponent = CreateDefaultSubobject<UClanhallWeaponTraceComponent>(TEXT("WeaponTraceComponent"));
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
		AttackOverheadHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGA_DirectionalAttack_Overhead::StaticClass(), 1, INDEX_NONE, this));
		AttackRightSlashHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGA_DirectionalAttack_RightSlash::StaticClass(), 1, INDEX_NONE, this));
		AttackLeftSlashHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGA_DirectionalAttack_LeftSlash::StaticClass(), 1, INDEX_NONE, this));
		AttackLowSweepHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGA_DirectionalAttack_LowSweep::StaticClass(), 1, INDEX_NONE, this));

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

		// Раздел 6: режим контрнавыка (Ctrl зажат).
		if (CounterModeAction)
		{
			EnhancedInputComponent->BindAction(CounterModeAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnCounterModePressed);
			EnhancedInputComponent->BindAction(CounterModeAction, ETriggerEvent::Completed, this, &AClanhallCharacter::OnCounterModeReleased);
			EnhancedInputComponent->BindAction(CounterModeAction, ETriggerEvent::Canceled, this, &AClanhallCharacter::OnCounterModeReleased);
		}
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
}

void AClanhallCharacter::OnAttackOverhead()
{
	// Раздел 6.5: парирование обрабатывает UClanhallWeaponTraceComponent при хите врага.
	// State.Parrying теперь на ASC врага (не игрока) — input-based проверка удалена.
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(AttackOverheadHandle);
	}
}

void AClanhallCharacter::OnAttackRightSlash()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(AttackRightSlashHandle);
	}
}

void AClanhallCharacter::OnAttackLeftSlash()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(AttackLeftSlashHandle);
	}
}

void AClanhallCharacter::OnAttackLowSweep()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(AttackLowSweepHandle);
	}
}

// ---------------------------------------------------------------------------
// Раздел 6: вспомогательная логика контрнавыка
// ---------------------------------------------------------------------------

void AClanhallCharacter::OnCounterModePressed()
{
	bCounterModeActive = true;
}

void AClanhallCharacter::OnCounterModeReleased()
{
	bCounterModeActive = false;
}

bool AClanhallCharacter::TryCounterNearestEnemy()
{
	if (!AbilitySystemComponent)
	{
		return false;
	}

	// Те же параметры поиска цели, что у GA_ClanhallAbilityBase::FindMeleeTarget.
	const float TraceRange = 200.0f;
	const float TraceRadius = 75.0f;
	const FVector Start = GetActorLocation() + GetActorForwardVector() * TraceRange;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	TArray<AActor*> Overlapping;
	UKismetSystemLibrary::SphereOverlapActors(this, Start, TraceRadius, ObjectTypes, nullptr, ActorsToIgnore, Overlapping);

	for (AActor* Candidate : Overlapping)
	{
		IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(Candidate);
		if (!Interface)
		{
			continue;
		}

		UAbilitySystemComponent* EnemyASC = Interface->GetAbilitySystemComponent();
		if (!EnemyASC)
		{
			continue;
		}

		// Проверяем, открыто ли окно контрнавыка у этого врага.
		if (!EnemyASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_CounterWindow.GetTag()))
		{
			continue;
		}

		// Окно открыто — отменяем активный Ability.Skill.* навык врага.
		// CancelAbilities использует иерархическое сопоставление тегов: родительский
		// "Ability.Skill" найдёт все листовые Ability.Skill.Knight.PowerStrike и т.д.
		FGameplayTagContainer SkillTags;
		SkillTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Skill")));
		EnemyASC->CancelAbilities(&SkillTags);

#if !UE_BUILD_SHIPPING
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("✓ КОНТРНАВЫК! Навык врага прерван"));
#endif
		return true;
	}

	return false;
}

// ---------------------------------------------------------------------------
// Активные навыки (Q/E/R/F) с поддержкой режима контрнавыка
// ---------------------------------------------------------------------------

static void ActivateSkillWithCounter(UAbilitySystemComponent* ASC, FGameplayAbilitySpecHandle Handle, bool bCountered)
{
	if (!ASC)
	{
		return;
	}

	if (bCountered)
	{
		// Навешиваем State.CounterActive перед активацией — GA_PhysicalSkill прочитает его
		// в CanActivateAbility/ActivateAbility и пропустит проверки Charges и КД.
		ClanhallGameplayEffects::ApplyTimedTag(ASC, ClanhallGameplayTags::State_CounterActive.GetTag(), 0.1f);
	}

	ASC->TryActivateAbility(Handle);
}

void AClanhallCharacter::OnActiveSkillQ()
{
	const bool bCountered = bCounterModeActive && TryCounterNearestEnemy();
	ActivateSkillWithCounter(AbilitySystemComponent, ActiveSkillQHandle, bCountered);
}

void AClanhallCharacter::OnActiveSkillE()
{
	const bool bCountered = bCounterModeActive && TryCounterNearestEnemy();
	ActivateSkillWithCounter(AbilitySystemComponent, ActiveSkillEHandle, bCountered);
}

void AClanhallCharacter::OnActiveSkillR()
{
	const bool bCountered = bCounterModeActive && TryCounterNearestEnemy();
	ActivateSkillWithCounter(AbilitySystemComponent, ActiveSkillRHandle, bCountered);
}

void AClanhallCharacter::OnActiveSkillF()
{
	const bool bCountered = bCounterModeActive && TryCounterNearestEnemy();
	ActivateSkillWithCounter(AbilitySystemComponent, ActiveSkillFHandle, bCountered);
}
