#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ParryWindowEnd.generated.h"

/** Закрывает окно парирования на ASC владельца (≈80% анимации удара врага). */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Parry Window End"))
class CLANHALL_API UAnimNotify_ParryWindowEnd : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
