#include "UI/ClanhallBossFrameContainerWidget.h"
#include "UI/ClanhallFighterFrameWidget.h"
#include "AbilitySystem/ClanhallBossSensorComponent.h"
#include "Components/VerticalBox.h"

void UClanhallBossFrameContainerWidget::SetSensor(UClanhallBossSensorComponent* InSensor)
{
	if (UClanhallBossSensorComponent* Old = Sensor.Get())
	{
		Old->OnFrameUnitEntered.RemoveDynamic(this, &UClanhallBossFrameContainerWidget::HandleUnitEntered);
		Old->OnFrameUnitExited.RemoveDynamic(this, &UClanhallBossFrameContainerWidget::HandleUnitExited);
	}
	ClearFrames();

	Sensor = InSensor;
	if (InSensor)
	{
		InSensor->OnFrameUnitEntered.AddUniqueDynamic(this, &UClanhallBossFrameContainerWidget::HandleUnitEntered);
		InSensor->OnFrameUnitExited.AddUniqueDynamic(this, &UClanhallBossFrameContainerWidget::HandleUnitExited);
	}
}

void UClanhallBossFrameContainerWidget::NativeDestruct()
{
	SetSensor(nullptr);
	Super::NativeDestruct();
}

void UClanhallBossFrameContainerWidget::HandleUnitEntered(AActor* Unit)
{
	if (!Unit || ActiveFrames.Contains(Unit) || !EnemyFrameClass || !FramesBox)
	{
		return;
	}
	UClanhallFighterFrameWidget* Frame = CreateWidget<UClanhallFighterFrameWidget>(GetOwningPlayer(), EnemyFrameClass);
	if (!Frame)
	{
		return;
	}
	FramesBox->AddChildToVerticalBox(Frame);
	Frame->SetFighter(Unit);
	ActiveFrames.Add(Unit, Frame);
}

void UClanhallBossFrameContainerWidget::HandleUnitExited(AActor* Unit)
{
	TObjectPtr<UClanhallFighterFrameWidget> Frame;
	if (ActiveFrames.RemoveAndCopyValue(Unit, Frame) && Frame)
	{
		Frame->RemoveFromParent();
	}
}

void UClanhallBossFrameContainerWidget::ClearFrames()
{
	for (const TPair<TObjectPtr<AActor>, TObjectPtr<UClanhallFighterFrameWidget>>& Entry : ActiveFrames)
	{
		if (Entry.Value)
		{
			Entry.Value->RemoveFromParent();
		}
	}
	ActiveFrames.Reset();
}
