#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/MarkData.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "ClanhallCombatSettings.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/World.h"

UClanhallMarkComponent::UClanhallMarkComponent()
{
	// Истечение меток - по времени мира; 0.1 с хватает с запасом: доля - 0.5 с.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

UAbilitySystemComponent* UClanhallMarkComponent::GetOwnerASC() const
{
	const IAbilitySystemInterface* Owner = Cast<IAbilitySystemInterface>(GetOwner());
	return Owner ? Owner->GetAbilitySystemComponent() : nullptr;
}

double UClanhallMarkComponent::Now() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

double UClanhallMarkComponent::LifetimeSeconds(int32 Level) const
{
	// Уровень - горизонт времени (`MarksSystem.md`, «Уровни меток»).
	const UClanhallCombatSettings& Settings = UClanhallCombatSettings::Get();
	const float Beats = Level >= 3 ? Settings.MarkL3Beats : (Level == 2 ? Settings.MarkL2Beats : Settings.MarkL1Beats);
	return ClanhallBeats::ToSeconds(Beats);
}

void UClanhallMarkComponent::Apply(FPlacedMark& Placed)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC || !Placed.Mark)
	{
		return;
	}
	if (Placed.Mark->MarkTag.IsValid())
	{
		ASC->AddLooseGameplayTag(Placed.Mark->MarkTag);
	}
	if (Placed.Mark->Effect)
	{
		Placed.EffectHandle = ClanhallGameplayEffects::ApplyEffect(ASC, ASC, Placed.Mark->Effect);
	}
}

void UClanhallMarkComponent::Remove(FPlacedMark& Placed)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC || !Placed.Mark)
	{
		return;
	}
	if (Placed.Mark->MarkTag.IsValid())
	{
		ASC->RemoveLooseGameplayTag(Placed.Mark->MarkTag);
	}
	if (Placed.EffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(Placed.EffectHandle);
		Placed.EffectHandle.Invalidate();
	}
}

bool UClanhallMarkComponent::PlaceMark(UMarkData* Mark, FGameplayTag SourceSchool, AActor* SourceActor)
{
	if (!Mark)
	{
		return false;
	}

	HeldMarkAssets.AddUnique(Mark);

	const double Time = Now();
	FPlacedMark Fresh;
	Fresh.Mark = Mark;
	Fresh.SourceSchool = SourceSchool;
	Fresh.SourceActor = SourceActor;
	Fresh.PlacedAt = Time;
	Fresh.ExpiresAt = Time + LifetimeSeconds(Mark->Level);

	if (Mark->Level <= 1)
	{
		// Один слот на цели и один на себе: новая метка заменяет лежащую, та же - обновляет таймер.
		TOptional<FPlacedMark>& Slot = LevelOneSlots[Mark->Kind == EClanhallMarkKind::Buff ? 1 : 0];
		if (Slot.IsSet() && Slot->Mark == Mark)
		{
			Slot->ExpiresAt = Fresh.ExpiresAt;
			Slot->SourceSchool = SourceSchool;
			Slot->SourceActor = SourceActor;
		}
		else
		{
			if (Slot.IsSet())
			{
				Remove(Slot.GetValue());
			}
			Slot = Fresh;
			Apply(Slot.GetValue());
		}
	}
	else
	{
		// Второй и третий уровни без потолка; копий нет - повтор обновляет таймер.
		TArray<FPlacedMark>& List = Mark->Level == 2 ? LevelTwo : LevelThree;
		FPlacedMark* Same = List.FindByPredicate([Mark](const FPlacedMark& P) { return P.Mark == Mark; });
		if (Same)
		{
			Same->ExpiresAt = Fresh.ExpiresAt;
			Same->SourceSchool = SourceSchool;
			Same->SourceActor = SourceActor;
		}
		else
		{
			FPlacedMark& Added = List.Add_GetRef(Fresh);
			Apply(Added);
		}
	}

	OnMarksChanged.Broadcast();
	return true;
}

