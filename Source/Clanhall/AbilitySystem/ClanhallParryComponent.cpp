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
#include "Engine/World.h"
#include "TimerManager.h"
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

	// Хитстоп — на владельце зоны (атакующем, task_parry_rework.md §1.5): его клинок и есть
	// та зона, что нанесла контакт.
	if (UClanhallHitboxComponent* OwnHitbox = GetOwner() ? GetOwner()->FindComponentByClass<UClanhallHitboxComponent>() : nullptr)
	{
		OwnHitbox->ApplyHitstop(OwnHitbox->HitstopDurationOnClash);
	}

	// Подавление — на цели (парировавшем): контакт пришёлся по ней, её собственная зона (если
	// вот-вот откроется — окно парирования всегда закрывается раньше её Hitbox-нотифая) гасится
	// так же, как при обычном пропущенном ударе (task_parry_rework.md §1.4).
	if (UClanhallHitboxComponent* TargetHitbox = HitTarget->FindComponentByClass<UClanhallHitboxComponent>())
	{
		TargetHitbox->SuppressHitboxes();
	}

	if (UAbilitySystemComponent* OwnASC = GetASC())
	{
		// Мне (владельцу зоны, атакующему) — усталость.
		ClanhallGameplayEffects::ApplyModifyEffect(OwnASC, OwnASC, UGE_ModifyStagger::StaticClass(), 1.0f);
		ScheduleStaggerDecay();

		const UClanhallAttributeSet* OwnAttributes = OwnASC->GetSet<UClanhallAttributeSet>();
		if (OwnAttributes && OwnAttributes->GetStagger() >= OwnAttributes->GetMaxStagger())
		{
			// Потолок — сброс в 0 и стан владельцу, парировавшему — снижение КД
			// (task_parry_rework.md §1.3). Одиночное парирование стан НЕ выдаёт никому.
			ClanhallGameplayEffects::ApplyModifyEffect(OwnASC, OwnASC, UGE_ModifyStagger::StaticClass(), -OwnAttributes->GetStagger());
			ClanhallGameplayEffects::ApplyTimedTag(OwnASC, ClanhallGameplayTags::State_Stunned.GetTag(), FullParryStunDuration);
			ReduceCooldowns(TargetASC, FullParryCooldownReduction);
		}

		// Парировавшему (цели) — заряд.
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

void UClanhallParryComponent::ScheduleStaggerDecay()
{
	GetWorld()->GetTimerManager().SetTimer(StaggerDecayTimer, this, &UClanhallParryComponent::OnStaggerDecayDelayElapsed, StaggerDecayDelay, false);
}

void UClanhallParryComponent::OnStaggerDecayDelayElapsed()
{
	GetWorld()->GetTimerManager().SetTimer(StaggerDecayTimer, this, &UClanhallParryComponent::DecayStaggerStep, StaggerDecayInterval, true);
}

void UClanhallParryComponent::DecayStaggerStep()
{
	UAbilitySystemComponent* ASC = GetASC();
	const UClanhallAttributeSet* Attributes = ASC ? ASC->GetSet<UClanhallAttributeSet>() : nullptr;
	if (!ASC || !Attributes || Attributes->GetStagger() <= 0.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(StaggerDecayTimer);
		return;
	}

	ClanhallGameplayEffects::ApplyModifyEffect(ASC, ASC, UGE_ModifyStagger::StaticClass(), -1.0f);
}

void UClanhallParryComponent::ReduceCooldowns(UAbilitySystemComponent* TargetASC, float ReductionSeconds) const
{
	if (!TargetASC) return;

	// MakeQuery_MatchAnyOwningTags в UE 5.3+ проверяет и asset-теги, и granted-теги (DynamicGrantedTags).
	// Наш GE_ApplyTimedTag хранит тег в DynamicGrantedTags → запрос с родительским тегом Cooldown его найдёт.
	FGameplayTagContainer CooldownRoot;
	CooldownRoot.AddTag(FGameplayTag::RequestGameplayTag("Cooldown"));
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownRoot);
	TArray<FActiveGameplayEffectHandle> Handles = TargetASC->GetActiveEffects(Query);

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	// Собрать теги и новые длительности до изменения контейнера.
	TArray<FGameplayTag> TagsToReapply;
	TArray<float> NewDurations;

	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		const FActiveGameplayEffect* ActiveEffect = TargetASC->GetActiveGameplayEffect(Handle);
		if (!ActiveEffect) continue;

		FGameplayTag CooldownTag;
		for (const FGameplayTag& Tag : ActiveEffect->Spec.DynamicGrantedTags)
		{
			if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag("Cooldown")))
			{
				CooldownTag = Tag;
				break;
			}
		}
		if (!CooldownTag.IsValid()) continue;

		float StartTime, TotalDuration;
		TargetASC->GetGameplayEffectStartTimeAndDuration(Handle, StartTime, TotalDuration);
		const float Remaining = (StartTime + TotalDuration) - CurrentTime;
		const float NewRemaining = Remaining - ReductionSeconds;

		if (NewRemaining > 0.0f)
		{
			TagsToReapply.Add(CooldownTag);
			NewDurations.Add(NewRemaining);
		}
		// NewRemaining <= 0 → КД истёк сразу, не перевешиваем
	}

	// Снять все найденные КД-эффекты, затем перевесить те, что ещё не истекли.
	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		TargetASC->RemoveActiveGameplayEffect(Handle);
	}
	for (int32 i = 0; i < TagsToReapply.Num(); ++i)
	{
		ClanhallGameplayEffects::ApplyTimedTag(TargetASC, TagsToReapply[i], NewDurations[i]);
	}
}
