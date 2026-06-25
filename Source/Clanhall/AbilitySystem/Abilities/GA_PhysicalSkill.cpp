#include "GA_PhysicalSkill.h"
#include "AbilitySystem/AbilityData.h"
#include "AbilitySystem/Fragments/GameplayFragments.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/ClanhallMarkTypes.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/Engine.h"

UGA_PhysicalSkill::UGA_PhysicalSkill()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
}

const UAbilityData* UGA_PhysicalSkill::GetAbilityData(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return nullptr;
	}

	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
	return Spec ? Cast<UAbilityData>(Spec->SourceObject.Get()) : nullptr;
}

bool UGA_PhysicalSkill::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UAbilityData* Data = GetAbilityData(Handle, ActorInfo);
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!Data || !ASC)
	{
		return false;
	}

	// Раздел 6: контрнавык успешен → пропускаем проверки КД и Charges.
	// State.CounterActive навешивается Character на 0.1 сек перед TryActivateAbility.
	if (ASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_CounterActive.GetTag()))
	{
		return true;
	}

	// КД проверяется здесь (тег навешивается только при подтверждённом попадании, см. ActivateAbility),
	// а Charges — на активацию (combat_system.md §1: "проверяется количество зарядов на активацию").
	if (Data->CooldownTag.IsValid() && ASC->HasMatchingGameplayTag(Data->CooldownTag))
	{
		return false;
	}

	if (Data->ChargeCost > 0)
	{
		const UClanhallAttributeSet* Attributes = ASC->GetSet<UClanhallAttributeSet>();
		if (!Attributes || Attributes->GetCharges() < static_cast<float>(Data->ChargeCost))
		{
			return false;
		}
	}

	return true;
}

void UGA_PhysicalSkill::ResolveMarkLogic(const UAbilityData* Data, UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, UClanhallMarkComponent* TargetMarkComponent) const
{
	if (!TargetMarkComponent)
	{
		return;
	}

	if (const UMarkTriggerFragment* Trigger = Data->FindFragment<UMarkTriggerFragment>())
	{
		const FGameplayTag CurrentMark = TargetMarkComponent->GetCurrentMark();
		if (CurrentMark.IsValid())
		{
			for (const FMarkSynergy& Synergy : Trigger->Synergies)
			{
				if (Synergy.RequiredMark != CurrentMark)
				{
					continue;
				}

				// mark_system.md §2 Правило 3: метка сгорает -> бафф на себя ИЛИ дебафф на цель, никогда оба.
				TargetMarkComponent->ClearMark();

				if (Synergy.EffectOnTarget)
				{
					ClanhallGameplayEffects::ApplyEffect(SourceASC, TargetASC, Synergy.EffectOnTarget);
				}
				else if (Synergy.EffectOnSelf)
				{
					ClanhallGameplayEffects::ApplyEffect(SourceASC, SourceASC, Synergy.EffectOnSelf);
				}

				ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, SourceASC, UGE_ModifyCharges::StaticClass(), 1.0f);
				break;
			}
		}
	}

	if (const UMarkApplyFragment* MarkApply = Data->FindFragment<UMarkApplyFragment>())
	{
		TargetMarkComponent->ApplyMark(MarkApply->MarkTag);
	}
}

void UGA_PhysicalSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const UAbilityData* Data = GetAbilityData(Handle, ActorInfo);

	if (SourceASC && Avatar && Data)
	{
		// Раздел 6: State.CounterActive → навык активирован через успешный контрнавык
		// → Charges и КД не расходуются (CLAUDE.md "ЗАБЛОКИРОВАННЫЙ КАНОН" §6).
		const bool bIsCounterActivation = SourceASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_CounterActive.GetTag());

		// Charges списываются на активацию, а не на попадание — в отличие от КД (см. CanActivateAbility).
		if (!bIsCounterActivation && Data->ChargeCost > 0)
		{
			ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, SourceASC, UGE_ModifyCharges::StaticClass(), -static_cast<float>(Data->ChargeCost));
		}

		if (AActor* Target = FindMeleeTarget(Avatar))
		{
			IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(Target);
			UAbilitySystemComponent* TargetASC = TargetInterface ? TargetInterface->GetAbilitySystemComponent() : nullptr;

			bool bConfirmedHit = false;
			if (const UDamageFragment* Damage = Data->FindFragment<UDamageFragment>())
			{
				bConfirmedHit = ResolveStandardDamage(SourceASC, TargetASC, Damage->BaseDamage);
			}
			else
			{
				// Утилитарный навык без урона — попадание подтверждено самим фактом найденной цели,
				// иначе КД и метки у таких навыков никогда бы не срабатывали.
				bConfirmedHit = (TargetASC != nullptr);
			}

			if (bConfirmedHit)
			{
				if (UClanhallMarkComponent* TargetMarkComponent = Target->FindComponentByClass<UClanhallMarkComponent>())
				{
					ResolveMarkLogic(Data, SourceASC, TargetASC, TargetMarkComponent);
				}

				if (const UBalanceFragment* Balance = Data->FindFragment<UBalanceFragment>())
				{
					ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, SourceASC, UGE_ModifyBalance::StaticClass(), Balance->Shift);
				}

				// КД только при подтверждённом попадании — CLAUDE.md, "ЗАБЛОКИРОВАННЫЙ КАНОН".
				// При контрнавыке (bIsCounterActivation) КД не ставится.
				if (!bIsCounterActivation && Data->CooldownTag.IsValid())
				{
					ClanhallGameplayEffects::ApplyTimedTag(SourceASC, Data->CooldownTag, Data->Cooldown);
				}

#if !UE_BUILD_SHIPPING
				if (const UClanhallAttributeSet* SelfAttributes = SourceASC->GetSet<UClanhallAttributeSet>())
				{
					const FString Prefix = bIsCounterActivation ? TEXT("[COUNTER] ") : TEXT("");
					GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Orange, FString::Printf(
						TEXT("%s%s hit | AP %.0f/%.0f  Charges %.0f/%.0f  Balance %.1f"),
						*Prefix, *Data->DisplayName.ToString(), SelfAttributes->GetAP(), SelfAttributes->GetMaxAP(),
						SelfAttributes->GetCharges(), SelfAttributes->GetMaxCharges(), SelfAttributes->GetBalance()));
				}
#endif
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
