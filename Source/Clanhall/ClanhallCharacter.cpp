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
#include "AbilitySystem/Abilities/GA_Dodge.h"
#include "AbilitySystem/Abilities/GA_Duck.h"
#include "AbilitySystem/ClanhallTargetingComponent.h"
#include "AbilitySystem/ClanhallBossSensorComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Engine/Engine.h"

AClanhallCharacter::AClanhallCharacter()
{
	// Нужен для доворота корпуса (`TickBodyTurn`) - без него bUseControllerDesiredRotation
	// держится молча выключенным навсегда, доворота не будет вовсе.
	PrimaryActorTick.bCanEverTick = true;

	// Дефолт - сам C++-класс отскока/ухода и приседа: если разработчик не завёл
	// Blueprint-наследника, всё продолжает работать на дефолтах кода (см. поля в заголовке).
	DodgeAbilityClass = UGA_Dodge::StaticClass();
	DuckAbilityClass = UGA_Duck::StaticClass();

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
}

void AClanhallCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Стартовые значения ресурсов инициализирует AClanhallCharacterBase::BeginPlay (Default*
	// поля) — общий путь для игрока и AI (`CombatSystem.md`, «Ресурсы персонажа»).

	if (AbilitySystemComponent)
	{
		// Грант ухода/рывка и приседа (`CombatSystem.md`). Активки Q/E/R/F гранятся выше по
		// иерархии — см. AClanhallHumanoidBase::BeginPlay. У противников ухода и приседа пока
		// нет. Гранится DodgeAbilityClass/DuckAbilityClass, не StaticClass() напрямую - иначе
		// EditDefaultsOnly-поля класса (дистанции, монтажи) негде было бы открыть в редакторе.
		DodgeAbilityHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DodgeAbilityClass, 1, INDEX_NONE, this));
		DuckAbilityHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DuckAbilityClass, 1, INDEX_NONE, this));
	}

	// Ротация и базовая скорость общей локомоции - один раз, не в конструкторе: TurnRate это
	// UPROPERTY, в конструкторе ещё не перезаписан значением из BP (`LocomotionStructure.md`).
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;   // страйф: корпус по камере
		bUseControllerRotationYaw = false;             // не мгновенное прилипание, доворот считает тик
		Movement->bUseControllerDesiredRotation = false;
		Movement->RotationRate.Yaw = GetTurnRate();
		Movement->MaxWalkSpeed = GetJogSpeed() * GetWeaponSpeedMultiplier();
	}
}

void AClanhallCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickBodyTurn();
}

bool AClanhallCharacter::CanJumpInternal_Implementation() const
{
	// Fullbody-активка занимает всё тело, включая ноги — та же причина, что в DoMove:
	// иначе персонаж подпрыгивает посреди рывка.
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

		// Активные навыки Knight (GA_PhysicalSkill) — Q/E/R/F.
		EnhancedInputComponent->BindAction(ActiveSkillQAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnActiveSkillQ);
		EnhancedInputComponent->BindAction(ActiveSkillEAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnActiveSkillE);
		EnhancedInputComponent->BindAction(ActiveSkillRAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnActiveSkillR);
		EnhancedInputComponent->BindAction(ActiveSkillFAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnActiveSkillF);

		// Пробел: прыжок / рывок, решает сам обработчик (`CombatSystem.md`).
		EnhancedInputComponent->BindAction(SpaceAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnSpacePressed);
		EnhancedInputComponent->BindAction(SpaceAction, ETriggerEvent::Completed, this, &AClanhallCharacter::OnSpaceReleased);
		EnhancedInputComponent->BindAction(SpaceAction, ETriggerEvent::Canceled, this, &AClanhallCharacter::OnSpaceReleased);

		// Shift: бег.
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnSprintPressed);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AClanhallCharacter::OnSprintReleased);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AClanhallCharacter::OnSprintReleased);

		// Ctrl: присед, окно а не удержание - биндится только Started (`CombatSystem.md`).
		EnhancedInputComponent->BindAction(DuckAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnDuckPressed);
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
	// Активка занимает слот fullbody целиком — свободных ног нет. State.SkillCommitted висит
	// весь каст-монтаж (и рывок) и снимается только в EndAbility.
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

		// Ход спиной и по задним диагоналям - только шагом (`LocomotionStructure.md`).
		// Отступать лицом к врагу это уступка, разрывать дистанцию надо бегом. На бегу кап
		// не нужен: корпус развёрнут по движению, спиной там не бегают.
		float Scale = 1.0f;
		if (!IsSprinting())
		{
			const FVector2D Dir = FVector2D(Right, Forward).GetSafeNormal();
			const float Backness = FMath::Clamp(-Dir.Y / 0.7071f, 0.0f, 1.0f);
			Scale = FMath::Lerp(1.0f, GetWalkSpeed() / FMath::Max(GetJogSpeed(), 1.0f), Backness);
		}

		// add movement
		AddMovementInput(ForwardDirection, Forward * Scale);
		AddMovementInput(RightDirection, Right * Scale);
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

void AClanhallCharacter::OnSprintPressed()
{
	StartSprint();
}

void AClanhallCharacter::OnSprintReleased()
{
	if (IsSprinting())
	{
		StopSprint();
	}
}

