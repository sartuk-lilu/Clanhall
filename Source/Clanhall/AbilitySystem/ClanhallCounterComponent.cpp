#include "AbilitySystem/ClanhallCounterComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/Engine.h"

void UClanhallCounterComponent::OpenWindow(FGameplayTag InCounterTag, FGameplayAbilitySpecHandle InCounteredHandle, FGameplayTag InCooldownTag, float InCooldownDuration)
{
	bWindowOpen = true;
	ActiveCounterTag = InCounterTag;
	CounteredHandle = InCounteredHandle;
	CounteredCooldownTag = InCooldownTag;
	CounteredCooldownDuration = InCooldownDuration;

	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->AddLooseGameplayTag(ClanhallGameplayTags::State_CounterWindow.GetTag());
	}
}

void UClanhallCounterComponent::CloseWindow()
{
	bWindowOpen = false;
	ActiveCounterTag = FGameplayTag();
	CounteredHandle = FGameplayAbilitySpecHandle();
	CounteredCooldownTag = FGameplayTag();
	CounteredCooldownDuration = 0.0f;

	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->RemoveLooseGameplayTag(ClanhallGameplayTags::State_CounterWindow.GetTag());
	}
}

bool UClanhallCounterComponent::IsCounterableBy(FGameplayTag IncomingTag) const
{
	return bWindowOpen && IncomingTag.IsValid() && ActiveCounterTag == IncomingTag;
}

void UClanhallCounterComponent::ConsumeCounter()
{
	if (!bWindowOpen)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->CancelAbilityHandle(CounteredHandle);

		if (CounteredCooldownTag.IsValid() && CounteredCooldownDuration > 0.0f)
		{
			ClanhallGameplayEffects::ApplyTimedTag(ASC, CounteredCooldownTag, CounteredCooldownDuration);
		}

#if !UE_BUILD_SHIPPING
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("✓ КОНТРНАВЫК! Навык прерван, полный КД"));
#endif
	}

	CloseWindow();
}

bool UClanhallCounterComponent::TryResolveCounter(AActor* Target, FGameplayTag IncomingCounterTag)
{
	if (!Target || !IncomingCounterTag.IsValid())
	{
		return false;
	}

	UClanhallCounterComponent* TargetComp = Target->FindComponentByClass<UClanhallCounterComponent>();
	if (!TargetComp || !TargetComp->IsCounterableBy(IncomingCounterTag))
	{
		return false;
	}

	TargetComp->ConsumeCounter();
	return true;
}

UAbilitySystemComponent* UClanhallCounterComponent::GetASC()
{
	if (!CachedASC.IsValid())
	{
		if (IAbilitySystemInterface* Owner = Cast<IAbilitySystemInterface>(GetOwner()))
		{
			CachedASC = Owner->GetAbilitySystemComponent();
		}
	}
	return CachedASC.Get();
}
