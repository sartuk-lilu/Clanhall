#include "AbilitySystem/ClanhallCombatStateComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"

UClanhallCombatStateComponent::UClanhallCombatStateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Не HUD-драйвер, но та же частота опроса радиуса, что у сенсора рамки — 10 Гц достаточно.
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UClanhallCombatStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateTrackedEnemies();

	if (TrackedEnemies.Num() > 0)
	{
		TimeSinceEmpty = 0.0f;
		SetInCombat(true);
	}
	else if (bInCombat)
	{
		TimeSinceEmpty += DeltaTime;
		if (TimeSinceEmpty >= ExitDelay)
		{
			SetInCombat(false);
		}
	}
}

void UClanhallCombatStateComponent::UpdateTrackedEnemies()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();

	// 1. Убрать всё, что вышло за ExitRadius или стало невалидным.
	for (auto It = TrackedEnemies.CreateIterator(); It; ++It)
	{
		AActor* TrackedActor = It->Get();
		const bool bStillValid = IsValid(TrackedActor) && TrackedActor->Implements<UAbilitySystemInterface>();
		const bool bStillInRange = bStillValid && FVector::Dist(OwnerLocation, TrackedActor->GetActorLocation()) <= ExitRadius;

		if (!bStillInRange)
		{
			It.RemoveCurrent();
		}
	}

	// 2. Найти кандидатов в EnterRadius и добавить новых.
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerActor);

	TArray<AActor*> Overlapping;
	UKismetSystemLibrary::SphereOverlapActors(this, OwnerLocation, EnterRadius, ObjectTypes, nullptr, ActorsToIgnore, Overlapping);

	// Фракций в проекте нет — «противник» = любой другой боец, размеченный Unit.Role.*
	// (`combat_system.md`, «Боевое состояние»). То же допущение, на котором стоят
	// FindPrototypeOpponent и HasOpponentWithMarkSynergy (`Character Hierarchy.md`);
	// заменяется вместе с ними, когда в сцене окажется больше двух бойцов.
	const FGameplayTag UnitRoleTag = ClanhallGameplayTags::Unit_Role.GetTag();

	for (AActor* Candidate : Overlapping)
	{
		if (TrackedEnemies.Contains(TWeakObjectPtr<AActor>(Candidate)))
		{
			continue;
		}

		IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(Candidate);
		UAbilitySystemComponent* CandidateASC = Interface ? Interface->GetAbilitySystemComponent() : nullptr;
		if (!CandidateASC || !CandidateASC->HasMatchingGameplayTag(UnitRoleTag))
		{
			continue;
		}

		TrackedEnemies.Add(Candidate);
	}
}

void UClanhallCombatStateComponent::SetInCombat(bool bNewInCombat)
{
	if (bInCombat == bNewInCombat)
	{
		return;
	}
	bInCombat = bNewInCombat;

	AActor* OwnerActor = GetOwner();
	IAbilitySystemInterface* Interface = OwnerActor ? Cast<IAbilitySystemInterface>(OwnerActor) : nullptr;
	UAbilitySystemComponent* ASC = Interface ? Interface->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	const FGameplayTag InCombatTag = ClanhallGameplayTags::State_InCombat.GetTag();
	if (bInCombat)
	{
		ASC->AddLooseGameplayTag(InCombatTag);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(InCombatTag);
	}
}

float UClanhallCombatStateComponent::GetExitTimeRemaining() const
{
	if (!bInCombat || TrackedEnemies.Num() > 0)
	{
		return 0.0f;
	}
	return FMath::Max(0.0f, ExitDelay - TimeSinceEmpty);
}
