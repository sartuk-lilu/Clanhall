#include "AnimNotify_ParryWindowEnd.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_ParryWindowEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	IAbilitySystemInterface* Interface = Owner ? Cast<IAbilitySystemInterface>(Owner) : nullptr;
	if (!Interface) return;

	UAbilitySystemComponent* ASC = Interface->GetAbilitySystemComponent();
	if (ASC)
	{
		ASC->RemoveLooseGameplayTag(ClanhallGameplayTags::State_Parrying.GetTag());
	}
}

FString UAnimNotify_ParryWindowEnd::GetNotifyName_Implementation() const
{
	return TEXT("ParryWindowEnd");
}
