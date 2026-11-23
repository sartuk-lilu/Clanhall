#include "AbilitySystem/ClanhallSkillCooldownComponent.h"
#include "ClanhallCombatSettings.h"
#include "Engine/World.h"

UClanhallSkillCooldownComponent::UClanhallSkillCooldownComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

double UClanhallSkillCooldownComponent::Now() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

void UClanhallSkillCooldownComponent::StartCooldown(FGameplayTag Skill, float Beats)
{
	if (!Skill.IsValid() || Beats <= 0.0f)
	{
		return;
	}

	FCooldownEntry& Entry = Cooldowns.FindOrAdd(Skill);
	Entry.ReadyAt = Now() + ClanhallBeats::ToSeconds(Beats);
	Entry.TotalBeats = Beats;

	OnCooldownStarted.Broadcast(Skill, Beats);
}

bool UClanhallSkillCooldownComponent::IsOnCooldown(FGameplayTag Skill) const
{
	return GetRemainingBeats(Skill) > 0.0f;
}

float UClanhallSkillCooldownComponent::GetRemainingBeats(FGameplayTag Skill) const
{
	const FCooldownEntry* Entry = Cooldowns.Find(Skill);
	if (!Entry)
	{
		return 0.0f;
	}
	return FMath::Max(0.0f, ClanhallBeats::FromSeconds(static_cast<float>(Entry->ReadyAt - Now())));
}

float UClanhallSkillCooldownComponent::GetTotalBeats(FGameplayTag Skill) const
{
	const FCooldownEntry* Entry = Cooldowns.Find(Skill);
	return Entry ? Entry->TotalBeats : 0.0f;
}

TArray<FGameplayTag> UClanhallSkillCooldownComponent::GetActiveCooldowns() const
{
	TArray<FGameplayTag> Result;
	const double Time = Now();
	for (const TPair<FGameplayTag, FCooldownEntry>& Pair : Cooldowns)
	{
		if (Pair.Value.ReadyAt > Time)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}
