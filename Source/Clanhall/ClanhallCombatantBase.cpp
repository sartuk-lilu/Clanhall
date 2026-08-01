#include "ClanhallCombatantBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "AbilitySystem/ClanhallCounterComponent.h"

AClanhallCombatantBase::AClanhallCombatantBase()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UClanhallAttributeSet>(TEXT("AttributeSet"));

	// Метка бойца — независимый трек (mark_system.md §3/§5), двусторонняя по построению.
	MarkComponent = CreateDefaultSubobject<UClanhallMarkComponent>(TEXT("MarkComponent"));

	// main_dev_plan.md §7: диспетчер зон поражения вместо жёстко зашитого weapon trace.
	// Имя сабобъекта сохранено символ в символ при переносе из AClanhallCharacter (Блок A) —
	// смена строки рвёт переопределения в BP-наследнике игрока. Класс переименован, имя — нет.
	HitboxComponent = CreateDefaultSubobject<UClanhallHitboxComponent>(TEXT("WeaponTraceComponent"));

	// Раздел 6 (переработан): симметричный компонент окна контрнавыка, тот же класс на враге.
	CounterComponent = CreateDefaultSubobject<UClanhallCounterComponent>(TEXT("CounterComponent"));
}

UAbilitySystemComponent* AClanhallCombatantBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AClanhallCombatantBase::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		// OwnerActor == AvatarActor == this: ASC не на PlayerState, ни у игрока, ни у врага.
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		if (RoleTag.IsValid())
		{
			AbilitySystemComponent->AddLooseGameplayTag(RoleTag);
		}
	}
}
