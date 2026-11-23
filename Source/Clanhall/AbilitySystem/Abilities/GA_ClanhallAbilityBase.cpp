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

	// Оглушение (успешный контрнавык — ClanhallCounterComponent) блокирует новые физдействия.
	// Тег симметричен (вешается и на игрока, и на противника), блокировка тоже.
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

bool UGA_ClanhallAbilityBase::ResolveStandardDamage(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float RawDamage) const
{
	if (!SourceASC || !TargetASC || RawDamage <= 0.0f)
	{
		return false;
	}

	const UClanhallAttributeSet* TargetAttributes = TargetASC->GetSet<UClanhallAttributeSet>();
	if (!TargetAttributes)
	{
		return false;
	}

	const float TargetCurrentAP = TargetAttributes->GetAP();
	const float APRemoved = FMath::Min(RawDamage, TargetCurrentAP);
	const float Overflow = RawDamage - APRemoved;

	if (APRemoved > 0.0f)
	{
		ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, TargetASC, UGE_ModifyAP::StaticClass(), -APRemoved);
	}

	if (Overflow > 0.0f)
	{
		ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, TargetASC, UGE_ModifyHP::StaticClass(), -Overflow);
	}

	// Подавление зоны цели (`CombatSystem.md`, «Сквозной принцип: контакт сбивает зону получателя»)
	// и хитстоп бьющему переехали на сам контакт (UClanhallHitboxComponent::TickHitbox, цикл по
	// PendingHits) — эта функция их больше
	// не делает. Здесь урон мог обнулиться (DT поглотил всё), а контакт при этом уже случился;
	// дублировать вызов тут значило бы делать его дважды на обычном хите и ни разу на поглощённом.
	return true;
}
