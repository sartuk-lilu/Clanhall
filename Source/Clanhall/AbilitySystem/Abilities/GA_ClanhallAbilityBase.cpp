#include "GA_ClanhallAbilityBase.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"

UGA_ClanhallAbilityBase::UGA_ClanhallAbilityBase()
{
	// Активка в фазе коммита (State.SkillCommitted, висит на UGA_PhysicalSkill от активации
	// до EndAbility) блокирует вторую активку — «начатую активку нельзя оборвать» (`CombatSystem.md`).
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_SkillCommitted.GetTag());

	// Смена оружия с оружием в руках стоит долю на уборку (`WeaponSystem.md`, «Свап оружия»).
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_WeaponSwap.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_Dead.GetTag());

	// Общие действия занимают тело: пока идёт уклонение или его хвост, насмешка или её
	// парирование - приём не начинается (`CombatSystem.md`, «Общие действия», «Насмешка»).
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_EvadeRecovery.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_Taunting.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_TauntParried.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_TauntParrying.GetTag());

	// Оглушение (State.Stunned — эффект синергии меток, FMarkSynergy::EffectOnTarget) блокирует
	// новые физдействия. Контрнавык стана не выдаёт (`CounterAbility.md`, «Наказание сбитого»).
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_Stunned.GetTag());
}

AActor* UGA_ClanhallAbilityBase::FindMeleeTarget(AActor* Avatar) const
{
	if (!Avatar)
	{
		return nullptr;
	}

	const FVector Start = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * TraceRange;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Avatar);

	TArray<AActor*> OverlappedActors;
	UKismetSystemLibrary::SphereOverlapActors(Avatar, Start, TraceRadius, ObjectTypes, nullptr, ActorsToIgnore, OverlappedActors);

	for (AActor* Candidate : OverlappedActors)
	{
		if (Candidate && Candidate->Implements<UAbilitySystemInterface>())
		{
			return Candidate;
		}
	}

	return nullptr;
}
