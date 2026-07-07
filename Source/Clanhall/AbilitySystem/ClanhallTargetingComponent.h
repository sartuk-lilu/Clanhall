// Компонент на AClanhallCharacter. Делает line trace из камеры вперёд каждые 0.05 с
// и хранит CurrentTarget — валидна только если актор реализует IAbilitySystemInterface.
// HUD-виджет подписывается на OnTargetChanged для обновления Enemy Frame.

#pragma once

#include "Components/ActorComponent.h"
#include "ClanhallTargetingComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClanhallTargetChanged, AActor*, NewTarget);

UCLASS(ClassGroup="Clanhall", meta=(BlueprintSpawnableComponent))
class CLANHALL_API UClanhallTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClanhallTargetingComponent();

	/** Дальность луча в см (hud_system.md: 20 м). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Targeting")
	float MaxRange = 2000.0f;

	/** Текущая цель. nullptr = цели нет, Enemy Frame скрыта. */
	UPROPERTY(BlueprintReadOnly, Category="Targeting")
	TObjectPtr<AActor> CurrentTarget;

	/** Срабатывает при смене цели (включая nullptr). Виджет биндится в EventConstruct. */
	UPROPERTY(BlueprintAssignable, Category="Targeting")
	FOnClanhallTargetChanged OnTargetChanged;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void UpdateTarget();
	TWeakObjectPtr<AActor> LastTarget;
};
