#include "ClanhallHumanoidCombatant.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ClanhallComboComponent.h"
#include "AbilitySystem/ClanhallParryComponent.h"
#include "AbilitySystem/Abilities/GA_PhysicalSkill.h"

AClanhallHumanoidCombatant::AClanhallHumanoidCombatant()
{
	// combo_system.md: ворота ввода + владелец активации WASD-ударов.
	ComboComponent = CreateDefaultSubobject<UClanhallComboComponent>(TEXT("ComboComponent"));
	ParryComponent = CreateDefaultSubobject<UClanhallParryComponent>(TEXT("ParryComponent"));
}

void AClanhallHumanoidCombatant::BeginPlay()
{
	Super::BeginPlay();

	if (!AbilitySystemComponent)
	{
		return;
	}

	// Грант способностей 4 направлений WASD-удара (combat_system.md §3-4). Классы дефолтно
	// заполнены соответствующим C++ GA (см. AClanhallCharacter), BP-наследник может переопределить.
	AttackOverheadHandle   = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackOverheadClass,   1, INDEX_NONE, this));
	AttackRightSlashHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackRightSlashClass, 1, INDEX_NONE, this));
	AttackLeftSlashHandle  = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackLeftSlashClass,  1, INDEX_NONE, this));
	AttackLowSweepHandle   = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackLowSweepClass,   1, INDEX_NONE, this));

	// Раздел 4: кит Knight — один класс GA_PhysicalSkill гранится 4 раза, каждый раз с разным
	// UAbilityData как SourceObject (main_dev_plan.md). Тот же путь обслужит и AClanhallHumanoidBoss
	// (§8, Блок C) — DataAsset'ы назначаются в Blueprint-наследнике, C++ не дублируется.
	if (KnightSkillQ_ShieldSlam)
	{
		ActiveSkillQHandle = AbilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(UGA_PhysicalSkill::StaticClass(), 1, INDEX_NONE, KnightSkillQ_ShieldSlam));
	}
	if (KnightSkillE_PowerStrike)
	{
		ActiveSkillEHandle = AbilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(UGA_PhysicalSkill::StaticClass(), 1, INDEX_NONE, KnightSkillE_PowerStrike));
	}
	if (KnightSkillR_ShieldCharge)
	{
		ActiveSkillRHandle = AbilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(UGA_PhysicalSkill::StaticClass(), 1, INDEX_NONE, KnightSkillR_ShieldCharge));
	}
	if (KnightSkillF_Retribution)
	{
		ActiveSkillFHandle = AbilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(UGA_PhysicalSkill::StaticClass(), 1, INDEX_NONE, KnightSkillF_Retribution));
	}
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
