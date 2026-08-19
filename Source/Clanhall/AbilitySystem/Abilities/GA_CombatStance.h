// Боевая стойка (ЛКМ зажат). Канон: (`combat_system.md`, «Боевая стойка и переключение режимов»).
// Держит тег State.InStance, пока активна (через ActivationOwnedTags, движок добавляет/снимает
// тег автоматически в PreActivate/EndAbility) — WASD-удары и активные навыки читают
// этот тег, чтобы понять, в стойке персонаж или нет. С этапа 4 ещё и переключает ротацию
// и скорость движения на правила стойки (`combat_system.md`, «Боевая стойка и переключение
// режимов»; `locomotion_structure.md`, «Локомоция стойки»): ход спиной и стрейф не должны
// разворачивать персонаж лицом по ходу движения, как обычная локомоция.
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

	/** Переключает ротацию на "лицом за камерой" и скорость на StanceBaseSpeed бойца *
	 *  StanceSpeedMultiplier активного оружия, сохранив прежние значения для EndAbility.
	 *  StopMovementImmediately() — резкий стоп вместо доката
	 *  (`combat_system.md`, «Боевая стойка и переключение режимов»). */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Возвращает сохранённые ротацию и скорость. Выход из стойки идёт через
	 *  CancelAbilityHandle, то есть это всегда EndAbility(bWasCancelled=true) — восстановление
	 *  обязано работать что на отменённой, что на штатной ветке. Не константами: MaxWalkSpeed
	 *  вне стойки задаётся в BP-персонаже, хардкод затёр бы её при первом же выходе. */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	bool bSavedOrientRotationToMovement = true;
	bool bSavedUseControllerRotationYaw = false;
	float SavedMaxWalkSpeed = 0.0f;
};
