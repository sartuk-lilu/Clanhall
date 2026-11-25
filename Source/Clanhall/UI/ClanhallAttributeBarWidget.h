// Полоска атрибута (AP, HP, MP): логика - здесь, вёрстка и дизайн - в WBP_AttributeBar
// (`Implementation/HUD.md`). Атрибуты и бойца задаёт родитель - рамка; полоска сама ничего не
// достаёт. Обновление - по делегатам изменения атрибута, а не поллингом.
//
// Имена полей отличаются от старых переменных Blueprint (TargetASC, FillColor, TrackedAttribute,
// MaxAttribute) намеренно: одноимённое поле родителя ломает компиляцию наследника до чистки графа.

#pragma once

#include "Blueprint/UserWidget.h"
#include "AttributeSet.h"
#include "ClanhallAttributeBarWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;

UCLASS(Abstract)
class CLANHALL_API UClanhallAttributeBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Боец и атрибуты полоски. Повторный вызов отписывает от прежнего бойца. */
	void SetAttributeSource(UAbilitySystemComponent* InASC, const FGameplayAttribute& InCurrent, const FGameplayAttribute& InMax);

	/** Текущий максимум; 0 - атрибута у бойца нет (рамка прячет такую полоску). */
	float GetMaxValue() const;

	/** Цвет заливки - дизайн конкретного экземпляра полоски на рамке. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clanhall|Bar")
	FLinearColor BarFillColor = FLinearColor::White;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText;

	virtual void NativePreConstruct() override;
	virtual void NativeDestruct() override;

private:
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	FGameplayAttribute CurrentAttribute;
	FGameplayAttribute MaxAttribute;
	FDelegateHandle CurrentHandle;
	FDelegateHandle MaxHandle;

	void Unbind();
	void OnAttributeChanged(const FOnAttributeChangeData& Data);
	void Refresh();
};
