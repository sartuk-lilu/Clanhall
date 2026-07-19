#include "AnimNotifyState_WeaponTrace.h"
#include "AbilitySystem/ClanhallWeaponTraceComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_WeaponTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner) return;

	if (UClanhallWeaponTraceComponent* TraceComp = Owner->FindComponentByClass<UClanhallWeaponTraceComponent>())
	{
		TraceComp->BeginTrace();
	}
}

void UAnimNotifyState_WeaponTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner) return;

	if (UClanhallWeaponTraceComponent* TraceComp = Owner->FindComponentByClass<UClanhallWeaponTraceComponent>())
	{
		TraceComp->EndTrace();
	}
}

FString UAnimNotifyState_WeaponTrace::GetNotifyName_Implementation() const
{
	return TEXT("WeaponTrace");
}
