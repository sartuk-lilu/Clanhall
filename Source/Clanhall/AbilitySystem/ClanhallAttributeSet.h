// Базовые ресурсы персонажа: AP, HP, MP - и модификаторы боя, которые меняют метки.
// Канон: (`CombatSystem.md`, «Ресурсы персонажа»).

#pragma once

#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ClanhallAttributeSet.generated.h"

// Engine предоставляет только четыре "строительных блока" ниже — комбинированный
// ATTRIBUTE_ACCESSORS каждый проект объявляет сам (паттерн из Lyra/GASDocumentation).
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class CLANHALL_API UClanhallAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UClanhallAttributeSet();

	// --- AP (Armor Points): поглощает урон первым, пока AP > 0 — HP не трогается ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AP, Category = "Clanhall|AP")
	FGameplayAttributeData AP;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, AP);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxAP, Category = "Clanhall|AP")
	FGameplayAttributeData MaxAP;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, MaxAP);

	// --- HP (Hit Points) ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HP, Category = "Clanhall|HP")
	FGameplayAttributeData HP;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, HP);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHP, Category = "Clanhall|HP")
	FGameplayAttributeData MaxHP;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, MaxHP);

	// --- MP (Mana Points): шкала заклинаний, с физических приёмов не приходит
	// (`EconomySystem.md`, «Мана») ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MP, Category = "Clanhall|MP")
	FGameplayAttributeData MP;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, MP);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMP, Category = "Clanhall|MP")
	FGameplayAttributeData MaxMP;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, MaxMP);

	// --- Модификаторы боя: база выставляется в AClanhallCharacterBase::BeginPlay, меняют их
	// эффекты меток (`MarksSystem.md`). Бонусы меток складываются, а не перемножаются: модификаторы
	// на множители - операция MultiplyAdditive (`MarksSystem.md`, «Слоты меток»). Читает их
	// ClanhallDamage::ResolveHit (`Implementation/Damage.md`). Не реплицируются: мультиплеера нет. ---

	/** Шанс крита, 0..1, обрезается потолком CritChanceCap (`CombatSystem.md`, «Случайность»). База 0. */
	UPROPERTY(BlueprintReadOnly, Category = "Clanhall|Combat")
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, CritChance);

	/** Снижение порога урона владельца, плоское (`CombatSystem.md`: «порог можно немного снизить
	 *  меткой, но не снять»). База 0. */
	UPROPERTY(BlueprintReadOnly, Category = "Clanhall|Combat")
	FGameplayAttributeData ThresholdReduction;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, ThresholdReduction);

	/** Множитель наносимого урона. База 1. */
	UPROPERTY(BlueprintReadOnly, Category = "Clanhall|Combat")
	FGameplayAttributeData DamageDealtMultiplier;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, DamageDealtMultiplier);

	/** Множитель получаемого урона. База 1. */
	UPROPERTY(BlueprintReadOnly, Category = "Clanhall|Combat")
	FGameplayAttributeData DamageTakenMultiplier;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, DamageTakenMultiplier);

	/** Множитель скорости перемещения. База 1. Применяется к CharacterMovement при изменении. */
	UPROPERTY(BlueprintReadOnly, Category = "Clanhall|Combat")
	FGameplayAttributeData MoveSpeedMultiplier;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, MoveSpeedMultiplier);

protected:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_AP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxAP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_HP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxHP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MP(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxMP(const FGameplayAttributeData& OldValue);

private:
	/** Держит AP/HP/MP в [0, Max], шанс крита в [0, 1], остальные модификаторы - не ниже нуля. */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};
