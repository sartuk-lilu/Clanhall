// Раздел 6 (переработан): базовый класс активных навыков врага с окном контрнавыка.
// При активации открывает окно на своём UClanhallCounterComponent (State.CounterWindow на ASC) —
// игрок контрит, активировав навык с тем же CounterTag, без модификатора (clanhall_claude_code_counter.md).
// Если НЕ прерван → по истечении окна наносит урон/метку игроку.
// Если прерван → ConsumeCounter вызывает CancelAbilityHandle → WaitDelay-задача снимается → урон не применяется.
//
// Интерим: реальных монтажей у врагов пока нет, поэтому окно открывается/закрывается прямо из кода
// (ActivateAbility/OnHitDelayExpired), а не через UAnimNotifyState_CounterWindow — тот класс уже
// построен и подключится сам, когда появятся монтажи (по образцу interim-подхода с State.Parrying,
// см. Раздел 6.5).

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

	FGameplayTag GetCounterTag() const { return CounterTag; }
	FGameplayTag GetCooldownTag() const { return CooldownTag; }
	float GetCooldownDuration() const { return Cooldown; }

protected:
	/** Идентичность навыка для контрнавыка — тот же тег, что и у навыка игрока (см. AbilityTags). */
	UPROPERTY(EditDefaultsOnly, Category = "Counter", meta = (Categories = "Ability.Skill"))
	FGameplayTag CounterTag;

	/** Тег слота КД, который получит владелец при успешном контре (ConsumeCounter). */
	UPROPERTY(EditDefaultsOnly, Category = "Counter", meta = (Categories = "Cooldown.Slot"))
	FGameplayTag CooldownTag;

	/** Длительность полного КД при контре, секунды — по тиру навыка (ability_system.md §3). */
	UPROPERTY(EditDefaultsOnly, Category = "Counter")
	float Cooldown = 10.0f;

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
