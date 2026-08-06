#include "AbilitySystem/ClanhallCounterComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallParryComponent.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/Engine.h"

void UClanhallCounterComponent::OpenWindow(const FGameplayTagContainer& InCounteredBy, FGameplayAbilitySpecHandle InCounteredHandle, FGameplayTag InCooldownTag, float InCooldownDuration)
{
	bWindowOpen = true;
	CounteredByTags = InCounteredBy;
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
	CounteredByTags.Reset();
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
	return bWindowOpen && IncomingTag.IsValid() && CounteredByTags.HasTag(IncomingTag);
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
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("✓ КОНТРНАВЫК! Навык прерван, полный КД, +1 Stagger"));
#endif
	}

	// task_stagger_control_code.md §5.2: +1 усталости сбитому и хитстоп — переиспользуем
	// UClanhallHitboxComponent::ApplyHitstop, ту же реализацию, что резолв клэша в TryParry.
	// Синхронно, не через OnCounterConsumed — тот делегат для будущей реакции получателя (флинч/VFX),
	// хитстоп должен ударить в тот же кадр, что и сам контр.
	if (AActor* Owner = GetOwner())
	{
		if (UClanhallParryComponent* OwnParry = Owner->FindComponentByClass<UClanhallParryComponent>())
		{
			OwnParry->AddStagger(1.0f);
		}

		if (UClanhallHitboxComponent* OwnHitbox = Owner->FindComponentByClass<UClanhallHitboxComponent>())
		{
			OwnHitbox->ApplyHitstop(OwnHitbox->HitstopDurationOnClash);
		}
	}

	OnCounterConsumed.Broadcast();

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
