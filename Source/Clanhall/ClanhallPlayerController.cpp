// Copyright Epic Games, Inc. All Rights Reserved.


#include "ClanhallPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Clanhall.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "UI/ClanhallHUDWidget.h"
#include "ClanhallCharacter.h"

void AClanhallPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// HUD - у контроллера: интерфейс и режим ввода принадлежат игроку, а не бойцу.
	if (IsLocalPlayerController() && HUDWidgetClass && !HUDWidget)
	{
		HUDWidget = CreateWidget<UClanhallHUDWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToPlayerScreen();
			HUDWidget->SetPlayerCharacter(Cast<AClanhallCharacter>(GetPawn()));
		}
	}

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogClanhall, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AClanhallPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);
	if (HUDWidget)
	{
		HUDWidget->SetPlayerCharacter(Cast<AClanhallCharacter>(P));
	}
}

void AClanhallPlayerController::OnHUDCursorStarted()
{
	// Курсор без выхода из боя: фокус виджету не отдаётся (иначе глохнет WASD), мышь не
	// запирается, курсор не прячется при захвате (иначе мигает при клике).
	SetShowMouseCursor(true);
	UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(this, nullptr, EMouseLockMode::DoNotLock, false);
}

void AClanhallPlayerController::OnHUDCursorCompleted()
{
	UWidgetBlueprintLibrary::SetInputMode_GameOnly(this);
	SetShowMouseCursor(false);
}

void AClanhallPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (HUDCursorAction)
		{
			EnhancedInput->BindAction(HUDCursorAction.Get(), ETriggerEvent::Started, this, &AClanhallPlayerController::OnHUDCursorStarted);
			EnhancedInput->BindAction(HUDCursorAction.Get(), ETriggerEvent::Completed, this, &AClanhallPlayerController::OnHUDCursorCompleted);
		}
	}

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AClanhallPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
