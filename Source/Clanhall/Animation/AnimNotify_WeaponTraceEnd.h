#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_WeaponTraceEnd.generated.h"

/** Закрывает окно weapon trace на UClanhallWeaponTraceComponent владельца.
 *  Размещается на анимациях ударов (~80% хода монтажа). */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Weapon Trace End"))
class CLANHALL_API UAnimNotify_WeaponTraceEnd : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
