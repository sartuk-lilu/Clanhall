#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_CounterWindow.generated.h"

/** Открывает окно контрнавыка на UClanhallCounterComponent владельца на время анимации (Begin→End).
 *  Своих данных не несёт — источник истины UAbilityData, тот же ассет, что описывает навык и для
 *  игрока, и для врага, которому этот навык выдан. Ключ поиска активной активки — сам монтаж, на
 *  котором стоит нотифай: NotifyBegin ищет активный спек владельца, чей UAbilityData::CastMontage
 *  совпадает с этим монтажом, и берёт CounteredBy из него (task_dash_and_counter_window.md,
 *  Задача 3). Слот навыка окну контра больше не нужен — кулдауна в проекте не осталось нигде
 *  (task_skill_economy_loops.md). */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Counter Window"))
class CLANHALL_API UAnimNotifyState_CounterWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
