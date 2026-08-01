#include "ClanhallTrainingDummy.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/ClanhallCounterComponent.h"
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

		// hud_dev_plan.md: Часовой — edge-case (не моб, не учит игрока),
		// провизорно Unit.Role.Boss.Humanoid, чтобы сохранить рамку боссфайта для теста.
		AbilitySystemComponent->AddLooseGameplayTag(ClanhallGameplayTags::Unit_Role_Boss_Humanoid);
	}
}
