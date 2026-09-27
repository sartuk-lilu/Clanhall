#include "AbilitySystem/ClanhallParryComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

void UClanhallParryComponent::ResetParry()
{
	bStepParried = false;
}

bool UClanhallParryComponent::TryParry(AActor* HitTarget, EClanhallAttackDirection MyDirection, FVector HitLocation)
{
	// Предотвращаем двойной засчёт одного шага (мультифазная/мультицелевая зона).
	if (bStepParried || !HitTarget) return false;

	IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(HitTarget);
	UAbilitySystemComponent* TargetASC = TargetInterface ? TargetInterface->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC) return false;

	// Маппинг: своё направление удара → тег направления цели, который его парирует. Свитч по
	// содержанию не меняется относительно старого Parry.Incoming.* (только имя тегов) —
	//   Overhead  (W) парируется целью, бьющей Attack.Direction.S
	//   LowSweep  (S) парируется целью, бьющей Attack.Direction.W
	//   RightSlash(D) парируется целью, бьющей Attack.Direction.A
	//   LeftSlash (A) парируется целью, бьющей Attack.Direction.D
	FGameplayTag ParriableTag;
	switch (MyDirection)
	{
	case EClanhallAttackDirection::Overhead:    ParriableTag = ClanhallGameplayTags::Attack_Direction_S.GetTag(); break;
	case EClanhallAttackDirection::LowSweep:    ParriableTag = ClanhallGameplayTags::Attack_Direction_W.GetTag(); break;
	case EClanhallAttackDirection::RightSlash:  ParriableTag = ClanhallGameplayTags::Attack_Direction_A.GetTag(); break;
	case EClanhallAttackDirection::LeftSlash:   ParriableTag = ClanhallGameplayTags::Attack_Direction_D.GetTag(); break;
	default: return false;
	}

	if (!TargetASC->HasMatchingGameplayTag(ParriableTag)) return false;

	bStepParried = true;

	if (ClashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ClashSound, HitLocation);
	}

#if !UE_BUILD_SHIPPING
	GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, TEXT("✓ КЛЭШ (парирование)!"));
#endif

	// Хитстоп — на владельце зоны (атакующем, `Parrying.md`): его клинок и есть
	// та зона, что нанесла контакт.
	if (UClanhallHitboxComponent* OwnHitbox = GetOwner() ? GetOwner()->FindComponentByClass<UClanhallHitboxComponent>() : nullptr)
	{
		OwnHitbox->ApplyHitstop(OwnHitbox->HitstopDurationOnClash);
	}

	// Подавление — на цели (парировавшем): контакт пришёлся по ней, её собственная зона (если
	// вот-вот откроется — окно парирования всегда закрывается раньше её Hitbox-нотифая) гасится
	// так же, как при обычном пропущенном ударе (`combat_system.md`, «Сквозной принцип: контакт сбивает зону получателя»).
	if (UClanhallHitboxComponent* TargetHitbox = HitTarget->FindComponentByClass<UClanhallHitboxComponent>())
	{
		TargetHitbox->SuppressHitboxes();
	}

	// Счётчик отражённых шагов текущей серии атакующего (нужен P2: раскрытие после чистого
	// отражения всей серии).
	++ParriedStepsThisSeries;

	if (UAbilitySystemComponent* OwnASC = GetASC())
	{
		// Парировавшему (цели) — заряд. Парирование даёт плоский +1 и оружием НЕ масштабируется (`economy_system.md`,
		// «Заряды: доход»). Масштабировать доход защищающегося его оружием значит «кинжалом
		// парировать невыгодно» — то есть штраф на защиту, а защита это пол дохода. Нормируется
		// только атакующий канал. Не заменять на ChargeIncome.
		ClanhallGameplayEffects::ApplyModifyEffect(OwnASC, TargetASC, UGE_ModifyCharges::StaticClass(), 1.0f);
	}

	return true;
}

UAbilitySystemComponent* UClanhallParryComponent::GetASC() const
{
	if (const IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return Interface->GetAbilitySystemComponent();
	}
	return nullptr;
}
