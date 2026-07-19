#include "AnimNotifyState_CounterWindow.h"
#include "AbilitySystem/ClanhallCounterComponent.h"
#include "AbilitySystem/Abilities/GA_EnemyActiveSkill.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_CounterWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	IAbilitySystemInterface* Interface = Owner ? Cast<IAbilitySystemInterface>(Owner) : nullptr;
	UAbilitySystemComponent* ASC = Interface ? Interface->GetAbilitySystemComponent() : nullptr;
	UClanhallCounterComponent* CounterComp = Owner ? Owner->FindComponentByClass<UClanhallCounterComponent>() : nullptr;
	if (!ASC || !CounterComp || !CounterTag.IsValid())
	{
		return;
	}

	// Находим активную способность владельца с этим CounterTag — это и есть контримая активка,
	// хендл и КД которой запоминает окно.
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.IsActive() || !Spec.Ability || !Spec.Ability->GetAssetTags().HasTagExact(CounterTag))
		{
			continue;
		}

		FGameplayTag CooldownTag;
		float CooldownDuration = 0.0f;
		if (const UGA_EnemyActiveSkill* EnemyAbility = Cast<UGA_EnemyActiveSkill>(Spec.Ability))
		{
			CooldownTag = EnemyAbility->GetCooldownTag();
			CooldownDuration = EnemyAbility->GetCooldownDuration();
		}

		CounterComp->OpenWindow(CounterTag, Spec.Handle, CooldownTag, CooldownDuration);
		break;
	}
}

void UAnimNotifyState_CounterWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (UClanhallCounterComponent* CounterComp = Owner ? Owner->FindComponentByClass<UClanhallCounterComponent>() : nullptr)
	{
		CounterComp->CloseWindow();
	}
}

FString UAnimNotifyState_CounterWindow::GetNotifyName_Implementation() const
{
	return TEXT("CounterWindow");
}
