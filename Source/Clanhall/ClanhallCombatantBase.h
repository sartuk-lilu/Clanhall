// Общий предок для всего, что дерётся — игрока и врагов (`Combatant Hierarchy.md`, «Три слоя»).
// ASC, атрибуты, метки, зоны поражения и окно контра нужны любому бойцу одинаково: метка
// и контр двусторонние по построению, а без диспетчера зон у врага не было бы
// урона вовсе (`Combatant Hierarchy.md`, «AClanhallCombatantBase»). Комбо-дерево и парирование —
// только гуманоидам, см. AClanhallHumanoidCombatant (`Combatant Hierarchy.md`, «Граница слоёв»).

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
class UClanhallCombatStateComponent;
class UGameplayAbility;

/** Denied-фидбек: TryActivateAbility отказал именно по нехватке Charges (не по
 *  State.SkillCommitted/State.Stunned — `Combatant Hierarchy.md`, «Denied-фидбек»). Точка подключения для HUD
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

	/** Диспетчер активных зон поражения (`Animation Setup.md`, «Диспетчер зон поражения»). Собственной геометрии не имеет —
	 *  форма и роль каждой зоны приходят из AnimNotifyState_Hitbox на монтаже. Имя сабобъекта
	 *  намеренно осталось прежним ("WeaponTraceComponent") — перенесено символ в символ из
	 *  AClanhallCharacter, переименование рвёт BP-данные игрока. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallHitboxComponent> HitboxComponent;

	/** Окно контрнавыка — симметричный компонент, нужен и монстру: его каст тоже сбивают. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallCounterComponent> CounterComponent;

	/** State.InCombat — стороне-нейтральный, нужен и монстру под A-life так же, как игроку
	 *  (`combat_system.md`, «Боевое состояние»). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallCombatStateComponent> CombatStateComponent;

	/** Unit.Role.* — вешается на ASC в BeginPlay (`Combatant Hierarchy.md`, «Unit.Role.*»). Незаполненный тег — легальное состояние
	 *  (актор не участвует в ролевой логике HUD/AI), так у игрока по умолчанию. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySystem", meta = (Categories = "Unit.Role"))
	FGameplayTag RoleTag;

	/** Стартовые значения ресурсов (`combat_system.md`, «Ресурсы персонажа»; `Combatant Hierarchy.md`,
	 *  «Стартовые значения атрибутов на базе») — хардкод-плейсхолдеры прототипа,
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

	/** Базовая скорость стойки — отдельная от MaxWalkSpeed бега, принадлежит бойцу, а не
	 *  оружию (`weapon_system.md`: множитель оружия применяется к ней, не к скорости бега).
	 *  Итоговая скорость в стойке = StanceBaseSpeed * UWeaponTypeData::StanceSpeedMultiplier
	 *  активного оружия (`UGA_CombatStance::ActivateAbility`). Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float StanceBaseSpeed = 300.0f;

	/** Бег вне стойки — удержание Пробела поднимает MaxWalkSpeed до этого значения, отпускание
	 *  возвращает прежнее (`combat_system.md`, «Отскок», блок «Бег»). Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintSpeed = 900.0f;

	/** Угол между камерой и корпусом (градусы), после которого стоящий боец начинает доворот
	 *  (`locomotion_structure.md`, «Локомоция стойки»). Ниже порога корпус не вращается вовсе —
	 *  визуально за камерой тянется только верх, это работа ABP. Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float StanceTurnThreshold = 60.0f;

	/** Угол, на котором начавшийся доворот считается завершённым. Меньше StanceTurnThreshold —
	 *  гистерезис нужен, иначе на границе порога доворот дёргается "начал — тут же перестал"
	 *  каждый кадр (`locomotion_structure.md`, «Локомоция стойки»). Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float StanceTurnSettleAngle = 10.0f;

	/** Скорость доворота корпуса в стойке, градусов в секунду. Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float StanceTurnRate = 300.0f;

	/** Играть ли подшаг (ABP) — взводится, когда угол между камерой и корпусом уходит за
	 *  StanceTurnThreshold, снимается на StanceTurnSettleAngle или при перемещении в стойке
	 *  (`locomotion_structure.md`, «Локомоция стойки»). */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bStanceTurning = false;

	/** Направление доворота для ABP: -1 влево, +1 вправо, 0 — не поворачивает. */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float StanceTurnDirection = 0.0f;

public:
	AClanhallCombatantBase();

	// ~begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	// ~end IAbilitySystemInterface

	/** Транслирует получателям (HUD), что этому бойцу отказали в активации именно из-за
	 *  нехватки Charges. */
	UPROPERTY(BlueprintAssignable, Category = "AbilitySystem")
	FOnClanhallChargesDenied OnChargesDenied;

	/** Читает UGA_CombatStance при входе в стойку, чтобы посчитать итоговый MaxWalkSpeed. */
	float GetStanceBaseSpeed() const { return StanceBaseSpeed; }

	/** Читает AClanhallCharacter при удержании Пробела вне стойки. */
	float GetSprintSpeed() const { return SprintSpeed; }

	/** Читает AClanhallCharacter::Tick — порог входа в доворот корпуса. */
	float GetStanceTurnThreshold() const { return StanceTurnThreshold; }

	/** Читает AClanhallCharacter::Tick — угол, на котором доворот считается завершённым. */
	float GetStanceTurnSettleAngle() const { return StanceTurnSettleAngle; }

	/** Читает UGA_CombatStance::ActivateAbility (RotationRate.Yaw на вход в стойку) и
	 *  AClanhallCharacter::Tick (сам доворот через движковый bUseControllerDesiredRotation). */
	float GetStanceTurnRate() const { return StanceTurnRate; }

	/** Сбрасывает состояние доворота для ABP — вызывается UGA_CombatStance::EndAbility на выходе
	 *  из стойки, а не изнутри Tick: тик перестаёт считаться сразу после снятия State.InStance,
	 *  и последнее значение bStanceTurning иначе могло бы остаться висеть "true". */
	void ResetStanceTurning() { bStanceTurning = false; StanceTurnDirection = 0.0f; }

protected:
	virtual void BeginPlay() override;

private:
	/** Слушает UAbilitySystemComponent::AbilityFailedCallbacks и ретранслирует в OnChargesDenied,
	 *  только когда причина отказа — Denied.Charges (`Combatant Hierarchy.md`, «Denied-фидбек»). */
	void HandleAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason);
};
