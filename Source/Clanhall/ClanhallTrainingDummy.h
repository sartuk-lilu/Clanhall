// Простая GAS-цель для тестирования WASD-обмена AP/HP (Раздел 2). Без AI и анимаций —
// просто стоит и принимает удары. Стартовые AP/HP взяты у Часового (enemy_prototypes.md),
// чтобы этот же актор естественно перерос в задел для Раздела 7.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"
#include "ClanhallTrainingDummy.generated.h"

class UAbilitySystemComponent;
class UClanhallAttributeSet;
class UClanhallMarkComponent;
class UClanhallCounterComponent;
class UGA_EnemyWASDSeries;
class UGA_EnemyActiveSkill;

UCLASS()
class AClanhallTrainingDummy : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallMarkComponent> MarkComponent;

	/** Раздел 6 (переработан): держит окно контрнавыка, когда играет UGA_Enemy_PowerStrike
	 *  (clanhall_claude_code_counter.md). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallCounterComponent> CounterComponent;

public:
	AClanhallTrainingDummy();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	virtual void BeginPlay() override;

private:
	FGameplayAbilitySpecHandle SeriesHandle;
	FTimerHandle AttackLoopTimer;

	// Раздел 6: Power Strike — активный навык врага для тестирования контрнавыка.
	FGameplayAbilitySpecHandle PowerStrikeHandle;
	FTimerHandle PowerStrikeLoopTimer;

	void TryStartSeries();
	void TryStartPowerStrike();
};