void AClanhallCharacter::OnSpacePressed()
{
	// На бегу Пробел - длинный рывок вперёд по корпусу; корпус на бегу развёрнут по движению,
	// так что отдельного направления не нужно.
	if (IsSprinting())
	{
		TriggerEvade(EClanhallEvadeDirection::ForwardDash);
		return;
	}

	Jump();
}

void AClanhallCharacter::OnSpaceReleased()
{
	StopJumping();
}

void AClanhallCharacter::OnDuckPressed()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(DuckAbilityHandle);
	}
}

void AClanhallCharacter::StartSprint()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || !AbilitySystemComponent)
	{
		return;
	}

	Movement->bOrientRotationToMovement = true;    // 360: корпус по движению
	Movement->bUseControllerDesiredRotation = false;
	Movement->MaxWalkSpeed = GetSprintSpeed() * GetWeaponSpeedMultiplier();
	AbilitySystemComponent->AddLooseGameplayTag(ClanhallGameplayTags::State_Sprinting.GetTag());
	ResetTurnInPlace(); // подшаг на бегу не играет
}

void AClanhallCharacter::StopSprint()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || !AbilitySystemComponent)
	{
		return;
	}

	Movement->bOrientRotationToMovement = false;
	Movement->MaxWalkSpeed = GetJogSpeed() * GetWeaponSpeedMultiplier();
	AbilitySystemComponent->RemoveLooseGameplayTag(ClanhallGameplayTags::State_Sprinting.GetTag());
}

void AClanhallCharacter::TriggerEvade(EClanhallEvadeDirection Direction)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventMagnitude = static_cast<float>(Direction);

	AbilitySystemComponent->TriggerAbilityFromGameplayEvent(DodgeAbilityHandle, AbilitySystemComponent->AbilityActorInfo.Get(),
		ClanhallGameplayTags::Event_Evade.GetTag(), &EventData, *AbilitySystemComponent);
}

void AClanhallCharacter::TickBodyTurn()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || !GetController())
	{
		return;
	}

	// На бегу ротацией заведует движковый bOrientRotationToMovement (StartSprint/StopSprint) -
	// тик сюда не лезет (`LocomotionStructure.md`).
	if (IsSprinting())
	{
		return;
	}

	const float YawDelta = FRotator::NormalizeAxis(GetControlRotation().Yaw - GetActorRotation().Yaw);

	// GetCurrentAcceleration() отражает ввод WASD этого шага (ноль без нажатых клавиш) — тот же
	// сигнал, каким движок сам гейтит bOrientRotationToMovement.
	const bool bMoving = !Movement->GetCurrentAcceleration().IsNearlyZero();

	if (bMoving)
	{
		// Двигается (страйф) - доворачивается к камере постоянно, без порога: иначе
		// боец несколько секунд бежит боком относительно взгляда (`LocomotionStructure.md`).
		// Подшаг тут не играет.
		Movement->bUseControllerDesiredRotation = true;
		bTurningInPlace = false;
	}
	else
	{
		// Гистерезис: порог входа (TurnThreshold) и угол выхода (TurnSettleAngle) - разные
		// числа, иначе на границе порога доворот дёргался бы "начал - тут же перестал" каждый кадр.
		if (!bTurningInPlace && FMath::Abs(YawDelta) > GetTurnThreshold())
		{
			bTurningInPlace = true;
		}

		if (bTurningInPlace)
		{
			// Направление обновляется каждый кадр, не только на взводе: если камера
			// перекладывается на другую сторону посреди уже идущего подшага, |YawDelta| остаётся
			// большим и bTurningInPlace не опускается - без этого ABP доигрывал бы шаг в сторону,
			// которая уже не актуальна, хотя капсулу движок довернул в новую верно.
			TurnDirection = FMath::Sign(YawDelta);
			Movement->bUseControllerDesiredRotation = true;
			if (FMath::Abs(YawDelta) <= GetTurnSettleAngle())
			{
				bTurningInPlace = false;
			}
		}
		else
		{
			// В пределах порога корпус не вращается вовсе — визуально за камерой тянется
			// только верх, это работа ABP, не движка.
			Movement->bUseControllerDesiredRotation = false;
		}
	}
}

// ---------------------------------------------------------------------------
// Активные навыки (Q/E/R/F). Контрнавык (`AbilitySystem.md`, «Контрнавык») больше не требует
// модификатора — распознаётся резолвером внутри GA_PhysicalSkill::ActivateAbility
// по совпадению CounterTag с открытым окном цели.
// ---------------------------------------------------------------------------

void AClanhallCharacter::OnActiveSkillQ()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(GetActiveSkillHandle(ClanhallGameplayTags::Slot_Q.GetTag()));
	}
}

void AClanhallCharacter::OnActiveSkillE()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(GetActiveSkillHandle(ClanhallGameplayTags::Slot_E.GetTag()));
	}
}

void AClanhallCharacter::OnActiveSkillR()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(GetActiveSkillHandle(ClanhallGameplayTags::Slot_R.GetTag()));
	}
}

void AClanhallCharacter::OnActiveSkillF()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(GetActiveSkillHandle(ClanhallGameplayTags::Slot_F.GetTag()));
	}
}
