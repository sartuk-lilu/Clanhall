// Общий предок для всего, что дерётся — игрока и врагов (main_dev_plan.md §8, Блок A).
// ASC, атрибуты, метки, зоны поражения и окно контра нужны любому бойцу одинаково: метка
// и контр двусторонние по построению (Разделы 3, 6), а без диспетчера зон у врага не было бы
// урона вовсе (main_dev_plan.md §8, «Открытый вопрос» 5). Комбо-дерево и парирование —
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

	/** Диспетчер активных зон поражения (main_dev_plan.md §7). Собственной геометрии не имеет —
	 *  форма и роль каждой зоны приходят из AnimNotifyState_Hitbox на монтаже. Имя сабобъекта
	 *  намеренно осталось прежним ("WeaponTraceComponent") — перенесено символ в символ из
	 *  AClanhallCharacter, переименование рвёт BP-данные игрока. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallHitboxComponent> HitboxComponent;

	/** Раздел 6: окно контрнавыка — симметричный компонент, нужен и монстру: его каст тоже сбивают. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallCounterComponent> CounterComponent;

	/** Unit.Role.* — вешается на ASC в BeginPlay. Незаполненный тег — легальное состояние
	 *  (актор не участвует в ролевой логике HUD/AI), так у игрока по умолчанию. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySystem", meta = (Categories = "Unit.Role"))
	FGameplayTag RoleTag;

	/** Стартовые значения ресурсов (combat_system.md §1) — хардкод-плейсхолдеры прототипа,
	 *  переопределяются per-class в defaults Blueprint-наследника (main_dev_plan.md §8, Блок G:
	 *  у Часового свои AP/HP/MP/Charges). Раньше жили только в AClanhallCharacter::BeginPlay —
	 *  экземпляр без этого пути (AClanhallHumanoidBoss, пустой конструктор) оставался с нулевыми
	 *  атрибутами: MaxStagger=0 клампил Stagger в [0,0], и GetStagger()>=GetMaxStagger() было
	 *  истиной уже на первом клэше — босс станился с одного парирования вместо положенных четырёх
	 *  (task_section8_blocks_fgh.md, блокер ревью §0.3). */
	UPROPERTY(EditDefaultsOnly, Category = "Attributes")
	float DefaultMaxAP = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attributes")
	float DefaultMaxHP = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attributes")
	float DefaultMaxMP = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attributes")
	float DefaultMaxCharges = 4.0f;

	/** task_parry_rework.md §1.3: потолок усталости парирования, плейсхолдер — подбирается
	 *  плейтестом (Часовой/Страж получат свой в Блоке G). */
	UPROPERTY(EditDefaultsOnly, Category = "Attributes")
	float DefaultMaxStagger = 4.0f;

public:
	AClanhallCombatantBase();

	// ~begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	// ~end IAbilitySystemInterface

protected:
	virtual void BeginPlay() override;
};
