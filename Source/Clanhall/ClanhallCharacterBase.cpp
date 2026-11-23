#include "ClanhallCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "AbilitySystem/ClanhallCounterComponent.h"
#include "AbilitySystem/ClanhallCombatStateComponent.h"
#include "AbilitySystem/ClanhallSkillCooldownComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"

AClanhallCharacterBase::AClanhallCharacterBase()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UClanhallAttributeSet>(TEXT("AttributeSet"));

	// Метка бойца — независимый трек (`MarksSystem.md`, «Получение метки от врага»), двусторонняя по построению.
	MarkComponent = CreateDefaultSubobject<UClanhallMarkComponent>(TEXT("MarkComponent"));

	// Диспетчер зон поражения вместо жёстко зашитого weapon trace (`AnimationSetup.md`, «Диспетчер зон поражения»).
	// Имя сабобъекта сохранено символ в символ при переносе из AClanhallCharacter —
	// смена строки рвёт переопределения в BP-наследнике игрока. Класс переименован, имя — нет.
	HitboxComponent = CreateDefaultSubobject<UClanhallHitboxComponent>(TEXT("WeaponTraceComponent"));

	// Симметричный компонент окна контрнавыка, тот же класс на враге.
	CounterComponent = CreateDefaultSubobject<UClanhallCounterComponent>(TEXT("CounterComponent"));

	// State.InCombat — стороне-нейтрален (`CombatSystem.md`, «Боевое состояние»).
	CombatStateComponent = CreateDefaultSubobject<UClanhallCombatStateComponent>(TEXT("CombatStateComponent"));

	SkillCooldownComponent = CreateDefaultSubobject<UClanhallSkillCooldownComponent>(TEXT("SkillCooldownComponent"));

	// Без этого Crouch() молча ничего не делает (`CombatSystem.md`) - UGA_Duck опускает
	// капсулу движковым Crouch(), не SetCapsuleSize.
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
}

UAbilitySystemComponent* AClanhallCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AClanhallCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (AttributeSet)
	{
		// Хардкод стартовых значений — плейсхолдеры прототипа (`CombatSystem.md`, «Ресурсы персонажа»);
		// DataAsset/GameplayEffect для инициализации атрибутов появится позже вместе с остальной
		// системой данных. Здесь, а не в AClanhallCharacter — нужно ЛЮБОМУ бойцу, включая AI без
		// собственного BeginPlay-пути.
		AttributeSet->InitMaxAP(DefaultMaxAP);
		AttributeSet->InitAP(DefaultMaxAP);
		AttributeSet->InitMaxHP(DefaultMaxHP);
		AttributeSet->InitHP(DefaultMaxHP);
		AttributeSet->InitMaxMP(DefaultMaxMP);
		AttributeSet->InitMP(DefaultMaxMP);

		// Модификаторы боя - нейтральная база, меняют их метки.
		AttributeSet->InitCritChance(0.0f);
		AttributeSet->InitThresholdReduction(0.0f);
		AttributeSet->InitDamageDealtMultiplier(1.0f);
		AttributeSet->InitDamageTakenMultiplier(1.0f);
		AttributeSet->InitMoveSpeedMultiplier(1.0f);
	}

	if (AbilitySystemComponent)
	{
		// OwnerActor == AvatarActor == this: ASC не на PlayerState, ни у игрока, ни у врага.
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		if (RoleTag.IsValid())
		{
			AbilitySystemComponent->AddLooseGameplayTag(RoleTag);
		}

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UClanhallAttributeSet::GetMoveSpeedMultiplierAttribute())
			.AddUObject(this, &AClanhallCharacterBase::OnMoveSpeedMultiplierChanged);
	}
}

void AClanhallCharacterBase::OnMoveSpeedMultiplierChanged(const FOnAttributeChangeData& Data)
{
	RefreshMovementSpeed();
}

float AClanhallCharacterBase::GetMoveSpeedMultiplier() const
{
	bool bFound = false;
	const float Multiplier = AbilitySystemComponent
		? AbilitySystemComponent->GetGameplayAttributeValue(UClanhallAttributeSet::GetMoveSpeedMultiplierAttribute(), bFound) : 1.0f;
	return bFound ? Multiplier : 1.0f;
}

void AClanhallCharacterBase::RefreshMovementSpeed()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = JogSpeed * GetMoveSpeedMultiplier();
	}
}

void AClanhallCharacterBase::NotifyHitReceived(const FClanhallHitResult& Hit)
{
	OnHitReceived.Broadcast(Hit);
	if (Hit.bArmorBroken)
	{
		OnArmorBroken.Broadcast(Hit.Source);
	}
}

void AClanhallCharacterBase::HandleOutOfHealth(AActor* Killer)
{
	if (bDead)
	{
		return;
	}
	bDead = true;

	// Мёртвый ничего не делает: способности отменены, State.Dead закрывает новые, движение и ввод
	// игрока гейтятся им же. Тело и рэгдолл - не здесь.
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();
		AbilitySystemComponent->AddLooseGameplayTag(ClanhallGameplayTags::State_Dead.GetTag());
	}
	OnDied.Broadcast(Killer);
}

bool AClanhallCharacterBase::IsSprinting() const
{
	return AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(ClanhallGameplayTags::State_Sprinting.GetTag());
}
