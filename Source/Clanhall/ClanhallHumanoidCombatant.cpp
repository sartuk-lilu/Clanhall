#include "ClanhallHumanoidCombatant.h"
#include "Clanhall.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ClanhallComboComponent.h"
#include "AbilitySystem/ClanhallParryComponent.h"
#include "AbilitySystem/ClassKitData.h"
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
	// combo_system.md: ворота ввода + владелец активации WASD-ударов.
	ComboComponent = CreateDefaultSubobject<UClanhallComboComponent>(TEXT("ComboComponent"));
	ParryComponent = CreateDefaultSubobject<UClanhallParryComponent>(TEXT("ParryComponent"));

	// WASD-классы дефолтно равны C++ классам — общий конструктор для игрока и
	// AClanhallHumanoidBoss (main_dev_plan.md §8, Блок A2): раньше жили в конструкторе
	// AClanhallCharacter, из-за чего у пустого конструктора Boss они оставались nullptr, и
	// GiveAbility грантовал WASD-удары с null-классом — серии у босса не было вообще. Не
	// UPROPERTY намеренно — значение одинаково у всех китов, это плумбинг GAS, не контент класса.
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

	// Грант способностей 4 направлений WASD-удара (combat_system.md §3-4). Классы дефолтно
	// заполнены соответствующим C++ GA (см. конструктор), BP-наследник может переопределить.
	AttackOverheadHandle   = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackOverheadClass,   1, INDEX_NONE, this));
	AttackRightSlashHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackRightSlashClass, 1, INDEX_NONE, this));
	AttackLeftSlashHandle  = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackLeftSlashClass,  1, INDEX_NONE, this));
	AttackLowSweepHandle   = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackLowSweepClass,   1, INDEX_NONE, this));

	// main_dev_plan.md §8, Блок A2: один класс GA_PhysicalSkill гранится по числу записей в
	// ClassKit->Skills, каждый раз с UAbilityData как SourceObject — ключ карты (Cooldown.Slot.*)
	// сохраняется как адрес хэндла для GetActiveSkillHandle(). Тот же цикл обслуживает и игрока,
	// и AClanhallHumanoidBoss (§8, Блок C) — DataAsset'ы назначаются в Blueprint-наследнике.
	if (ClassKit)
	{
		for (const TPair<FGameplayTag, TObjectPtr<UAbilityData>>& Skill : ClassKit->Skills)
		{
			// Невалидный ключ ИЛИ сам корень Cooldown.Slot (а не лист Q/E/R/F/...) грантится, но
			// GetActiveSkillHandle(Cooldown_Slot_Q) его никогда не найдёт — ключ карты другой.
			// Корень стал выбираемым значением поля, как только завели native-тег Cooldown.Slot
			// под фильтр GetCooldownSlotTag (ревью Опуса, Блок A2): meta=(Categories="Cooldown.Slot")
			// пропускает и его самого, не только листья. Симптом без этой проверки — "Q не
			// нажимается", причина не ищется.
			if (!Skill.Key.IsValid() || Skill.Key == ClanhallGameplayTags::Cooldown_Slot.GetTag())
			{
				UE_LOG(LogClanhall, Warning, TEXT("%s: запись в ClassKit->Skills с невалидным ключом или корнем Cooldown.Slot вместо листа (Q/E/R/F/...) — навык не будет вызываем."), *GetName());
				continue;
			}

			if (!Skill.Value)
			{
				UE_LOG(LogClanhall, Warning, TEXT("%s: слот %s в ClassKit->Skills не заполнен — навык не грантится."), *GetName(), *Skill.Key.ToString());
				continue;
			}

			// Слот доносится до способности штатным путём GAS — динамическим тегом спека
			// (не полем в UAbilityData, main_dev_plan.md §8, Блок A2): один и тот же
			// UAbilityData может лежать сразу в двух китах, слот же принадлежит гранту.
			FGameplayAbilitySpec Spec(UGA_PhysicalSkill::StaticClass(), 1, INDEX_NONE, Skill.Value);
			Spec.GetDynamicSpecSourceTags().AddTag(Skill.Key);
			ActiveSkillHandles.Add(Skill.Key, AbilitySystemComponent->GiveAbility(Spec));
		}
	}
}

FGameplayTag AClanhallHumanoidCombatant::GetClassTag() const
{
	return ClassKit ? ClassKit->ClassTag : FGameplayTag();
}

const UComboData* AClanhallHumanoidCombatant::GetComboData() const
{
	return ClassKit ? ClassKit->ComboData : nullptr;
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

FGameplayAbilitySpecHandle AClanhallHumanoidCombatant::GetActiveSkillHandle(FGameplayTag CooldownSlotTag) const
{
	return ActiveSkillHandles.FindRef(CooldownSlotTag);
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
	if (!ClassKit || !RequiredMark.IsValid())
	{
		return false;
	}

	for (const TPair<FGameplayTag, TObjectPtr<UAbilityData>>& Skill : ClassKit->Skills)
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
