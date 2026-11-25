// Реестр отладочных консольных команд. Добавление нового атрибута = одна строка
// в GetDebugAttributeMap(); добавление новой команды = один новый
// FAutoConsoleCommandWithWorldAndArgs ниже. Своей логики поверх существующих
// систем не пишем — только резолв цели/атрибута и вывод результата.

#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallCombatStateComponent.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/MarkData.h"
#include "AbilitySystem/ClanhallTargetingComponent.h"
#include "AbilitySystem/ClanhallSkillCooldownComponent.h"
#include "AbilitySystem/ClanhallCrystalComponent.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallHumanoidBase.h"
#include "ClanhallCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "AssetRegistry/IAssetRegistry.h"

#if !UE_BUILD_SHIPPING

DEFINE_LOG_CATEGORY_STATIC(LogClanhallDebug, Log, All);

// Стабильный ключ экранного сообщения на команду — повторный вызов заменяет
// строку, а не копит новую.
enum class EDebugMsgKey : int32
{
	PlayerSetStat = 9001,
	EnemySetStat,
	PlayerShowStats,
	EnemyShowStats,
	PlayerAddMark,
	EnemyAddMark,
	ListStats,
	PlayerShowCombatState,
	EnemyUseSkill,
	PlayerShowCooldowns,
	EnemyShowCooldowns,
	EnemySelectWeapon,
	EnemyTaunt,
	PlayerSetPower,
	EnemySetPower,
	PlayerShowMarks,
	EnemyShowMarks,
	PlayerAddCrystal,
	EnemyAddCrystal,
	EnemyUseCrystal,
	PlayerUseSkill,
	PlayerSelectWeapon,
	PlayerHoldLayer,
	PlayerDodge,
};

static const TMap<FName, FGameplayAttribute>& GetDebugAttributeMap()
{
	static const TMap<FName, FGameplayAttribute> Map = {
		{ TEXT("AP"),          UClanhallAttributeSet::GetAPAttribute() },
		{ TEXT("MaxAP"),       UClanhallAttributeSet::GetMaxAPAttribute() },
		{ TEXT("HP"),          UClanhallAttributeSet::GetHPAttribute() },
		{ TEXT("MaxHP"),       UClanhallAttributeSet::GetMaxHPAttribute() },
		{ TEXT("MP"),          UClanhallAttributeSet::GetMPAttribute() },
		{ TEXT("MaxMP"),       UClanhallAttributeSet::GetMaxMPAttribute() },
		{ TEXT("CritChance"),  UClanhallAttributeSet::GetCritChanceAttribute() },
		{ TEXT("ThresholdReduction"), UClanhallAttributeSet::GetThresholdReductionAttribute() },
	};
	return Map;
}

// FName сравнивается регистронезависимо, поэтому поиск в TMap<FName, ...>
// уже даёт регистронезависимое сопоставление (hp == HP).
static const TPair<FName, FGameplayAttribute>* FindAttributeEntry(const FString& Name)
{
	for (const TPair<FName, FGameplayAttribute>& Entry : GetDebugAttributeMap())
	{
		if (Entry.Key.IsEqual(FName(*Name)))
		{
			return &Entry;
		}
	}
	return nullptr;
}

static FString FormatValue(float Value)
{
	return FString::SanitizeFloat(Value, 0);
}

static void PrintResult(EDebugMsgKey Key, const FString& Message, const FColor& Color = FColor::Yellow)
{
	UE_LOG(LogClanhallDebug, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(static_cast<int32>(Key), 5.0f, Color, Message);
	}
}

static void PrintError(EDebugMsgKey Key, const FString& Message)
{
	PrintResult(Key, Message, FColor::Red);
}

static APawn* ResolvePlayerPawn(UWorld* World, EDebugMsgKey Key)
{
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!Pawn)
	{
		PrintError(Key, TEXT("No player pawn found"));
		return nullptr;
	}
	return Pawn;
}

static AActor* ResolveTargetActor(UWorld* World, EDebugMsgKey Key)
{
	APawn* PlayerPawn = ResolvePlayerPawn(World, Key);
	if (!PlayerPawn)
	{
		return nullptr;
	}

	const UClanhallTargetingComponent* TargetingComponent = PlayerPawn->FindComponentByClass<UClanhallTargetingComponent>();
	if (!TargetingComponent || !TargetingComponent->CurrentTarget)
	{
		PrintError(Key, TEXT("No current target"));
		return nullptr;
	}

	return TargetingComponent->CurrentTarget;
}

