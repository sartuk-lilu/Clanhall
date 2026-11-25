// Корневой HUD (`Implementation/HUD.md`). Логика - здесь, вёрстка и расстановка элементов - в
// WBP_HUD. Принцип: дочерние виджеты - дизайн, данные и поведение им задаёт этот виджет.
//
// - Боец игрока: рамке игрока - его ASC, контейнеру боссов - его сенсор, прицелу - видимость по
//   слою ввода (`Controls.md`, «Ввод»: на ЛКМ появляется прицел). Событиями, без поллинга.
// - Перетаскивание рамок мышью в режиме курсора и раскладка - через UClanhallUILibrary.
//
// Имена полей отличаются от переменных старого графа (TargetASC, ActiveFrame, DragOffset,
// DraggableFrames, MouseAbs) намеренно: одноимённое поле родителя ломает компиляцию наследника.

#pragma once

#include "Blueprint/UserWidget.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallHUDWidget.generated.h"

class UCanvasPanel;
class UClanhallFighterFrameWidget;
class UClanhallBossFrameContainerWidget;
class AClanhallCharacter;

UCLASS(Abstract)
class CLANHALL_API UClanhallHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Боец игрока. Зовёт контроллер, когда завладел пешкой; повторный вызов перепривязывает. */
	void SetPlayerCharacter(AClanhallCharacter* InCharacter);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> HudCanvas;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UClanhallFighterFrameWidget> WBP_PlayerFrame;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UClanhallBossFrameContainerWidget> WBP_BossFrameContainer;

	/** Прицел - только дизайн; видимость задаёт HUD. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UUserWidget> WBP_Crosshair;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	TWeakObjectPtr<AClanhallCharacter> PlayerCharacter;

	/** Рамка, которую тащат сейчас, и смещение курсора от её угла. */
	TWeakObjectPtr<UWidget> DraggedFrame;
	FVector2D DragGrabOffset = FVector2D::ZeroVector;

	UFUNCTION()
	void HandleInputLayerChanged(EClanhallInputLayer Layer);

	void ApplySavedLayout();
	void SaveLayout() const;
	TArray<UWidget*> GetDraggableFrames() const;
	FVector2D ToCanvasLocal(const FPointerEvent& MouseEvent) const;
};
