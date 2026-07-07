// Blueprint Function Library для HUD-утилит: сохранение/загрузка позиций рамок.
// Вызывается из виджетов Blueprint без ссылки на конкретный Actor.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ClanhallUILibrary.generated.h"

UCLASS()
class CLANHALL_API UClanhallUILibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Сохраняет позиции рамок в слот "HUDLayout". Вызывать в OnDragEnd каждой рамки. */
	UFUNCTION(BlueprintCallable, Category="Clanhall|UI")
	static void SaveHUDLayout(FVector2D PlayerFramePosition, FVector2D EnemyFramePosition);

	/** Загружает позиции рамок. Возвращает false при первом запуске (используй дефолты).
	 *  Вызывать в Event Construct главного HUD-виджета. */
	UFUNCTION(BlueprintCallable, Category="Clanhall|UI")
	static bool LoadHUDLayout(FVector2D& PlayerFramePosition, FVector2D& EnemyFramePosition);
};
