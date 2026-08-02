#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_ParryWindow.generated.h"

/** Окно парирования: Begin/End добавляют/снимают loose-тег State.Parrying на ASC владельца
 *  (владелец = тот, кто играет этот удар-монтаж — паррируемый актор, симметрично для игрока
 *  и AI, main_dev_plan.md §8, Блок D; см. combat_system.md §5).
 *  Заменил пару AnimNotify_ParryWindowStart/End — одиночный End не срабатывал при
 *  прерывании монтажа и оставлял State.Parrying висеть бессрочно. */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Parry Window"))
class CLANHALL_API UAnimNotifyState_ParryWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
