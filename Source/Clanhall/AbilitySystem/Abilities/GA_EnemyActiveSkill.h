// Раздел 6: базовый класс активных навыков врага с окном контрнавыка.
// При активации открывает State.CounterWindow на своём ASC — игрок может
// прервать через LMB+Ctrl+<matching skill> в течение CounterWindowDuration.
// Если НЕ прерван → по истечении окна наносит урон/метку игроку.
// Если прерван → CancelAbility снимает WaitDelay-задачи → урон не применяется.
//
// AbilityTags наследника (UGA_Enemy_PowerStrike) содержат тот же тег, что у
// соответствующего навыка игрока → CancelAbilities(Ability.Skill.*) его находит.

#pragma once

#include "GA_ClanhallAbilityBase.h"
#include "GameplayTagContainer.h"
#include "GA_EnemyActiveSkill.generated.h"

UCLASS(Abstract)
class CLANHALL_API UGA_EnemyActiveSkill : public UGA_ClanhallAbilityBase
{
	GENERATED_BODY()

public:
	UGA_EnemyActiveSkill();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

protected:
	/** Длительность окна контрнавыка в секундах. Одновременно — задержка до удара. */
	UPROPERTY(EditDefaultsOnly, Category = "Counter")
	float CounterWindowDuration = 1.2f;

	/** Урон по AP игрока при непрерванном ударе. */
	UPROPERTY(EditDefaultsOnly, Category = "Hit")
	float HitDamage = 40.0f;

	/** Опциональная метка, накладываемая на игрока при попадании. */
	UPROPERTY(EditDefaultsOnly, Category = "Hit", meta = (Categories = "Mark"))
	FGameplayTag HitMarkTag;

private:
	/** Вызывается когда окно контрнавыка истекло без прерывания — применяем удар. */
	UFUNCTION()
	void OnHitDelayExpired();
};

// ---------------------------------------------------------------------------
// UGA_Enemy_PowerStrike — конкретная реализация «Power Strike» для врагов.
// AbilityTags = {Ability.Skill.Knight.PowerStrike}: тот же тег, что у игрока (Knight E),
// — контрнавык детектирует через CancelAbilities(Ability.Skill.*) (development_plan.md §6).
// ---------------------------------------------------------------------------
UCLASS()
class CLANHALL_API UGA_Enemy_PowerStrike : public UGA_EnemyActiveSkill
{
	GENERATED_BODY()
public:
	UGA_Enemy_PowerStrike();
};
