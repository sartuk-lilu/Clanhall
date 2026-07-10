#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "AnimNotifyState_CounterWindow.generated.h"

/** Открывает окно контрнавыка на UClanhallCounterComponent владельца на время анимации (Begin→End).
 *  CounterTag — идентичность контримой активки (напр. Ability.Skill.Knight.PowerStrike), ставит
 *  разработчик на монтаже. Пока реальных монтажей у врагов нет — GA_EnemyActiveSkill открывает
 *  окно напрямую через код (interim); этот нотифай — для будущих монтажей.
 *  clanhall_claude_code_counter.md, Раздел 6. */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Counter Window"))
class CLANHALL_API UAnimNotifyState_CounterWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Counter", meta = (Categories = "Ability.Skill"))
	FGameplayTag CounterTag;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