bool UClanhallMarkComponent::ConsumeMark(int32 Level, FGameplayTag MarkTag, FGameplayTag& OutSchool)
{
	if (!MarkTag.IsValid())
	{
		return false;
	}

	if (Level <= 1)
	{
		for (TOptional<FPlacedMark>& Slot : LevelOneSlots)
		{
			if (Slot.IsSet() && Slot->Mark && Slot->Mark->MarkTag == MarkTag)
			{
				OutSchool = Slot->SourceSchool;
				Remove(Slot.GetValue());
				Slot.Reset();
				OnMarksChanged.Broadcast();
				return true;
			}
		}
		return false;
	}

	TArray<FPlacedMark>& List = Level == 2 ? LevelTwo : LevelThree;
	const int32 Index = List.IndexOfByPredicate([&MarkTag](const FPlacedMark& P) { return P.Mark && P.Mark->MarkTag == MarkTag; });
	if (Index == INDEX_NONE)
	{
		return false;
	}
	OutSchool = List[Index].SourceSchool;
	Remove(List[Index]);
	List.RemoveAt(Index);
	OnMarksChanged.Broadcast();
	return true;
}

bool UClanhallMarkComponent::HasMark(int32 Level, FGameplayTag MarkTag) const
{
	if (!MarkTag.IsValid())
	{
		return false;
	}
	if (Level <= 1)
	{
		for (const TOptional<FPlacedMark>& Slot : LevelOneSlots)
		{
			if (Slot.IsSet() && Slot->Mark && Slot->Mark->MarkTag == MarkTag)
			{
				return true;
			}
		}
		return false;
	}
	const TArray<FPlacedMark>& List = Level == 2 ? LevelTwo : LevelThree;
	return List.ContainsByPredicate([&MarkTag](const FPlacedMark& P) { return P.Mark && P.Mark->MarkTag == MarkTag; });
}

bool UClanhallMarkComponent::RemoveOldest(EClanhallMarkKind Kind)
{
	int32 Oldest = INDEX_NONE;
	for (int32 Index = 0; Index < LevelThree.Num(); ++Index)
	{
		if (LevelThree[Index].Mark && LevelThree[Index].Mark->Kind == Kind
			&& (Oldest == INDEX_NONE || LevelThree[Index].PlacedAt < LevelThree[Oldest].PlacedAt))
		{
			Oldest = Index;
		}
	}
	if (Oldest == INDEX_NONE)
	{
		return false;
	}
	Remove(LevelThree[Oldest]);
	LevelThree.RemoveAt(Oldest);
	OnMarksChanged.Broadcast();
	return true;
}

void UClanhallMarkComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Истёкшая метка пропадает целиком, на уровень ниже не опускается.
	const double Time = Now();
	bool bChanged = false;
	for (TOptional<FPlacedMark>& Slot : LevelOneSlots)
	{
		if (Slot.IsSet() && Slot->ExpiresAt <= Time)
		{
			Remove(Slot.GetValue());
			Slot.Reset();
			bChanged = true;
		}
	}
	TArray<FPlacedMark>* Lists[] = { &LevelTwo, &LevelThree };
	for (TArray<FPlacedMark>* List : Lists)
	{
		for (int32 Index = List->Num() - 1; Index >= 0; --Index)
		{
			if ((*List)[Index].ExpiresAt <= Time)
			{
				Remove((*List)[Index]);
				List->RemoveAt(Index);
				bChanged = true;
			}
		}
	}
	if (bChanged)
	{
		OnMarksChanged.Broadcast();
	}
}

void UClanhallMarkComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	LevelOneSlots[0].Reset();
	LevelOneSlots[1].Reset();
	LevelTwo.Reset();
	LevelThree.Reset();
	Super::EndPlay(EndPlayReason);
}

FClanhallMarkView UClanhallMarkComponent::MakeView(const FPlacedMark& Placed) const
{
	FClanhallMarkView View;
	View.Mark = Placed.Mark;
	if (Placed.Mark)
	{
		View.Tag = Placed.Mark->MarkTag;
		View.Level = Placed.Mark->Level;
		View.Kind = Placed.Mark->Kind;
	}
	View.SourceSchool = Placed.SourceSchool;
	View.RemainingBeats = FMath::Max(0.0f, ClanhallBeats::FromSeconds(static_cast<float>(Placed.ExpiresAt - Now())));
	return View;
}

TArray<FClanhallMarkView> UClanhallMarkComponent::GetMarks() const
{
	TArray<FClanhallMarkView> Views;
	for (const TOptional<FPlacedMark>& Slot : LevelOneSlots)
	{
		if (Slot.IsSet())
		{
			Views.Add(MakeView(Slot.GetValue()));
		}
	}
	for (const FPlacedMark& Placed : LevelTwo)
	{
		Views.Add(MakeView(Placed));
	}
	for (const FPlacedMark& Placed : LevelThree)
	{
		Views.Add(MakeView(Placed));
	}
	return Views;
}