static UAbilitySystemComponent* ResolveASCFromActor(AActor* Actor, EDebugMsgKey Key)
{
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Actor);
	UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		PrintError(Key, TEXT("Actor has no AbilitySystemComponent"));
		return nullptr;
	}
	return ASC;
}

static UAbilitySystemComponent* ResolvePlayerASC(UWorld* World, EDebugMsgKey Key)
{
	APawn* Pawn = ResolvePlayerPawn(World, Key);
	return Pawn ? ResolveASCFromActor(Pawn, Key) : nullptr;
}

static UAbilitySystemComponent* ResolveTargetASC(UWorld* World, EDebugMsgKey Key)
{
	AActor* Target = ResolveTargetActor(World, Key);
	return Target ? ResolveASCFromActor(Target, Key) : nullptr;
}

// Для атрибута X, упёршегося в потолок, ограничивающий его MaxX так и называется.
// Для самого MaxX (кламп — жёсткий потолок движка, не другой атрибут) точного
// имени нет.
static FString GetClampReferenceName(const FName& CanonicalName)
{
	const FString NameStr = CanonicalName.ToString();
	if (NameStr.StartsWith(TEXT("Max")))
	{
		return TEXT("клампом");
	}
	return FString::Printf(TEXT("Max%s"), *NameStr);
}

static void HandleSetStat(UWorld* World, const TArray<FString>& Args, bool bTargetEnemy)
{
	const EDebugMsgKey Key = bTargetEnemy ? EDebugMsgKey::EnemySetStat : EDebugMsgKey::PlayerSetStat;

	if (Args.Num() < 2)
	{
		PrintError(Key, bTargetEnemy
			? TEXT("Usage: Clanhall.Enemy.SetStat <Name> <Value>")
			: TEXT("Usage: Clanhall.Player.SetStat <Name> <Value>"));
		return;
	}

	UAbilitySystemComponent* ASC = bTargetEnemy ? ResolveTargetASC(World, Key) : ResolvePlayerASC(World, Key);
	if (!ASC)
	{
		return;
	}

	const TPair<FName, FGameplayAttribute>* Entry = FindAttributeEntry(Args[0]);
	if (!Entry)
	{
		PrintError(Key, FString::Printf(TEXT("Unknown stat '%s'. Use Clanhall.Debug.ListStats"), *Args[0]));
		return;
	}

	if (!FCString::IsNumeric(*Args[1]))
	{
		PrintError(Key, FString::Printf(TEXT("Value must be a number, got '%s'"), *Args[1]));
		return;
	}

	const FGameplayAttribute& Attribute = Entry->Value;
	const float RequestedValue = FCString::Atof(*Args[1]);
	const float BeforeValue = ASC->GetNumericAttribute(Attribute);
	ASC->ApplyModToAttribute(Attribute, EGameplayModOp::Override, RequestedValue);
	const float AfterValue = ASC->GetNumericAttribute(Attribute);

	const FString CanonicalName = Entry->Key.ToString();
	FString Message;
	if (FMath::IsNearlyEqual(AfterValue, RequestedValue))
	{
		Message = FString::Printf(TEXT("%s: %s -> %s"), *CanonicalName, *FormatValue(BeforeValue), *FormatValue(AfterValue));
	}
	else if (AfterValue < RequestedValue)
	{
		Message = FString::Printf(TEXT("%s: %s -> %s (запрошено %s, ограничено %s)"),
			*CanonicalName, *FormatValue(BeforeValue), *FormatValue(AfterValue), *FormatValue(RequestedValue), *GetClampReferenceName(Entry->Key));
	}
	else
	{
		Message = FString::Printf(TEXT("%s: %s -> %s (запрошено %s, ограничено минимумом 0)"),
			*CanonicalName, *FormatValue(BeforeValue), *FormatValue(AfterValue), *FormatValue(RequestedValue));
	}
	PrintResult(Key, Message);
}

