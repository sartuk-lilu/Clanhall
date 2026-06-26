#include "GA_DirectionalAttackBase.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallWeaponTraceComponent.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
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
		// Раздел 6.5: сообщаем WeaponTraceComponent направление до BeginTrace (который придёт
		// из AnimNotify_WeaponTraceStart во время монтажа).
		if (ACharacter* Char = Cast<ACharacter>(Avatar))
		{
			if (UClanhallWeaponTraceComponent* TraceComp = Char->FindComponentByClass<UClanhallWeaponTraceComponent>())
			{
				TraceComp->SetCurrentDirection(GetDirection());
			}
		}

		// Мгновенный урон (sphere hit) — placeholder до полной animation-driven системы.
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

		// Раздел 6.5: монтаж воспроизводится как косметика. AnimNotify_WeaponTraceStart/End
		// в нём открывают/закрывают weapon trace независимо от жизненного цикла этой абилки.
		if (AttackMontage)
		{
			ACharacter* Char = Cast<ACharacter>(Avatar);
			if (Char && Char->GetMesh())
			{
				if (UAnimInstance* AnimInst = Char->GetMesh()->GetAnimInstance())
				{
					AnimInst->Montage_Play(AttackMontage);
				}
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
