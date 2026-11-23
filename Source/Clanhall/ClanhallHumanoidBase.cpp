#include "ClanhallHumanoidBase.h"
#include "Clanhall.h"
#include "ClanhallCombatSettings.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CharacterSheetData.h"
#include "AbilitySystem/WeaponData.h"
#include "AbilitySystem/WeaponTypeData.h"
#include "AbilitySystem/Fragments/WeaponFragments.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/AbilityData.h"
#include "AbilitySystem/Abilities/GA_PhysicalSkill.h"
#include "AbilitySystem/Abilities/GA_Taunt.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "ClanhallWeaponActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

AClanhallHumanoidBase::AClanhallHumanoidBase()
{
	// Разворот противника в приёме считается в тике (TickSkillTurn).
	PrimaryActorTick.bCanEverTick = true;
	TauntAbilityClass = UGA_Taunt::StaticClass();
}

void AClanhallHumanoidBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// PostInitializeComponents, а не BeginPlay — см. комментарий у объявления. Пустой Loadout —
	// законное состояние: CurrentWeapon остаётся null, фолбэки работают.
	const int32 LoadoutNum = CharacterSheet ? CharacterSheet->Loadout.Num() : 0;
	LastSkillTimeByWeapon.Init(-1.0, LoadoutNum);
	SetSelectedWeaponIndex(LoadoutNum > 0 ? 0 : INDEX_NONE);
	bWeaponInHands = bKeepsWeaponInHands && CurrentWeapon != nullptr;
}

void AClanhallHumanoidBase::SetSelectedWeaponIndex(int32 LoadoutIndex)
{
	SelectedWeaponIndex = LoadoutIndex;
	CurrentWeapon = (CharacterSheet && CharacterSheet->Loadout.IsValidIndex(LoadoutIndex))
		? CharacterSheet->Loadout[LoadoutIndex].Get() : nullptr;

	// Множитель скорости оружия применяется при выборе ко всем базовым скоростям разом
	// (`WeaponSystem.md`) - тяжёлое оружие медленное всегда, а не в момент удара.
	RefreshMovementSpeed();
}

void AClanhallHumanoidBase::RefreshMovementSpeed()
{
	// От базового JogSpeed бойца, не от текущего MaxWalkSpeed: повторный вызов при смене оружия
	// иначе умножил бы второй раз.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = GetJogSpeed() * GetWeaponSpeedMultiplier() * GetMoveSpeedMultiplier();
	}
}

void AClanhallHumanoidBase::BeginPlay()
{
	Super::BeginPlay();

	if (!AbilitySystemComponent)
	{
		return;
	}

	// Насмешка - общее действие любого гуманоида (`CombatSystem.md`, «Насмешка»).
	if (TauntAbilityClass)
	{
		TauntAbilityHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(TauntAbilityClass, 1, INDEX_NONE, this));
	}

	// Отсутствие данных не должно ломать бой (`WeaponSystem.md`, «Шаблон и живой лист») — боец
	// без листа живёт на фолбэках. Один варнинг на бойца, здесь и только здесь.
	if (!CurrentWeapon || !CurrentWeapon->Type)
	{
		UE_LOG(LogClanhall, Warning, TEXT("%s: цепочка CharacterSheet -> Loadout -> Type неполна — "
			"приёмов оружия нет, скорость на фолбэке."), *GetName());
	}

	// Весь лоадаут носится на теле (`WeaponSystem.md`, «Свап оружия»): акторы спавнятся все
	// сразу, выбранное у противника - сразу в руки.
	const int32 LoadoutNum = CharacterSheet ? CharacterSheet->Loadout.Num() : 0;
	SpawnedWeapons.SetNum(LoadoutNum);
	for (int32 Index = 0; Index < LoadoutNum; ++Index)
	{
		const UWeaponData* Weapon = CharacterSheet->Loadout[Index];
		if (!Weapon)
		{
			continue;
		}

		SpawnedWeapons[Index].Main = SpawnWeaponActor(Weapon->WeaponClass);
		if (const UWeaponOffhandFragment* Offhand = Weapon->FindFragment<UWeaponOffhandFragment>())
		{
			SpawnedWeapons[Index].Offhand = SpawnWeaponActor(Offhand->OffhandClass);
		}
		AttachLoadoutWeapon(Index, /*bHands*/ bWeaponInHands && Index == SelectedWeaponIndex);

		// Приёмы гранятся у каждого класса оружия лоадаута: перезарядка идёт у всего оружия,
		// а хэндлы выбранного ряда - просто другая карта (`EconomySystem.md`, «Перезарядка приёмов»).
		GrantSkillsOfWeaponType(Weapon->Type.Get());
	}
}

