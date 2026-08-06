#include "ClanhallCombatantBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "AbilitySystem/ClanhallCounterComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"

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

	if (AttributeSet)
	{
		// Хардкод стартовых значений — плейсхолдеры прототипа (combat_system.md §1); DataAsset/
		// GameplayEffect для инициализации атрибутов появится позже вместе с остальной системой
		// данных. Здесь, а не в AClanhallCharacter — нужно ЛЮБОМУ бойцу, включая AI без
		// собственного BeginPlay-пути (task_section8_blocks_fgh.md, блокер ревью §0.3).
		AttributeSet->InitMaxAP(DefaultMaxAP);
		AttributeSet->InitAP(DefaultMaxAP);
		AttributeSet->InitMaxHP(DefaultMaxHP);
		AttributeSet->InitHP(DefaultMaxHP);
		AttributeSet->InitMaxMP(DefaultMaxMP);
		AttributeSet->InitMP(DefaultMaxMP);
		AttributeSet->InitMaxCharges(DefaultMaxCharges);
		AttributeSet->InitCharges(DefaultMaxCharges);
		AttributeSet->InitBalance(0.0f);
		AttributeSet->InitMaxStagger(DefaultMaxStagger);
		AttributeSet->InitStagger(0.0f);
	}

	if (AbilitySystemComponent)
	{
		// OwnerActor == AvatarActor == this: ASC не на PlayerState, ни у игрока, ни у врага.
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		if (RoleTag.IsValid())
		{
			AbilitySystemComponent->AddLooseGameplayTag(RoleTag);
		}

		// Denied-фидбек (main_dev_plan.md): AbilityFailedCallbacks — обычный C++ мультикаст, не
		// BlueprintAssignable, поэтому ретранслируем в OnChargesDenied для Blueprint HUD.
		AbilitySystemComponent->AbilityFailedCallbacks.AddUObject(this, &AClanhallCombatantBase::HandleAbilityFailed);
	}
}

void AClanhallCombatantBase::HandleAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	if (FailureReason.HasTagExact(ClanhallGameplayTags::Ability_Denied_Charges.GetTag()))
	{
		OnChargesDenied.Broadcast();
	}
}
