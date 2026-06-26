#include "GA_EnemyWASDSeries.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallParryComponent.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

UGA_EnemyWASDSeries::UGA_EnemyWASDSeries()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	// Серия AI — не требует стойки (ActivationRequiredTags наследуется от GA_ClanhallAbilityBase).
	ActivationRequiredTags.RemoveTag(ClanhallGameplayTags::State_InStance.GetTag());

	// Во время оглушения или восстановления после комбо — повторный запуск невозможен.
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_Stunned.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_ComboRecovery.GetTag());
}

void UGA_EnemyWASDSeries::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (AttackDirections.IsEmpty())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Найти игрока (прототип: единственный игрок в мире).
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (IAbilitySystemInterface* PlayerInterface = Cast<IAbilitySystemInterface>(PlayerPawn))
	{
		TargetASC = PlayerInterface->GetAbilitySystemComponent();
	}
	TargetParryComponent = PlayerPawn->FindComponentByClass<UClanhallParryComponent>();

	if (!TargetASC.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	CurrentHitIndex = 0;
	ParriedCount = 0;
	PrepareHit();
}

void UGA_EnemyWASDSeries::PrepareHit()
{
	if (!TargetASC.IsValid() || CurrentHitIndex >= AttackDirections.Num())
	{
		FinalizeSeries();
		return;
	}

	if (TargetParryComponent.IsValid())
	{
		TargetParryComponent->ResetParry();
	}

	UAbilitySystemComponent* SelfASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayTag IncomingTag = AttackDirections[CurrentHitIndex];

	// Визуальный индикатор (прототип: экранное сообщение; Раздел 6.5 заменит на VFX/звук).
	const FString DirectionChar = IncomingTag.GetTagName().ToString().Right(1);
#if !UE_BUILD_SHIPPING
	GEngine->AddOnScreenDebugMessage(-1, WindowDuration + 0.15f, FColor::Yellow,
		FString::Printf(TEXT("↓ INCOMING [%s]  — press opposite!"), *DirectionChar));
#endif

	// Раздел 6.5: State.Parrying вешается на SELF (враг — паррируемый актор).
	// Это interim-замена AnimNotify_ParryWindowStart/End — когда animation setup будет готов
	// в редакторе, этот вызов можно убрать, и тег будет управляться только нотифаями.
	ClanhallGameplayEffects::ApplyTimedTag(SelfASC, ClanhallGameplayTags::State_Parrying.GetTag(), WindowDuration);
	// Parry.Incoming.* по-прежнему вешается на игрока — он должен знать, какое направление контрить.
	ClanhallGameplayEffects::ApplyTimedTagToTarget(SelfASC, TargetASC.Get(), IncomingTag, WindowDuration);

	// Дождаться истечения окна.
	UAbilityTask_WaitDelay* WindowTask = UAbilityTask_WaitDelay::WaitDelay(this, WindowDuration);
	WindowTask->OnFinish.AddDynamic(this, &UGA_EnemyWASDSeries::OnWindowExpired);
	WindowTask->ReadyForActivation();
}

void UGA_EnemyWASDSeries::OnWindowExpired()
{
	const bool bParried = TargetParryComponent.IsValid() && TargetParryComponent->bParrySuccessful;

	if (bParried)
	{
		ParriedCount++;
#if !UE_BUILD_SHIPPING
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, TEXT("✓ Parried!"));
#endif
	}
	else
	{
		// Пропущенный удар — урон по AP игрока (ResolveStandardDamage: AI получает 50% AP обратно).
		UAbilitySystemComponent* SelfASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
		if (SelfASC && TargetASC.IsValid())
		{
			ResolveStandardDamage(SelfASC, TargetASC.Get(), HitDamage);
		}
#if !UE_BUILD_SHIPPING
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, TEXT("✗ Hit taken!"));
#endif
	}

	CurrentHitIndex++;

	// Пауза перед следующим ударом (или финализация).
	UAbilityTask_WaitDelay* PauseTask = UAbilityTask_WaitDelay::WaitDelay(this, DelayBetweenHits);
	PauseTask->OnFinish.AddDynamic(this, &UGA_EnemyWASDSeries::OnInterHitPauseExpired);
	PauseTask->ReadyForActivation();
}

