// Реестр отладочных консольных команд. Добавление нового атрибута = одна строка
// в GetDebugAttributeMap(); добавление новой команды = один новый
// FAutoConsoleCommandWithWorldAndArgs ниже. Своей логики поверх существующих
// систем не пишем — только резолв цели/атрибута и вывод результата.

#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/ClanhallTargetingComponent.h"
#include "AbilitySystem/CharacterSheetData.h"
#include "AbilitySystem/WeaponData.h"
#include "AbilitySystem/WeaponTypeData.h"
#include "AbilitySystem/Fragments/ComboData.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallHumanoidCombatant.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimMontage.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

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
	PlayerShowWeaponEconomy,
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
		{ TEXT("Charges"),     UClanhallAttributeSet::GetChargesAttribute() },
		{ TEXT("MaxCharges"),  UClanhallAttributeSet::GetMaxChargesAttribute() },
		{ TEXT("Stagger"),     UClanhallAttributeSet::GetStaggerAttribute() },
		{ TEXT("MaxStagger"),  UClanhallAttributeSet::GetMaxStaggerAttribute() },
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

	const FString Message = FString::Printf(
		TEXT("%s AP %s/%s · HP %s/%s · MP %s/%s · Charges %s/%s · Stagger %s/%s"),
		bTargetEnemy ? TEXT("[Enemy]") : TEXT("[Player]"),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetAPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetMaxAPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetHPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetMaxHPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetMPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetMaxMPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetChargesAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetMaxChargesAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetStaggerAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetMaxStaggerAttribute())));

	PrintResult(Key, Message, FColor::Cyan);
}

static void HandleAddMark(UWorld* World, const TArray<FString>& Args, bool bTargetEnemy)
{
	const EDebugMsgKey Key = bTargetEnemy ? EDebugMsgKey::EnemyAddMark : EDebugMsgKey::PlayerAddMark;

	if (Args.Num() < 1)
	{
		PrintError(Key, bTargetEnemy
			? TEXT("Usage: Clanhall.Enemy.AddMark <TagName>")
			: TEXT("Usage: Clanhall.Player.AddMark <TagName>"));
		return;
	}

	AActor* TargetActor = bTargetEnemy ? ResolveTargetActor(World, Key) : static_cast<AActor*>(ResolvePlayerPawn(World, Key));
	if (!TargetActor)
	{
		return;
	}

	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Args[0]), /*ErrorIfNotFound=*/false);
	if (!Tag.IsValid())
	{
		PrintError(Key, FString::Printf(TEXT("Unknown gameplay tag '%s'"), *Args[0]));
		return;
	}

	UClanhallMarkComponent* MarkComponent = TargetActor->FindComponentByClass<UClanhallMarkComponent>();
	if (!MarkComponent)
	{
		PrintError(Key, TEXT("Actor has no MarkComponent"));
		return;
	}

	MarkComponent->ApplyMark(Tag);
	PrintResult(Key, FString::Printf(TEXT("%s mark %s applied"), bTargetEnemy ? TEXT("[Enemy]") : TEXT("[Player]"), *Tag.ToString()));
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

static AClanhallHumanoidCombatant* ResolvePlayerCombatant(UWorld* World, EDebugMsgKey Key)
{
	APawn* Pawn = ResolvePlayerPawn(World, Key);
	AClanhallHumanoidCombatant* Combatant = Pawn ? Cast<AClanhallHumanoidCombatant>(Pawn) : nullptr;
	if (!Combatant)
	{
		PrintError(Key, TEXT("Player pawn is not an AClanhallHumanoidCombatant"));
		return nullptr;
	}
	return Combatant;
}

static TCHAR DirectionToChar(EClanhallAttackDirection Direction)
{
	switch (Direction)
	{
	case EClanhallAttackDirection::Overhead:   return TEXT('W');
	case EClanhallAttackDirection::RightSlash: return TEXT('D');
	case EClanhallAttackDirection::LeftSlash:  return TEXT('A');
	case EClanhallAttackDirection::LowSweep:   return TEXT('S');
	default:                                   return TEXT('?');
	}
}

