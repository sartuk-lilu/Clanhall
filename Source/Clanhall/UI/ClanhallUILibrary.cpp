#include "UI/ClanhallUILibrary.h"
#include "UI/ClanhallHUDSaveGame.h"
#include "Kismet/GameplayStatics.h"

namespace { const FString HUDSaveSlot = TEXT("HUDLayout"); }

void UClanhallUILibrary::SaveHUDLayout(FVector2D PlayerFramePosition, FVector2D EnemyFramePosition)
{
	UClanhallHUDSaveGame* Save = Cast<UClanhallHUDSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UClanhallHUDSaveGame::StaticClass()));
	if (!Save) return;

	Save->PlayerFramePosition = PlayerFramePosition;
	Save->EnemyFramePosition  = EnemyFramePosition;
	UGameplayStatics::SaveGameToSlot(Save, HUDSaveSlot, 0);
}

bool UClanhallUILibrary::LoadHUDLayout(FVector2D& PlayerFramePosition, FVector2D& EnemyFramePosition)
{
	UClanhallHUDSaveGame* Save = Cast<UClanhallHUDSaveGame>(
		UGameplayStatics::LoadGameFromSlot(HUDSaveSlot, 0));
	if (!Save) return false;

	PlayerFramePosition = Save->PlayerFramePosition;
	EnemyFramePosition  = Save->EnemyFramePosition;
	return true;
}
