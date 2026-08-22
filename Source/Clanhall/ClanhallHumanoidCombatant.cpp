#include "ClanhallHumanoidCombatant.h"
#include "Clanhall.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ClanhallComboComponent.h"
#include "AbilitySystem/ClanhallParryComponent.h"
#include "AbilitySystem/CharacterSheetData.h"
#include "AbilitySystem/WeaponData.h"
#include "AbilitySystem/WeaponTypeData.h"
#include "AbilitySystem/Fragments/WeaponFragments.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/Fragments/ComboData.h"
#include "AbilitySystem/Fragments/GameplayFragments.h"
#include "AbilitySystem/ClanhallMarkTypes.h"
#include "AbilitySystem/AbilityData.h"
#include "AbilitySystem/Abilities/GA_PhysicalSkill.h"
#include "AbilitySystem/Abilities/GA_DirectionalAttacks.h"
#include "ClanhallWeaponActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AClanhallHumanoidCombatant::AClanhallHumanoidCombatant()
{
	// (`Combat Stance and WASD Attacks.md`): ворота ввода + владелец активации WASD-ударов.
	ComboComponent = CreateDefaultSubobject<UClanhallComboComponent>(TEXT("ComboComponent"));
	ParryComponent = CreateDefaultSubobject<UClanhallParryComponent>(TEXT("ParryComponent"));

	// WASD-классы дефолтно равны C++ классам — общий конструктор для игрока и
	// AClanhallHumanoidBoss: раньше жили в конструкторе
	// AClanhallCharacter, из-за чего у пустого конструктора Boss они оставались nullptr, и
	// GiveAbility грантовал WASD-удары с null-классом — серии у босса не было вообще. Не
	// UPROPERTY намеренно — значение одинаково у всех китов, это плумбинг GAS, не контент класса
	// (`Combatant Hierarchy.md`, «Грант в BeginPlay»).
	AttackOverheadClass   = UGA_DirectionalAttack_Overhead::StaticClass();
	AttackRightSlashClass = UGA_DirectionalAttack_RightSlash::StaticClass();
	AttackLeftSlashClass  = UGA_DirectionalAttack_LeftSlash::StaticClass();
	AttackLowSweepClass   = UGA_DirectionalAttack_LowSweep::StaticClass();
}

void AClanhallHumanoidCombatant::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// PostInitializeComponents отрабатывает у ВСЕХ акторов уровня до того, как хоть у одного
	// стартует BeginPlay — в отличие от BeginPlay, где UClanhallParryComponent::BeginPlay уже
	// читает GetWeaponType() через HasOpponentWithMarkSynergy раньше, чем выполнилось бы тело
	// BeginPlay этого актора. Пустой Loadout — законное состояние, CurrentWeapon остаётся null,
	// фолбэки в BeginPlay ниже отрабатывают как и раньше.
	CurrentWeapon = (CharacterSheet && CharacterSheet->Loadout.IsValidIndex(0))
		? CharacterSheet->Loadout[0] : nullptr;

	// Множитель скорости оружия применяется при экипировке ко всем трём базовым скоростям
	// разом (`weapon_system.md`; `stage4_rev2_handoff.md`) - не в момент входа в стойку, тяжёлое
	// оружие медленное всегда. Считаем от базового JogSpeed бойца, не от текущего MaxWalkSpeed:
	// повторный вызов при будущем свапе оружия иначе умножил бы второй раз.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = GetJogSpeed() * GetWeaponSpeedMultiplier();
	}
}

