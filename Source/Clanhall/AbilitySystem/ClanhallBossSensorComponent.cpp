#include "AbilitySystem/ClanhallBossSensorComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Pawn.h"

UClanhallBossSensorComponent::UClanhallBossSensorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Рамка — не прицел, 10 Гц достаточно (changelog_enemyframe_unitroles.md §3).
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UClanhallBossSensorComponent::BeginPlay()
{
	Super::BeginPlay();

	// Как и UClanhallTargetingComponent: сенсор нужен только тому, кто рисует свой HUD.
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsPlayerControlled())
	{
		SetComponentTickEnabled(false);
	}
}

void UClanhallBossSensorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateTrackedUnits();
}

void UClanhallBossSensorComponent::UpdateTrackedUnits()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();

	// 1. Убрать всё, что вышло за ExitRadius или стало невалидным.
	for (auto It = TrackedUnits.CreateIterator(); It; ++It)
	{
		AActor* TrackedActor = It->Get();
		const bool bStillValid = IsValid(TrackedActor) && TrackedActor->Implements<UAbilitySystemInterface>();
		const bool bStillInRange = bStillValid && FVector::Dist(OwnerLocation, TrackedActor->GetActorLocation()) <= ExitRadius;

		if (!bStillInRange)
		{
			It.RemoveCurrent();
			OnFrameUnitExited.Broadcast(TrackedActor);
		}
	}

	// 2. Найти кандидатов в EnterRadius и добавить новых.
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerActor);

	TArray<AActor*> Overlapping;
	UKismetSystemLibrary::SphereOverlapActors(this, OwnerLocation, EnterRadius, ObjectTypes, nullptr, ActorsToIgnore, Overlapping);

	const FGameplayTag BossRoleTag = ClanhallGameplayTags::Unit_Role_Boss.GetTag();

	for (AActor* Candidate : Overlapping)
	{
		if (TrackedUnits.Contains(TWeakObjectPtr<AActor>(Candidate)))
		{
			continue;
		}

		IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(Candidate);
		UAbilitySystemComponent* CandidateASC = Interface ? Interface->GetAbilitySystemComponent() : nullptr;
		if (!CandidateASC || !CandidateASC->HasMatchingGameplayTag(BossRoleTag))
		{
			continue;
		}

		TrackedUnits.Add(Candidate);
		OnFrameUnitEntered.Broadcast(Candidate);
	}
}
