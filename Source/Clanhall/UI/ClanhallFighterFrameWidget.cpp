#include "UI/ClanhallFighterFrameWidget.h"
#include "UI/ClanhallAttributeBarWidget.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UClanhallFighterFrameWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	APBar = Cast<UClanhallAttributeBarWidget>(GetWidgetFromName(TEXT("WBP_AP_Bar")));
	HPBar = Cast<UClanhallAttributeBarWidget>(GetWidgetFromName(TEXT("WBP_HP_Bar")));
	MPBar = Cast<UClanhallAttributeBarWidget>(GetWidgetFromName(TEXT("WBP_MP_Bar")));
}

void UClanhallFighterFrameWidget::SetFighter(AActor* Fighter)
{
	FighterActor = Fighter;
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Fighter);
	UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;

	// Какую пару атрибутов показывает полоска, решает её место на рамке, а не настройка экземпляра.
	if (APBar)
	{
		APBar->SetAttributeSource(ASC, UClanhallAttributeSet::GetAPAttribute(), UClanhallAttributeSet::GetMaxAPAttribute());
	}
	if (HPBar)
	{
		HPBar->SetAttributeSource(ASC, UClanhallAttributeSet::GetHPAttribute(), UClanhallAttributeSet::GetMaxHPAttribute());
	}
	if (MPBar)
	{
		MPBar->SetAttributeSource(ASC, UClanhallAttributeSet::GetMPAttribute(), UClanhallAttributeSet::GetMaxMPAttribute());
	}

	UpdateBarVisibility();
	// Максимумы бойца инициализируются в его BeginPlay - видимость ещё раз на следующем тике.
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UClanhallFighterFrameWidget::UpdateBarVisibility));
	}
}

void UClanhallFighterFrameWidget::UpdateBarVisibility()
{
	if (!bHideEmptyBars)
	{
		return;
	}
	for (UClanhallAttributeBarWidget* BarWidget : { APBar.Get(), MPBar.Get() })
	{
		if (BarWidget)
		{
			BarWidget->SetVisibility(BarWidget->GetMaxValue() > 0.0f ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
}