void AClanhallHumanoidBase::GrantSkillsOfWeaponType(const UWeaponTypeData* WeaponType)
{
	if (!WeaponType || SkillHandlesByWeaponType.Contains(WeaponType))
	{
		return;
	}

	TMap<FGameplayTag, FGameplayAbilitySpecHandle>& Handles = SkillHandlesByWeaponType.Add(WeaponType);
	int32 NumGranted = 0;
	int32 NumRejectedByLearned = 0;

	// Один класс GA_PhysicalSkill гранится по числу записей в WeaponType->Skills
	// (`CharacterHierarchy.md`, «Грант в BeginPlay»; `WeaponSystem.md`, «Класс ↔ лист
	// персонажа») — набор приёмов принадлежит классу оружия, не листу. Гейт - «выучен ли».
	for (const TPair<FGameplayTag, TObjectPtr<UAbilityData>>& Skill : WeaponType->Skills)
	{
		// Невалидный ключ ИЛИ сам корень Slot (а не лист Q/E/R/F/...) грантится, но
		// GetActiveSkillHandle(Slot_Q) его никогда не найдёт — ключ карты другой.
		// meta=(Categories="Slot") пропускает в пикер и корень. Симптом без этой проверки -
		// "Q не нажимается", причина не ищется.
		if (!Skill.Key.IsValid() || Skill.Key == ClanhallGameplayTags::Slot.GetTag())
		{
			UE_LOG(LogClanhall, Warning, TEXT("%s: запись в %s->Skills с невалидным ключом или корнем Slot вместо листа (Q/E/R/F/...) — навык не будет вызываем."), *GetName(), *WeaponType->GetName());
			continue;
		}

		if (!Skill.Value)
		{
			UE_LOG(LogClanhall, Warning, TEXT("%s: слот %s в %s->Skills не заполнен — навык не грантится."), *GetName(), *Skill.Key.ToString(), *WeaponType->GetName());
			continue;
		}

		if (!Skill.Value->CounterTag.IsValid())
		{
			UE_LOG(LogClanhall, Warning, TEXT("%s: навык в слоте %s (%s) без CounterTag — выучить его нечем, LearnedSkills не может на него сослаться."), *GetName(), *Skill.Key.ToString(), *WeaponType->GetName());
			continue;
		}

		// Не выучен — штатное состояние, не ошибка данных. Verbose, не Warning — иначе лог
		// заливается на каждом бойце.
		if (!CharacterSheet || !CharacterSheet->LearnedSkills.HasTag(Skill.Value->CounterTag))
		{
			UE_LOG(LogClanhall, Verbose, TEXT("%s: навык в слоте %s (%s) не выучен (LearnedSkills) — не грантится."), *GetName(), *Skill.Key.ToString(), *WeaponType->GetName());
			++NumRejectedByLearned;
			continue;
		}

		// Слот доносится до способности динамическим тегом спека (не полем в UAbilityData,
		// `CharacterHierarchy.md`, «Ключ по слоту, а не по имени навыка»): один и тот же
		// UAbilityData может лежать сразу в двух классах оружия, слот же принадлежит гранту.
		FGameplayAbilitySpec Spec(UGA_PhysicalSkill::StaticClass(), 1, INDEX_NONE, Skill.Value);
		Spec.GetDynamicSpecSourceTags().AddTag(Skill.Key);
		const FGameplayAbilitySpecHandle Granted = AbilitySystemComponent->GiveAbility(Spec);
		Handles.Add(Skill.Key, Granted);
		SkillHandlesByIdentity.FindOrAdd(WeaponType).Add(Skill.Value->CounterTag, Granted);
		++NumGranted;
	}

	// Итоговая строка, чтобы гейт не отлаживался вслепую: игрок жмёт Q, ничего не происходит,
	// и без этой строки причина (изученность) не ищется.
	UE_LOG(LogClanhall, Log, TEXT("%s: %s - активок гранто %d, отсеяно изученностью %d."),
		*GetName(), *WeaponType->GetName(), NumGranted, NumRejectedByLearned);
}

void AClanhallHumanoidBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AutoHolsterTimer);

	// Без явного Destroy оружие переживает своего носителя — PIE, запущенный дважды,
	// оставляет мечи на уровне.
	for (FClanhallSpawnedWeapon& Spawned : SpawnedWeapons)
	{
		if (Spawned.Main)
		{
			Spawned.Main->Destroy();
		}
		if (Spawned.Offhand)
		{
			Spawned.Offhand->Destroy();
		}
	}
	SpawnedWeapons.Reset();

	Super::EndPlay(EndPlayReason);
}

AClanhallWeaponActor* AClanhallHumanoidBase::SpawnWeaponActor(TSubclassOf<AClanhallWeaponActor> WeaponClass)
{
	if (!WeaponClass)
	{
		// Законное состояние, не ошибка — оружие без визуала тестируется (`WeaponSystem.md`,
		// «Оружие как актор»). Verbose, но не молчит.
		UE_LOG(LogClanhall, Verbose, TEXT("%s: WeaponClass не задан — оружие без визуала."), *GetName());
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	AClanhallWeaponActor* SpawnedActor = GetWorld()->SpawnActor<AClanhallWeaponActor>(WeaponClass, SpawnParams);
	if (!SpawnedActor)
	{
		// Реальный случай — WeaponClass указывает сам AClanhallWeaponActor: он Abstract, но пикер
		// TSubclassOf абстрактные классы всё равно показывает. Симптом без этой строки — оружия
		// нет, лог чист.
		UE_LOG(LogClanhall, Warning, TEXT("%s: SpawnActor(%s) вернул nullptr — оружия не будет."),
			*GetName(), *WeaponClass->GetName());
	}
	return SpawnedActor;
}

void AClanhallHumanoidBase::AttachWeaponActor(AClanhallWeaponActor* Actor, bool bHands)
{
	if (!Actor)
	{
		return;
	}

	USkeletalMeshComponent* OwnerMesh = GetMesh();
	if (!OwnerMesh)
	{
		UE_LOG(LogClanhall, Warning, TEXT("%s: GetMesh() вернул nullptr — крепить %s некуда, оружие остаётся в мире рут-трансформом."),
			*GetName(), *Actor->GetClass()->GetName());
		return;
	}

	const FName SocketName = bHands ? Actor->AttachSocketName : Actor->HolsterSocketName;
	const FTransform& RelativeTransform = bHands ? Actor->AttachRelativeTransform : Actor->HolsterRelativeTransform;
	const TCHAR* Where = bHands ? TEXT("руки") : TEXT("тела");

	const bool bSocketNamed = !SocketName.IsNone();
	const bool bSocketExists = bSocketNamed && OwnerMesh->DoesSocketExist(SocketName);

	if (!bSocketNamed)
	{
		// Молча ронять оружие в центр персонажа нельзя: симптом «меч торчит из живота» ищется
		// глазами полчаса.
		UE_LOG(LogClanhall, Warning, TEXT("%s: у %s не задан сокет %s — крепление в корень меша."),
			*GetName(), *Actor->GetClass()->GetName(), Where);
	}
	else if (!bSocketExists)
	{
		UE_LOG(LogClanhall, Warning, TEXT("%s: сокет %s %s (заданный в %s) не найден на скелете — крепление в корень меша."),
			*GetName(), Where, *SocketName.ToString(), *Actor->GetClass()->GetName());
	}

	// SnapToTarget обнуляет относительный трансформ относительно сокета, и только после этого
	// SetActorRelativeTransform накладывает оффсет поверх нуля. В обратном порядке снап стёр бы
	// оффсет.
	const FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, false);
	Actor->AttachToComponent(OwnerMesh, AttachRules, bSocketExists ? SocketName : NAME_None);
	Actor->SetActorRelativeTransform(RelativeTransform);
}

void AClanhallHumanoidBase::AttachLoadoutWeapon(int32 LoadoutIndex, bool bHands)
{
	if (!SpawnedWeapons.IsValidIndex(LoadoutIndex))
	{
		return;
	}
	AttachWeaponActor(SpawnedWeapons[LoadoutIndex].Main, bHands);
	AttachWeaponActor(SpawnedWeapons[LoadoutIndex].Offhand, bHands);
}

