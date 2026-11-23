#include "UI/ClanhallUILibrary.h"
#include "UI/ClanhallHUDSaveGame.h"
#include "ClanhallHumanoidBase.h"
#include "ClanhallCharacter.h"
#include "AbilitySystem/AbilityData.h"
#include "AbilitySystem/WeaponData.h"
#include "AbilitySystem/Fragments/GameplayFragments.h"
#include "AbilitySystem/ClanhallTargetingComponent.h"
#include "AbilitySystem/ClanhallSkillCooldownComponent.h"
#include "AbilitySystem/ClanhallCrystalComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FString HUDSaveSlot = TEXT("HUDLayout");

	// Слот один на раскладку рамок и клавиши приёмов: запись одного не должна стирать другое.
	UClanhallHUDSaveGame* LoadOrCreateHUDSave()
	{
		if (UClanhallHUDSaveGame* Existing = Cast<UClanhallHUDSaveGame>(UGameplayStatics::LoadGameFromSlot(HUDSaveSlot, 0)))
		{
			return Existing;
		}
		return Cast<UClanhallHUDSaveGame>(UGameplayStatics::CreateSaveGameObject(UClanhallHUDSaveGame::StaticClass()));
	}
}

void UClanhallUILibrary::SaveHUDLayout(FVector2D PlayerFramePosition, FVector2D EnemyFramePosition)
{
	UClanhallHUDSaveGame* Save = LoadOrCreateHUDSave();
	if (!Save) return;

	Save->PlayerFramePosition = PlayerFramePosition;
	Save->EnemyFramePosition  = EnemyFramePosition;
	UGameplayStatics::SaveGameToSlot(Save, HUDSaveSlot, 0);
}

bool UClanhallUILibrary::LoadHUDLayout(FVector2D& PlayerFramePosition, FVector2D& EnemyFramePosition)
{
	UClanhallHUDSaveGame* Save = Cast<UClanhallHUDSaveGame>(
		UGameplayStatics::LoadGameFromSlot(HUDSaveSlot, 0));
	if (!Save) return false;

	PlayerFramePosition = Save->PlayerFramePosition;
	EnemyFramePosition  = Save->EnemyFramePosition;
	return true;
}

void UClanhallUILibrary::SaveSkillSlots(const TArray<FClanhallSlotAssignment>& Assignments)
{
	UClanhallHUDSaveGame* Save = LoadOrCreateHUDSave();
	if (!Save) return;

	Save->SkillSlots = Assignments;
	UGameplayStatics::SaveGameToSlot(Save, HUDSaveSlot, 0);
}

bool UClanhallUILibrary::LoadSkillSlots(TArray<FClanhallSlotAssignment>& OutAssignments)
{
	const UClanhallHUDSaveGame* Save = Cast<UClanhallHUDSaveGame>(UGameplayStatics::LoadGameFromSlot(HUDSaveSlot, 0));
	if (!Save || Save->SkillSlots.Num() == 0) return false;

	OutAssignments = Save->SkillSlots;
	return true;
}

int32 UClanhallUILibrary::PreviewSkillLevel(const AClanhallHumanoidBase* Fighter, const UAbilityData* Skill)
{
	const USkillLevelsFragment* Levels = Skill ? Skill->FindFragment<USkillLevelsFragment>() : nullptr;
	if (!Fighter || !Levels)
	{
		return 1;
	}

	const UClanhallMarkComponent* Holder = nullptr;
	if (Levels->Target == EClanhallMarkTarget::Self)
	{
		Holder = Fighter->FindComponentByClass<UClanhallMarkComponent>();
	}
	else if (const UClanhallTargetingComponent* Targeting = Fighter->FindComponentByClass<UClanhallTargetingComponent>())
	{
		Holder = Targeting->CurrentTarget ? Targeting->CurrentTarget->FindComponentByClass<UClanhallMarkComponent>() : nullptr;
	}

	const int32 Index = Levels->ChooseLevelIndex(Holder);
	return Levels->Levels.IsValidIndex(Index) ? Levels->Levels[Index].Level : 1;
}

TArray<FClanhallPanelRow> UClanhallUILibrary::GetSkillPanel(AClanhallHumanoidBase* Fighter)
{
	TArray<FClanhallPanelRow> Rows;
	if (!Fighter)
	{
		return Rows;
	}

	const UClanhallSkillCooldownComponent* Cooldowns = Fighter->FindComponentByClass<UClanhallSkillCooldownComponent>();
	const AClanhallCharacter* Player = Cast<AClanhallCharacter>(Fighter);
	const FGameplayTag SelectedSlot = Player ? Player->GetSelectedSlot() : FGameplayTag();
	const TArray<UWeaponData*> Loadout = Fighter->GetLoadout();

	for (int32 Index = 0; Index < Loadout.Num(); ++Index)
	{
		FClanhallPanelRow& Row = Rows.AddDefaulted_GetRef();
		Row.Weapon = Loadout[Index];
		Row.Icon = Loadout[Index] ? Loadout[Index]->Icon : nullptr;
		Row.LoadoutIndex = Index;
		Row.bSelected = Index == Fighter->GetSelectedWeaponIndex();
		Row.bInHands = Row.bSelected && Fighter->IsWeaponInHands();

		for (UAbilityData* Skill : Fighter->GetLearnedSkillsOfWeapon(Index))
		{
			FClanhallPanelSkill& Entry = Row.Skills.AddDefaulted_GetRef();
			Entry.Skill = Skill;
			Entry.SkillIdentity = Skill->CounterTag;
			Entry.Slot = Fighter->GetSkillSlot(Index, Skill->CounterTag);
			Entry.TotalCooldownBeats = Skill->CooldownBeats;
			Entry.RemainingCooldownBeats = Cooldowns ? Cooldowns->GetRemainingBeats(Skill->CounterTag) : 0.0f;
			Entry.bSelected = Row.bSelected && Entry.Slot.IsValid() && Entry.Slot == SelectedSlot;
			Entry.Level = PreviewSkillLevel(Fighter, Skill);
		}
	}
	return Rows;
}

void UClanhallUILibrary::GetFighterMarks(AActor* Fighter, TArray<FClanhallMarkView>& Marks, TArray<FClanhallMarkView>& TraumasAndAdvantages)
{
	Marks.Reset();
	TraumasAndAdvantages.Reset();
	const UClanhallMarkComponent* MarkComponent = Fighter ? Fighter->FindComponentByClass<UClanhallMarkComponent>() : nullptr;
	if (!MarkComponent)
	{
		return;
	}
	for (const FClanhallMarkView& View : MarkComponent->GetMarks())
	{
		(View.Level >= 3 ? TraumasAndAdvantages : Marks).Add(View);
	}
}

float UClanhallUILibrary::GetFighterCrystal(const AActor* Fighter)
{
	const UClanhallCrystalComponent* Crystal = Fighter ? Fighter->FindComponentByClass<UClanhallCrystalComponent>() : nullptr;
	return Crystal ? Crystal->GetCrystal() : 0.0f;
}
