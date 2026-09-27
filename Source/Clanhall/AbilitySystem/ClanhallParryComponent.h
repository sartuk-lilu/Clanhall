// Компонент на бойце (игрок и AI, симметрично), который распознаёт клэш парирования.
//
// Модель (`Parrying.md`, «`UClanhallParryComponent`»): окно парирования размечается на монтаже защищающегося
// (AnimNotifyState_ParryWindow -> State.Parrying), резолв запускает КОНТАКТ КЛИНКА АТАКУЮЩЕГО —
// собственный хитбокс защищающегося в проверке не участвует вообще. TryParry вызывается на
// ParryComponent АТАКУЮЩЕГО (владельца зоны, задевшей цель); если цель держит State.Parrying и
// обратный тег направления (Attack.Direction.*, UClanhallComboComponent::ActivateStep) —
// атакующий оказывается ОТПАРИРОВАН, а цель — парировавшим. Кредит именно такой: клинок
// атакующего долетает первым, значит и реагирует на клэш он, а не тот, кто среагировал вовремя.
//
// Устарело: TryParry когда-то вызывался из обработчиков ввода WASD персонажа и записывал
// парировавшим владельца хитбокса — инверсия, исправлена. Сейчас: владелец хитбокса
// (атакующий) получает хитстоп, цель (парировавший) — Charge и подавление
// собственной зоны.

#pragma once

#include "Components/ActorComponent.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallParryComponent.generated.h"

class USoundBase;
class UAbilitySystemComponent;

UCLASS(ClassGroup="Clanhall", meta=(BlueprintSpawnableComponent))
class CLANHALL_API UClanhallParryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** true, если ТЕКУЩИЙ шаг владельца (атакующего) уже засчитан отпарированным в этом окне —
	 *  дедуп-guard внутри TryParry: один замах не должен засчитаться отпарированным дважды, даже
	 *  если его многофазная зона задела нескольких парирующих. Было bParrySuccessful ("я парировал");
	 *  переименовано, т.к. смысл развернулся — теперь "мой текущий шаг уже отпарирован"
	 *  (`Parrying.md`, «`UClanhallParryComponent`»). Сбрасывается ActivateStep перед новым своим шагом. */
	bool bStepParried = false;

	/** Звук столкновения оружий (воспроизводится TryParry при успехе). */
	UPROPERTY(EditAnywhere, Category="VFX")
	TObjectPtr<USoundBase> ClashSound;

	/** Вызывается UClanhallComboComponent::ActivateStep перед каждым новым шагом владельца —
	 *  дедуп многофазной зоны ВНУТРИ шага. Не путать с ResetParriedSteps() (счётчик серии). */
	void ResetParry();

	/** Сбрасывает счётчик отражённых шагов серии (ParriedStepsThisSeries) — зовётся
	 *  UClanhallComboComponent::TryStartSequence в момент, когда реально стартует новая серия
	 *  ВЛАДЕЛЬЦА. Отличается от ResetParry(): тот дедуп-guard на шаге, этот — счётчик отражённых
	 *  шагов на серии (нужен P2: раскрытие после чистого отражения всей серии). */
	void ResetParriedSteps() { ParriedStepsThisSeries = 0; }

	/** Зовётся из UClanhallHitboxComponent::CheckAndHandleParry на ЗОНЕ АТАКУЮЩЕГО (владельца
	 *  этого компонента) — только для зон с bParryable == true (`ability_system.md`, «Контрнавык»: активки
	 *  в клэше не участвуют). HitTarget — актор, которого задела ЭТА зона, уже проверен
	 *  вызывающим на State.Parrying; MyDirection — направление СВОЕГО удара. Успех — MyDirection
	 *  обратно направлению, которое HitTarget повесил на СЕБЯ тегом Attack.Direction.*
	 *  (UClanhallComboComponent::ActivateStep). При успехе: владелец (атакующий) получает
	 *  хитстоп, HitTarget (парировавший) — Charge, его собственная зона подавляется. */
	bool TryParry(AActor* HitTarget, EClanhallAttackDirection MyDirection, FVector HitLocation);

private:
	/** Сколько шагов ТЕКУЩЕЙ серии владельца уже отражено — растёт на 1 на каждом успешном
	 *  TryParry. Приватно и без геттера: это счётчик отражённых шагов серии АТАКУЮЩЕГО (нужен P2:
	 *  раскрытие после чистого отражения всей серии), а не признак "подряд идущих" парирований,
	 *  который понадобится BT для решения об отступлении — разная семантика (пример D->A->D),
	 *  общий счётчик под оба назначения не подходит. Сбрасывается ИСКЛЮЧИТЕЛЬНО через
	 *  ResetParriedSteps(), из UClanhallComboComponent::TryStartSequence — не здесь и не в
	 *  ResetParry(). */
	int32 ParriedStepsThisSeries = 0;

	UAbilitySystemComponent* GetASC() const;
};
