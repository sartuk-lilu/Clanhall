// Раздел 5: GameplayAbility для AI-серии WASD-ударов с окном парирования.
// Один класс — одна серия. Конкретные серии задаются через AttackDirections:
//   Parry.Incoming.A + Parry.Incoming.D = «Перекрёстный» A→D Часового.
// Логика: для каждого удара открывается окно WindowDuration — игрок должен
// нажать обратное направление (UClanhallParryComponent::TryParry).
// Все удары отпарированы → AI оглушён + КД игрока −CDReduction сек.
// Пропущен хоть один → только урон, без оглушения.

#pragma once

#include "GA_ClanhallAbilityBase.h"
#include "GameplayTagContainer.h"
#include "GA_EnemyWASDSeries.generated.h"

class UClanhallParryComponent;
class UAbilityTask_WaitDelay;

UCLASS()
class CLANHALL_API UGA_EnemyWASDSeries : public UGA_ClanhallAbilityBase
{
	GENERATED_BODY()

public:
	UGA_EnemyWASDSeries();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** Теги Parry.Incoming.* по порядку ударов серии. */
	UPROPERTY(EditDefaultsOnly, Category = "Series", meta = (Categories = "Parry.Incoming"))
	TArray<FGameplayTag> AttackDirections;

	/** Длительность окна парирования (сек). Также время, на которое вешается State.Parrying. */
	UPROPERTY(EditDefaultsOnly, Category = "Series")
	float WindowDuration = 0.5f;

	/** Пауза между ударами серии (сек). */
	UPROPERTY(EditDefaultsOnly, Category = "Series")
	float DelayBetweenHits = 0.6f;

	/** Урон по AP на каждый пропущенный удар (combat_system.md §AP). */
	UPROPERTY(EditDefaultsOnly, Category = "Series")
	float HitDamage = 30.0f;

	/** Длительность оглушения AI при полном парировании серии (канон: 2 сек). */
	UPROPERTY(EditDefaultsOnly, Category = "Series")
	float StunDuration = 2.0f;

	/** Снижение КД навыков игрока при полном парировании (канон: −5 сек). */
	UPROPERTY(EditDefaultsOnly, Category = "Series")
	float CDReduction = 5.0f;

private:
	int32 CurrentHitIndex = 0;
	int32 ParriedCount = 0;

	TWeakObjectPtr<UAbilitySystemComponent> TargetASC;
	TWeakObjectPtr<UClanhallParryComponent> TargetParryComponent;

	/** Подготовить удар CurrentHitIndex: показать индикатор, открыть окно, запустить таймер. */
	UFUNCTION()
	void PrepareHit();

	/** Вызывается по истечении окна парирования. */
	UFUNCTION()
	void OnWindowExpired();

	/** Вызывается по истечении паузы между ударами. */
	UFUNCTION()
	void OnInterHitPauseExpired();

	void FinalizeSeries();

	/** Снизить КД навыков игрока на CDReduction сек.
	 *  Находит активные эффекты с Cooldown.* тегом, удаляет и перевешивает с меньшей длительностью. */
	void ReducePlayerCooldowns(UAbilitySystemComponent* PlayerASC);
};

/** Предустановленная серия «Перекрёстный» A→D (Часовой, Раздел 5 прототип).
 *  Привязывается к Training Dummy напрямую — без редакторских шагов. */
UCLASS()
class CLANHALL_API UGA_Series_Crosscut : public UGA_EnemyWASDSeries
{
	GENERATED_BODY()
public:
	UGA_Series_Crosscut();
};
