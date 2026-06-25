// Базовые ресурсы персонажа: AP, HP, MP, Charges, Balance.
// Канон: combat_system.md §1-2, CLAUDE.md "ЗАБЛОКИРОВАННЫЙ КАНОН".

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

	// --- MP (Mana Points): восполняется только WASD-ударами и пассивным регеном ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MP, Category = "Clanhall|MP")
	FGameplayAttributeData MP;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, MP);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMP, Category = "Clanhall|MP")
	FGameplayAttributeData MaxMP;
	ATTRIBUTE_ACCESSORS(UClanhallAttributeSet, MaxMP);

	// --- Charges: ресурс активных навыков (Q/E=0, R/F=2, Z/X=4, C/V=6) ---
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

private:
	/** Держит AP/HP/MP/Charges в [0, Max] и Balance в [-100, 100] при любом источнике изменения. */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};
