#include "UI/ClanhallHUDWidget.h"
#include "UI/ClanhallFighterFrameWidget.h"
#include "UI/ClanhallBossFrameContainerWidget.h"
#include "UI/ClanhallUILibrary.h"
#include "ClanhallCharacter.h"
#include "AbilitySystem/ClanhallBossSensorComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UClanhallHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplySavedLayout();

	// HUD, созданный, когда пешка уже есть, берёт её сам; иначе её передаст контроллер.
	if (AClanhallCharacter* Owner = Cast<AClanhallCharacter>(GetOwningPlayerPawn()))
	{
		SetPlayerCharacter(Owner);
	}
	else
	{
		HandleInputLayerChanged(EClanhallInputLayer::None);
	}
}

void UClanhallHUDWidget::NativeDestruct()
{
	SetPlayerCharacter(nullptr);
	Super::NativeDestruct();
}

void UClanhallHUDWidget::SetPlayerCharacter(AClanhallCharacter* InCharacter)
{
	if (AClanhallCharacter* Old = PlayerCharacter.Get())
	{
		Old->OnInputLayerChanged.RemoveDynamic(this, &UClanhallHUDWidget::HandleInputLayerChanged);
	}
	PlayerCharacter = InCharacter;

	if (WBP_PlayerFrame)
	{
		WBP_PlayerFrame->SetFighter(InCharacter);
	}
	if (WBP_BossFrameContainer)
	{
		WBP_BossFrameContainer->SetSensor(InCharacter ? InCharacter->FindComponentByClass<UClanhallBossSensorComponent>() : nullptr);
	}
	if (InCharacter)
	{
		InCharacter->OnInputLayerChanged.AddUniqueDynamic(this, &UClanhallHUDWidget::HandleInputLayerChanged);
	}
	HandleInputLayerChanged(InCharacter ? InCharacter->GetInputLayer() : EClanhallInputLayer::None);
}

void UClanhallHUDWidget::HandleInputLayerChanged(EClanhallInputLayer Layer)
{
	if (WBP_Crosshair)
	{
		WBP_Crosshair->SetVisibility(Layer == EClanhallInputLayer::Body ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

// ---------------------------------------------------------------------------
// Перетаскивание рамок (режим курсора, Alt) и раскладка
// ---------------------------------------------------------------------------

TArray<UWidget*> UClanhallHUDWidget::GetDraggableFrames() const
{
	TArray<UWidget*> Frames;
	if (WBP_PlayerFrame)
	{
		Frames.Add(WBP_PlayerFrame);
	}
	if (WBP_BossFrameContainer)
	{
		Frames.Add(WBP_BossFrameContainer);
	}
	return Frames;
}

FVector2D UClanhallHUDWidget::ToCanvasLocal(const FPointerEvent& MouseEvent) const
{
	return HudCanvas ? HudCanvas->GetCachedGeometry().AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()) : FVector2D::ZeroVector;
}

FReply UClanhallHUDWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	for (UWidget* Frame : GetDraggableFrames())
	{
		UCanvasPanelSlot* FrameSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Frame);
		if (FrameSlot && Frame->IsVisible() && Frame->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
		{
			DraggedFrame = Frame;
			DragGrabOffset = ToCanvasLocal(InMouseEvent) - FrameSlot->GetPosition();
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UClanhallHUDWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (UWidget* Frame = DraggedFrame.Get())
	{
		if (UCanvasPanelSlot* FrameSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Frame))
		{
			FrameSlot->SetPosition(ToCanvasLocal(InMouseEvent) - DragGrabOffset);
			return FReply::Handled();
		}
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UClanhallHUDWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (DraggedFrame.IsValid())
	{
		DraggedFrame.Reset();
		SaveLayout();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UClanhallHUDWidget::ApplySavedLayout()
{
	FVector2D PlayerPosition;
	FVector2D BossPosition;
	if (!UClanhallUILibrary::LoadHUDLayout(PlayerPosition, BossPosition))
	{
		return;
	}
	if (UCanvasPanelSlot* PlayerSlot = WBP_PlayerFrame ? UWidgetLayoutLibrary::SlotAsCanvasSlot(WBP_PlayerFrame) : nullptr)
	{
		PlayerSlot->SetPosition(PlayerPosition);
	}
	if (UCanvasPanelSlot* BossSlot = WBP_BossFrameContainer ? UWidgetLayoutLibrary::SlotAsCanvasSlot(WBP_BossFrameContainer) : nullptr)
	{
		BossSlot->SetPosition(BossPosition);
	}
}

void UClanhallHUDWidget::SaveLayout() const
{
	const UCanvasPanelSlot* PlayerSlot = WBP_PlayerFrame ? UWidgetLayoutLibrary::SlotAsCanvasSlot(WBP_PlayerFrame) : nullptr;
	const UCanvasPanelSlot* BossSlot = WBP_BossFrameContainer ? UWidgetLayoutLibrary::SlotAsCanvasSlot(WBP_BossFrameContainer) : nullptr;
	if (PlayerSlot && BossSlot)
	{
		UClanhallUILibrary::SaveHUDLayout(PlayerSlot->GetPosition(), BossSlot->GetPosition());
	}
}
