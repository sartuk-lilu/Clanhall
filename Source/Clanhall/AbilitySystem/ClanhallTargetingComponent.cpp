#include "AbilitySystem/ClanhallTargetingComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

UClanhallTargetingComponent::UClanhallTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// 20 Гц достаточно для HUD-прицеливания, не нагружает каждый кадр
	PrimaryComponentTick.TickInterval = 0.05f;
}

void UClanhallTargetingComponent::BeginPlay()
{
	Super::BeginPlay();

	// Компонент тикает только для персонажа под управлением игрока
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsPlayerControlled())
	{
		SetComponentTickEnabled(false);
	}
}

void UClanhallTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateTarget();
}

void UClanhallTargetingComponent::UpdateTarget()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PC) return;

	FVector  CameraLoc;
	FRotator CameraRot;
	PC->GetPlayerViewPoint(CameraLoc, CameraRot);

	const FVector End = CameraLoc + CameraRot.Vector() * MaxRange;

	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("ClanhallTargeting"), false, GetOwner());

	AActor* NewTarget = nullptr;
	if (GetWorld()->LineTraceSingleByChannel(Hit, CameraLoc, End, ECC_Visibility, Params))
	{
		// Цель валидна только если у неё есть ASC (игровой персонаж / враг)
		if (Hit.GetActor() && Hit.GetActor()->Implements<UAbilitySystemInterface>())
		{
			NewTarget = Hit.GetActor();
		}
	}

	if (LastTarget.Get() != NewTarget)
	{
		CurrentTarget = NewTarget;
		LastTarget    = NewTarget;
		OnTargetChanged.Broadcast(NewTarget);
	}
}
