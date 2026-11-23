#include "AbilitySystem/ClanhallCrystalComponent.h"
#include "ClanhallCombatSettings.h"

void UClanhallCrystalComponent::AddShare(float Share)
{
	const float Cap = static_cast<float>(UClanhallCombatSettings::Get().CrystalCap);
	const float Before = Crystal;
	Crystal = FMath::Clamp(Crystal + Share, 0.0f, Cap);
	if (!FMath::IsNearlyEqual(Before, Crystal))
	{
		OnCrystalChanged.Broadcast(Crystal);
	}
}

bool UClanhallCrystalComponent::SpendWhole()
{
	if (!HasWholeCrystal())
	{
		return false;
	}
	Crystal -= 1.0f;
	OnCrystalChanged.Broadcast(Crystal);
	return true;
}
