// Кристалл (`EconomySystem.md`, «Кристалл»; `CombatSystem.md`, «Кристалл»): единственный источник
// AP в бою. Доля кристалла копится за убийства - сколько даёт жертва, решает она сама
// (AClanhallCharacterBase::CrystalShareOnKill). Целый кристалл тратит UGA_UseCrystal.
//
// Симметрично: компонент на любом бойце, у босса свои кристаллы. Убийство зачисляет
// AClanhallCharacterBase::HandleOutOfHealth жертвы - компоненту не нужно слушать чужие смерти.

#pragma once

#include "Components/ActorComponent.h"
#include "ClanhallCrystalComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClanhallCrystalChanged, float, Crystal);

UCLASS(ClassGroup = "Clanhall", meta = (BlueprintSpawnableComponent))
class CLANHALL_API UClanhallCrystalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Добавить долю. Сверх потолка (CrystalCap целых) не копится. */
	void AddShare(float Share);

	/** Есть ли целый кристалл - условие применения. */
	bool HasWholeCrystal() const { return Crystal >= 1.0f; }

	/** Списать целый кристалл - в конце применения, не в начале: прерванное не тратит. */
	bool SpendWhole();

	/** Накопленное, в кристаллах: 2.3 - два целых и доля следующего. */
	UFUNCTION(BlueprintPure, Category = "Clanhall|Crystal")
	float GetCrystal() const { return Crystal; }

	UPROPERTY(BlueprintAssignable, Category = "Clanhall|Crystal")
	FOnClanhallCrystalChanged OnCrystalChanged;

private:
	float Crystal = 0.0f;
};
