#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_WeaponTraceStart.generated.h"

/** Открывает окно weapon trace на UClanhallWeaponTraceComponent владельца.
 *  Размещается на анимациях ударов (~20% хода монтажа). */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Weapon Trace Start"))
class CLANHALL_API UAnimNotify_WeaponTraceStart : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
