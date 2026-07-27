#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "AnimNotifyState_CounterWindow.generated.h"

/** Открывает окно контрнавыка на UClanhallCounterComponent владельца на время анимации (Begin→End).
 *  CounterTag — идентичность контримой активки (напр. Ability.Skill.Knight.PowerStrike), ставит
 *  разработчик на монтаже; по ней находится активный спек (хендл, КД). CounteredBy — набор навыков,
 *  которыми эту активку можно прервать (counter_tags_task.md); пусто = не контрится.
 *  Пока реальных монтажей у врагов нет — GA_EnemyActiveSkill открывает окно напрямую через код
 *  (interim); этот нотифай — для будущих монтажей. */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Counter Window"))
class CLANHALL_API UAnimNotifyState_CounterWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** Идентичность контримой активки — по ней ищется активный спек владельца (хендл, КД). */
	UPROPERTY(EditAnywhere, Category = "Counter", meta = (Categories = "Ability.Skill"))
	FGameplayTag CounterTag;

	/** Набор навыков, которыми эту активку можно прервать; пусто = не контрится. */
	UPROPERTY(EditAnywhere, Category = "Counter", meta = (Categories = "Ability.Skill"))
	FGameplayTagContainer CounteredBy;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
