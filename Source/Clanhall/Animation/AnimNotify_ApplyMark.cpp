#include "AnimNotify_ApplyMark.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_ApplyMark::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	IAbilitySystemInterface* Interface = Owner ? Cast<IAbilitySystemInterface>(Owner) : nullptr;
	if (!Interface) return;

	UAbilitySystemComponent* ASC = Interface->GetAbilitySystemComponent();
	if (ASC)
	{
		FGameplayEventData EventData;
		ASC->HandleGameplayEvent(ClanhallGameplayTags::Event_ApplyMark.GetTag(), &EventData);
	}
}

FString UAnimNotify_ApplyMark::GetNotifyName_Implementation() const
{
	return TEXT("ApplyMark");
}
