// Боевая стойка (ЛКМ зажат). Канон: (`combat_system.md`).
// Держит тег State.InStance, пока активна (через ActivationOwnedTags, движок добавляет/снимает
// тег автоматически в PreActivate/EndAbility) — WASD-удары и активные навыки читают
// этот тег, чтобы понять, в стойке персонаж или нет.
//
// Ротацию и скорость стойка больше не трогает вовсе - страйф с доворотом по камере теперь
// общая локомоция (AClanhallCharacter::BeginPlay/TickBodyTurn), а не привилегия стойки
// (`locomotion_structure.md`); множитель скорости оружия применяется при экипировке
// (`AClanhallHumanoidBase::PostInitializeComponents`), не в момент входа в стойку.
// Единственное, что стойка ещё делает с движением - гасит бег (CancelSprint) и текущий разгон
// (StopMovementImmediately), потому что скорость стойки обязана победить скорость бега.
//
// Активируется явно по хэндлу на нажатие ЛКМ, завершается по CancelAbilityHandle на отпускание —
// см. AClanhallCharacter::OnStancePressed/OnStanceReleased.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "GA_CombatStance.generated.h"

UCLASS()
class CLANHALL_API UGA_CombatStance : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_CombatStance();

	/** Гасит бег (CancelSprint) и текущий разгон (StopMovementImmediately) - скорость стойки
	 *  обязана победить скорость бега. Ротацию и скорость не трогает: обе - общая локомоция. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
