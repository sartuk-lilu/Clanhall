#include "UI/ClanhallHUDWidget.h"
#include "UI/ClanhallFighterFrameWidget.h"
#include "UI/ClanhallBossFrameContainerWidget.h"
#include "UI/ClanhallUILibrary.h"
#include "ClanhallCharacter.h"
#include "AbilitySystem/ClanhallBossSensorComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UClanhallHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	PlayerFrame = Cast<UClanhallFighterFrameWidget>(GetWidgetFromName(TEXT("WBP_PlayerFrame")));
	BossFrames = Cast<UClanhallBossFrameContainerWidget>(GetWidgetFromName(TEXT("WBP_BossFrameContainer")));
	Crosshair = GetWidgetFromName(TEXT("WBP_Crosshair"));
}

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

	if (PlayerFrame)
	{
		PlayerFrame->SetFighter(InCharacter);
	}
	if (BossFrames)
	{
		BossFrames->SetSensor(InCharacter ? InCharacter->FindComponentByClass<UClanhallBossSensorComponent>() : nullptr);
	}
	if (InCharacter)
	{
		InCharacter->OnInputLayerChanged.AddUniqueDynamic(this, &UClanhallHUDWidget::HandleInputLayerChanged);
	}
	HandleInputLayerChanged(InCharacter ? InCharacter->GetInputLayer() : EClanhallInputLayer::None);
}

void UClanhallHUDWidget::HandleInputLayerChanged(EClanhallInputLayer Layer)
{
	if (Crosshair)
	{
		Crosshair->SetVisibility(Layer == EClanhallInputLayer::Body ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

// ---------------------------------------------------------------------------
// Перетаскивание рамок (режим курсора, Alt) и раскладка
// ---------------------------------------------------------------------------

TArray<UWidget*> UClanhallHUDWidget::GetDraggableFrames() const
{
	TArray<UWidget*> Frames;
	if (PlayerFrame)
	{
		Frames.Add(PlayerFrame);
	}
	if (BossFrames)
	{
		Frames.Add(BossFrames);
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
	if (UCanvasPanelSlot* PlayerSlot = PlayerFrame ? UWidgetLayoutLibrary::SlotAsCanvasSlot(PlayerFrame) : nullptr)
	{
		PlayerSlot->SetPosition(PlayerPosition);
	}
	if (UCanvasPanelSlot* BossSlot = BossFrames ? UWidgetLayoutLibrary::SlotAsCanvasSlot(BossFrames) : nullptr)
	{
		BossSlot->SetPosition(BossPosition);
	}
}

void UClanhallHUDWidget::SaveLayout() const
{
	const UCanvasPanelSlot* PlayerSlot = PlayerFrame ? UWidgetLayoutLibrary::SlotAsCanvasSlot(PlayerFrame) : nullptr;
	const UCanvasPanelSlot* BossSlot = BossFrames ? UWidgetLayoutLibrary::SlotAsCanvasSlot(BossFrames) : nullptr;
	if (PlayerSlot && BossSlot)
	{
		UClanhallUILibrary::SaveHUDLayout(PlayerSlot->GetPosition(), BossSlot->GetPosition());
	}
}
