// Раздел 4: один навык-класс для всех физических активных способностей (Q/E/R/F/...).
// Канон CLAUDE.md: "Логика абилки не меняется при правке данных — меняется только DataAsset".
// Конкретное содержание (урон/метка/синергия/баланс/КД/стоимость) приходит из UAbilityData,
// который привязывается к каждому гранту через FGameplayAbilitySpec::SourceObject —
// то есть один и тот же класс граннтится 4 раза (Shield Slam/Power Strike/Shield Charge/
// Retribution), и каждый раз с другим SourceObject.

#pragma once

#include "GA_ClanhallAbilityBase.h"
#include "GA_PhysicalSkill.generated.h"

class UAbilityData;
class UClanhallMarkComponent;
struct FMarkSynergy;

UCLASS()
class CLANHALL_API UGA_PhysicalSkill : public UGA_ClanhallAbilityBase
{
	GENERATED_BODY()

public:
	UGA_PhysicalSkill();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	/** SourceObject ищется через Handle, а не GetCurrentSourceObject() — у InstancedPerExecution
	 *  абилки на момент CanActivateAbility ещё нет персистентного инстанса (см. движок,
	 *  AbilitySystemComponent_Abilities.cpp: CanActivateAbility может вызываться на CDO). */
	const UAbilityData* GetAbilityData(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;

	void ResolveMarkLogic(const UAbilityData* Data, UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, UClanhallMarkComponent* TargetMarkComponent) const;
};
