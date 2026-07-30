#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_ComboWindow.generated.h"

/** Окно приёма следующего WASD-ввода для чейна комбо (≈40-60% замаха → 100%, ставит разработчик
 *  на монтаже удара). Begin/End зовут OnComboWindowOpen/Close на UClanhallComboComponent владельца.
 *  Канон: combo_system.md. */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Combo Window"))
class CLANHALL_API UAnimNotifyState_ComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
