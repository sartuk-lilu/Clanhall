// Боевая стойка (ЛКМ зажат). Канон: combat_system.md §3.
// Сама способность не делает ничего активно — её единственная роль — держать тег
// State.InStance, пока она активна (через ActivationOwnedTags, движок добавляет/снимает
// тег автоматически в PreActivate/EndAbility). WASD-удары и активные навыки читают
// этот тег, чтобы понять, в стойке персонаж или нет.
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
};
