#include "AnimNotifyState_CounterWindow.h"
#include "Clanhall.h"
#include "AbilitySystem/AbilityData.h"
#include "AbilitySystem/ClanhallCounterComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_CounterWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	IAbilitySystemInterface* Interface = Owner ? Cast<IAbilitySystemInterface>(Owner) : nullptr;
	UAbilitySystemComponent* ASC = Interface ? Interface->GetAbilitySystemComponent() : nullptr;
	UClanhallCounterComponent* CounterComp = Owner ? Owner->FindComponentByClass<UClanhallCounterComponent>() : nullptr;
	if (!ASC || !CounterComp)
	{
		return;
	}

	const UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	if (!Montage)
	{
		UE_LOG(LogClanhall, Warning, TEXT("CounterWindow: notify is baked into a UAnimSequence, not a montage track — unsupported layout on %s"), *GetNameSafe(Owner));
		return;
	}

	// Ключ поиска — сам монтаж: находим активный спек владельца, чей UAbilityData::CastMontage
	// совпадает с этим монтажом. Идентичность навыка (CounteredBy/Cooldown/CounterStunDuration)
	// приходит из ассета — один и тот же ассет размечает окно и для игрока, и для врага, которому
	// этот навык выдан.
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.IsActive())
		{
			continue;
		}

		const UAbilityData* Data = Cast<UAbilityData>(Spec.SourceObject.Get());
		if (!Data || Data->CastMontage != Montage)
		{
			continue;
		}

		if (Data->CounteredBy.IsEmpty())
		{
			// Навык не контрится ничем — вешать State.CounterWindow не за чем.
			break;
		}

		CounterComp->OpenWindow(Data->CounteredBy, Spec.Handle, Data->CooldownTag, Data->Cooldown, Data->CounterStunDuration);
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
