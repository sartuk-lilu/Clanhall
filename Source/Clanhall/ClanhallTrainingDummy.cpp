#include "ClanhallTrainingDummy.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/ClanhallCounterComponent.h"
#include "AbilitySystem/Abilities/GA_EnemyWASDSeries.h"
#include "AbilitySystem/Abilities/GA_EnemyActiveSkill.h"
#include "AbilitySystem/ClanhallGameplayTags.h"

AClanhallTrainingDummy::AClanhallTrainingDummy()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UClanhallAttributeSet>(TEXT("AttributeSet"));
	MarkComponent = CreateDefaultSubobject<UClanhallMarkComponent>(TEXT("MarkComponent"));
	CounterComponent = CreateDefaultSubobject<UClanhallCounterComponent>(TEXT("CounterComponent"));
}

UAbilitySystemComponent* AClanhallTrainingDummy::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AClanhallTrainingDummy::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// changelog_enemyframe_unitroles.md §1-2: Часовой — edge-case (не моб, не учит игрока),
		// провизорно Unit.Role.Boss.Humanoid, чтобы сохранить рамку боссфайта для теста.
		AbilitySystemComponent->AddLooseGameplayTag(ClanhallGameplayTags::Unit_Role_Boss_Humanoid);

		// Раздел 5: грантируем «Перекрёстный» A→D и запускаем цикл каждые 4 сек.
		SeriesHandle = AbilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(UGA_Series_Crosscut::StaticClass(), 1, INDEX_NONE, this));

		GetWorldTimerManager().SetTimer(AttackLoopTimer, this, &AClanhallTrainingDummy::TryStartSeries, 4.0f, true, 2.0f);

		// Раздел 6: Power Strike — активный навык для теста контрнавыка.
		// Запускается каждые 7 сек со стартовой задержкой 5 сек (чтобы не пересекаться с первой серией).
		PowerStrikeHandle = AbilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(UGA_Enemy_PowerStrike::StaticClass(), 1, INDEX_NONE, this));

		GetWorldTimerManager().SetTimer(PowerStrikeLoopTimer, this, &AClanhallTrainingDummy::TryStartPowerStrike, 7.0f, true, 5.0f);
	}

	if (AttributeSet)
	{
		// Часовой (enemy_prototypes.md): AP 150/150, HP 300/300.
		AttributeSet->InitMaxAP(150.0f);
		AttributeSet->InitAP(150.0f);
		AttributeSet->InitMaxHP(300.0f);
		AttributeSet->InitHP(300.0f);
	}
}

void AClanhallTrainingDummy::TryStartSeries()
{
	// GAS не запустит способность если она уже активна или если блокирующие теги (Stunned/ComboRecovery) висят.
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(SeriesHandle);
	}
}

void AClanhallTrainingDummy::TryStartPowerStrike()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbility(PowerStrikeHandle);
	}
}
