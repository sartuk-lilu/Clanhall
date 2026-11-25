// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ClanhallPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class UClanhallHUDWidget;
struct FInputActionValue;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AClanhallPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** HUD игрока (`Implementation/HUD.md`): вёрстка WBP_HUD на базе UClanhallHUDWidget. Пусто - HUD
	 *  не создаётся (пока его создаёт граф персонажа). */
	UPROPERTY(EditAnywhere, Category = "Clanhall|HUD")
	TSubclassOf<UClanhallHUDWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UClanhallHUDWidget> HUDWidget;

	/** Alt (удержание) - курсор без выхода из боя (`Controls.md`, «Курсор»). */
	UPROPERTY(EditAnywhere, Category = "Clanhall|HUD")
	TObjectPtr<UInputAction> HUDCursorAction;

	void OnHUDCursorStarted();
	void OnHUDCursorCompleted();

	/** HUD получает бойца, когда контроллер им завладел, - без таймеров ожидания. */
	virtual void AcknowledgePossession(APawn* P) override;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

};