static void HandleShowStats(UWorld* World, bool bTargetEnemy)
{
	const EDebugMsgKey Key = bTargetEnemy ? EDebugMsgKey::EnemyShowStats : EDebugMsgKey::PlayerShowStats;

	UAbilitySystemComponent* ASC = bTargetEnemy ? ResolveTargetASC(World, Key) : ResolvePlayerASC(World, Key);
	if (!ASC)
	{
		return;
	}

	FString Message = FString::Printf(
		TEXT("%s AP %s/%s · HP %s/%s · MP %s/%s"),
		bTargetEnemy ? TEXT("[Enemy]") : TEXT("[Player]"),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetAPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetMaxAPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetHPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetMaxHPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetMPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetMaxMPAttribute())));

	// Модификаторы боя - то, что меняют метки (`Implementation/Damage.md`).
	Message += FString::Printf(TEXT("\ncrit %s · threshold -%s · dealt x%s · taken x%s · speed x%s"),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetCritChanceAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetThresholdReductionAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetDamageDealtMultiplierAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetDamageTakenMultiplierAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetMoveSpeedMultiplierAttribute())));

	// Кристалл (`EconomySystem.md`, «Кристалл»): накоплено, целых и доля следующего.
	if (const UClanhallCrystalComponent* Crystal = ASC->GetAvatarActor() ? ASC->GetAvatarActor()->FindComponentByClass<UClanhallCrystalComponent>() : nullptr)
	{
		Message += FString::Printf(TEXT(" · crystal %.2f"), Crystal->GetCrystal());
	}

	// Пороги урона по типам (`CombatSystem.md`, «Damage Threshold (DT)»).
	if (const AClanhallCharacterBase* Fighter = Cast<AClanhallCharacterBase>(ASC->GetAvatarActor()))
	{
		for (const TCHAR* TypeName : { TEXT("Damage.Type.Slash"), TEXT("Damage.Type.Pierce"), TEXT("Damage.Type.Blunt") })
		{
			const FGameplayTag Type = FGameplayTag::RequestGameplayTag(FName(TypeName), false);
			Message += FString::Printf(TEXT(" · %s %s"), TypeName + 12, *FormatValue(Fighter->GetDamageThreshold(Type)));
		}
	}

	PrintResult(Key, Message, FColor::Cyan);
}

// Ассет метки по имени (DA_Mark_Bleed), регистронезависимо. Ищем через реестр ассетов: метки
// в бою могут быть ещё не загружены, если их не держит ни один приём.
static UMarkData* FindMarkAsset(const FString& Name)
{
	const IAssetRegistry* Registry = IAssetRegistry::Get();
	if (!Registry)
	{
		return nullptr;
	}
	TArray<FAssetData> Assets;
	Registry->GetAssetsByClass(UMarkData::StaticClass()->GetClassPathName(), Assets, /*bSearchSubClasses=*/true);
	for (const FAssetData& Asset : Assets)
	{
		if (Asset.AssetName.ToString().Equals(Name, ESearchCase::IgnoreCase))
		{
			return Cast<UMarkData>(Asset.GetAsset());
		}
	}
	return nullptr;
}

static const TCHAR* MarkKindName(EClanhallMarkKind Kind)
{
	switch (Kind)
	{
	case EClanhallMarkKind::Buff: return TEXT("buff");
	case EClanhallMarkKind::Trauma: return TEXT("trauma");
	case EClanhallMarkKind::Advantage: return TEXT("advantage");
	default: return TEXT("debuff");
	}
}

// Метка кладётся без школы: третий уровень, сравнивая школы, видит пустую как чужую
// (`MarksSystem.md`, «Травмы и преимущества»). Необязательный второй аргумент - школа (School.Dagger).
static void HandleAddMark(UWorld* World, const TArray<FString>& Args, bool bTargetEnemy)
{
	const EDebugMsgKey Key = bTargetEnemy ? EDebugMsgKey::EnemyAddMark : EDebugMsgKey::PlayerAddMark;

	if (Args.Num() < 1)
	{
		PrintError(Key, bTargetEnemy
			? TEXT("Usage: Clanhall.Enemy.AddMark <MarkDataAssetName> [School.X]")
			: TEXT("Usage: Clanhall.Player.AddMark <MarkDataAssetName> [School.X]"));
		return;
	}

	AActor* TargetActor = bTargetEnemy ? ResolveTargetActor(World, Key) : static_cast<AActor*>(ResolvePlayerPawn(World, Key));
	if (!TargetActor)
	{
		return;
	}

	UMarkData* Mark = FindMarkAsset(Args[0]);
	if (!Mark)
	{
		PrintError(Key, FString::Printf(TEXT("No UMarkData asset named '%s'"), *Args[0]));
		return;
	}

	FGameplayTag School;
	if (Args.Num() >= 2)
	{
		School = FGameplayTag::RequestGameplayTag(FName(*Args[1]), /*ErrorIfNotFound=*/false);
		if (!School.IsValid())
		{
			PrintError(Key, FString::Printf(TEXT("Unknown school tag '%s'"), *Args[1]));
			return;
		}
	}

	UClanhallMarkComponent* MarkComponent = TargetActor->FindComponentByClass<UClanhallMarkComponent>();
	if (!MarkComponent)
	{
		PrintError(Key, TEXT("Actor has no MarkComponent"));
		return;
	}

	MarkComponent->PlaceMark(Mark, School, /*SourceActor=*/nullptr);
	PrintResult(Key, FString::Printf(TEXT("%s mark %s placed (level %d %s)"), bTargetEnemy ? TEXT("[Enemy]") : TEXT("[Player]"),
		*Mark->MarkTag.ToString(), Mark->Level, MarkKindName(Mark->Kind)));
}

