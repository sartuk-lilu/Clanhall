// Рамка бойца - игрока или противника: три полоски AP / HP / MP. Логика - здесь, вёрстка -
// в WBP_PlayerFrame и WBP_EnemyFrame. Бойца задаёт родитель (HUD или контейнер рамок боссов).
// Будущие значки меток, кристалл и фраза на рамке (`Controls.md`, «Плашка противника») - сюда же.

#pragma once

#include "Blueprint/UserWidget.h"
#include "ClanhallFighterFrameWidget.generated.h"

class UClanhallAttributeBarWidget;

UCLASS(Abstract)
class CLANHALL_API UClanhallFighterFrameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Боец рамки: ASC полоскам, полоски без атрибута скрыты. nullptr - рамка пустая. */
	UFUNCTION(BlueprintCallable, Category = "Clanhall|HUD")
	void SetFighter(AActor* Fighter);

	UFUNCTION(BlueprintPure, Category = "Clanhall|HUD")
	AActor* GetFighter() const { return FighterActor.Get(); }

protected:
	/** Полоска с максимумом 0 скрывается: боссы-монстры без AP и MP показывают только HP
	 *  (`Implementation/HUD.md`, «Enemy Frame»). */
	UPROPERTY(EditAnywhere, Category = "Clanhall|HUD")
	bool bHideEmptyBars = true;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UClanhallAttributeBarWidget> WBP_AP_Bar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UClanhallAttributeBarWidget> WBP_HP_Bar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UClanhallAttributeBarWidget> WBP_MP_Bar;

private:
	TWeakObjectPtr<AActor> FighterActor;

	void UpdateBarVisibility();
};