void AClanhallHumanoidCombatant::BeginPlay()
{
	Super::BeginPlay();

	if (!AbilitySystemComponent)
	{
		return;
	}

	// Отсутствие данных не должно ломать бой (`weapon_system.md`, «Шаблон и живой лист») — боец
	// без листа дерётся на фолбэках, а не встаёт с нулевым доходом. Один варнинг на бойца,
	// здесь и только здесь: варнить в местах чтения означало бы залить лог на каждом ударе.
	if (!CurrentWeapon || !CurrentWeapon->Type)
	{
		UE_LOG(LogClanhall, Warning, TEXT("%s: цепочка CurrentWeapon -> Type неполна — "
			"бой идёт на фолбэках (ChargeIncome %d, SeriesLength %d), комбо-дерева нет."),
			*GetName(), ClanhallWeaponDefaults::ChargeIncome, ClanhallWeaponDefaults::SeriesLength);
	}

	// Спавн и крепление визуала текущего оружия и оффхенда, если он есть у экземпляра
	// (`weapon_system.md`, «Оружие как актор»). Свапа в этом задании нет — акторы ставятся
	// один раз здесь.
	if (CurrentWeapon)
	{
		SpawnedWeapon = SpawnAndAttachWeapon(CurrentWeapon->WeaponClass);

		if (const UWeaponOffhandFragment* Offhand = CurrentWeapon->FindFragment<UWeaponOffhandFragment>())
		{
			SpawnedOffhand = SpawnAndAttachWeapon(Offhand->OffhandClass);
		}
	}

	// Грант способностей 4 направлений WASD-удара (`combat_system.md`, «Боевая стойка и переключение режимов», «Направления атаки (WASD)»). Классы дефолтно
	// заполнены соответствующим C++ GA (см. конструктор), BP-наследник может переопределить.
	AttackOverheadHandle   = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackOverheadClass,   1, INDEX_NONE, this));
	AttackRightSlashHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackRightSlashClass, 1, INDEX_NONE, this));
	AttackLeftSlashHandle  = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackLeftSlashClass,  1, INDEX_NONE, this));
	AttackLowSweepHandle   = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackLowSweepClass,   1, INDEX_NONE, this));

	// Один класс GA_PhysicalSkill гранится по числу записей в
	// GetWeaponType()->Skills (`Combatant Hierarchy.md`, «Грант в BeginPlay»; `weapon_system.md`,
	// «Владение оружием») — набор активок принадлежит оружию, не листу. Каждая запись проходит
	// два гейта владения: открыт ли тир слота рангом (CharacterSheet->Perks) и выучен ли сам
	// навык (CharacterSheet->LearnedSkills). Тот же цикл обслуживает и игрока,
	// и AClanhallHumanoidBoss — DataAsset'ы назначаются в Blueprint-наследнике.
	const UWeaponTypeData* WeaponType = GetWeaponType();
	if (WeaponType)
	{
		int32 NumGranted = 0;
		int32 NumRejectedByRank = 0;
		int32 NumRejectedByLearned = 0;

		for (const TPair<FGameplayTag, TObjectPtr<UAbilityData>>& Skill : WeaponType->Skills)
		{
			// Невалидный ключ ИЛИ сам корень Slot (а не лист Q/E/R/F/...) грантится, но
			// GetActiveSkillHandle(Slot_Q) его никогда не найдёт — ключ карты другой.
			// Корень стал выбираемым значением поля, как только завели native-тег Slot
			// под фильтр GetAbilitySlotTag: meta=(Categories="Slot") пропускает и его
			// самого, не только листья. Симптом без этой проверки — "Q не нажимается", причина
			// не ищется.
			if (!Skill.Key.IsValid() || Skill.Key == ClanhallGameplayTags::Slot.GetTag())
			{
				UE_LOG(LogClanhall, Warning, TEXT("%s: запись в WeaponType->Skills с невалидным ключом или корнем Slot вместо листа (Q/E/R/F/...) — навык не будет вызываем."), *GetName());
				continue;
			}

			if (!Skill.Value)
			{
				UE_LOG(LogClanhall, Warning, TEXT("%s: слот %s в WeaponType->Skills не заполнен — навык не грантится."), *GetName(), *Skill.Key.ToString());
				continue;
			}

			if (UWeaponTypeData::GetRequiredProficiencyRank(Skill.Key) == 0)
			{
				UE_LOG(LogClanhall, Warning, TEXT("%s: слот %s не опознан GetRequiredProficiencyRank — навык не грантится."), *GetName(), *Skill.Key.ToString());
				continue;
			}

			// Тир закрыт — штатное состояние владения, не ошибка данных
			// (`weapon_system.md`, «Владение оружием»): боец без ранга новым оружием бьёт
			// и паррирует, но тратить заряды не на что. Verbose, не Warning — иначе лог
			// заливается на каждом бойце без прокачки.
			if (!WeaponType->IsSlotUnlocked(Skill.Key, CharacterSheet ? CharacterSheet->Perks : FGameplayTagContainer::EmptyContainer))
			{
				UE_LOG(LogClanhall, Verbose, TEXT("%s: слот %s закрыт рангом владения — навык не грантится."), *GetName(), *Skill.Key.ToString());
				++NumRejectedByRank;
				continue;
			}

			if (!Skill.Value->CounterTag.IsValid())
			{
				UE_LOG(LogClanhall, Warning, TEXT("%s: навык в слоте %s без CounterTag — выучить его нечем, LearnedSkills не может на него сослаться."), *GetName(), *Skill.Key.ToString());
				continue;
			}

			// Не выучен — тоже штатное состояние владения, Verbose по той же причине, что и
			// закрытый тир выше.
			if (!CharacterSheet || !CharacterSheet->LearnedSkills.HasTag(Skill.Value->CounterTag))
			{
				UE_LOG(LogClanhall, Verbose, TEXT("%s: навык в слоте %s не выучен (LearnedSkills) — не грантится."), *GetName(), *Skill.Key.ToString());
				++NumRejectedByLearned;
				continue;
			}

			// Слот доносится до способности штатным путём GAS — динамическим тегом спека
			// (не полем в UAbilityData, `Combatant Hierarchy.md`, «Ключ по слоту, а не по имени навыка»): один и тот же
			// UAbilityData может лежать сразу в двух китах, слот же принадлежит гранту.
			FGameplayAbilitySpec Spec(UGA_PhysicalSkill::StaticClass(), 1, INDEX_NONE, Skill.Value);
			Spec.GetDynamicSpecSourceTags().AddTag(Skill.Key);
			ActiveSkillHandles.Add(Skill.Key, AbilitySystemComponent->GiveAbility(Spec));
			++NumGranted;
		}

		// Итоговая строка, чтобы гейт не отлаживался вслепую: игрок жмёт Q, ничего не
		// происходит, и без этой строки причина (ранг или изученность) не ищется.
		UE_LOG(LogClanhall, Log, TEXT("%s: активок гранто %d, отсеяно рангом %d, отсеяно изученностью %d."),
			*GetName(), NumGranted, NumRejectedByRank, NumRejectedByLearned);
	}
}