bool AClanhallHumanoidBase::SelectWeapon(int32 LoadoutIndex)
{
	if (!CharacterSheet || !CharacterSheet->Loadout.IsValidIndex(LoadoutIndex) || !CharacterSheet->Loadout[LoadoutIndex])
	{
		return false;
	}

	if (LoadoutIndex == SelectedWeaponIndex)
	{
		return true;
	}

	// Начатый приём доигрывает (`CombatSystem.md`, «Ввод»): оружие посреди него не меняется.
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(ClanhallGameplayTags::State_SkillCommitted.GetTag()))
	{
		return false;
	}

	// Цена - одна доля, и только если в руках было оружие: её съедает уборка
	// (`WeaponSystem.md`, «Свап оружия»). Руки пусты - выбор бесплатный, жать можно сколько угодно.
	if (bWeaponInHands)
	{
		AttachLoadoutWeapon(SelectedWeaponIndex, /*bHands*/ false);
		bWeaponInHands = false;
		GetWorldTimerManager().ClearTimer(AutoHolsterTimer);

		if (AbilitySystemComponent)
		{
			ClanhallGameplayEffects::ApplyTimedTag(AbilitySystemComponent, ClanhallGameplayTags::State_WeaponSwap.GetTag(),
				ClanhallBeats::ToSeconds(UClanhallCombatSettings::Get().WeaponSwapBeats));
		}
	}

	SetSelectedWeaponIndex(LoadoutIndex);

	// Противник оружие не убирает: старое убрал, новое сразу достал - та же доля.
	if (bKeepsWeaponInHands)
	{
		AttachLoadoutWeapon(SelectedWeaponIndex, /*bHands*/ true);
		bWeaponInHands = true;
	}

	OnWeaponSelected.Broadcast(SelectedWeaponIndex);
	return true;
}

bool AClanhallHumanoidBase::DrawSelectedWeapon()
{
	if (bWeaponInHands || SelectedWeaponIndex == INDEX_NONE)
	{
		return false;
	}
	// Приём выхватывает выбранное оружие и бьёт одним движением - лишней доли на доставание нет.
	AttachLoadoutWeapon(SelectedWeaponIndex, /*bHands*/ true);
	bWeaponInHands = true;
	return true;
}

void AClanhallHumanoidBase::NotifySkillStarted()
{
	GetWorldTimerManager().ClearTimer(AutoHolsterTimer);
	if (LastSkillTimeByWeapon.IsValidIndex(SelectedWeaponIndex))
	{
		LastSkillTimeByWeapon[SelectedWeaponIndex] = GetWorld()->GetTimeSeconds();
	}
}

void AClanhallHumanoidBase::NotifySkillEnded()
{
	// После приёма оружие остаётся в руках, опущенным; через несколько долей без дела - на тело.
	if (bKeepsWeaponInHands || !bWeaponInHands)
	{
		return;
	}
	const float Delay = ClanhallBeats::ToSeconds(UClanhallCombatSettings::Get().AutoHolsterBeats);
	GetWorldTimerManager().SetTimer(AutoHolsterTimer, this, &AClanhallHumanoidBase::HolsterByTimer, FMath::Max(Delay, 0.01f), false);
}

void AClanhallHumanoidBase::HolsterByTimer()
{
	// Начатый приём не трогаем - таймер перезапустится на его конце.
	if (!bWeaponInHands || (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(ClanhallGameplayTags::State_SkillCommitted.GetTag())))
	{
		return;
	}
	AttachLoadoutWeapon(SelectedWeaponIndex, /*bHands*/ false);
	bWeaponInHands = false;
}

double AClanhallHumanoidBase::GetSecondsSinceSelectedWeaponUsed() const
{
	if (!LastSkillTimeByWeapon.IsValidIndex(SelectedWeaponIndex) || LastSkillTimeByWeapon[SelectedWeaponIndex] < 0.0)
	{
		return TNumericLimits<float>::Max();
	}
	return GetWorld()->GetTimeSeconds() - LastSkillTimeByWeapon[SelectedWeaponIndex];
}

TArray<UWeaponData*> AClanhallHumanoidBase::GetLoadout() const
{
	TArray<UWeaponData*> Result;
	if (CharacterSheet)
	{
		for (const TObjectPtr<UWeaponData>& Weapon : CharacterSheet->Loadout)
		{
			Result.Add(Weapon.Get());
		}
	}
	return Result;
}

float AClanhallHumanoidBase::GetSkillTurnRate() const
{
	const UWeaponTypeData* WeaponType = GetWeaponType();
	return WeaponType ? WeaponType->SkillTurnRate : ClanhallWeaponDefaults::SkillTurnRate;
}

void AClanhallHumanoidBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickSkillTurn(DeltaSeconds);
}

