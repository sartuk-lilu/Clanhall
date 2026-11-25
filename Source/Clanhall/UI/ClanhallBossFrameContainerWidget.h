// Контейнер рамок боссов: по рамке на каждого Unit.Role.Boss.* в радиусе сенсора
// (`Implementation/HUD.md`, «Сенсор Enemy Frame»). Логика - здесь, вёрстка - в
// WBP_BossFrameContainer. Сенсор задаёт родитель - HUD.

#pragma once

#include "Blueprint/UserWidget.h"
#include "ClanhallBossFrameContainerWidget.generated.h"

class UVerticalBox;
class UClanhallFighterFrameWidget;
class UClanhallBossSensorComponent;

UCLASS(Abstract)
class CLANHALL_API UClanhallBossFrameContainerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Сенсор игрока; повторный вызов отписывает от прежнего и убирает его рамки. */
	void SetSensor(UClanhallBossSensorComponent* InSensor);

protected:
	/** Рамка одного босса - вёрстка WBP_EnemyFrame. */
	UPROPERTY(EditAnywhere, Category = "Clanhall|HUD")
	TSubclassOf<UClanhallFighterFrameWidget> EnemyFrameClass;

	UPROPERTY(BlueprintReadOnly, Category = "Clanhall|HUD", meta = (BindWidget))
	TObjectPtr<UVerticalBox> FramesBox;

	virtual void NativeDestruct() override;

private:
	TWeakObjectPtr<UClanhallBossSensorComponent> Sensor;

	/** Рамки по бойцам. Имя не Frames: так называется переменная старого графа. */
	UPROPERTY(Transient)
	TMap<TObjectPtr<AActor>, TObjectPtr<UClanhallFighterFrameWidget>> ActiveFrames;

	UFUNCTION()
	void HandleUnitEntered(AActor* Unit);

	UFUNCTION()
	void HandleUnitExited(AActor* Unit);

	void ClearFrames();
};
