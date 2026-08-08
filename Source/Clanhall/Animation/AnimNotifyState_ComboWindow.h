#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_ComboWindow.generated.h"

/** Окно приёма следующего WASD-ввода для чейна комбо (начало ~70% монтажа, конец ровно на
 *  предпоследнем кадре, ставит разработчик на монтаже удара). Begin/End зовут
 *  OnComboWindowOpen/Close на UClanhallComboComponent владельца. Канон: main_dev_plan.md §7. */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Combo Window"))
class CLANHALL_API UAnimNotifyState_ComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
