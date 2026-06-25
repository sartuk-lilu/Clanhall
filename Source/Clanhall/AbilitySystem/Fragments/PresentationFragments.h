// Презентационные фрагменты — форма заложена в Разделе 4 (development_plan.md требует
// "Все фрагменты из таблицы" сразу), но GA_PhysicalSkill пока их не читает: монтажей,
// VFX и звука у проекта ещё нет (Animation Setup — Раздел 6.5). Просто FindFragment<T>()
// будет возвращать nullptr, пока эти данные не появятся в конкретном UAbilityData.

#pragma once

#include "AbilityFragment.h"
#include "PresentationFragments.generated.h"

class UAnimMontage;
class USoundBase;

UCLASS()
class CLANHALL_API UAnimationFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<UAnimMontage> CastMontage;

	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<UAnimMontage> ImpactMontage;
};

UCLASS()
class CLANHALL_API UVFXFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	// Niagara или Cascade — решится в Разделе 6.5, пока без конкретного типа.
	UPROPERTY(EditAnywhere, Category = "VFX")
	TSoftObjectPtr<UObject> CastEffect;

	UPROPERTY(EditAnywhere, Category = "VFX")
	TSoftObjectPtr<UObject> ImpactEffect;
};

UCLASS()
class CLANHALL_API USFXFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "SFX")
	TObjectPtr<USoundBase> CastSound;

	UPROPERTY(EditAnywhere, Category = "SFX")
	TObjectPtr<USoundBase> ImpactSound;
};
