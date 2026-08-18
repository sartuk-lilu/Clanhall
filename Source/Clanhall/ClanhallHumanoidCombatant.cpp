#include "ClanhallHumanoidCombatant.h"
#include "Clanhall.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ClanhallComboComponent.h"
#include "AbilitySystem/ClanhallParryComponent.h"
#include "AbilitySystem/CharacterSheetData.h"
#include "AbilitySystem/WeaponData.h"
#include "AbilitySystem/WeaponTypeData.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/Fragments/ComboData.h"
#include "AbilitySystem/Fragments/GameplayFragments.h"
#include "AbilitySystem/ClanhallMarkTypes.h"
#include "AbilitySystem/AbilityData.h"
#include "AbilitySystem/Abilities/GA_PhysicalSkill.h"
#include "AbilitySystem/Abilities/GA_DirectionalAttacks.h"
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
	if (!CharacterSheet || !CharacterSheet->Weapon || !CharacterSheet->Weapon->Type)
	{
		UE_LOG(LogClanhall, Warning, TEXT("%s: цепочка CharacterSheet -> Weapon -> Type неполна — "
			"бой идёт на фолбэках (ChargeIncome %d, SeriesLength %d), комбо-дерева нет."),
			*GetName(), ClanhallWeaponDefaults::ChargeIncome, ClanhallWeaponDefaults::SeriesLength);
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
			// Невалидный ключ ИЛИ сам корень Ability.Slot (а не лист Q/E/R/F/...) грантится, но
			// GetActiveSkillHandle(Ability_Slot_Q) его никогда не найдёт — ключ карты другой.
			// Корень стал выбираемым значением поля, как только завели native-тег Ability.Slot
			// под фильтр GetAbilitySlotTag: meta=(Categories="Ability.Slot") пропускает и его
			// самого, не только листья. Симптом без этой проверки — "Q не нажимается", причина
			// не ищется.
			if (!Skill.Key.IsValid() || Skill.Key == ClanhallGameplayTags::Ability_Slot.GetTag())
			{
				UE_LOG(LogClanhall, Warning, TEXT("%s: запись в WeaponType->Skills с невалидным ключом или корнем Ability.Slot вместо листа (Q/E/R/F/...) — навык не будет вызываем."), *GetName());
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

const UWeaponTypeData* AClanhallHumanoidCombatant::GetWeaponType() const
{
	return CharacterSheet && CharacterSheet->Weapon ? CharacterSheet->Weapon->Type : nullptr;
}

const UComboData* AClanhallHumanoidCombatant::GetComboData() const
{
	const UWeaponTypeData* WeaponType = GetWeaponType();
	return WeaponType ? WeaponType->ComboData : nullptr;
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
