#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ApplyMark.generated.h"

/** Отправляет GameplayEvent "Event.ApplyMark" в ASC владельца в момент визуального попадания.
 *  Используется GA_PhysicalSkill как сигнал подтверждённого хита (async-версия).
 *  Размещается на активных навыках Q/E/R/F на фрейме удара. */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Apply Mark"))
class CLANHALL_API UAnimNotify_ApplyMark : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
