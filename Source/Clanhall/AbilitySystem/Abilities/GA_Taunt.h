// Насмешка (`CombatSystem.md`, «Насмешка») - общее действие, как бег и прыжок: не зависит от
// оружия в руках, своя клавиша и своя перезарядка. Цена - короткая анимация, в которой боец
// открыт (State.Taunting на всё время).
//
// - Подтверждённое попадание по насмехающемуся срывает насмешку без награды - это делает
//   ClanhallDamage::ResolveHit (отмена по тегу Ability.Common.Taunt).
// - Против гораздо более слабого насмешка - парирование: попадание приёма слабого по
//   насмехающемуся останавливается до урона, меток и контра, оба уходят в анимацию
//   (State.TauntParried / State.TauntParrying) - это делает UGA_PhysicalSkill::ResolveHitOn.
//   Работает в обе стороны. Окна под урон нет.
//
// Наследуется от UGameplayAbility напрямую, той же причиной, что UGA_Dodge: урона не наносит.
// Гранится всем гуманоидам (AClanhallHumanoidBase): противник тоже насмехается.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "GA_Taunt.generated.h"

class UAnimMontage;

UCLASS()
class CLANHALL_API UGA_Taunt : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Taunt();

	/** Своя перезарядка насмешки - через компонент перезарядки, ключ Ability.Common.Taunt. */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** Косметический монтаж насмешки - механика не зависит от него. nullptr законен. Подгоняется
	 *  под TauntBeats play rate'ом. */
	UPROPERTY(EditDefaultsOnly, Category = "Taunt|Animation")
	TObjectPtr<UAnimMontage> TauntMontage;

private:
	UFUNCTION()
	void OnTauntElapsed();

	TWeakObjectPtr<UAnimMontage> PlayedMontage;
};
