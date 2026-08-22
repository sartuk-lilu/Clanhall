// Присед — Ctrl вне стойки (`combat_system.md`; `stage4_rev2_handoff.md`,
// «Словарь защиты зеркален словарю атаки»). Наследуется от UGameplayAbility напрямую, той же
// причиной, что UGA_Dodge: не наносит урона, ActivationRequiredTags(State.InStance) на
// UGA_ClanhallAbilityBase присед вне стойки прямо противопоказан.
//
// Присед — окно, а не удержание: игрок нажимает Ctrl и должен попасть в замах — капсула
// опускается через DuckWindupTime, остаётся опущенной DuckWindowDuration, потом поднимается
// сама. Все остальные механики проекта работают окнами (парирование, окно чтения серии,
// Recovery), удержание выбивалось бы из ряда — AClanhallCharacter не биндит Completed на
// DuckAction вовсе.
//
// Капсулу опускает движковый Character->Crouch(), не SetCapsuleSize — движок сам разбирается
// со смещением меша, коллизией и проверкой «есть ли место разогнуться». Требует
// NavAgentProps.bCanCrouch == true (см. AClanhallCombatantBase, конструктор) — без него
// Crouch() молча ничего не делает.
//
// Грант — в AClanhallCharacter::BeginPlay, рядом со стойкой и уходом.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "GA_Duck.generated.h"

class UAnimMontage;

UCLASS()
class CLANHALL_API UGA_Duck : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Duck();

	/** Живой удар-монтаж блокирует присед — та же причина и тот же предикат, что у UGA_Dodge. */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Активация -> WaitDelay(DuckWindupTime) -> Crouch() -> WaitDelay(DuckWindowDuration) ->
	 *  UnCrouch() -> Recovery-монтаж и тег, если монтаж стартовал -> EndAbility. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** Задержка до опускания капсулы — то, во что игрок обязан попасть. Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Duck")
	float DuckWindupTime = 0.05f;

	/** Сколько капсула остаётся опущенной. Плейсхолдер. */
	UPROPERTY(EditDefaultsOnly, Category = "Duck")
	float DuckWindowDuration = 0.35f;

	/** Косметический монтаж приседа — не посадка на корточки, а сгибание корпуса вниз и вперёд
	 *  (открытый вопрос, клипа нет — до его появления Crouch() меняет только капсулу, и меш
	 *  визуально просядет). nullptr — законное состояние (`CLAUDE.md`, «Механика работает без
	 *  анимационных ассетов»). */
	UPROPERTY(EditDefaultsOnly, Category = "Duck|Animation")
	TObjectPtr<UAnimMontage> DuckMontage;

	/** Хвост восстановления — длительность State.EvadeRecovery равна ровно его GetPlayLength(),
	 *  вешается только если монтаж реально стартовал. nullptr — законное состояние: лока тогда
	 *  нет вовсе. */
	UPROPERTY(EditDefaultsOnly, Category = "Duck|Animation")
	TObjectPtr<UAnimMontage> DuckRecoveryMontage;

private:
	UFUNCTION()
	void OnWindupFinished();

	UFUNCTION()
	void OnWindowFinished();

	void FinishWithRecovery();
};