static void HandleShowMarks(UWorld* World, bool bTargetEnemy)
{
	const EDebugMsgKey Key = bTargetEnemy ? EDebugMsgKey::EnemyShowMarks : EDebugMsgKey::PlayerShowMarks;
	AActor* Actor = bTargetEnemy ? ResolveTargetActor(World, Key) : ResolvePlayerPawn(World, Key);
	const UClanhallMarkComponent* Marks = Actor ? Actor->FindComponentByClass<UClanhallMarkComponent>() : nullptr;
	if (!Marks)
	{
		PrintError(Key, TEXT("No MarkComponent"));
		return;
	}

	FString Message = bTargetEnemy ? TEXT("[Enemy] marks:") : TEXT("[Player] marks:");
	const TArray<FClanhallMarkView> Views = Marks->GetMarks();
	if (Views.Num() == 0)
	{
		Message += TEXT(" none");
	}
	for (const FClanhallMarkView& View : Views)
	{
		Message += FString::Printf(TEXT("\nL%d %s %s  %.1f beats  [%s]"), View.Level, MarkKindName(View.Kind), *View.Tag.ToString(),
			View.RemainingBeats, View.SourceSchool.IsValid() ? *View.SourceSchool.ToString() : TEXT("no school"));
	}
	PrintResult(Key, Message, FColor::Cyan);
}

static void HandleListStats(EDebugMsgKey Key)
{
	TArray<FString> Names;
	for (const TPair<FName, FGameplayAttribute>& Entry : GetDebugAttributeMap())
	{
		Names.Add(Entry.Key.ToString());
	}
	PrintResult(Key, FString::Printf(TEXT("Valid stats: %s"), *FString::Join(Names, TEXT(", "))), FColor::Green);
}

// Читает UClanhallCombatStateComponent на игроке: без этой команды граница боя невидима,
// а от неё зависит цена дальнего отскока и работоспособность вне-боевого лечения
// (`CombatSystem.md`, «Боевое состояние»).
static void HandleShowCombatState(UWorld* World)
{
	const EDebugMsgKey Key = EDebugMsgKey::PlayerShowCombatState;

	APawn* Pawn = ResolvePlayerPawn(World, Key);
	if (!Pawn)
	{
		return;
	}

	const UClanhallCombatStateComponent* CombatState = Pawn->FindComponentByClass<UClanhallCombatStateComponent>();
	if (!CombatState)
	{
		PrintError(Key, TEXT("Pawn has no CombatStateComponent"));
		return;
	}

	// Sprinting и TurningInPlace - общие для любого AClanhallCharacterBase. Без них редакторная
	// проверка локомоции превращается в угадайку.
	const AClanhallCharacterBase* CharacterBase = Cast<AClanhallCharacterBase>(Pawn);
	const FString SprintingStr = CharacterBase ? (CharacterBase->IsSprinting() ? TEXT("yes") : TEXT("no")) : TEXT("n/a");
	const FString TurningStr = CharacterBase ? (CharacterBase->IsTurningInPlace() ? TEXT("yes") : TEXT("no")) : TEXT("n/a");

	const FString Message = FString::Printf(
		TEXT("%s | enemies in radius: %d | exit timer: %.1fs | Sprinting: %s | TurningInPlace: %s"),
		CombatState->IsInCombat() ? TEXT("IN COMBAT") : TEXT("out of combat"),
		CombatState->GetTrackedEnemyCount(),
		CombatState->GetExitTimeRemaining(),
		*SprintingStr, *TurningStr);

	PrintResult(Key, Message, CombatState->IsInCombat() ? FColor::Red : FColor::Green);
}

