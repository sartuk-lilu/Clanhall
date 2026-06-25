#include "ClanhallGameplayEffects.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/Effects/GE_ApplyTimedTag.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

namespace
{
	// Общий шаблон для всех четырёх классов: один Instant Additive-модификатор,
	// магнитуда которого выставляется вызывающим кодом через SetByCaller.
	FGameplayModifierInfo MakeSetByCallerModifier(const FGameplayAttribute& Attribute)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = ClanhallGameplayTags::SetByCaller_Magnitude.GetTag();

		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		return Modifier;
	}
}

UGE_ModifyAP::UGE_ModifyAP()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeSetByCallerModifier(UClanhallAttributeSet::GetAPAttribute()));
}

UGE_ModifyHP::UGE_ModifyHP()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeSetByCallerModifier(UClanhallAttributeSet::GetHPAttribute()));
}

UGE_ModifyMP::UGE_ModifyMP()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeSetByCallerModifier(UClanhallAttributeSet::GetMPAttribute()));
}

UGE_ModifyBalance::UGE_ModifyBalance()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeSetByCallerModifier(UClanhallAttributeSet::GetBalanceAttribute()));
}

UGE_ModifyCharges::UGE_ModifyCharges()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeSetByCallerModifier(UClanhallAttributeSet::GetChargesAttribute()));
}

void ClanhallGameplayEffects::ApplyModifyEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, float Magnitude)
{
	if (!SourceASC || !TargetASC || !EffectClass)
	{
		return;
	}

	const FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 1.0f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ClanhallGameplayTags::SetByCaller_Magnitude.GetTag(), Magnitude);
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

void ClanhallGameplayEffects::ApplyEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass)
{
	if (!SourceASC || !TargetASC || !EffectClass)
	{
		return;
	}

	const FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 1.0f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

FActiveGameplayEffectHandle ClanhallGameplayEffects::ApplyTimedTagToTarget(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, FGameplayTag Tag, float DurationSeconds)
{
	if (!SourceASC || !TargetASC || !Tag.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	const FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(UGE_ApplyTimedTag::StaticClass(), 1.0f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	SpecHandle.Data->DynamicGrantedTags.AddTag(Tag);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ClanhallGameplayTags::SetByCaller_Magnitude.GetTag(), DurationSeconds);
	return SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

FActiveGameplayEffectHandle ClanhallGameplayEffects::ApplyTimedTag(UAbilitySystemComponent* ASC, FGameplayTag Tag, float DurationSeconds)
{
	return ApplyTimedTagToTarget(ASC, ASC, Tag, DurationSeconds);
}
