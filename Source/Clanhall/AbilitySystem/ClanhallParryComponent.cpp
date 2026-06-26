#include "AbilitySystem/ClanhallParryComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

void UClanhallParryComponent::ResetParry()
{
	bParrySuccessful = false;
}

bool UClanhallParryComponent::TryParry(AActor* HitEnemy, EClanhallAttackDirection PlayerDirection, FVector HitLocation)
{
	// Предотвращаем двойное срабатывание в одном окне
	if (bParrySuccessful) return false;

	UAbilitySystemComponent* PlayerASC = GetASC();
	if (!PlayerASC) return false;

	// Маппинг: направление удара игрока → тег входящей атаки врага, который он парирует
	//   Overhead  (W) парирует Parry.Incoming.S (враг бил снизу S)
	//   LowSweep  (S) парирует Parry.Incoming.W (враг бил сверху W)
	//   RightSlash(D) парирует Parry.Incoming.A (враг бил влево A)
	//   LeftSlash (A) парирует Parry.Incoming.D (враг бил вправо D)
	FGameplayTag ParriableTag;
	switch (PlayerDirection)
	{
	case EClanhallAttackDirection::Overhead:    ParriableTag = ClanhallGameplayTags::Parry_Incoming_S.GetTag(); break;
	case EClanhallAttackDirection::LowSweep:    ParriableTag = ClanhallGameplayTags::Parry_Incoming_W.GetTag(); break;
	case EClanhallAttackDirection::RightSlash:  ParriableTag = ClanhallGameplayTags::Parry_Incoming_A.GetTag(); break;
	case EClanhallAttackDirection::LeftSlash:   ParriableTag = ClanhallGameplayTags::Parry_Incoming_D.GetTag(); break;
	default: return false;
	}

	// GA_EnemyWASDSeries навесил Parry.Incoming.* на игрока — проверяем совпадение направления
	if (!PlayerASC->HasMatchingGameplayTag(ParriableTag)) return false;

	bParrySuccessful = true;

	if (ClashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ClashSound, HitLocation);
	}

#if !UE_BUILD_SHIPPING
	GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, TEXT("✓ ПАРИРОВАНИЕ (weapon trace)!"));
#endif

	return true;
}

UAbilitySystemComponent* UClanhallParryComponent::GetASC()
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