void AClanhallHumanoidCombatant::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Без явного Destroy оружие переживает своего носителя — PIE, запущенный дважды,
	// оставляет мечи на уровне.
	if (SpawnedWeapon)
	{
		SpawnedWeapon->Destroy();
		SpawnedWeapon = nullptr;
	}
	if (SpawnedOffhand)
	{
		SpawnedOffhand->Destroy();
		SpawnedOffhand = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

AClanhallWeaponActor* AClanhallHumanoidCombatant::SpawnAndAttachWeapon(TSubclassOf<AClanhallWeaponActor> WeaponClass)
{
	if (!WeaponClass)
	{
		// Законное состояние, не ошибка — оружие без визуала тестируется (`weapon_system.md`,
		// «Оружие как актор»). Verbose, не Warning, но не молчит: иначе "меча нет" ищется
		// глазами так же долго, как и "меч торчит из живота".
		UE_LOG(LogClanhall, Verbose, TEXT("%s: WeaponClass не задан — оружие без визуала."), *GetName());
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	AClanhallWeaponActor* SpawnedActor = GetWorld()->SpawnActor<AClanhallWeaponActor>(WeaponClass, SpawnParams);
	if (!SpawnedActor)
	{
		// Реальный случай, не теоретический — WeaponClass указывает сам AClanhallWeaponActor:
		// он Abstract, но пикер TSubclassOf абстрактные классы всё равно показывает. Симптом
		// без этой строки — оружия нет, лог чист.
		UE_LOG(LogClanhall, Warning, TEXT("%s: SpawnActor(%s) вернул nullptr — оружия не будет."),
			*GetName(), *WeaponClass->GetName());
		return nullptr;
	}

	USkeletalMeshComponent* OwnerMesh = GetMesh();
	if (!OwnerMesh)
	{
		UE_LOG(LogClanhall, Warning, TEXT("%s: GetMesh() вернул nullptr — крепить %s некуда, оружие остаётся в мире рут-трансформом."),
			*GetName(), *SpawnedActor->GetClass()->GetName());
		return SpawnedActor;
	}

	const bool bSocketNamed = !SpawnedActor->AttachSocketName.IsNone();
	const bool bSocketExists = bSocketNamed && OwnerMesh->DoesSocketExist(SpawnedActor->AttachSocketName);

	if (!bSocketNamed)
	{
		// Молча ронять оружие в центр персонажа нельзя: симптом «меч торчит из живота»
		// ищется глазами полчаса.
		UE_LOG(LogClanhall, Warning, TEXT("%s: у %s не задан AttachSocketName — крепление в корень меша."),
			*GetName(), *SpawnedActor->GetClass()->GetName());
	}
	else if (!bSocketExists)
	{
		UE_LOG(LogClanhall, Warning, TEXT("%s: сокет %s (заданный в %s) не найден на скелете — крепление в корень меша."),
			*GetName(), *SpawnedActor->AttachSocketName.ToString(), *SpawnedActor->GetClass()->GetName());
	}

	const FName SocketToUse = bSocketExists ? SpawnedActor->AttachSocketName : NAME_None;
	const FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, false);
	SpawnedActor->AttachToComponent(OwnerMesh, AttachRules, SocketToUse);
	SpawnedActor->SetActorRelativeTransform(SpawnedActor->AttachRelativeTransform);

	return SpawnedActor;
}

const UWeaponTypeData* AClanhallHumanoidCombatant::GetWeaponType() const
{
	return CurrentWeapon ? CurrentWeapon->Type : nullptr;
}

const UComboData* AClanhallHumanoidCombatant::GetComboData() const
{
	const UWeaponTypeData* WeaponType = GetWeaponType();
	return WeaponType ? WeaponType->ComboData : nullptr;
}

float AClanhallHumanoidCombatant::GetWeaponSpeedMultiplier() const
{
	const UWeaponTypeData* WeaponType = GetWeaponType();
	return WeaponType ? WeaponType->WeaponSpeedMultiplier : ClanhallWeaponDefaults::WeaponSpeedMultiplier;
}

FGameplayAbilitySpecHandle AClanhallHumanoidCombatant::GetAttackHandle(EClanhallAttackDirection Direction) const
{
	switch (Direction)
	{
	case EClanhallAttackDirection::Overhead:   return AttackOverheadHandle;
	case EClanhallAttackDirection::RightSlash: return AttackRightSlashHandle;
	case EClanhallAttackDirection::LeftSlash:  return AttackLeftSlashHandle;
	case EClanhallAttackDirection::LowSweep:   return AttackLowSweepHandle;
	default:                                   return FGameplayAbilitySpecHandle();
	}
}

FGameplayAbilitySpecHandle AClanhallHumanoidCombatant::GetActiveSkillHandle(FGameplayTag AbilitySlotTag) const
{
	return ActiveSkillHandles.FindRef(AbilitySlotTag);
}

bool AClanhallHumanoidCombatant::HasOpponentWithMarkSynergy(FGameplayTag RequiredMark) const
{
	const AClanhallHumanoidCombatant* Opponent = FindPrototypeOpponent();
	return Opponent && Opponent->HasAbilityWithMarkSynergy(RequiredMark);
}

AClanhallHumanoidCombatant* AClanhallHumanoidCombatant::FindPrototypeOpponent() const
{
	for (TActorIterator<AClanhallHumanoidCombatant> It(GetWorld()); It; ++It)
	{
		if (*It != this)
		{
			return *It;
		}
	}
	return nullptr;
}

bool AClanhallHumanoidCombatant::HasAbilityWithMarkSynergy(FGameplayTag RequiredMark) const
{
	// Гейтами владения намеренно не фильтруется (`weapon_system.md`, «Владение оружием»):
	// вопрос «есть ли чем обналичить Staggered» решает, копится ли шкала усталости у
	// ПРОТИВНИКА этого бойца вообще (см. HasOpponentWithMarkSynergy) — фильтровать её рангом
	// значило бы завязать чужую шкалу на прогрессию, что нигде не решено.
	const UWeaponTypeData* WeaponType = GetWeaponType();
	if (!WeaponType || !RequiredMark.IsValid())
	{
		return false;
	}

	for (const TPair<FGameplayTag, TObjectPtr<UAbilityData>>& Skill : WeaponType->Skills)
	{
		const UAbilityData* Data = Skill.Value;
		const UMarkTriggerFragment* Trigger = Data ? Data->FindFragment<UMarkTriggerFragment>() : nullptr;
		if (!Trigger)
		{
			continue;
		}

		for (const FMarkSynergy& Synergy : Trigger->Synergies)
		{
			// Тот же приём, что в GA_PhysicalSkill::ResolveMarkLogic: MatchesTag, а не == —
			// корневой RequiredMark (родовой "Mark") матчит любую конкретную метку, в т.ч. Staggered.
			if (Synergy.RequiredMark.IsValid() && RequiredMark.MatchesTag(Synergy.RequiredMark))
			{
				return true;
			}
		}
	}

	return false;
}
