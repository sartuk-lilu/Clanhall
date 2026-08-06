#include "AbilitySystem/ClanhallParryComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "ClanhallHumanoidCombatant.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

void UClanhallParryComponent::BeginPlay()
{
	Super::BeginPlay();

	// task_stagger_control_code.md §7: гейт подсистемы, посчитан один раз на входе в бой.
	// Прототип 1v1 — "противник" ищет AClanhallHumanoidCombatant::HasOpponentWithMarkSynergy,
	// у которой пока нет полноценного таргетинга (см. её комментарий); пересчёт при смене
	// оружия сознательно не делается.
	if (const AClanhallHumanoidCombatant* Character = Cast<AClanhallHumanoidCombatant>(GetOwner()))
	{
		bStaggerGateOpen = Character->HasOpponentWithMarkSynergy(ClanhallGameplayTags::Mark_Staggered.GetTag());
	}
}

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

	// task_stagger_control_code.md §2: со второго отпарированного шага серии — первый идёт
	// бесплатно (рвёт слив, перезапускает паузу, но шкалу не растит).
	++ParriedStepsThisSeries;
	AddStagger(ParriedStepsThisSeries > 1 ? 1.0f : 0.0f);

	if (UAbilitySystemComponent* OwnASC = GetASC())
	{
		// Парировавшему (цели) — заряд. Заряд платится за само действие, не зависит от AddStagger/гейта.
		ClanhallGameplayEffects::ApplyModifyEffect(OwnASC, TargetASC, UGE_ModifyCharges::StaticClass(), 1.0f);
	}

	return true;
}

void UClanhallParryComponent::AddStagger(float Amount)
{
	// §7: без обналичивающего навыка у противника шкала не существует — no-op целиком,
	// включая перезапуск таймеров распада.
	if (!bStaggerGateOpen)
	{
		return;
	}

	// §3: любой вызов, включая AddStagger(0), прерывает текущий слив и сохраняет остаток —
	// тот же FTimerHandle, что ниже перезапускает ScheduleStaggerDecay().
	GetWorld()->GetTimerManager().ClearTimer(StaggerDecayTimer);

	UAbilitySystemComponent* ASC = GetASC();
	const UClanhallAttributeSet* Attributes = ASC ? ASC->GetSet<UClanhallAttributeSet>() : nullptr;
	if (ASC && Attributes)
	{
		if (Amount > 0.0f)
		{
			ClanhallGameplayEffects::ApplyModifyEffect(ASC, ASC, UGE_ModifyStagger::StaticClass(), Amount);
		}

		if (Attributes->GetStagger() >= Attributes->GetMaxStagger())
		{
			// Потолок (task_stagger_control_code.md §4): сброс в 0 и метка Staggered владельцу —
			// стан отсюда больше не выдаётся, State.Stunned выдаёт только обналичивающая синергия (§6).
			ClanhallGameplayEffects::ApplyModifyEffect(ASC, ASC, UGE_ModifyStagger::StaticClass(), -Attributes->GetStagger());

			if (UClanhallMarkComponent* MarkComp = GetOwner() ? GetOwner()->FindComponentByClass<UClanhallMarkComponent>() : nullptr)
			{
				// Источник неизвестен на этом уровне (AddStagger несёт только Amount) — nullptr
				// легален, IsOwnMark для Mark.Staggered потребителя пока не имеет.
				MarkComp->ApplyMark(ClanhallGameplayTags::Mark_Staggered.GetTag(), nullptr);
			}
		}
	}

	ScheduleStaggerDecay();
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
	// task_stagger_control_code.md §3: скорость слива фиксирована, не длительность — интервал
	// между тиками пересчитывается от ТЕКУЩЕГО потолка владельца (свой у каждого бойца и тира).
	const UAbilitySystemComponent* ASC = GetASC();
	const UClanhallAttributeSet* Attributes = ASC ? ASC->GetSet<UClanhallAttributeSet>() : nullptr;
	const float MaxStagger = Attributes ? Attributes->GetMaxStagger() : 0.0f;
	if (MaxStagger <= 0.0f)
	{
		return;
	}

	const float TickInterval = StaggerDrainDuration / MaxStagger;
	GetWorld()->GetTimerManager().SetTimer(StaggerDecayTimer, this, &UClanhallParryComponent::DecayStaggerStep, TickInterval, true);
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