// Приём бойца в слоте его выбранного оружия. Слот - короткое имя (Q, E, R, F, Z, X, C, V).
// Манекену нечем драться без BT; приём игрока - для проверок без клавиатуры (скрипт в редакторе).
static void UseSkillInSlot(AClanhallHumanoidBase* Fighter, const TArray<FString>& Args, EDebugMsgKey Key, const TCHAR* Who)
{
	if (Args.Num() < 1)
	{
		PrintError(Key, TEXT("Usage: Clanhall.<Player|Enemy>.UseSkill <Slot>, e.g. Q"));
		return;
	}
	if (!Fighter)
	{
		PrintError(Key, TEXT("Fighter is not an AClanhallHumanoidBase"));
		return;
	}

	const FGameplayTag Slot = FGameplayTag::RequestGameplayTag(FName(*(TEXT("Slot.") + Args[0].ToUpper())), /*ErrorIfNotFound*/ false);
	if (!Slot.IsValid())
	{
		PrintError(Key, FString::Printf(TEXT("Unknown slot '%s'"), *Args[0]));
		return;
	}

	const FGameplayAbilitySpecHandle Handle = Fighter->GetActiveSkillHandle(Slot);
	UAbilitySystemComponent* ASC = Fighter->GetAbilitySystemComponent();
	if (!Handle.IsValid() || !ASC)
	{
		PrintError(Key, FString::Printf(TEXT("%s has no skill in %s"), *Fighter->GetName(), *Slot.ToString()));
		return;
	}

	const bool bActivated = ASC->TryActivateAbility(Handle);
	PrintResult(Key, FString::Printf(TEXT("[%s] %s: %s"), Who, *Slot.ToString(), bActivated ? TEXT("activated") : TEXT("refused (cooldown, commit or state)")),
		bActivated ? FColor::Green : FColor::Red);
}

static void HandlePlayerUseSkill(UWorld* World, const TArray<FString>& Args)
{
	UseSkillInSlot(Cast<AClanhallHumanoidBase>(ResolvePlayerPawn(World, EDebugMsgKey::PlayerUseSkill)), Args, EDebugMsgKey::PlayerUseSkill, TEXT("Player"));
}

static void HandlePlayerSelectWeapon(UWorld* World, const TArray<FString>& Args)
{
	const EDebugMsgKey Key = EDebugMsgKey::PlayerSelectWeapon;
	AClanhallHumanoidBase* Player = Cast<AClanhallHumanoidBase>(ResolvePlayerPawn(World, Key));
	if (!Player || Args.Num() < 1)
	{
		PrintError(Key, TEXT("Usage: Clanhall.Player.SelectWeapon <LoadoutIndex>"));
		return;
	}
	const int32 Index = FCString::Atoi(*Args[0]);
	const bool bSelected = Player->SelectWeapon(Index);
	PrintResult(Key, FString::Printf(TEXT("[Player] select weapon %d: %s"), Index, bSelected ? TEXT("ok") : TEXT("refused")),
		bSelected ? FColor::Green : FColor::Red);
}

static void HandleEnemyUseSkill(UWorld* World, const TArray<FString>& Args)
{
	const EDebugMsgKey Key = EDebugMsgKey::EnemyUseSkill;
	UseSkillInSlot(Cast<AClanhallHumanoidBase>(ResolveTargetActor(World, Key)), Args, Key, TEXT("Enemy"));
}

// Противник оружие не убирает - смена стоит ему ту же долю (`WeaponSystem.md`, «Свап оружия»).
static void HandleEnemySelectWeapon(UWorld* World, const TArray<FString>& Args)
{
	const EDebugMsgKey Key = EDebugMsgKey::EnemySelectWeapon;
	if (Args.Num() < 1)
	{
		PrintError(Key, TEXT("Usage: Clanhall.Enemy.SelectWeapon <LoadoutIndex>"));
		return;
	}
	AClanhallHumanoidBase* Enemy = Cast<AClanhallHumanoidBase>(ResolveTargetActor(World, Key));
	if (!Enemy)
	{
		PrintError(Key, TEXT("Current target is not an AClanhallHumanoidBase"));
		return;
	}
	const int32 Index = FCString::Atoi(*Args[0]);
	const bool bSelected = Enemy->SelectWeapon(Index);
	PrintResult(Key, FString::Printf(TEXT("[Enemy] select weapon %d: %s"), Index, bSelected ? TEXT("ok") : TEXT("refused")),
		bSelected ? FColor::Green : FColor::Red);
}