void AClanhallHumanoidBase::TickSkillTurn(float DeltaSeconds)
{
	if (IsPlayerControlled() || !AbilitySystemComponent
		|| !AbilitySystemComponent->HasMatchingGameplayTag(ClanhallGameplayTags::State_SkillCommitted.GetTag()))
	{
		return;
	}

	// Цель противника - игрок: прототипное допущение 1v1 (`Implementation/CharacterHierarchy.md`),
	// пока нет AIController и выбора цели.
	const APawn* Opponent = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Opponent)
	{
		return;
	}
	FVector ToOpponent = Opponent->GetActorLocation() - GetActorLocation();
	ToOpponent.Z = 0.0f;
	if (ToOpponent.IsNearlyZero())
	{
		return;
	}
	const FRotator Current = GetActorRotation();
	const FRotator Desired(0.0f, ToOpponent.Rotation().Yaw, 0.0f);
	SetActorRotation(FMath::RInterpConstantTo(Current, Desired, DeltaSeconds, GetSkillTurnRate()));
}

const UWeaponTypeData* AClanhallHumanoidBase::GetWeaponType() const
{
	return CurrentWeapon ? CurrentWeapon->Type.Get() : nullptr;
}

float AClanhallHumanoidBase::GetWeaponSpeedMultiplier() const
{
	const UWeaponTypeData* WeaponType = GetWeaponType();
	return WeaponType ? WeaponType->WeaponSpeedMultiplier : ClanhallWeaponDefaults::WeaponSpeedMultiplier;
}

FGameplayAbilitySpecHandle AClanhallHumanoidBase::GetActiveSkillHandle(FGameplayTag AbilitySlotTag) const
{
	return GetSkillHandleForWeapon(SelectedWeaponIndex, AbilitySlotTag);
}

const UWeaponTypeData* AClanhallHumanoidBase::GetLoadoutWeaponType(int32 LoadoutIndex) const
{
	if (!CharacterSheet || !CharacterSheet->Loadout.IsValidIndex(LoadoutIndex) || !CharacterSheet->Loadout[LoadoutIndex])
	{
		return nullptr;
	}
	return CharacterSheet->Loadout[LoadoutIndex]->Type.Get();
}

FGameplayAbilitySpecHandle AClanhallHumanoidBase::GetSkillHandleForWeapon(int32 LoadoutIndex, FGameplayTag AbilitySlotTag) const
{
	const UWeaponTypeData* Type = GetLoadoutWeaponType(LoadoutIndex);
	if (!Type)
	{
		return FGameplayAbilitySpecHandle();
	}

	// Игрок переназначал клавиши этого класса - карта переопределений полная, данные не читаются.
	if (const TMap<FGameplayTag, FGameplayTag>* Overrides = SlotOverrides.Find(Type))
	{
		const FGameplayTag Identity = Overrides->FindRef(AbilitySlotTag);
		const TMap<FGameplayTag, FGameplayAbilitySpecHandle>* ByIdentity = SkillHandlesByIdentity.Find(Type);
		return (Identity.IsValid() && ByIdentity) ? ByIdentity->FindRef(Identity) : FGameplayAbilitySpecHandle();
	}

	const TMap<FGameplayTag, FGameplayAbilitySpecHandle>* Handles = SkillHandlesByWeaponType.Find(Type);
	return Handles ? Handles->FindRef(AbilitySlotTag) : FGameplayAbilitySpecHandle();
}

TArray<UAbilityData*> AClanhallHumanoidBase::GetLearnedSkillsOfWeapon(int32 LoadoutIndex) const
{
	TArray<UAbilityData*> Result;
	const UWeaponTypeData* Type = GetLoadoutWeaponType(LoadoutIndex);
	const TMap<FGameplayTag, FGameplayAbilitySpecHandle>* ByIdentity = Type ? SkillHandlesByIdentity.Find(Type) : nullptr;
	if (!ByIdentity)
	{
		return Result;
	}
	for (const TPair<FGameplayTag, TObjectPtr<UAbilityData>>& Skill : Type->Skills)
	{
		if (Skill.Value && ByIdentity->Contains(Skill.Value->CounterTag))
		{
			Result.AddUnique(Skill.Value.Get());
		}
	}
	return Result;
}

