// Применение кристалла (`EconomySystem.md`, «Кристалл») - общее действие, как насмешка: не
// зависит от оружия, своя клавиша. Требует целый кристалл. Длина - CrystalUseBeats, всё это время
// боец открыт (State.UsingCrystal): приёмы, уклонение и насмешка закрыты, движение - нет.
//
// В конце - восполнение AP (доля MaxAP, CrystalAPRestore) и списание кристалла. Прерванное
// применение кристалл не тратит. Прерывают стан и смерть; срыв попаданием - открытый пункт
// дизайна, не строится.
//
// Наследуется от UGameplayAbility напрямую: урона не наносит. Гранится любому бойцу
// (AClanhallCharacterBase) - у босса свои кристаллы.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "GA_UseCrystal.generated.h"

class UAnimMontage;

UCLASS()
class CLANHALL_API UGA_UseCrystal : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_UseCrystal();

	/** Нет целого кристалла - отказ. */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** Косметика - механика не зависит от монтажа. nullptr законен. Подгоняется под CrystalUseBeats. */
	UPROPERTY(EditDefaultsOnly, Category = "Crystal|Animation")
	TObjectPtr<UAnimMontage> UseMontage;

private:
	UFUNCTION()
	void OnUseElapsed();

	UFUNCTION()
	void OnInterrupted();

	TWeakObjectPtr<UAnimMontage> PlayedMontage;
};