static void HandleEnemyTaunt(UWorld* World)
{
	const EDebugMsgKey Key = EDebugMsgKey::EnemyTaunt;
	AClanhallHumanoidBase* Enemy = Cast<AClanhallHumanoidBase>(ResolveTargetActor(World, Key));
	UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		PrintError(Key, TEXT("Current target is not an AClanhallHumanoidBase with ASC"));
		return;
	}
	const bool bActivated = ASC->TryActivateAbility(Enemy->GetTauntHandle());
	PrintResult(Key, FString::Printf(TEXT("[Enemy] taunt: %s"), bActivated ? TEXT("activated") : TEXT("refused")),
		bActivated ? FColor::Green : FColor::Red);
}

// Сила для насмешки-парирования (`CombatSystem.md`, «Насмешка»): проверить парирование в обе стороны.
static void HandleSetPower(UWorld* World, const TArray<FString>& Args, bool bTargetEnemy)
{
	const EDebugMsgKey Key = bTargetEnemy ? EDebugMsgKey::EnemySetPower : EDebugMsgKey::PlayerSetPower;
	if (Args.Num() < 1)
	{
		PrintError(Key, TEXT("Usage: Clanhall.<Player|Enemy>.SetPower <Value>"));
		return;
	}
	AActor* Actor = bTargetEnemy ? ResolveTargetActor(World, Key) : ResolvePlayerPawn(World, Key);
	AClanhallCharacterBase* Fighter = Cast<AClanhallCharacterBase>(Actor);
	if (!Fighter)
	{
		PrintError(Key, TEXT("Not an AClanhallCharacterBase"));
		return;
	}
	Fighter->SetPower(FCString::Atof(*Args[0]));
	PrintResult(Key, FString::Printf(TEXT("%s power = %.2f"), *Fighter->GetName(), Fighter->GetPower()), FColor::Green);
}

static void HandleAddCrystal(UWorld* World, const TArray<FString>& Args, bool bTargetEnemy)
{
	const EDebugMsgKey Key = bTargetEnemy ? EDebugMsgKey::EnemyAddCrystal : EDebugMsgKey::PlayerAddCrystal;
	if (Args.Num() < 1 || !FCString::IsNumeric(*Args[0]))
	{
		PrintError(Key, TEXT("Usage: Clanhall.<Player|Enemy>.AddCrystal <Share>, e.g. 1 or 0.1"));
		return;
	}
	AActor* Actor = bTargetEnemy ? ResolveTargetActor(World, Key) : ResolvePlayerPawn(World, Key);
	UClanhallCrystalComponent* Crystal = Actor ? Actor->FindComponentByClass<UClanhallCrystalComponent>() : nullptr;
	if (!Crystal)
	{
		PrintError(Key, TEXT("No CrystalComponent"));
		return;
	}
	Crystal->AddShare(FCString::Atof(*Args[0]));
	PrintResult(Key, FString::Printf(TEXT("%s crystal = %.2f"), bTargetEnemy ? TEXT("[Enemy]") : TEXT("[Player]"), Crystal->GetCrystal()), FColor::Green);
}

static void HandleEnemyUseCrystal(UWorld* World)
{
	const EDebugMsgKey Key = EDebugMsgKey::EnemyUseCrystal;
	AClanhallCharacterBase* Enemy = Cast<AClanhallCharacterBase>(ResolveTargetActor(World, Key));
	UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		PrintError(Key, TEXT("Current target is not an AClanhallCharacterBase with ASC"));
		return;
	}
	const bool bActivated = ASC->TryActivateAbility(Enemy->GetCrystalHandle());
	PrintResult(Key, FString::Printf(TEXT("[Enemy] use crystal: %s"), bActivated ? TEXT("activated") : TEXT("refused (no whole crystal or state)")),
		bActivated ? FColor::Green : FColor::Red);
}

static void HandleShowCooldowns(UWorld* World, bool bTargetEnemy)
{
	const EDebugMsgKey Key = bTargetEnemy ? EDebugMsgKey::EnemyShowCooldowns : EDebugMsgKey::PlayerShowCooldowns;
	AActor* Actor = bTargetEnemy ? ResolveTargetActor(World, Key) : ResolvePlayerPawn(World, Key);
	const UClanhallSkillCooldownComponent* Cooldowns = Actor ? Actor->FindComponentByClass<UClanhallSkillCooldownComponent>() : nullptr;
	if (!Cooldowns)
	{
		PrintError(Key, TEXT("No SkillCooldownComponent"));
		return;
	}

	FString Message = bTargetEnemy ? TEXT("[Enemy] cooldowns:") : TEXT("[Player] cooldowns:");
	const TArray<FGameplayTag> Active = Cooldowns->GetActiveCooldowns();
	if (Active.Num() == 0)
	{
		Message += TEXT(" all ready");
	}
	for (const FGameplayTag& Skill : Active)
	{
		Message += FString::Printf(TEXT("\n%s  %.1f / %.1f beats"), *Skill.ToString(), Cooldowns->GetRemainingBeats(Skill), Cooldowns->GetTotalBeats(Skill));
	}
	PrintResult(Key, Message, FColor::Cyan);
}

