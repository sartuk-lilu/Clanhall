#include "AbilitySystem/ClanhallCounterComponent.h"
#include "Debug/ClanhallDebugMessage.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "ClanhallCombatSettings.h"
#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/Engine.h"

void UClanhallCounterComponent::OpenWindow(const FGameplayTagContainer& InCounteredBy, FGameplayAbilitySpecHandle InCounteredHandle)
{
	bWindowOpen = true;
	CounteredByTags = InCounteredBy;
	CounteredHandle = InCounteredHandle;

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

	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->RemoveLooseGameplayTag(ClanhallGameplayTags::State_CounterWindow.GetTag());
	}
}

void UClanhallCounterComponent::NotifySkillActivated(const FGameplayTagContainer& InCounteredBy, FGameplayAbilitySpecHandle InHandle)
{
	ActiveSkillCounteredBy = InCounteredBy;
	ActiveSkillHandle = InHandle;
	ActiveSkillStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

void UClanhallCounterComponent::NotifySkillEnded(FGameplayAbilitySpecHandle InHandle)
{
	if (InHandle == ActiveSkillHandle)
	{
		ActiveSkillCounteredBy.Reset();
		ActiveSkillHandle = FGameplayAbilitySpecHandle();
		ActiveSkillStartTime = -1.0;
	}
}

bool UClanhallCounterComponent::IsCounterableBy(FGameplayTag IncomingTag) const
{
	return bWindowOpen && IncomingTag.IsValid() && CounteredByTags.HasTag(IncomingTag);
}

void UClanhallCounterComponent::ConsumeCounter()
{
	// Окно открыто - сбиваем приём окна; встык (окна ещё нет) - идущий приём.
	const FGameplayAbilitySpecHandle HandleToCancel = bWindowOpen ? CounteredHandle : ActiveSkillHandle;
	if (!HandleToCancel.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->CancelAbilityHandle(HandleToCancel);

		// Короткий стан с отменой приёма - только от контрнавыка и контрмагии, одинаковый для всех
		// (`CombatSystem.md`, «Контроль»). Хватает на продолжение связки, не на спам.
		ClanhallGameplayEffects::ApplyTimedTag(ASC, ClanhallGameplayTags::State_Stunned.GetTag(),
			ClanhallBeats::ToSeconds(UClanhallCombatSettings::Get().CounterStunBeats));

#if !UE_BUILD_SHIPPING
		ClanhallDebug::Screen(FColor::Green, TEXT("✓ КОНТРНАВЫК! Навык прерван"));
#endif
	}

	// Хитстоп сбитому — переиспользуем UClanhallHitboxComponent::ApplyHitstop, длительность —
	// HitstopDurationOnClash (хитстоп контра). Синхронно, не через OnCounterConsumed — тот делегат для будущей
	// реакции получателя (флинч/VFX), хитстоп должен ударить в тот же кадр, что и сам контр.
	if (AActor* Owner = GetOwner())
	{
		if (UClanhallHitboxComponent* OwnHitbox = Owner->FindComponentByClass<UClanhallHitboxComponent>())
		{
			OwnHitbox->ApplyHitstop(OwnHitbox->HitstopDurationOnClash);
		}
	}

	OnCounterConsumed.Broadcast();

	CloseWindow();
}

bool UClanhallCounterComponent::TryResolveCounter(AActor* Target, FGameplayTag IncomingCounterTag, AActor* Source, double SourceActivationTime)
{
	if (!Target || !IncomingCounterTag.IsValid())
	{
		return false;
	}

	UClanhallCounterComponent* TargetComp = Target->FindComponentByClass<UClanhallCounterComponent>();
	if (!TargetComp)
	{
		return false;
	}

	if (TargetComp->IsCounterableBy(IncomingCounterTag))
	{
		TargetComp->ConsumeCounter();
		return true;
	}

	// Одновременная активация (`AbilitySystem.md`, «Контрнавык»): оба выпустили приём встык - окно
	// цели ещё не открылось, но её приём контрится этим. Работает как перехват.
	if (TargetComp->ActiveSkillHandle.IsValid() && TargetComp->ActiveSkillCounteredBy.HasTag(IncomingCounterTag) && SourceActivationTime >= 0.0)
	{
		const double Delta = SourceActivationTime - TargetComp->ActiveSkillStartTime;
		const double Window = ClanhallBeats::ToSeconds(UClanhallCombatSettings::Get().SimultaneousBeats);
		if (Delta >= 0.0 && Delta <= Window)
		{
			// Кадр в кадр - редкий VFX-взрыв на обоих.
			const UWorld* World = Target->GetWorld();
			if (World && Delta <= World->GetDeltaSeconds())
			{
				TargetComp->OnPerfectClash.Broadcast(Source);
				if (UClanhallCounterComponent* SourceComp = Source ? Source->FindComponentByClass<UClanhallCounterComponent>() : nullptr)
				{
					SourceComp->OnPerfectClash.Broadcast(Target);
				}
			}
			TargetComp->ConsumeCounter();
			return true;
		}
	}

	return false;
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
