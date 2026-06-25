#include "ClanhallAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffectTypes.h"
#include "Net/UnrealNetwork.h"

UClanhallAttributeSet::UClanhallAttributeSet()
{
}

void UClanhallAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UClanhallAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetAPAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, MaxAP.GetCurrentValue());
	}
	else if (Attribute == GetHPAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, MaxHP.GetCurrentValue());
	}
	else if (Attribute == GetMPAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, MaxMP.GetCurrentValue());
	}
	else if (Attribute == GetChargesAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, MaxCharges.GetCurrentValue());
	}
	else if (Attribute == GetBalanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, -100.0f, 100.0f);
	}
}

void UClanhallAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// PreAttributeChange выше клампит мгновенные правки (например SetHP в коде),
	// но модификаторы GameplayEffect идут через Aggregator и могут на шаг обогнать
	// пересчёт лимита — повторный кламп здесь гарантирует итоговое значение в каноничных границах.
	const FGameplayAttribute& ChangedAttribute = Data.EvaluatedData.Attribute;

	if (ChangedAttribute == GetAPAttribute())
	{
		SetAP(FMath::Clamp(GetAP(), 0.0f, GetMaxAP()));
	}
	else if (ChangedAttribute == GetHPAttribute())
	{
		SetHP(FMath::Clamp(GetHP(), 0.0f, GetMaxHP()));
	}
	else if (ChangedAttribute == GetMPAttribute())
	{
		SetMP(FMath::Clamp(GetMP(), 0.0f, GetMaxMP()));
	}
	else if (ChangedAttribute == GetChargesAttribute())
	{
		SetCharges(FMath::Clamp(GetCharges(), 0.0f, GetMaxCharges()));
	}
	else if (ChangedAttribute == GetBalanceAttribute())
	{
		SetBalance(FMath::Clamp(GetBalance(), -100.0f, 100.0f));
	}
}

void UClanhallAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UClanhallAttributeSet, AP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UClanhallAttributeSet, MaxAP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UClanhallAttributeSet, HP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UClanhallAttributeSet, MaxHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UClanhallAttributeSet, MP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UClanhallAttributeSet, MaxMP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UClanhallAttributeSet, Charges, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UClanhallAttributeSet, MaxCharges, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UClanhallAttributeSet, Balance, COND_None, REPNOTIFY_Always);
}

void UClanhallAttributeSet::OnRep_AP(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UClanhallAttributeSet, AP, OldValue);
}

void UClanhallAttributeSet::OnRep_MaxAP(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UClanhallAttributeSet, MaxAP, OldValue);
}

void UClanhallAttributeSet::OnRep_HP(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UClanhallAttributeSet, HP, OldValue);
}

void UClanhallAttributeSet::OnRep_MaxHP(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UClanhallAttributeSet, MaxHP, OldValue);
}

void UClanhallAttributeSet::OnRep_MP(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UClanhallAttributeSet, MP, OldValue);
}

void UClanhallAttributeSet::OnRep_MaxMP(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UClanhallAttributeSet, MaxMP, OldValue);
}

void UClanhallAttributeSet::OnRep_Charges(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UClanhallAttributeSet, Charges, OldValue);
}

void UClanhallAttributeSet::OnRep_MaxCharges(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UClanhallAttributeSet, MaxCharges, OldValue);
}

void UClanhallAttributeSet::OnRep_Balance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UClanhallAttributeSet, Balance, OldValue);
}
