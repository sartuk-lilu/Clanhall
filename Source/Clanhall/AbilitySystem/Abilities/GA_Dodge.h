// Отскок — Пробел. Канон: `combat_system.md`, «Отскок». Наследуется от UGameplayAbility
// напрямую, не от UGA_ClanhallAbilityBase: тот существует ради урона и поиска цели
// (FindMeleeTarget, ResolveStandardDamage), а отскок не наносит урона вообще, и его
// ActivationRequiredTags(State.InStance) отскоку вне стойки прямо противопоказан — форма
// выбирается ПО State.InStance, обеими значениями тега.
//
// Форма — на активации, по State.InStance: короткая в стойке (чистое репозиционирование,
// зарядов не стоит никогда) или дальняя вне стойки (заряд, только если есть State.InCombat —
// вне боя разменивать нечем, платный отскок там не создаёт решения). Кадров неуязвимости
// нет ни у одной формы: отскок уводит тело из зоны контакта физически, и это вся его защита.
//
// Грант — в AClanhallCharacter::BeginPlay, рядом с StanceAbilityHandle. У противников
// отскока пока нет — стойки как способности у них тоже нет; сам класс при этом пишется
// стороне-нейтрально, как парирование и WASD-серия.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "GA_Dodge.generated.h"

class UAnimMontage;

UCLASS()
class CLANHALL_API UGA_Dodge : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Dodge();

	/** Дальний отскок в бою — заряд, только если есть State.InCombat. Не хватает — отказ,
	 *  а не бесплатный отскок. Вне боя и короткий отскок в стойке цену не проверяют вовсе. */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** Дистанция короткого отскока в стойке — репозиционирование, а не побег. Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float ShortDodgeDistance = 200.0f;

	/** Дистанция дальнего отскока вне стойки — «большой перекат в соулс-играх». Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float LongDodgeDistance = 600.0f;

	/** За сколько секунд проходится дистанция, той и другой формой — Root Motion Source, тем же
	 *  способом, что рывок в UGA_PhysicalSkill/UDashFragment. Не названо явно в задании как
	 *  отдельный плейсхолдер, но без него ApplyRootMotionMoveToForce нечем параметризовать —
	 *  тот же класс задачи, что и у UDashFragment, требует Distance+Duration парой. Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float DodgeDuration = 0.2f;

	/** Цена дальнего отскока в бою — списывается тем же механизмом, что в UGA_PhysicalSkill:
	 *  на активации и безвозвратно. Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	int32 DodgeChargeCost = 1;

	/** Косметический монтаж короткой формы — механика не зависит от него (`CLAUDE.md`,
	 *  «Механика работает без анимационных ассетов»). nullptr — законное состояние. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge|Animation")
	TObjectPtr<UAnimMontage> ShortDodgeMontage;

	/** Косметический монтаж дальней формы — тот же принцип, что у ShortDodgeMontage. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge|Animation")
	TObjectPtr<UAnimMontage> LongDodgeMontage;

	/** Хвост восстановления КОРОТКОГО отскока — длительность State.DodgeRecovery равна ровно
	 *  его GetPlayLength(), вешается только если монтаж реально стартовал (`combat_system.md`,
	 *  «Отскок»). nullptr — законное состояние: лока тогда нет вовсе, невидимого лока в системе
	 *  не бывает ни при каких условиях. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge|Animation")
	TObjectPtr<UAnimMontage> DodgeRecoveryMontage;

private:
	/** Конец Root Motion Source — дублирован на оба финальных делегата задачи, тем же паттерном,
	 *  что UGA_PhysicalSkill::OnDashFinished. */
	UFUNCTION()
	void OnDodgeFinished();

	/** true — это применение было короткой формой (в стойке), и по завершении движения нужно
	 *  разыграть DodgeRecoveryMontage и повесить State.DodgeRecovery. Дальняя форма — без лока. */
	bool bShortFormPending = false;
};
