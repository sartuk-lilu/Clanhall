#include "AnimNotifyState_CounterWindow.h"
#include "Clanhall.h"
#include "AbilitySystem/AbilityData.h"
#include "AbilitySystem/ClanhallCounterComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	/** Общий поиск для NotifyBegin/NotifyEnd: активный спек владельца, чей UAbilityData::CastMontage
	 *  совпадает с Montage и у которого непустой CounteredBy — тот же критерий в обоих местах,
	 *  иначе Begin откроет окно одного навыка, а End закроет по критерию, не совпадающему с ним
	 *  (task_counterwindow_symmetry.md, Задача 1). nullptr, если такого спека нет. */
	const UAbilityData* FindOwningAbilityData(UAbilitySystemComponent* ASC, const UAnimMontage* Montage, FGameplayAbilitySpecHandle& OutHandle)
	{
		if (!ASC || !Montage)
		{
			return nullptr;
		}

		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (!Spec.IsActive())
			{
				continue;
			}

			const UAbilityData* Data = Cast<UAbilityData>(Spec.SourceObject.Get());
			if (!Data || Data->CastMontage != Montage || Data->CounteredBy.IsEmpty())
			{
				continue;
			}

			OutHandle = Spec.Handle;
			return Data;
		}

		return nullptr;
	}
}

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

	FGameplayAbilitySpecHandle Handle;
	if (const UAbilityData* Data = FindOwningAbilityData(ASC, Montage, Handle))
	{
		CounterComp->OpenWindow(Data->CounteredBy, Handle, Data->CooldownTag, Data->Cooldown, Data->CounterStunDuration);
	}
}

void UAnimNotifyState_CounterWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	IAbilitySystemInterface* Interface = Owner ? Cast<IAbilitySystemInterface>(Owner) : nullptr;
	UAbilitySystemComponent* ASC = Interface ? Interface->GetAbilitySystemComponent() : nullptr;
	UClanhallCounterComponent* CounterComp = Owner ? Owner->FindComponentByClass<UClanhallCounterComponent>() : nullptr;
	if (!ASC || !CounterComp)
	{
		return;
	}

	// Тот же поиск, что в NotifyBegin: закрываем только окно, которое сами открыли бы —
	// UAnimNotifyState разделяемый const-объект, состояние на нём не удержать, поэтому вместо
	// хранения хендла между Begin/End критерий пересчитывается заново (см. FindOwningAbilityData).
	const UAnimMontage* Montage = Cast<UAnimMontage>(Animation);
	FGameplayAbilitySpecHandle Handle;
	if (FindOwningAbilityData(ASC, Montage, Handle))
	{
		CounterComp->CloseWindow();
	}
}

FString UAnimNotifyState_CounterWindow::GetNotifyName_Implementation() const
{
	return TEXT("CounterWindow");
}
