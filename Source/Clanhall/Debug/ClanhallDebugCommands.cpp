// Реестр отладочных консольных команд. Добавление нового атрибута = одна строка
// в GetDebugAttributeMap(); добавление новой команды = один новый
// FAutoConsoleCommandWithWorldAndArgs ниже. Своей логики поверх существующих
// систем не пишем — только резолв цели/атрибута и вывод результата.

#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallCombatStateComponent.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/ClanhallTargetingComponent.h"
#include "AbilitySystem/ClanhallSkillCooldownComponent.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallHumanoidBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
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
	PlayerShowCombatState,
	EnemyUseSkill,
	PlayerShowCooldowns,
	EnemyShowCooldowns,
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
		TEXT("%s AP %s/%s · HP %s/%s · MP %s/%s"),
		bTargetEnemy ? TEXT("[Enemy]") : TEXT("[Player]"),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetAPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetMaxAPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetHPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetMaxHPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetMPAttribute())),
		*FormatValue(ASC->GetNumericAttribute(UClanhallAttributeSet::GetMaxMPAttribute())));

	PrintResult(Key, Message, FColor::Cyan);
}

static void HandleAddMark(UWorld* World, const TArray<FString>& Args, bool bTargetEnemy)
{
	const EDebugMsgKey Key = bTargetEnemy ? EDebugMsgKey::EnemyAddMark : EDebugMsgKey::PlayerAddMark;

	if (Args.Num() < 1)
	{
		PrintError(Key, bTargetEnemy
			? TEXT("Usage: Clanhall.Enemy.AddMark <TagName> [Physical|Magic]")
			: TEXT("Usage: Clanhall.Player.AddMark <TagName> [Physical|Magic]"));
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

	// Необязательный второй аргумент — трек метки, регистронезависимо, по умолчанию Physical
	// (`MarksSystem.md`, «Концепция»).
	EClanhallMarkTrack Track = EClanhallMarkTrack::Physical;
	if (Args.Num() >= 2)
	{
		if (Args[1].Equals(TEXT("Magic"), ESearchCase::IgnoreCase))
		{
			Track = EClanhallMarkTrack::Magic;
		}
		else if (!Args[1].Equals(TEXT("Physical"), ESearchCase::IgnoreCase))
		{
			PrintError(Key, FString::Printf(TEXT("Unknown mark track '%s' (expected Physical or Magic)"), *Args[1]));
			return;
		}
	}

	UClanhallMarkComponent* MarkComponent = TargetActor->FindComponentByClass<UClanhallMarkComponent>();
	if (!MarkComponent)
	{
		PrintError(Key, TEXT("Actor has no MarkComponent"));
		return;
	}

	MarkComponent->ApplyMark(Track, Tag);
	PrintResult(Key, FString::Printf(TEXT("%s mark %s applied (%s track)"), bTargetEnemy ? TEXT("[Enemy]") : TEXT("[Player]"), *Tag.ToString(),
		Track == EClanhallMarkTrack::Magic ? TEXT("Magic") : TEXT("Physical")));
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

// Манекену нечем драться без BT, а контр, метки и насмешка проверяются на его приёмах.
// Слот - короткое имя (Q, E, R, F, Z, X, C, V), приём - из ряда его выбранного оружия.
static void HandleEnemyUseSkill(UWorld* World, const TArray<FString>& Args)
{
	const EDebugMsgKey Key = EDebugMsgKey::EnemyUseSkill;
	if (Args.Num() < 1)
	{
		PrintError(Key, TEXT("Usage: Clanhall.Enemy.UseSkill <Slot>, e.g. Q"));
		return;
	}

	AClanhallHumanoidBase* Enemy = Cast<AClanhallHumanoidBase>(ResolveTargetActor(World, Key));
	if (!Enemy)
	{
		PrintError(Key, TEXT("Current target is not an AClanhallHumanoidBase"));
		return;
	}

	const FGameplayTag Slot = FGameplayTag::RequestGameplayTag(FName(*(TEXT("Slot.") + Args[0].ToUpper())), /*ErrorIfNotFound*/ false);
	if (!Slot.IsValid())
	{
		PrintError(Key, FString::Printf(TEXT("Unknown slot '%s'"), *Args[0]));
		return;
	}

	const FGameplayAbilitySpecHandle Handle = Enemy->GetActiveSkillHandle(Slot);
	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (!Handle.IsValid() || !ASC)
	{
		PrintError(Key, FString::Printf(TEXT("%s has no skill in %s"), *Enemy->GetName(), *Slot.ToString()));
		return;
	}

	const bool bActivated = ASC->TryActivateAbility(Handle);
	PrintResult(Key, FString::Printf(TEXT("[Enemy] %s: %s"), *Slot.ToString(), bActivated ? TEXT("activated") : TEXT("refused (cooldown, commit or state)")),
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
	TEXT("Apply a mark to the player via UClanhallMarkComponent. Usage: Clanhall.Player.AddMark <TagName> [Physical|Magic]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		HandleAddMark(World, Args, /*bTargetEnemy=*/false);
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarEnemyAddMark(
	TEXT("Clanhall.Enemy.AddMark"),
	TEXT("Apply a mark to the current target via UClanhallMarkComponent. Usage: Clanhall.Enemy.AddMark <TagName> [Physical|Magic]"),
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

#endif // !UE_BUILD_SHIPPING
