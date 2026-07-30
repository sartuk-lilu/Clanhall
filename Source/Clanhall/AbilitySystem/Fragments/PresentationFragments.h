// Презентационные фрагменты — есть не у всех навыков (VFX/SFX), в отличие от CastMontage,
// который переехал в заголовок UAbilityData (отсутствие ≡ CastMontage == nullptr).

#pragma once

#include "AbilityFragment.h"
#include "PresentationFragments.generated.h"

class USoundBase;

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
