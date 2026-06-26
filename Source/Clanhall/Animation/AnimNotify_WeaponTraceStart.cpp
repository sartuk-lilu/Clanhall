#include "AnimNotify_WeaponTraceStart.h"
#include "AbilitySystem/ClanhallWeaponTraceComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_WeaponTraceStart::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner) return;

	if (UClanhallWeaponTraceComponent* TraceComp = Owner->FindComponentByClass<UClanhallWeaponTraceComponent>())
	{
		TraceComp->BeginTrace();
	}
}

FString UAnimNotify_WeaponTraceStart::GetNotifyName_Implementation() const
{
	return TEXT("WeaponTraceStart");
}
