// Фрагменты, которые реально читает GA_PhysicalSkill (development_plan.md: "сначала
// механика работает, потом она красиво выглядит" — это механика). Презентационные
// фрагменты (Animation/VFX/SFX) — в PresentationFragments.h.

#pragma once

#include "AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/ClanhallMarkTypes.h"
#include "GameplayFragments.generated.h"

/** У навыков, наносящих урон. AP/HP-обмен по стандартной формуле (combat_system.md §AP) —
 *  считает GA_ClanhallAbilityBase::ResolveStandardDamage, этот фрагмент только хранит число. */
UCLASS()
class CLANHALL_API UDamageFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Damage")
	float BaseDamage = 0.0f;
};

/** У навыков, накладывающих метку на цель после попадания (mark_system.md §2, Правила 1 и 3). */
UCLASS()
class CLANHALL_API UMarkApplyFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Mark", meta = (Categories = "Mark"))
	FGameplayTag MarkTag;
};

/** У навыков, потребляющих метки на цели (mark_system.md §2, Правило 2). */
UCLASS()
class CLANHALL_API UMarkTriggerFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Mark")
	TArray<FMarkSynergy> Synergies;
};

/** У всех физических навыков — сдвиг шкалы DEX↔STR (combat_system.md §2). */
UCLASS()
class CLANHALL_API UBalanceFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Balance")
	float Shift = 0.0f;
};
