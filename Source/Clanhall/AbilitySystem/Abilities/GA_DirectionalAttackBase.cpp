#include "GA_DirectionalAttackBase.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/Engine.h"

UGA_DirectionalAttackBase::UGA_DirectionalAttackBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
}

void UGA_DirectionalAttackBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;

	if (SourceASC && Avatar)
	{
		if (AActor* Target = FindMeleeTarget(Avatar))
		{
			IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(Target);
			UAbilitySystemComponent* TargetASC = TargetInterface ? TargetInterface->GetAbilitySystemComponent() : nullptr;

			if (ResolveStandardDamage(SourceASC, TargetASC, RawDamage))
			{
				// combat_system.md §4: STR-удар → MP +10, Balance +5..+15; DEX-удар → MP +5, Balance -5..-15.
				const bool bIsSTR = SourceASC->HasMatchingGameplayTag(ClanhallGameplayTags::Weapon_Type_STR.GetTag());
				const float MPGain = bIsSTR ? 10.0f : 5.0f;
				const float BalanceShift = bIsSTR ? FMath::FRandRange(5.0f, 15.0f) : FMath::FRandRange(-15.0f, -5.0f);

				ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, SourceASC, UGE_ModifyMP::StaticClass(), MPGain);
				ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, SourceASC, UGE_ModifyBalance::StaticClass(), BalanceShift);

#if !UE_BUILD_SHIPPING
				// Временный debug-вывод на экран — Раздел 2 ещё без HUD (development_plan.md, "минимальный HUD").
				// Убрать, когда атрибуты станут видны на полосках HUD.
				if (const UClanhallAttributeSet* SelfAttributes = SourceASC->GetSet<UClanhallAttributeSet>())
				{
					GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, FString::Printf(
						TEXT("WASD hit | self AP %.0f/%.0f  MP %.0f/%.0f  Balance %.1f"),
						SelfAttributes->GetAP(), SelfAttributes->GetMaxAP(),
						SelfAttributes->GetMP(), SelfAttributes->GetMaxMP(), SelfAttributes->GetBalance()));
				}
#endif
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
