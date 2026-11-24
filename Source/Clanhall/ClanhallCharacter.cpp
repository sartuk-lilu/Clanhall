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
#include "ClanhallCombatSettings.h"
#include "UI/ClanhallUILibrary.h"
#include "Debug/ClanhallDebugMessage.h"
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

	// Клавиши, переназначенные в прошлых сессиях. Приёмы уже гранчены (AClanhallHumanoidBase::BeginPlay).
	TArray<FClanhallSlotAssignment> SavedSlots;
	if (UClanhallUILibrary::LoadSkillSlots(SavedSlots))
	{
		ImportSlotOverrides(SavedSlots);
	}

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
	}
	RefreshMovementSpeed();
}

void AClanhallCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickBodyTurn();
}

bool AClanhallCharacter::CanJumpInternal_Implementation() const
{
	// Fullbody-активка занимает всё тело, включая ноги — та же причина, что в DoMove:
	// иначе персонаж подпрыгивает посреди приёма.
	if (IsBodyBusy())
	{
		return false;
	}

	return Super::CanJumpInternal_Implementation();
}

void AClanhallCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Кнопки мыши - слои ввода (`Controls.md`, «Ввод»): удержание, а не нажатие. Биндятся первыми:
		// делегаты одного кадра идут в порядке биндинга, и кнопка мыши, нажатая в том же кадре, что
		// клавиша слота или движение, обязана сменить слой раньше, чем их обработают.
		EnhancedInputComponent->BindAction(BodyLayerAction.Get(), ETriggerEvent::Started, this, &AClanhallCharacter::OnLayerPressed, EClanhallInputLayer::Body);
		EnhancedInputComponent->BindAction(BodyLayerAction.Get(), ETriggerEvent::Completed, this, &AClanhallCharacter::OnLayerReleased, EClanhallInputLayer::Body);
		EnhancedInputComponent->BindAction(BodyLayerAction.Get(), ETriggerEvent::Canceled, this, &AClanhallCharacter::OnLayerReleased, EClanhallInputLayer::Body);
		EnhancedInputComponent->BindAction(MouthLayerAction.Get(), ETriggerEvent::Started, this, &AClanhallCharacter::OnLayerPressed, EClanhallInputLayer::Mouth);
		EnhancedInputComponent->BindAction(MouthLayerAction.Get(), ETriggerEvent::Completed, this, &AClanhallCharacter::OnLayerReleased, EClanhallInputLayer::Mouth);
		EnhancedInputComponent->BindAction(MouthLayerAction.Get(), ETriggerEvent::Canceled, this, &AClanhallCharacter::OnLayerReleased, EClanhallInputLayer::Mouth);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AClanhallCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AClanhallCharacter::OnMoveReleased);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AClanhallCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AClanhallCharacter::Look);

		// Клавиши слотов - одни и те же для приёмов и слогов, смысл решает слой. W A S D в том же
		// контексте ещё и движение: обе привязки срабатывают, Move сам молчит под слоем. Один
		// контекст, а не IMC на слой: при смене контекста зажатая клавиша либо глушится вместе с
		// кнопкой мыши, либо W, зажатая для бега, стреляет приёмом.
		for (const FClanhallSlotInput& SlotInput : SlotInputs)
		{
			if (!SlotInput.Action || !SlotInput.Slot.IsValid())
			{
				UE_LOG(LogClanhall, Warning, TEXT("'%s': запись SlotInputs без Action или Slot - клавиша слота не привязана."), *GetNameSafe(this));
				continue;
			}
			EnhancedInputComponent->BindAction(SlotInput.Action.Get(), ETriggerEvent::Started, this, &AClanhallCharacter::OnSlotKeyPressed, SlotInput.Slot);
		}

		for (int32 Index = 0; Index < WeaponInputs.Num(); ++Index)
		{
			if (!WeaponInputs[Index].Action)
			{
				UE_LOG(LogClanhall, Warning, TEXT("'%s': запись WeaponInputs[%d] без Action - клавиша оружия не привязана."), *GetNameSafe(this), Index);
				continue;
			}
			EnhancedInputComponent->BindAction(WeaponInputs[Index].Action.Get(), ETriggerEvent::Started, this, &AClanhallCharacter::OnWeaponKeyPressed, Index);
		}


		// Пробел: уклонение «Пробел + направление» (`CombatSystem.md`, «Уклонение»), под ПКМ - выпуск
		// фразы; прыжок - своя клавиша.
		EnhancedInputComponent->BindAction(SpaceAction, ETriggerEvent::Started, this, &AClanhallCharacter::OnSpacePressed);
		EnhancedInputComponent->BindAction(JumpAction.Get(), ETriggerEvent::Started, this, &AClanhallCharacter::OnJumpPressed);
		EnhancedInputComponent->BindAction(JumpAction.Get(), ETriggerEvent::Completed, this, &AClanhallCharacter::OnJumpReleased);
		EnhancedInputComponent->BindAction(TauntAction.Get(), ETriggerEvent::Started, this, &AClanhallCharacter::OnTauntPressed);
		EnhancedInputComponent->BindAction(CrystalAction.Get(), ETriggerEvent::Started, this, &AClanhallCharacter::OnCrystalPressed);

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
	// Под зажатой кнопкой мыши W A S D - клавиши слотов: персонаж стоит (`Controls.md`, «Ввод»).
	// Это добровольная остановка, а не потеря управления - IsBodyBusy её не знает.
	if (GetInputLayer() != EClanhallInputLayer::None)
	{
		return;
	}

	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();
	CurrentMoveInput = MovementVector;

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AClanhallCharacter::OnMoveReleased()
{
	CurrentMoveInput = FVector2D::ZeroVector;
}

void AClanhallCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

bool AClanhallCharacter::IsBodyBusy() const
{
	if (!AbilitySystemComponent)
	{
		return false;
	}
	// WASD - движение всегда (`CombatSystem.md`, «Ввод»), кроме: начатого приёма на всё тело и
	// разрешённой потери управления - короткий стан, анимация насмешки-парирования
	// (`Invariants.md`, «Метка не отнимает ввод»), - и смерти.
	FGameplayTagContainer BusyTags;
	BusyTags.AddTag(ClanhallGameplayTags::State_SkillCommitted.GetTag());
	BusyTags.AddTag(ClanhallGameplayTags::State_Dead.GetTag());
	BusyTags.AddTag(ClanhallGameplayTags::State_Stunned.GetTag());
	BusyTags.AddTag(ClanhallGameplayTags::State_TauntParried.GetTag());
	BusyTags.AddTag(ClanhallGameplayTags::State_TauntParrying.GetTag());
	return AbilitySystemComponent->HasAnyMatchingGameplayTags(BusyTags);
}

void AClanhallCharacter::DoMove(float Right, float Forward)
{
	// Активка занимает слот fullbody целиком — свободных ног нет. State.SkillCommitted висит
	// весь приём и снимается только в EndAbility. Плюс потеря управления и смерть.
	if (IsBodyBusy())
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
	// Под ПКМ Пробел - выпуск фразы. Под ЛКМ - ничего: направления уклонения - W A S D, а под ЛКМ
	// это клавиши приёмов, «Пробел + направление» не собрать (решение P5j, подтвердить в игре).
	switch (GetInputLayer())
	{
	case EClanhallInputLayer::Mouth:
		OnReleasePressed();
		return;
	case EClanhallInputLayer::Body:
		return;
	default:
		break;
	}

	// «Пробел + направление» - отскок в эту сторону (`CombatSystem.md`, «Уклонение»). Направление
	// нажатой клавиши, а мировое направление отскока потом берётся от корпуса. Нет направления -
	// нет отскока: «назад по умолчанию» не существует.
	if (CurrentMoveInput.IsNearlyZero())
	{
		return;
	}

	EClanhallEvadeDirection Direction;
	if (FMath::Abs(CurrentMoveInput.X) > FMath::Abs(CurrentMoveInput.Y))
	{
		Direction = CurrentMoveInput.X > 0.0f ? EClanhallEvadeDirection::Right : EClanhallEvadeDirection::Left;
	}
	else
	{
		Direction = CurrentMoveInput.Y > 0.0f ? EClanhallEvadeDirection::Forward : EClanhallEvadeDirection::Back;
	}
	TriggerEvade(Direction);
}

void AClanhallCharacter::OnJumpPressed()
{
	Jump();
}

void AClanhallCharacter::OnJumpReleased()
{
	StopJumping();
}

void AClanhallCharacter::OnTauntPressed()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(GetTauntHandle());
	}
}

void AClanhallCharacter::OnCrystalPressed()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(GetCrystalHandle());
	}
}

