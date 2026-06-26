#include "AnimNotify_WeaponTraceEnd.h"
#include "AbilitySystem/ClanhallWeaponTraceComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_WeaponTraceEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner) return;

	if (UClanhallWeaponTraceComponent* TraceComp = Owner->FindComponentByClass<UClanhallWeaponTraceComponent>())
	{
		TraceComp->EndTrace();
	}
}

FString UAnimNotify_WeaponTraceEnd::GetNotifyName_Implementation() const
{
	return TEXT("WeaponTraceEnd");
}
