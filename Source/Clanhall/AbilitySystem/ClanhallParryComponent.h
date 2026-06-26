// Компонент на Character-акторе, который может парировать атаки врага.
// Хранит флаг bParrySuccessful, который GA_EnemyWASDSeries читает после истечения
// окна и решает: оглушить врага или нанести урон.
//
// Раздел 5 (placeholder): TryParry вызывался из обработчиков ввода WASD персонажа.
// Раздел 6.5 (финал):    TryParry вызывается из UClanhallWeaponTraceComponent::CheckAndHandleParry
//                         когда weapon trace игрока задевает врага со State.Parrying.

#pragma once

#include "Components/ActorComponent.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallParryComponent.generated.h"

class UAbilitySystemComponent;
class USoundBase;

UCLASS(ClassGroup="Clanhall", meta=(BlueprintSpawnableComponent))
class CLANHALL_API UClanhallParryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** true, если в текущем окне weapon trace игрока задел врага в нужном направлении.
	 *  GA_EnemyWASDSeries читает это после истечения WindowDuration. */
	bool bParrySuccessful = false;

	/** Звук столкновения оружий (воспроизводится TryParry при успехе). */
	UPROPERTY(EditAnywhere, Category="VFX")
	TObjectPtr<USoundBase> ClashSound;

	/** Вызывается AI-способностью перед каждым ударом серии. */
	void ResetParry();

	/** Вызывается из UClanhallWeaponTraceComponent::CheckAndHandleParry.
	 *  Проверяет направление удара игрока против Parry.Incoming.* тега на своём ASC.
	 *  Возвращает true при успешном парировании. */
	bool TryParry(AActor* HitEnemy, EClanhallAttackDirection PlayerDirection, FVector HitLocation);

private:
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	UAbilitySystemComponent* GetASC();
};