static bool TryParseDirection(TCHAR Ch, EClanhallAttackDirection& OutDirection)
{
	switch (FChar::ToUpper(Ch))
	{
	case TEXT('W'): OutDirection = EClanhallAttackDirection::Overhead;   return true;
	case TEXT('D'): OutDirection = EClanhallAttackDirection::RightSlash; return true;
	case TEXT('A'): OutDirection = EClanhallAttackDirection::LeftSlash;  return true;
	case TEXT('S'): OutDirection = EClanhallAttackDirection::LowSweep;   return true;
	default:        return false;
	}
}

static float GetMontagePlayLength(const UAnimMontage* Montage)
{
	return Montage ? Montage->GetPlayLength() : 0.0f;
}

// Разбор направлений, резолв цепочки CharacterSheet -> Weapon -> Type -> ComboData и печать
// инварианта экономики оружия (`economy_system.md`, «Инвариант экономики оружия»). Своей
// логики поверх систем не пишем — только чтение UWeaponTypeData/UComboData и вывод.
static void HandleShowWeaponEconomy(UWorld* World, const TArray<FString>& Args)
{
	const EDebugMsgKey Key = EDebugMsgKey::PlayerShowWeaponEconomy;

	if (Args.Num() < 1 || Args[0].IsEmpty())
	{
		PrintError(Key, TEXT("Usage: Clanhall.Player.ShowWeaponEconomy <directions, e.g. WDAS>"));
		return;
	}

	AClanhallHumanoidCombatant* Combatant = ResolvePlayerCombatant(World, Key);
	if (!Combatant)
	{
		return;
	}

	// Цепочка неполна -> сказать явно, ГДЕ обрыв, а не просто "нет данных".
	const UCharacterSheetData* Sheet = Combatant->GetCharacterSheet();
	if (!Sheet)
	{
		PrintError(Key, TEXT("Chain broken: no CharacterSheet"));
		return;
	}

	const UWeaponData* Weapon = Sheet->Weapon;
	if (!Weapon)
	{
		PrintError(Key, TEXT("Chain broken: CharacterSheet has no Weapon"));
		return;
	}

	const UWeaponTypeData* WeaponType = Weapon->Type;
	if (!WeaponType)
	{
		PrintError(Key, TEXT("Chain broken: Weapon has no Type"));
		return;
	}

	const UComboData* Combo = WeaponType->ComboData;
	if (!Combo)
	{
		PrintError(Key, TEXT("Chain broken: WeaponType has no ComboData"));
		return;
	}

	// Разбор направлений: неизвестный символ -> ошибка с перечнем допустимых, выход.
	TArray<EClanhallAttackDirection> Requested;
	for (int32 CharIndex = 0; CharIndex < Args[0].Len(); ++CharIndex)
	{
		const TCHAR Ch = Args[0][CharIndex];
		EClanhallAttackDirection Direction;
		if (!TryParseDirection(Ch, Direction))
		{
			PrintError(Key, FString::Printf(TEXT("Unknown direction '%c'. Valid: W, A, S, D"), Ch));
			return;
		}
		Requested.Add(Direction);
	}

	TArray<FString> Lines;

	// Аргумент длиннее SeriesLength -> посчитать по первым SeriesLength шагам, хвост отброшен явно.
	if (Requested.Num() > WeaponType->SeriesLength)
	{
		Lines.Add(FString::Printf(TEXT("Series capped by SeriesLength %d — trailing %d input(s) dropped."),
			WeaponType->SeriesLength, Requested.Num() - WeaponType->SeriesLength));
		Requested.SetNum(WeaponType->SeriesLength);
	}

	// Идём по шагам, резолвя опенер/переходы; nullptr в слоте — законное состояние
	// («этим оружием такая связка невозможна») — останавливаемся и считаем по тому, что успели.
	TArray<EClanhallAttackDirection> Resolved;
	TArray<float> StepDurations;

	UAnimMontage* OpenerMontage = Combo->FindOpenerMontage(Requested[0]);
	if (!OpenerMontage)
	{
		PrintError(Key, FString::Printf(TEXT("Chain breaks at step 1: no opener for %c — nothing to compute."), DirectionToChar(Requested[0])));
		return;
	}
	Resolved.Add(Requested[0]);
	StepDurations.Add(GetMontagePlayLength(OpenerMontage));
	Lines.Add(FString::Printf(TEXT("  %c (opener)  %.2f s"), DirectionToChar(Requested[0]), StepDurations.Last()));

	for (int32 i = 1; i < Requested.Num(); ++i)
	{
		const EClanhallAttackDirection From = Resolved.Last();
		const EClanhallAttackDirection To = Requested[i];
		UAnimMontage* TransitionMontage = Combo->FindTransitionMontage(From, To);
		if (!TransitionMontage)
		{
			Lines.Add(FString::Printf(TEXT("Chain breaks at step %d: no transition %c->%c — computing on the %d step(s) before it."),
				i + 1, DirectionToChar(From), DirectionToChar(To), Resolved.Num()));
			break;
		}
		Resolved.Add(To);
		StepDurations.Add(GetMontagePlayLength(TransitionMontage));
		Lines.Add(FString::Printf(TEXT("  %c->%c        %.2f s"), DirectionToChar(From), DirectionToChar(To), StepDurations.Last()));
	}

	const UAnimMontage* RecoveryMontage = Combo->FindRecoveryMontage(Resolved.Last());
	const float RecoveryDuration = GetMontagePlayLength(RecoveryMontage);
	Lines.Add(FString::Printf(TEXT("  recovery %c  %.2f s"), DirectionToChar(Resolved.Last()), RecoveryDuration));

	float DurationWithoutRecovery = 0.0f;
	for (const float StepDuration : StepDurations)
	{
		DurationWithoutRecovery += StepDuration;
	}
	const float DurationWithRecovery = DurationWithoutRecovery + RecoveryDuration;

	// Доход начиная со ВТОРОГО удара серии (`economy_system.md`, «Заряды: доход») — раз за
	// взмах, шагов - 1 взмахов с доходом.
	const int32 Income = WeaponType->ChargeIncome * FMath::Max(0, Resolved.Num() - 1);
	const float IncomePerSecNoRecovery = DurationWithoutRecovery > 0.0f ? Income / DurationWithoutRecovery : 0.0f;
	const float IncomePerSecWithRecovery = DurationWithRecovery > 0.0f ? Income / DurationWithRecovery : 0.0f;

	FString Message = FString::Printf(TEXT("Weapon: %s   Type: %s\nSeriesLength %d   ChargeIncome %d\n"),
		*Weapon->GetName(), *WeaponType->GetName(), WeaponType->SeriesLength, WeaponType->ChargeIncome);

	FString RequestedSequence;
	for (const EClanhallAttackDirection Direction : Requested)
	{
		RequestedSequence += FString::Printf(TEXT("%c"), DirectionToChar(Direction));
	}
	Message += FString::Printf(TEXT("Sequence %s: %d step(s)\n"), *RequestedSequence, Resolved.Num());

	for (const FString& Line : Lines)
	{
		Message += Line + TEXT("\n");
	}

	Message += FString::Printf(TEXT("Duration: %.2f s without recovery, %.2f s with recovery\n"), DurationWithoutRecovery, DurationWithRecovery);
	Message += FString::Printf(TEXT("Income: ChargeIncome x (steps - 1) = %d x %d = %d charges\n"),
		WeaponType->ChargeIncome, FMath::Max(0, Resolved.Num() - 1), Income);
	Message += FString::Printf(TEXT("Income/sec: %.2f without recovery, %.2f with recovery"), IncomePerSecNoRecovery, IncomePerSecWithRecovery);

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
	TEXT("Apply a mark to the player via UClanhallMarkComponent. Usage: Clanhall.Player.AddMark <TagName>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		HandleAddMark(World, Args, /*bTargetEnemy=*/false);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarEnemyAddMark(
	TEXT("Clanhall.Enemy.AddMark"),
	TEXT("Apply a mark to the current target via UClanhallMarkComponent. Usage: Clanhall.Enemy.AddMark <TagName>"),
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

static FAutoConsoleCommandWithWorldAndArgs CVarPlayerShowWeaponEconomy(
	TEXT("Clanhall.Player.ShowWeaponEconomy"),
	TEXT("Print series duration, charge income and income/sec for a direction sequence on the player's weapon. Usage: Clanhall.Player.ShowWeaponEconomy <directions, e.g. WDAS>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		HandleShowWeaponEconomy(World, Args);
	}));

#endif // !UE_BUILD_SHIPPING
