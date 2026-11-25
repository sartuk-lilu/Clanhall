#include "UI/ClanhallAttributeBarWidget.h"
#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UClanhallAttributeBarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	// В дизайнере тоже: цвет экземпляра виден на рамке без запуска игры.
	if (Bar)
	{
		Bar->SetFillColorAndOpacity(BarFillColor);
	}
}

void UClanhallAttributeBarWidget::NativeDestruct()
{
	Unbind();
	Super::NativeDestruct();
}

void UClanhallAttributeBarWidget::SetAttributeSource(UAbilitySystemComponent* InASC, const FGameplayAttribute& InCurrent, const FGameplayAttribute& InMax)
{
	Unbind();
	SourceASC = InASC;
	CurrentAttribute = InCurrent;
	MaxAttribute = InMax;

	if (InASC)
	{
		if (CurrentAttribute.IsValid())
		{
			CurrentHandle = InASC->GetGameplayAttributeValueChangeDelegate(CurrentAttribute).AddUObject(this, &UClanhallAttributeBarWidget::OnAttributeChanged);
		}
		if (MaxAttribute.IsValid())
		{
			MaxHandle = InASC->GetGameplayAttributeValueChangeDelegate(MaxAttribute).AddUObject(this, &UClanhallAttributeBarWidget::OnAttributeChanged);
		}
	}
	Refresh();

	// Стартовые значения атрибутов ставятся в BeginPlay бойца без делегата изменения (Init*), а
	// HUD может получить бойца раньше: ещё одно обновление на следующем тике, без таймеров-костылей.
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UClanhallAttributeBarWidget::Refresh));
	}
}

float UClanhallAttributeBarWidget::GetMaxValue() const
{
	const UAbilitySystemComponent* ASC = SourceASC.Get();
	return (ASC && MaxAttribute.IsValid()) ? ASC->GetNumericAttribute(MaxAttribute) : 0.0f;
}

void UClanhallAttributeBarWidget::Unbind()
{
	if (UAbilitySystemComponent* ASC = SourceASC.Get())
	{
		if (CurrentHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(CurrentAttribute).Remove(CurrentHandle);
		}
		if (MaxHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(MaxAttribute).Remove(MaxHandle);
		}
	}
	CurrentHandle.Reset();
	MaxHandle.Reset();
	SourceASC.Reset();
}

void UClanhallAttributeBarWidget::OnAttributeChanged(const FOnAttributeChangeData& Data)
{
	Refresh();
}

void UClanhallAttributeBarWidget::Refresh()
{
	const UAbilitySystemComponent* ASC = SourceASC.Get();
	const float Current = (ASC && CurrentAttribute.IsValid()) ? ASC->GetNumericAttribute(CurrentAttribute) : 0.0f;
	const float Max = GetMaxValue();

	if (Bar)
	{
		Bar->SetPercent(Max > 0.0f ? FMath::Clamp(Current / Max, 0.0f, 1.0f) : 0.0f);
	}
	if (ValueText)
	{
		ValueText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), FMath::RoundToInt(Current), FMath::RoundToInt(Max))));
	}
}
