#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AbilitySystem/ClanhallHitboxTypes.h"
#include "AnimNotifyState_Hitbox.generated.h"

class UAnimMontage;

/** Активная зона поражения на время отрезка монтажа.
 *  NotifyBegin → UClanhallHitboxComponent::BeginHitbox(Hitbox, this)
 *  NotifyEnd   → UClanhallHitboxComponent::EndHitbox(this)
 *
 *  Ставится на удар-монтажах игрока и врага вокруг фазы контакта. Montage Tick Type =
 *  Branching Point (main_dev_plan.md §7, «Разметка нотифай-стейтов»).
 *
 *  Хендл зоны здесь НЕ хранится: UAnimNotifyState — разделяемый const-объект, состояние
 *  конкретного проигрывания на нём жить не может. Пара Begin/End связывается по `this`. */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="Hitbox"))
class CLANHALL_API UAnimNotifyState_Hitbox : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Hitbox", meta = (ShowOnlyInnerProperties))
	FClanhallHitboxDesc Hitbox;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

	/** Есть ли на монтаже хотя бы один Hitbox-нотифай. Нужен GA_DirectionalAttackBase, чтобы
	 *  выбрать режим резолва: контактный (ждём Event.Hitbox.Hit) или мгновенный фолбэк.
	 *  Фолбэк — то, что сохраняет инвариант «дерево комбо тестируется до нарезки анимаций». */
	static bool MontageHasHitbox(const UAnimMontage* Montage);
};
