// Уклонение (`CombatSystem.md`, «Уклонение») - общее действие, а не приём: «Пробел + направление»,
// отскок в эту сторону. Логика одна у всех оружий, а анимация, дальность и длина в долях - свои
// у оружия (поля UWeaponTypeData). Цена - доли: пока отскок и его хвост идут, новые действия
// закрыты; отдельной перезарядки нет.
//
// Касание - свой отклик оружия: если во время отскока зона оружия (Hitbox-нотифай на монтаже
// уклонения) задела противника, это просто удар - урон без меток, контра и перезарядок.
//
// Наследуется от UGameplayAbility напрямую, не от UGA_ClanhallAbilityBase: тот существует ради
// приёмов. Урон отклика считает общий ClanhallDamage::ResolveHit.
//
// Один хэндл на все направления (EClanhallEvadeDirection) - направление приходит событием через
// FGameplayEventData::EventMagnitude (см. AClanhallCharacter::TriggerEvade). Направление в мире -
// от корпуса, а не от камеры и не от вектора ввода. Кадров неуязвимости нет.
//
// Грант - в AClanhallCharacter::BeginPlay. У противников уклонения пока нет (ИИ реактивно не
// уклоняется - `Invariants.md`, «Разрешённые асимметрии»); класс при этом стороне-нейтрален.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "ClanhallCombatTypes.h"
#include "GA_Dodge.generated.h"

class UAnimMontage;

UCLASS()
class CLANHALL_API UGA_Dodge : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Dodge();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** Хвост восстановления - общий для всех направлений и оружий. Длительность State.EvadeRecovery
	 *  после отскока равна ровно его GetPlayLength(), вешается только если монтаж реально стартовал.
	 *  nullptr - законное состояние: хвоста тогда нет, лок - только сам отскок. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge|Animation")
	TObjectPtr<UAnimMontage> EvadeRecoveryMontage;

private:
	/** Конец Root Motion Source — дублирован на оба финальных делегата задачи. */
	UFUNCTION()
	void OnDodgeFinished();

	/** Зона оружия задела цель во время отскока - отклик: урон без меток. */
	UFUNCTION()
	void OnTouchHit(FGameplayEventData Payload);

	/** Урон отклика по касанию - из класса выбранного оружия на момент отскока. */
	float TouchDamage = 0.0f;
};
