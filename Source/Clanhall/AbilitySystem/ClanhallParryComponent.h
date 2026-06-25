// Компонент на Character-акторе (игрок/болванчик), который будет парироваться.
// Хранит состояние текущего окна парирования и флаг успешного ответа.
// GA_EnemyWASDSeries открывает окно (ResetParry + ApplyTimedTagToTarget), читает результат
// после истечения окна. Сам Character вызывает TryParry из обработчиков ввода WASD.

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ClanhallParryComponent.generated.h"

class UAbilitySystemComponent;

UCLASS(ClassGroup="Clanhall", meta=(BlueprintSpawnableComponent))
class CLANHALL_API UClanhallParryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** true, если в текущем окне игрок нажал правильное направление.
	 *  GA_EnemyWASDSeries читает это после истечения WindowDuration. */
	bool bParrySuccessful = false;

	/** Вызывается AI-способностью перед каждым ударом серии. */
	void ResetParry();

	/** Вызывается из обработчиков WASD-ввода игрока при наличии State.Parrying на ASC.
	 *  ParriableTag — тег Parry.Incoming.*, который эта клавиша перекрывает.
	 *  Например, W (Overhead) парирует Parry.Incoming.S (AI бьёт S). */
	void TryParry(FGameplayTag ParriableTag);

private:
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	UAbilitySystemComponent* GetASC();
};
