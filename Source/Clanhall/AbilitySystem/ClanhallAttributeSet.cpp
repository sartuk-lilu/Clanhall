#include "ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffectTypes.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float BalanceOverloadThreshold = 60.0f;
}

bool UClanhallAttributeSet::IsBalanceOverloaded(float Balance, bool bIsSTR)
{
	return bIsSTR ? (Balance >= BalanceOverloadThreshold) : (Balance <= -BalanceOverloadThreshold);
}

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
	else if (Attribute == GetMaxChargesAttribute())
	{
		// Предел отрисовки WBP_ChargesPanel (четыре ряда ромбов), не дизайнерское решение
		// (combat_system.md §1). Открытый вопрос «что делать с прибавками сверх трёх физических
		// веток ранга 4» — main_dev_plan.md, «Открытые вопросы», п.16.
		NewValue = FMath::Clamp(NewValue, 0.0f, 12.0f);
	}
	else if (Attribute == GetBalanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, -100.0f, 100.0f);
	}
	else if (Attribute == GetStaggerAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, MaxStagger.GetCurrentValue());
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
	else if (ChangedAttribute == GetMaxChargesAttribute())
	{
		SetMaxCharges(FMath::Clamp(GetMaxCharges(), 0.0f, 12.0f));
	}
	else if (ChangedAttribute == GetBalanceAttribute())
	{
		const float ClampedBalance = FMath::Clamp(GetBalance(), -100.0f, 100.0f);
		SetBalance(ClampedBalance);

		// Тег — состояние зоны шкалы САМОЙ ПО СЕБЕ (combat_system.md §2), не «текущее оружие
		// перегружено»: тем же значением пользуется UGA_ClanhallAbilityBase::IsBalanceOverloaded
		// для навыка конкретного оружия, здесь же навешивается видимый снаружи (HUD/Blueprint)
		// сигнал «шкала в зоне перегруза X», живущий независимо от того, что сейчас в руке.
		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			// AddLooseGameplayTag/RemoveLooseGameplayTag ref-считанные: вызов Add на уже
			// добавленном теге просто копит рефкаунт, который потом не с чем свести к нулю
			// одним Remove. PostGameplayEffectExecute может сработать много раз подряд, пока
			// Balance стоит в той же зоне (каждый WASD-удар), поэтому переключаем тег ровно
			// один раз на границе, а не на каждый вызов.
			const FGameplayTag STRTag = ClanhallGameplayTags::Balance_Overload_STR.GetTag();
			const bool bSTROverloaded = IsBalanceOverloaded(ClampedBalance, /*bIsSTR*/ true);
			const bool bSTRTagPresent = ASC->HasMatchingGameplayTag(STRTag);
			if (bSTROverloaded && !bSTRTagPresent)
			{
				ASC->AddLooseGameplayTag(STRTag);
			}
			else if (!bSTROverloaded && bSTRTagPresent)
			{
				ASC->RemoveLooseGameplayTag(STRTag);
			}

			const FGameplayTag DEXTag = ClanhallGameplayTags::Balance_Overload_DEX.GetTag();
			const bool bDEXOverloaded = IsBalanceOverloaded(ClampedBalance, /*bIsSTR*/ false);
			const bool bDEXTagPresent = ASC->HasMatchingGameplayTag(DEXTag);
			if (bDEXOverloaded && !bDEXTagPresent)
			{
				ASC->AddLooseGameplayTag(DEXTag);
			}
			else if (!bDEXOverloaded && bDEXTagPresent)
			{
				ASC->RemoveLooseGameplayTag(DEXTag);
			}
		}
	}
	else if (ChangedAttribute == GetStaggerAttribute())
	{
		SetStagger(FMath::Clamp(GetStagger(), 0.0f, GetMaxStagger()));
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
	DOREPLIFETIME_CONDITION_NOTIFY(UClanhallAttributeSet, Stagger, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UClanhallAttributeSet, MaxStagger, COND_None, REPNOTIFY_Always);
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

void UClanhallAttributeSet::OnRep_Stagger(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UClanhallAttributeSet, Stagger, OldValue);
}

void UClanhallAttributeSet::OnRep_MaxStagger(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UClanhallAttributeSet, MaxStagger, OldValue);
}
