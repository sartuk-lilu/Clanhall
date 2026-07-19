#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_WeaponTrace.generated.h"

/** Окно weapon trace: Begin → UClanhallWeaponTraceComponent::BeginTrace, End → EndTrace.
 *  Ставится на удар-монтажах игрока и врага вокруг фазы контакта (≈20–80%).
 *  Заменил пару AnimNotify_WeaponTraceStart/End — одиночный End не срабатывал при
 *  прерывании монтажа и оставлял трейс активным навсегда.
 *  development_plan.md Раздел 6.5, animation_setup_6_5.md §2. */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Weapon Trace"))
class CLANHALL_API UAnimNotifyState_WeaponTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
