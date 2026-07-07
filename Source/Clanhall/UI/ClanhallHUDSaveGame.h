#pragma once

#include "GameFramework/SaveGame.h"
#include "ClanhallHUDSaveGame.generated.h"

/** Хранит позиции перетаскиваемых рамок HUD между сессиями (hud_system.md). */
UCLASS()
class CLANHALL_API UClanhallHUDSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Позиция Player Frame (якорь top-left виджета). Дефолт — левый нижний угол. */
	UPROPERTY() FVector2D PlayerFramePosition = FVector2D(50.0f, 700.0f);

	/** Позиция Enemy Frame. Дефолт — правее центра. */
	UPROPERTY() FVector2D EnemyFramePosition  = FVector2D(1870.0f, 700.0f);
};