static FAutoConsoleCommandWithWorldAndArgs CVarPlayerSetStat(
	TEXT("Clanhall.Player.SetStat"),
	TEXT("Set an attribute on the player, clamped by UClanhallAttributeSet. Usage: Clanhall.Player.SetStat <Name> <Value>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		HandleSetStat(World, Args, /*bTargetEnemy=*/false);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarEnemySetStat(
	TEXT("Clanhall.Enemy.SetStat"),
	TEXT("Set an attribute on the current target, clamped by UClanhallAttributeSet. Usage: Clanhall.Enemy.SetStat <Name> <Value>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		HandleSetStat(World, Args, /*bTargetEnemy=*/true);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarPlayerShowStats(
	TEXT("Clanhall.Player.ShowStats"),
	TEXT("Dump all resource attributes of the player."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		HandleShowStats(World, /*bTargetEnemy=*/false);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarEnemyShowStats(
	TEXT("Clanhall.Enemy.ShowStats"),
	TEXT("Dump all resource attributes of the current target."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		HandleShowStats(World, /*bTargetEnemy=*/true);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarPlayerAddMark(
	TEXT("Clanhall.Player.AddMark"),
	TEXT("Place a mark (UMarkData asset) on the player. Usage: Clanhall.Player.AddMark <MarkDataAssetName> [School.X]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		HandleAddMark(World, Args, /*bTargetEnemy=*/false);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarEnemyAddMark(
	TEXT("Clanhall.Enemy.AddMark"),
	TEXT("Place a mark (UMarkData asset) on the current target. Usage: Clanhall.Enemy.AddMark <MarkDataAssetName> [School.X]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		HandleAddMark(World, Args, /*bTargetEnemy=*/true);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarListStats(
	TEXT("Clanhall.Debug.ListStats"),
	TEXT("List valid attribute names for Clanhall.Player.SetStat / Clanhall.Enemy.SetStat."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& /*Args*/, UWorld* /*World*/)
	{
		HandleListStats(EDebugMsgKey::ListStats);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarPlayerShowCombatState(
	TEXT("Clanhall.Player.ShowCombatState"),
	TEXT("Print combat state (in/out of combat), tracked enemy count and exit timer for the player."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		HandleShowCombatState(World);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarEnemyUseSkill(
	TEXT("Clanhall.Enemy.UseSkill"),
	TEXT("Activate the current target's skill in a slot of its selected weapon. Usage: Clanhall.Enemy.UseSkill <Slot>, e.g. Q"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		HandleEnemyUseSkill(World, Args);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarEnemySelectWeapon(
	TEXT("Clanhall.Enemy.SelectWeapon"),
	TEXT("Select a loadout weapon on the current target. Usage: Clanhall.Enemy.SelectWeapon <LoadoutIndex>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		HandleEnemySelectWeapon(World, Args);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarEnemyTaunt(
	TEXT("Clanhall.Enemy.Taunt"),
	TEXT("The current target taunts."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		HandleEnemyTaunt(World);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarPlayerSetPower(
	TEXT("Clanhall.Player.SetPower"),
	TEXT("Set the player's Power (taunt-parry strength). Usage: Clanhall.Player.SetPower <Value>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		HandleSetPower(World, Args, /*bTargetEnemy=*/false);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarEnemySetPower(
	TEXT("Clanhall.Enemy.SetPower"),
	TEXT("Set the current target's Power (taunt-parry strength). Usage: Clanhall.Enemy.SetPower <Value>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		HandleSetPower(World, Args, /*bTargetEnemy=*/true);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarPlayerShowCooldowns(
	TEXT("Clanhall.Player.ShowCooldowns"),
	TEXT("Print remaining cooldown of every skill of the player, in beats."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		HandleShowCooldowns(World, /*bTargetEnemy=*/false);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarEnemyShowCooldowns(
	TEXT("Clanhall.Enemy.ShowCooldowns"),
	TEXT("Print remaining cooldown of every skill of the current target, in beats."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		HandleShowCooldowns(World, /*bTargetEnemy=*/true);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarPlayerShowMarks(
	TEXT("Clanhall.Player.ShowMarks"),
	TEXT("Print marks on the player: level, kind, tag, remaining beats, source school."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		HandleShowMarks(World, /*bTargetEnemy=*/false);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarEnemyShowMarks(
	TEXT("Clanhall.Enemy.ShowMarks"),
	TEXT("Print marks on the current target: level, kind, tag, remaining beats, source school."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		HandleShowMarks(World, /*bTargetEnemy=*/true);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarPlayerAddCrystal(
	TEXT("Clanhall.Player.AddCrystal"),
	TEXT("Add a crystal share to the player (capped by CrystalCap). Usage: Clanhall.Player.AddCrystal <Share>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		HandleAddCrystal(World, Args, /*bTargetEnemy=*/false);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarEnemyAddCrystal(
	TEXT("Clanhall.Enemy.AddCrystal"),
	TEXT("Add a crystal share to the current target (capped by CrystalCap). Usage: Clanhall.Enemy.AddCrystal <Share>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		HandleAddCrystal(World, Args, /*bTargetEnemy=*/true);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarEnemyUseCrystal(
	TEXT("Clanhall.Enemy.UseCrystal"),
	TEXT("The current target uses a whole crystal."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		HandleEnemyUseCrystal(World);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarPlayerUseSkill(
	TEXT("Clanhall.Player.UseSkill"),
	TEXT("Activate the player's skill in a slot of the selected weapon, as LMB with the slot held. Usage: Clanhall.Player.UseSkill <Slot>, e.g. Q"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		HandlePlayerUseSkill(World, Args);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarPlayerSelectWeapon(
	TEXT("Clanhall.Player.SelectWeapon"),
	TEXT("Select a loadout weapon on the player, as a weapon key. Usage: Clanhall.Player.SelectWeapon <LoadoutIndex>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		HandlePlayerSelectWeapon(World, Args);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarPlayerHoldLayer(
	TEXT("Clanhall.Player.HoldLayer"),
	TEXT("Hold an input layer as if a mouse button were held: Body (LMB), Mouth (RMB), None (release all). For scripted input checks."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const EDebugMsgKey Key = EDebugMsgKey::PlayerHoldLayer;
		AClanhallCharacter* Player = Cast<AClanhallCharacter>(ResolvePlayerPawn(World, Key));
		if (!Player || Args.Num() < 1)
		{
			PrintError(Key, TEXT("Usage: Clanhall.Player.HoldLayer <Body|Mouth|None>"));
			return;
		}
		const EClanhallInputLayer Layer = Args[0].Equals(TEXT("Body"), ESearchCase::IgnoreCase) ? EClanhallInputLayer::Body
			: Args[0].Equals(TEXT("Mouth"), ESearchCase::IgnoreCase) ? EClanhallInputLayer::Mouth : EClanhallInputLayer::None;
		Player->DebugHoldInputLayer(Layer);
		PrintResult(Key, FString::Printf(TEXT("[Player] input layer: %s"), *UEnum::GetValueAsString(Player->GetInputLayer())), FColor::Green);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarPlayerDodge(
	TEXT("Clanhall.Player.Dodge"),
	TEXT("Dodge in a direction relative to the body, as Space + direction. Usage: Clanhall.Player.Dodge <Forward|Back|Left|Right>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		const EDebugMsgKey Key = EDebugMsgKey::PlayerDodge;
		AClanhallCharacter* Player = Cast<AClanhallCharacter>(ResolvePlayerPawn(World, Key));
		if (!Player || Args.Num() < 1)
		{
			PrintError(Key, TEXT("Usage: Clanhall.Player.Dodge <Forward|Back|Left|Right>"));
			return;
		}
		const FString& D = Args[0];
		const EClanhallEvadeDirection Direction = D.Equals(TEXT("Back"), ESearchCase::IgnoreCase) ? EClanhallEvadeDirection::Back
			: D.Equals(TEXT("Left"), ESearchCase::IgnoreCase) ? EClanhallEvadeDirection::Left
			: D.Equals(TEXT("Right"), ESearchCase::IgnoreCase) ? EClanhallEvadeDirection::Right : EClanhallEvadeDirection::Forward;
		Player->TriggerEvade(Direction);
		PrintResult(Key, FString::Printf(TEXT("[Player] dodge %s"), *D), FColor::Green);
	}));

#endif // !UE_BUILD_SHIPPING