FGameplayTag AClanhallHumanoidBase::GetSkillSlot(int32 LoadoutIndex, FGameplayTag SkillIdentity) const
{
	const UWeaponTypeData* Type = GetLoadoutWeaponType(LoadoutIndex);
	if (!Type || !SkillIdentity.IsValid())
	{
		return FGameplayTag();
	}
	if (const TMap<FGameplayTag, FGameplayTag>* Overrides = SlotOverrides.Find(Type))
	{
		for (const TPair<FGameplayTag, FGameplayTag>& Entry : *Overrides)
		{
			if (Entry.Value == SkillIdentity)
			{
				return Entry.Key;
			}
		}
		return FGameplayTag();
	}
	const TMap<FGameplayTag, FGameplayAbilitySpecHandle>* Handles = SkillHandlesByWeaponType.Find(Type);
	for (const TPair<FGameplayTag, TObjectPtr<UAbilityData>>& Skill : Type->Skills)
	{
		if (Skill.Value && Skill.Value->CounterTag == SkillIdentity && Handles && Handles->Contains(Skill.Key))
		{
			return Skill.Key;
		}
	}
	return FGameplayTag();
}

TMap<FGameplayTag, FGameplayTag>& AClanhallHumanoidBase::EnsureSlotOverrides(const UWeaponTypeData* WeaponType)
{
	if (TMap<FGameplayTag, FGameplayTag>* Existing = SlotOverrides.Find(WeaponType))
	{
		return *Existing;
	}
	TMap<FGameplayTag, FGameplayTag>& Fresh = SlotOverrides.Add(WeaponType);
	if (const TMap<FGameplayTag, FGameplayAbilitySpecHandle>* Handles = SkillHandlesByWeaponType.Find(WeaponType))
	{
		for (const TPair<FGameplayTag, TObjectPtr<UAbilityData>>& Skill : WeaponType->Skills)
		{
			if (Skill.Value && Handles->Contains(Skill.Key))
			{
				Fresh.Add(Skill.Key, Skill.Value->CounterTag);
			}
		}
	}
	return Fresh;
}

bool AClanhallHumanoidBase::AssignSlot(int32 LoadoutIndex, FGameplayTag SkillIdentity, FGameplayTag Slot)
{
	const UWeaponTypeData* Type = GetLoadoutWeaponType(LoadoutIndex);
	const TMap<FGameplayTag, FGameplayAbilitySpecHandle>* ByIdentity = Type ? SkillHandlesByIdentity.Find(Type) : nullptr;
	if (!ByIdentity || !ByIdentity->Contains(SkillIdentity) || !Slot.IsValid() || Slot == ClanhallGameplayTags::Slot.GetTag())
	{
		return false;
	}

	TMap<FGameplayTag, FGameplayTag>& Overrides = EnsureSlotOverrides(Type);
	for (auto It = Overrides.CreateIterator(); It; ++It)
	{
		if (It->Value == SkillIdentity)
		{
			It.RemoveCurrent();
		}
	}
	// Add перезаписывает: прежний приём этой клавиши остаётся на панели без клавиши.
	Overrides.Add(Slot, SkillIdentity);

	OnSkillSlotsChanged.Broadcast();
	OnSkillSlotsAssigned();
	return true;
}

TArray<FClanhallSlotAssignment> AClanhallHumanoidBase::ExportSlotOverrides() const
{
	TArray<FClanhallSlotAssignment> Result;
	for (const TPair<const UWeaponTypeData*, TMap<FGameplayTag, FGameplayTag>>& TypeEntry : SlotOverrides)
	{
		if (!TypeEntry.Key)
		{
			continue;
		}
		for (const TPair<FGameplayTag, FGameplayTag>& Entry : TypeEntry.Value)
		{
			FClanhallSlotAssignment& Assignment = Result.AddDefaulted_GetRef();
			Assignment.WeaponType = FSoftObjectPath(TypeEntry.Key);
			Assignment.Slot = Entry.Key;
			Assignment.Skill = Entry.Value;
		}
	}
	return Result;
}

void AClanhallHumanoidBase::ImportSlotOverrides(const TArray<FClanhallSlotAssignment>& Assignments)
{
	bool bImported = false;
	for (const FClanhallSlotAssignment& Assignment : Assignments)
	{
		if (!Assignment.Slot.IsValid())
		{
			continue;
		}
		for (const TPair<const UWeaponTypeData*, TMap<FGameplayTag, FGameplayAbilitySpecHandle>>& TypeEntry : SkillHandlesByIdentity)
		{
			if (TypeEntry.Key && FSoftObjectPath(TypeEntry.Key) == Assignment.WeaponType && TypeEntry.Value.Contains(Assignment.Skill))
			{
				// Сохранённая карта класса полная - собирается как есть, без копии данных.
				SlotOverrides.FindOrAdd(TypeEntry.Key).Add(Assignment.Slot, Assignment.Skill);
				bImported = true;
			}
		}
	}
	if (bImported)
	{
		OnSkillSlotsChanged.Broadcast();
	}
}
