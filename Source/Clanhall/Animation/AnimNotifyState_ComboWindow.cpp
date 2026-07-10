#include "AnimNotifyState_ComboWindow.h"
#include "AbilitySystem/ClanhallComboComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (UClanhallComboComponent* ComboComp = Owner ? Owner->FindComponentByClass<UClanhallComboComponent>() : nullptr)
	{
		ComboComp->OnComboWindowOpen();
	}
}

void UAnimNotifyState_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (UClanhallComboComponent* ComboComp = Owner ? Owner->FindComponentByClass<UClanhallComboComponent>() : nullptr)
	{
		ComboComp->OnComboWindowClose();
	}
}

FString UAnimNotifyState_ComboWindow::GetNotifyName_Implementation() const
{
	return TEXT("ComboWindow");
}
