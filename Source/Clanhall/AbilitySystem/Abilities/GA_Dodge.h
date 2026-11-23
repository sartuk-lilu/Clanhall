// Уход/рывок (`CombatSystem.md`, «Уклонение»). Наследуется от UGameplayAbility напрямую, не от
// UGA_ClanhallAbilityBase: тот существует ради урона и поиска цели (FindMeleeTarget,
// ResolveStandardDamage), а уход не наносит урона вообще. Сегодня с ввода доступен только рывок
// вперёд на бегу (Пробел); боковой уход получит клавишу в P5 («Пробел + направление», `Controls.md`).
//
// Один хэндл на все три направления (Left/Right/ForwardDash, EClanhallEvadeDirection) -
// направление приходит событием через FGameplayEventData::EventMagnitude (см.
// AClanhallCharacter::TriggerEvade).
// Форма выбирается ПО направлению: боковой уход (Left/Right) - короткое чистое
// репозиционирование; рывок вперёд (ForwardDash, доступен только на бегу - см.
// AClanhallCharacter::OnSpacePressed). Цена - доли, отдельной перезарядки нет (`CombatSystem.md`,
// «Уклонение»). Кадров неуязвимости нет ни у одной формы: уход/рывок уводит тело из зоны
// контакта физически, и это вся его защита.
//
// Грант - в AClanhallCharacter::BeginPlay, рядом с приседом. У противников ухода пока нет; сам
// класс при этом пишется стороне-нейтрально.

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
	/** Дистанция бокового ухода - репозиционирование, а не побег. Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float SideDodgeDistance = 200.0f;

	/** За сколько секунд проходится боковой уход - Root Motion Source. Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float SideDodgeDuration = 0.2f;

	/** Дистанция длинного рывка вперёд на бегу - «большой перекат в соулс-играх». Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float LongDashDistance = 600.0f;

	/** За сколько секунд проходится рывок. Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float LongDashDuration = 0.25f;

	/** Косметический монтаж бокового ухода влево - механика не зависит от него (`CLAUDE.md`,
	 *  «Механика работает без анимационных ассетов»). nullptr — законное состояние. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge|Animation")
	TObjectPtr<UAnimMontage> SideDodgeLeftMontage;

	/** Косметический монтаж бокового ухода вправо. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge|Animation")
	TObjectPtr<UAnimMontage> SideDodgeRightMontage;

	/** Косметический монтаж рывка вперёд. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge|Animation")
	TObjectPtr<UAnimMontage> LongDashMontage;

	/** Хвост восстановления - общий для всех трёх форм. Длительность State.EvadeRecovery равна
	 *  ровно его GetPlayLength(), вешается только если монтаж реально стартовал
	 *  (`CombatSystem.md`). nullptr - законное состояние: лока тогда нет вовсе,
	 *  невидимого лока в системе не бывает ни при каких условиях. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge|Animation")
	TObjectPtr<UAnimMontage> EvadeRecoveryMontage;

private:
	/** Конец Root Motion Source — дублирован на оба финальных делегата задачи, тем же паттерном,
	 *  что UGA_PhysicalSkill::OnDashFinished. */
	UFUNCTION()
	void OnDodgeFinished();
};
