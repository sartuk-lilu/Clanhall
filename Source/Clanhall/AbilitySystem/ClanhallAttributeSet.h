// Базовые ресурсы персонажа: AP, HP, MP, Charges, Balance.
// Канон: combat_system.md §1-2.

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

	// --- MP (Mana Points): восполняется подтверждённым попаданием физической активки, раз за
	// применение (UAbilityData::ManaGain, ability_system.md §1) — WASD-удары маны не дают ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MP, Category = "Clanhall|MP")
	FGameplayAttributeData MP;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, MP);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMP, Category = "Clanhall|MP")
	FGameplayAttributeData MaxMP;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, MaxMP);

	// --- Charges: ресурс активных навыков (Q/E=2, R/F=4, Z/X=6, C/V=8) — единственный гейт
	// применения, кулдауна в проекте не осталось нигде. MaxCharges клампится на 12 —
	// предел отрисовки WBP_ChargesPanel, не дизайнерское решение (combat_system.md §1). ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Charges, Category = "Clanhall|Charges")
	FGameplayAttributeData Charges;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, Charges);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxCharges, Category = "Clanhall|Charges")
	FGameplayAttributeData MaxCharges;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, MaxCharges);

	// --- Balance: шкала DEX ↔ STR, диапазон жёстко -100..+100 (нет MaxBalance) ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Balance, Category = "Clanhall|Balance")
	FGameplayAttributeData Balance;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, Balance);

	// --- Stagger: усталость от парирования (task_parry_rework.md §1.3). Копится владельцу
	// зоны на каждом отпарированном шаге, распадается таймером на UClanhallParryComponent
	// после паузы без парирований; на потолке — сброс в 0 + State.Stunned владельцу. ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stagger, Category = "Clanhall|Stagger")
	FGameplayAttributeData Stagger;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, Stagger);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStagger, Category = "Clanhall|Stagger")
	FGameplayAttributeData MaxStagger;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, MaxStagger);

	/** Единственное место, где живёт порог перегруза (combat_system.md §2: |Balance| >= 60).
	 *  Источник истины и для тега Balance.Overload.* (навешивается здесь же, в
	 *  PostGameplayEffectExecute), и для UGA_ClanhallAbilityBase::IsBalanceOverloaded —
	 *  раньше число было захардкожено в двух местах порознь. */
	static bool IsBalanceOverloaded(float Balance, bool bIsSTR);

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
	UFUNCTION()
	void OnRep_Charges(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxCharges(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Balance(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_Stagger(const FGameplayAttributeData& OldValue);
	UFUNCTION()
	void OnRep_MaxStagger(const FGameplayAttributeData& OldValue);

private:
	/** Держит AP/HP/MP/Charges в [0, Max] и Balance в [-100, 100] при любом источнике изменения. */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};
