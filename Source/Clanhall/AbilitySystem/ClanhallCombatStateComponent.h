// Боевое состояние: вешает State.InCombat на ASC владельца, пока в радиусе есть живой
// противник, и снимает с задержкой после того, как последний покинул радиус —
// без задержки состояние мигало бы на границе (`combat_system.md`, «Боевое состояние»).
// Стороне-нейтрален: создаётся в AClanhallCombatantBase, а не в AClanhallCharacter,
// противнику он понадобится под A-life так же, как игроку. Геометрия поиска и гистерезис
// EnterRadius/ExitRadius — по образцу UClanhallBossSensorComponent::UpdateTrackedUnits,
// он уже решает ровно ту же задачу.

#pragma once

#include "Components/ActorComponent.h"
#include "ClanhallCombatStateComponent.generated.h"

UCLASS(ClassGroup="Clanhall", meta=(BlueprintSpawnableComponent))
class CLANHALL_API UClanhallCombatStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClanhallCombatStateComponent();

	/** Дистанция, на которой противник засчитывается вошедшим в бой. Плейсхолдер — ставит разработчик. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState")
	float EnterRadius = 800.0f;

	/** Дистанция, на которой противник перестаёт засчитываться (гистерезис, должна быть больше EnterRadius). Плейсхолдер. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState")
	float ExitRadius = 1000.0f;

	/** Задержка снятия State.InCombat после того, как последний противник покинул ExitRadius —
	 *  вторая половина защиты от мигания на границе, первая — сам гистерезис выше. Плейсхолдер. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatState")
	float ExitDelay = 4.0f;

	bool IsInCombat() const { return bInCombat; }
	int32 GetTrackedEnemyCount() const { return TrackedEnemies.Num(); }

	/** 0, если не в бою или противник всё ещё в радиусе — таймер идёт только когда радиус пуст. */
	float GetExitTimeRemaining() const;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void UpdateTrackedEnemies();
	void SetInCombat(bool bNewInCombat);

	TSet<TWeakObjectPtr<AActor>> TrackedEnemies;
	float TimeSinceEmpty = 0.0f;
	bool bInCombat = false;
};
