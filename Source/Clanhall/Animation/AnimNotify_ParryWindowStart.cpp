#include "AnimNotify_ParryWindowStart.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_ParryWindowStart::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	IAbilitySystemInterface* Interface = Owner ? Cast<IAbilitySystemInterface>(Owner) : nullptr;
	if (!Interface) return;

	UAbilitySystemComponent* ASC = Interface->GetAbilitySystemComponent();
	if (ASC)
	{
		ASC->AddLooseGameplayTag(ClanhallGameplayTags::State_Parrying.GetTag());
	}
}

FString UAnimNotify_ParryWindowStart::GetNotifyName_Implementation() const
{
	return TEXT("ParryWindowStart");
}
