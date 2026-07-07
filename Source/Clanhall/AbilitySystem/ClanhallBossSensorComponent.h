// changelog_enemyframe_unitroles.md §3: заменяет рейкаст как драйвер Enemy Frame.
// Держит множество актуальных «юнитов под рамку» (Unit.Role.Boss.*) в радиусе игрока
// и вещает вход/выход через делегаты. UClanhallTargetingComponent остаётся мягкой целью
// под удар/метку и рамку больше не водит.

#pragma once

#include "Components/ActorComponent.h"
#include "ClanhallBossSensorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFrameUnitEntered, AActor*, Unit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFrameUnitExited, AActor*, Unit);

UCLASS(ClassGroup="Clanhall", meta=(BlueprintSpawnableComponent))
class CLANHALL_API UClanhallBossSensorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClanhallBossSensorComponent();

	/** Дистанция, на которой юнит добавляется под рамку. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BossSensor")
	float EnterRadius = 1200.0f;

	/** Дистанция, на которой юнит убирается из-под рамки. Должна быть больше EnterRadius (гистерезис). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BossSensor")
	float ExitRadius = 1800.0f;

	/** Срабатывает, когда новый босс попал в EnterRadius. Виджет-контейнер создаёт под него WBP_EnemyFrame. */
	UPROPERTY(BlueprintAssignable, Category="BossSensor")
	FOnFrameUnitEntered OnFrameUnitEntered;

	/** Срабатывает, когда отслеживаемый босс вышел за ExitRadius или стал невалиден. */
	UPROPERTY(BlueprintAssignable, Category="BossSensor")
	FOnFrameUnitExited OnFrameUnitExited;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void UpdateTrackedUnits();

	TSet<TWeakObjectPtr<AActor>> TrackedUnits;
};