void AClanhallCharacter::OnDuckPressed()
{
	// Под ПКМ Ctrl - сброс фразы; под ЛКМ присед закрыт для единообразия с Пробелом.
	switch (GetInputLayer())
	{
	case EClanhallInputLayer::Mouth:
		OnPhraseResetPressed();
		return;
	case EClanhallInputLayer::Body:
		return;
	default:
		break;
	}

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
	AbilitySystemComponent->AddLooseGameplayTag(ClanhallGameplayTags::State_Sprinting.GetTag());
	RefreshMovementSpeed();
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
	AbilitySystemComponent->RemoveLooseGameplayTag(ClanhallGameplayTags::State_Sprinting.GetTag());
	RefreshMovementSpeed();
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

	// Разворот в приёме (`CombatSystem.md`, «Контроль»): пока идёт свой приём, корпус доворачивается
	// к камере медленно, со скоростью оружия и без порога. Правило общее с противником
	// (AClanhallHumanoidBase::TickSkillTurn), у игрока цель - направление камеры.
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(ClanhallGameplayTags::State_SkillCommitted.GetTag()))
	{
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = true;
		Movement->RotationRate.Yaw = GetSkillTurnRate();
		bTurningInPlace = false;
		return;
	}
	Movement->RotationRate.Yaw = GetTurnRate();

	// На бегу ротацией заведует движковый bOrientRotationToMovement (StartSprint/StopSprint) -
	// тик сюда не лезет (`LocomotionStructure.md`).
	if (IsSprinting())
	{
		Movement->bOrientRotationToMovement = true;
		Movement->bUseControllerDesiredRotation = false;
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
// Ввод удержанием кнопки мыши (`Controls.md`, «Ввод»). Контрнавык модификатора не требует -
// распознаётся по контакту в GA_PhysicalSkill::ResolveHitOn по совпадению CounterTag с открытым
// окном цели; момент нажатия клавиши под ЛКМ и есть момент приёма.
// ---------------------------------------------------------------------------

void AClanhallCharacter::OnLayerPressed(EClanhallInputLayer Layer)
{
	const EClanhallInputLayer Before = GetInputLayer();
	HeldLayers.Remove(Layer);
	HeldLayers.Add(Layer);
	HandleInputLayerChange(Before);
}

void AClanhallCharacter::OnLayerReleased(EClanhallInputLayer Layer)
{
	const EClanhallInputLayer Before = GetInputLayer();
	HeldLayers.Remove(Layer);
	HandleInputLayerChange(Before);
}

void AClanhallCharacter::HandleInputLayerChange(EClanhallInputLayer Before)
{
	const EClanhallInputLayer Now = GetInputLayer();
	if (Now == Before)
	{
		return;
	}
	// Вход в слой - персонаж встаёт: уклонение под слоем не должно взять старое направление.
	if (Before == EClanhallInputLayer::None)
	{
		CurrentMoveInput = FVector2D::ZeroVector;
	}
	OnInputLayerChanged.Broadcast(Now);
}

void AClanhallCharacter::DebugHoldInputLayer(EClanhallInputLayer Layer)
{
	if (Layer == EClanhallInputLayer::None)
	{
		const EClanhallInputLayer Before = GetInputLayer();
		HeldLayers.Reset();
		HandleInputLayerChange(Before);
		return;
	}
	OnLayerPressed(Layer);
}

void AClanhallCharacter::OnSlotKeyPressed(FGameplayTag Slot)
{
	// Клавиша, нажатая до кнопки мыши, приёмом не становится: её Started пришёл без слоя.
	switch (GetInputLayer())
	{
	case EClanhallInputLayer::Body:
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->TryActivateAbility(GetActiveSkillHandle(Slot));
		}
		break;
	case EClanhallInputLayer::Mouth:
		OnSyllableKey(Slot);
		break;
	default:
		// Голая клавиша без кнопки мыши ничего не запускает (`Controls.md`, «Ввод»).
		break;
	}
}

void AClanhallCharacter::OnSyllableKey(FGameplayTag Slot)
{
	// Фраза, мана и выпуск - P6 (`MagicSystem.md`, «Каст»).
	ClanhallDebug::Screen(FColor::Purple, FString::Printf(TEXT("syllable %s"), *Slot.ToString()));
}

void AClanhallCharacter::OnReleasePressed()
{
	ClanhallDebug::Screen(FColor::Purple, TEXT("phrase release"));
}

void AClanhallCharacter::OnPhraseResetPressed()
{
	ClanhallDebug::Screen(FColor::Purple, TEXT("phrase reset"));
}

void AClanhallCharacter::OnWeaponKeyPressed(int32 WeaponInputIndex)
{
	if (!WeaponInputs.IsValidIndex(WeaponInputIndex))
	{
		return;
	}
	const FClanhallWeaponInput& Input = WeaponInputs[WeaponInputIndex];
	const double Now = GetWorld()->GetTimeSeconds();

	// Двойное нажатие той же клавиши (как «11» в песочнице - парные мечи) - DoubleTapLoadoutIndex.
	const bool bDoubleTap = Input.DoubleTapLoadoutIndex >= 0
		&& LastWeaponInputIndex == WeaponInputIndex
		&& Now - LastWeaponInputTime <= UClanhallCombatSettings::Get().WeaponDoubleTapSeconds;

	LastWeaponInputIndex = WeaponInputIndex;
	LastWeaponInputTime = Now;

	SelectWeapon(bDoubleTap ? Input.DoubleTapLoadoutIndex : Input.LoadoutIndex);
}

void AClanhallCharacter::RefreshMovementSpeed()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}
	Movement->MaxWalkSpeed = (IsSprinting() ? GetSprintSpeed() : GetJogSpeed()) * GetWeaponSpeedMultiplier() * GetMoveSpeedMultiplier();
}

void AClanhallCharacter::OnSkillSlotsAssigned()
{
	UClanhallUILibrary::SaveSkillSlots(ExportSlotOverrides());
}
