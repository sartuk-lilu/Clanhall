#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ParryWindowStart.generated.h"

/** Открывает окно парирования на ASC владельца (≈20% анимации удара врага).
 *  State.Parrying вешается как loose tag на ASC самого врага — игрок может паррировать
 *  weapon trace'ом. Парная нотифай AnimNotify_ParryWindowEnd снимает тег на ≈80%. */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Parry Window Start"))
class CLANHALL_API UAnimNotify_ParryWindowStart : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
