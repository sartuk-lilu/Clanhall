#include "AbilitySystem/ClanhallParryComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallComboComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

void UClanhallParryComponent::ResetParry()
{
	bParrySuccessful = false;
}

bool UClanhallParryComponent::TryParry(AActor* HitTarget, EClanhallAttackDirection MyDirection, FVector HitLocation)
{
	// Предотвращаем двойное срабатывание в одном окне.
	if (bParrySuccessful || !HitTarget) return false;

	IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(HitTarget);
	UAbilitySystemComponent* TargetASC = TargetInterface ? TargetInterface->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC) return false;

	// Маппинг: своё направление удара → тег входящей атаки цели, который оно парирует.
	//   Overhead  (W) парирует Parry.Incoming.S (цель бьёт снизу S)
	//   LowSweep  (S) парирует Parry.Incoming.W (цель бьёт сверху W)
	//   RightSlash(D) парирует Parry.Incoming.A (цель бьёт влево A)
	//   LeftSlash (A) парирует Parry.Incoming.D (цель бьёт вправо D)
	FGameplayTag ParriableTag;
	switch (MyDirection)
	{
	case EClanhallAttackDirection::Overhead:    ParriableTag = ClanhallGameplayTags::Parry_Incoming_S.GetTag(); break;
	case EClanhallAttackDirection::LowSweep:    ParriableTag = ClanhallGameplayTags::Parry_Incoming_W.GetTag(); break;
	case EClanhallAttackDirection::RightSlash:  ParriableTag = ClanhallGameplayTags::Parry_Incoming_A.GetTag(); break;
	case EClanhallAttackDirection::LeftSlash:   ParriableTag = ClanhallGameplayTags::Parry_Incoming_D.GetTag(); break;
	default: return false;
	}

	// main_dev_plan.md §8, Блок D: тег больше не приходит от третьей стороны — цель сама
	// повесила его на себя (UClanhallComboComponent::ActivateStep), пока играла свой шаг.
	if (!TargetASC->HasMatchingGameplayTag(ParriableTag)) return false;

	bParrySuccessful = true;

	if (ClashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ClashSound, HitLocation);
	}

#if !UE_BUILD_SHIPPING
	GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, TEXT("✓ ПАРИРОВАНИЕ (hitbox)!"));
#endif

	// main_dev_plan.md §8, Блок D: исход серии живёт на ComboComponent цели — сообщаем ей,
	// что её текущий шаг только что отпарирован, и кем (для снижения КД при полной серии).
	if (UClanhallComboComponent* TargetCombo = HitTarget->FindComponentByClass<UClanhallComboComponent>())
	{
		TargetCombo->NotifyStepParried(GetOwner());
	}

	return true;
}
