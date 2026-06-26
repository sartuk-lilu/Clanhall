#include "AbilitySystem/ClanhallWeaponTraceComponent.h"
#include "AbilitySystem/ClanhallParryComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

UClanhallWeaponTraceComponent::UClanhallWeaponTraceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UClanhallWeaponTraceComponent::SetCurrentDirection(EClanhallAttackDirection Dir)
{
	CurrentDirection = Dir;
}

void UClanhallWeaponTraceComponent::BeginTrace()
{
	bTraceActive = true;
	AlreadyHit.Empty();
	SetComponentTickEnabled(true);

	AActor* Owner = GetOwner();
	if (!Owner) return;

	ACharacter* Char = Cast<ACharacter>(Owner);
	USkeletalMeshComponent* Mesh = Char ? Char->GetMesh() : Owner->FindComponentByClass<USkeletalMeshComponent>();
	if (Mesh)
	{
		LastSocketPosition = Mesh->GetSocketLocation(WeaponSocketName);
	}
}

void UClanhallWeaponTraceComponent::EndTrace()
{
	bTraceActive = false;
	SetComponentTickEnabled(false);
	AlreadyHit.Empty();
}

void UClanhallWeaponTraceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bTraceActive)
	{
		DoTrace();
	}
}

void UClanhallWeaponTraceComponent::DoTrace()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	ACharacter* Char = Cast<ACharacter>(Owner);
	USkeletalMeshComponent* Mesh = Char ? Char->GetMesh() : Owner->FindComponentByClass<USkeletalMeshComponent>();
	if (!Mesh) return;

	const FVector CurrentPos = Mesh->GetSocketLocation(WeaponSocketName);

	// Сокет ещё не переместился — пропускаем (первый тик после BeginTrace)
	if (FVector::DistSquared(LastSocketPosition, CurrentPos) < KINDA_SMALL_NUMBER)
	{
		LastSocketPosition = CurrentPos;
		return;
	}

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ClanhallWeaponTrace), false, Owner);

	GetWorld()->SweepMultiByObjectType(
		Hits,
		LastSocketPosition,
		CurrentPos,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(TraceRadius),
		Params
	);

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == Owner || AlreadyHit.Contains(HitActor)) continue;

		AlreadyHit.Add(HitActor);

		if (!CheckAndHandleParry(HitActor, Hit))
		{
			OnWeaponHit.Broadcast(HitActor, Hit.ImpactPoint);
		}
	}

	LastSocketPosition = CurrentPos;
}

bool UClanhallWeaponTraceComponent::CheckAndHandleParry(AActor* HitActor, const FHitResult& Hit)
{
	// Враг должен быть в паррируемом состоянии
	IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(HitActor);
	if (!Interface) return false;

	UAbilitySystemComponent* HitASC = Interface->GetAbilitySystemComponent();
	if (!HitASC) return false;

	if (!HitASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_Parrying.GetTag()))
	{
		return false;
	}

	// Передаём в ParryComponent: он проверяет совпадение направления и ставит флаг
	UClanhallParryComponent* ParryComp = GetOwner()->FindComponentByClass<UClanhallParryComponent>();
	if (!ParryComp) return false;

	return ParryComp->TryParry(HitActor, CurrentDirection, Hit.ImpactPoint);
}