void UGA_EnemyWASDSeries::OnInterHitPauseExpired()
{
	PrepareHit();
}

void UGA_EnemyWASDSeries::FinalizeSeries()
{
	UAbilitySystemComponent* SelfASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;

	const bool bFullParry = (ParriedCount == AttackDirections.Num()) && !AttackDirections.IsEmpty();

	if (bFullParry)
	{
		// Полное парирование: AI оглушён + КД игрока −CDReduction сек (CLAUDE.md канон).
		if (SelfASC)
		{
			ClanhallGameplayEffects::ApplyTimedTag(SelfASC, ClanhallGameplayTags::State_Stunned.GetTag(), StunDuration);
		}
		if (TargetASC.IsValid())
		{
			ReducePlayerCooldowns(TargetASC.Get());
		}
#if !UE_BUILD_SHIPPING
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Green, FString::Printf(
			TEXT("FULL PARRY! Enemy stunned %.0fsec, CDs -%.0fsec"), StunDuration, CDReduction));
#endif
	}

	// State.ComboRecovery предотвращает немедленный перезапуск серии (CLAUDE.md канон).
	if (SelfASC)
	{
		const float RecoveryDuration = bFullParry ? StunDuration + 0.5f : 1.0f;
		ClanhallGameplayEffects::ApplyTimedTag(SelfASC, ClanhallGameplayTags::State_ComboRecovery.GetTag(), RecoveryDuration);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_EnemyWASDSeries::ReducePlayerCooldowns(UAbilitySystemComponent* PlayerASC)
{
	if (!PlayerASC) return;

	// MakeQuery_MatchAnyOwningTags в UE 5.3+ проверяет и asset-теги, и granted-теги (DynamicGrantedTags).
	// Наш GE_ApplyTimedTag хранит тег в DynamicGrantedTags → запрос с родительским тегом Cooldown его найдёт.
	FGameplayTagContainer CooldownRoot;
	CooldownRoot.AddTag(FGameplayTag::RequestGameplayTag("Cooldown"));
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownRoot);
	TArray<FActiveGameplayEffectHandle> Handles = PlayerASC->GetActiveEffects(Query);

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	// Собрать теги и новые длительности до изменения контейнера.
	TArray<FGameplayTag> TagsToReapply;
	TArray<float> NewDurations;

	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		const FActiveGameplayEffect* ActiveEffect = PlayerASC->GetActiveGameplayEffect(Handle);
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
		PlayerASC->GetGameplayEffectStartTimeAndDuration(Handle, StartTime, TotalDuration);
		const float Remaining = (StartTime + TotalDuration) - CurrentTime;
		const float NewRemaining = Remaining - CDReduction;

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
		PlayerASC->RemoveActiveGameplayEffect(Handle);
	}
	for (int32 i = 0; i < TagsToReapply.Num(); ++i)
	{
		ClanhallGameplayEffects::ApplyTimedTag(PlayerASC, TagsToReapply[i], NewDurations[i]);
	}
}

// --- UGA_Series_Crosscut ---

UGA_Series_Crosscut::UGA_Series_Crosscut()
{
	// Серия «Перекрёстный» из enemy_prototypes.md: A → D.
	// Игрок должен ответить: D → A.
	AttackDirections = {
		ClanhallGameplayTags::Parry_Incoming_A.GetTag(), // первый удар: AI бьёт A → игрок жмёт D
		ClanhallGameplayTags::Parry_Incoming_D.GetTag(), // второй удар: AI бьёт D → игрок жмёт A
	};
}
