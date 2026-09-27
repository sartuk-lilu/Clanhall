#include "ClanhallMarkComponent.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

namespace
{
	// (`mark_system.md`, «Время жизни метки»): метка живёт 5 секунд с момента наложения,
	// одинаково для обоих треков.
	constexpr float MarkDurationSeconds = 5.0f;
}

UClanhallMarkComponent::UClanhallMarkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UAbilitySystemComponent* UClanhallMarkComponent::GetOwnerASC() const
{
	if (const IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return Interface->GetAbilitySystemComponent();
	}
	return nullptr;
}

void UClanhallMarkComponent::ApplyMark(EClanhallMarkTrack Track, FGameplayTag NewMark, UAbilitySystemComponent* InSourceASC)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC || !NewMark.IsValid())
	{
		return;
	}

	// Правило максимума: старая метка ЭТОГО трека снимается перед накладыванием новой, без
	// стека (`mark_system.md`, «Перезапись»). Метка другого трека не трогается.
	ClearMark(Track);

	FMarkTrackState& State = Tracks[static_cast<uint8>(Track)];
	State.ActiveMarkEffectHandle = ClanhallGameplayEffects::ApplyTimedTag(ASC, NewMark, MarkDurationSeconds);
	if (State.ActiveMarkEffectHandle.IsValid())
	{
		State.CachedMarkTag = NewMark;
		State.CurrentMarkSourceASC = InSourceASC;
	}
}

void UClanhallMarkComponent::ClearMark(EClanhallMarkTrack Track)
{
	FMarkTrackState& State = Tracks[static_cast<uint8>(Track)];

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		if (State.ActiveMarkEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(State.ActiveMarkEffectHandle);
		}
	}

	State.ActiveMarkEffectHandle.Invalidate();
	State.CachedMarkTag = FGameplayTag();
	State.CurrentMarkSourceASC = nullptr;
}

bool UClanhallMarkComponent::IsOwnMark(EClanhallMarkTrack Track, const UAbilitySystemComponent* QueryASC) const
{
	const FMarkTrackState& State = Tracks[static_cast<uint8>(Track)];
	return GetCurrentMark(Track).IsValid()
		&& State.CurrentMarkSourceASC.IsValid()
		&& State.CurrentMarkSourceASC.Get() == QueryASC;
}

FGameplayTag UClanhallMarkComponent::GetCurrentMark(EClanhallMarkTrack Track) const
{
	const FMarkTrackState& State = Tracks[static_cast<uint8>(Track)];
	const UAbilitySystemComponent* ASC = GetOwnerASC();
	if (ASC && State.CachedMarkTag.IsValid() && ASC->HasMatchingGameplayTag(State.CachedMarkTag))
	{
		return State.CachedMarkTag;
	}
	return FGameplayTag();
}

void UClanhallMarkComponent::ApplySelfBuff(TSubclassOf<UGameplayEffect> EffectClass)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC || !EffectClass)
	{
		return;
	}

	// Снятие по устаревшему/невалидному хендлу безопасно — не нужно отдельно проверять,
	// истёк ли прежний бафф.
	if (SelfBuffHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(SelfBuffHandle);
	}

	SelfBuffHandle = ClanhallGameplayEffects::ApplyEffect(ASC, ASC, EffectClass);
}

void UClanhallMarkComponent::ApplyDebuffFrom(UAbilitySystemComponent* SourceASC, TSubclassOf<UGameplayEffect> EffectClass)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC || !SourceASC || !EffectClass)
	{
		return;
	}

	// Протухшие ключи (создатель уничтожен) не мешают карте расти бесконечно — чистим их
	// при каждой вставке, а не отдельным проходом по таймеру.
	for (auto It = DebuffHandlesBySource.CreateIterator(); It; ++It)
	{
		if (!It->Key.IsValid())
		{
			It.RemoveCurrent();
		}
	}

	const TWeakObjectPtr<UAbilitySystemComponent> SourceKey(SourceASC);
	if (const FActiveGameplayEffectHandle* Existing = DebuffHandlesBySource.Find(SourceKey))
	{
		if (Existing->IsValid())
		{
			ASC->RemoveActiveGameplayEffect(*Existing);
		}
	}

	DebuffHandlesBySource.Add(SourceKey, ClanhallGameplayEffects::ApplyEffect(SourceASC, ASC, EffectClass));
}
