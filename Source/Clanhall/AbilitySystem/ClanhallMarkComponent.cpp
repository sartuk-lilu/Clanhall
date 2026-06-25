#include "ClanhallMarkComponent.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

namespace
{
	// mark_system.md §4: метка живёт 5 секунд с момента наложения.
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

void UClanhallMarkComponent::ApplyMark(FGameplayTag NewMark)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC || !NewMark.IsValid())
	{
		return;
	}

	// Правило максимума: старая метка снимается перед накладыванием новой, без стека (mark_system.md §2).
	ClearMark();

	ActiveMarkEffectHandle = ClanhallGameplayEffects::ApplyTimedTag(ASC, NewMark, MarkDurationSeconds);
	if (ActiveMarkEffectHandle.IsValid())
	{
		CachedMarkTag = NewMark;
	}
}

void UClanhallMarkComponent::ClearMark()
{
	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		if (ActiveMarkEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(ActiveMarkEffectHandle);
		}
	}

	ActiveMarkEffectHandle.Invalidate();
	CachedMarkTag = FGameplayTag();
}

FGameplayTag UClanhallMarkComponent::GetCurrentMark() const
{
	const UAbilitySystemComponent* ASC = GetOwnerASC();
	if (ASC && CachedMarkTag.IsValid() && ASC->HasMatchingGameplayTag(CachedMarkTag))
	{
		return CachedMarkTag;
	}
	return FGameplayTag();
}
