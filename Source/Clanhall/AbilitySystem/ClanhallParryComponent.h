// Компонент на бойце (игрок и AI, симметрично), который распознаёт клэш парирования.
// Хранит флаг bParrySuccessful — дедуп на одно собственное успешное парирование за окно
// (чтобы одна и та же зона не засчиталась дважды). Сбрасывается
// UClanhallComboComponent::ActivateStep перед каждым СВОИМ новым шагом (main_dev_plan.md §8, Блок D).
//
// Симметрия (Блок D): у State.Parrying и Parry.Incoming.* больше нет разделения
// "атакующий/защищающийся" — оба тега вешает на СЕБЯ тот, кто играет удар-монтаж
// (State.Parrying — AnimNotifyState_ParryWindow, Parry.Incoming.* — UClanhallComboComponent::
// ActivateStep). TryParry читает Parry.Incoming.* с актора, которого задели, а не с себя —
// сторона (игрок/AI) в резолве больше не участвует.
//
// Раздел 5 (устарело): TryParry вызывался из обработчиков ввода WASD персонажа.
// Раздел 7 -> сейчас: вызывается из UClanhallHitboxComponent::CheckAndHandleParry,
//         и только для зон с bParryable == true — активки Q/E/R/F в клэш не попадают.

#pragma once

#include "Components/ActorComponent.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallParryComponent.generated.h"

class USoundBase;

UCLASS(ClassGroup="Clanhall", meta=(BlueprintSpawnableComponent))
class CLANHALL_API UClanhallParryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** true, если этим компонентом уже засчитано успешное парирование в текущем окне —
	 *  дедуп-guard внутри TryParry. Сбрасывается ActivateStep перед новым своим шагом. */
	bool bParrySuccessful = false;

	/** Звук столкновения оружий (воспроизводится TryParry при успехе). */
	UPROPERTY(EditAnywhere, Category="VFX")
	TObjectPtr<USoundBase> ClashSound;

	/** Вызывается UClanhallComboComponent::ActivateStep перед каждым новым шагом владельца. */
	void ResetParry();

	/** Зовётся из UClanhallHitboxComponent::CheckAndHandleParry — только для зон с
	 *  bParryable == true (ability_system.md §2: активки в клэше не участвуют).
	 *  HitTarget — актор, которого задела ЭТА зона (уже проверен вызывающим на State.Parrying);
	 *  MyDirection — направление СВОЕГО удара. Успех — MyDirection обратно направлению,
	 *  которое HitTarget повесил на СЕБЯ тегом Parry.Incoming.* (main_dev_plan.md §8, Блок D:
	 *  тег больше не приходит от третьей стороны — HitTarget сам его на себя повесил, пока
	 *  играл свой шаг). При успехе уведомляет UClanhallComboComponent цели — её текущий шаг
	 *  засчитан как отпарированный. */
	bool TryParry(AActor* HitTarget, EClanhallAttackDirection MyDirection, FVector HitLocation);
};
