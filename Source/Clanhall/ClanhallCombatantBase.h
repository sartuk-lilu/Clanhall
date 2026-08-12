// Общий предок для всего, что дерётся — игрока и врагов.
// ASC, атрибуты, метки, зоны поражения и окно контра нужны любому бойцу одинаково: метка
// и контр двусторонние по построению, а без диспетчера зон у врага не было бы
// урона вовсе (открытый вопрос). Комбо-дерево и парирование —
// только гуманоидам, см. AClanhallHumanoidCombatant.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "ClanhallCombatantBase.generated.h"

class UAbilitySystemComponent;
class UClanhallAttributeSet;
class UClanhallMarkComponent;
class UClanhallHitboxComponent;
class UClanhallCounterComponent;
class UGameplayAbility;

/** Denied-фидбек: TryActivateAbility отказал именно по нехватке Charges (не по
 *  State.SkillCommitted/State.Stunned — `DataAsset and Fragments.md`, «Denied-фидбек (только делегат)»). Точка подключения для HUD
 *  (звук + вспышка WBP_ChargesPanel), сама реакция сюда не входит — тот же паттерн, что
 *  UClanhallCounterComponent::OnCounterConsumed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClanhallChargesDenied);

UCLASS(abstract)
class AClanhallCombatantBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallMarkComponent> MarkComponent;

	/** Диспетчер активных зон поражения (`Animation Setup.md`). Собственной геометрии не имеет —
	 *  форма и роль каждой зоны приходят из AnimNotifyState_Hitbox на монтаже. Имя сабобъекта
	 *  намеренно осталось прежним ("WeaponTraceComponent") — перенесено символ в символ из
	 *  AClanhallCharacter, переименование рвёт BP-данные игрока. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallHitboxComponent> HitboxComponent;

	/** Окно контрнавыка — симметричный компонент, нужен и монстру: его каст тоже сбивают. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallCounterComponent> CounterComponent;

	/** Unit.Role.* — вешается на ASC в BeginPlay. Незаполненный тег — легальное состояние
	 *  (актор не участвует в ролевой логике HUD/AI), так у игрока по умолчанию. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySystem", meta = (Categories = "Unit.Role"))
	FGameplayTag RoleTag;

	/** Стартовые значения ресурсов (`combat_system.md`, «Ресурсы персонажа») — хардкод-плейсхолдеры прототипа,
	 *  переопределяются per-class в defaults Blueprint-наследника (у Часового свои AP/HP/MP/Charges).
	 *  Раньше жили только в AClanhallCharacter::BeginPlay —
	 *  экземпляр без этого пути (AClanhallHumanoidBoss, пустой конструктор) оставался с нулевыми
	 *  атрибутами: MaxStagger=0 клампил Stagger в [0,0], и GetStagger()>=GetMaxStagger() было
	 *  истиной уже на первом клэше — босс станился с одного парирования вместо положенных четырёх
	 *  (`combat_system.md`, «Stagger — усталость»). */
	UPROPERTY(EditDefaultsOnly, Category = "Attributes")
	float DefaultMaxAP = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attributes")
	float DefaultMaxHP = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attributes")
	float DefaultMaxMP = 200.0f;

	/** (`combat_system.md`, «Ресурсы персонажа»): базовый банк — 6 (было 4), не косметическая
	 *  правка — при базе 4 ранг 3 открывает Z/X ценой 6 при банке 4, навык физически
	 *  недоступен. Потолок 16 — UClanhallAttributeSet::ClampAttribute. */
	UPROPERTY(EditDefaultsOnly, Category = "Attributes")
	float DefaultMaxCharges = 6.0f;

	/** (`combat_system.md`, «Stagger — усталость»): потолок усталости парирования, плейсхолдер — подбирается
	 *  плейтестом (Часовой/Страж получат свой). */
	UPROPERTY(EditDefaultsOnly, Category = "Attributes")
	float DefaultMaxStagger = 4.0f;

public:
	AClanhallCombatantBase();

	// ~begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	// ~end IAbilitySystemInterface

	/** Транслирует получателям (HUD), что этому бойцу отказали в активации именно из-за
	 *  нехватки Charges. */
	UPROPERTY(BlueprintAssignable, Category = "AbilitySystem")
	FOnClanhallChargesDenied OnChargesDenied;

protected:
	virtual void BeginPlay() override;

private:
	/** Слушает UAbilitySystemComponent::AbilityFailedCallbacks и ретранслирует в OnChargesDenied,
	 *  только когда причина отказа — Ability.Denied.Charges (`DataAsset and Fragments.md`, «Denied-фидбек (только делегат)»). */
	void HandleAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason);
};
