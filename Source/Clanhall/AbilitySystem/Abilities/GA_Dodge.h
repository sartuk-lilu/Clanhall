// Уход/рывок - Q/E/Пробел вне стойки (`combat_system.md`). Наследуется от UGameplayAbility
// напрямую, не от UGA_ClanhallAbilityBase: тот существует ради урона и поиска цели (FindMeleeTarget,
// ResolveStandardDamage), а уход не наносит урона вообще, и его
// ActivationRequiredTags(State.InStance) уходу прямо противопоказан - уход возможен только
// вне стойки (защиты в стойке нет).
//
// Один хэндл на все три направления (Left/Right/ForwardDash, EClanhallEvadeDirection) -
// направление приходит событием через FGameplayEventData::EventMagnitude, тем же способом,
// что удары серии в UClanhallComboComponent::ActivateStep (см. AClanhallCharacter::TriggerEvade).
// Форма выбирается ПО направлению: боковой уход (Left/Right) - короткое чистое
// репозиционирование, зарядов не стоит никогда; рывок вперёд (ForwardDash, доступен только на
// бегу - см. AClanhallCharacter::OnSpacePressed) - заряд, только если есть State.InCombat, вне
// боя разменивать нечем. Кадров неуязвимости нет ни у одной формы: уход/рывок уводит тело из
// зоны контакта физически, и это вся его защита.
//
// Грант - в AClanhallCharacter::BeginPlay, рядом со стойкой и приседом. У противников ухода
// пока нет - стойки как способности у них тоже нет; сам класс при этом пишется
// стороне-нейтрально, как парирование и WASD-серия.

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

	/** Не может знать направление (оно приходит в TriggerEventData, а сигнатура
	 *  CanActivateAbility событие не несёт) - гейтит только то, что известно заранее: общие
	 *  блокировки и живой удар-монтаж. Цена рывка вперёд в бою проверяется и списывается в
	 *  ActivateAbility, где направление уже известно (см. комментарий там). */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** Дистанция бокового ухода - репозиционирование, а не побег. Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float SideDodgeDistance = 200.0f;

	/** За сколько секунд проходится боковой уход - Root Motion Source, тем же способом, что
	 *  рывок в UGA_PhysicalSkill/UDashFragment. Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float SideDodgeDuration = 0.2f;

	/** Дистанция длинного рывка вперёд на бегу - «большой перекат в соулс-играх». Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float LongDashDistance = 600.0f;

	/** За сколько секунд проходится рывок. Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float LongDashDuration = 0.25f;

	/** Цена рывка вперёд - единственная платная форма ухода, и только в бою. Списывается тем же
	 *  механизмом, что в UGA_PhysicalSkill: на активации и безвозвратно
	 *  (`economy_system.md`, «Почему кулдаунов нет»). Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	int32 LongDashChargeCost = 1;

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
	 *  (`combat_system.md`). nullptr - законное состояние: лока тогда нет вовсе,
	 *  невидимого лока в системе не бывает ни при каких условиях. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge|Animation")
	TObjectPtr<UAnimMontage> EvadeRecoveryMontage;

private:
	/** Конец Root Motion Source — дублирован на оба финальных делегата задачи, тем же паттерном,
	 *  что UGA_PhysicalSkill::OnDashFinished. */
	UFUNCTION()
	void OnDodgeFinished();
};
