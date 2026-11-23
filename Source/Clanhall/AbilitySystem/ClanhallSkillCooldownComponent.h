// Перезарядка приёмов бойца (`EconomySystem.md`, «Перезарядка приёмов»). Гейт приёма - его
// собственная перезарядка, заданная в данных (UAbilityData::CooldownBeats); общего отката нет.
//
// Свой компонент, а не Cooldown GameplayEffect: ключ перезарядки - идентичность приёма
// (UAbilityData::CounterTag), а не спек. Приёмы всего лоадаута гранятся разом, и перезарядка
// обязана идти у всего оружия независимо от того, какой ряд выбран; GE-кулдаун потребовал бы
// нативного тега на каждый приём. Панель читает остаток в долях напрямую.
//
// Живёт на AClanhallCharacterBase: противник перезаряжается по тем же правилам.

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ClanhallSkillCooldownComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnClanhallCooldownStarted, FGameplayTag, Skill, float, Beats);

UCLASS(ClassGroup = "Clanhall", meta = (BlueprintSpawnableComponent))
class CLANHALL_API UClanhallSkillCooldownComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClanhallSkillCooldownComponent();

	/** Запустить перезарядку приёма на Beats долей. Ноль и меньше - ничего не делает (приём
	 *  без перезарядки законен для отладки). Повторный старт перезаписывает остаток. */
	void StartCooldown(FGameplayTag Skill, float Beats);

	UFUNCTION(BlueprintPure, Category = "Clanhall|Cooldown")
	bool IsOnCooldown(FGameplayTag Skill) const;

	/** Остаток в долях, 0 - готов. */
	UFUNCTION(BlueprintPure, Category = "Clanhall|Cooldown")
	float GetRemainingBeats(FGameplayTag Skill) const;

	/** Полная длина последней запущенной перезарядки приёма в долях - знаменатель для
	 *  оверлея на иконке. 0 - приём ни разу не перезаряжался. */
	UFUNCTION(BlueprintPure, Category = "Clanhall|Cooldown")
	float GetTotalBeats(FGameplayTag Skill) const;

	/** Все приёмы, у которых сейчас идёт перезарядка - для отладки. */
	TArray<FGameplayTag> GetActiveCooldowns() const;

	UPROPERTY(BlueprintAssignable, Category = "Clanhall|Cooldown")
	FOnClanhallCooldownStarted OnCooldownStarted;

private:
	struct FCooldownEntry
	{
		double ReadyAt = 0.0;     // время мира, когда приём готов
		float TotalBeats = 0.0f;  // длина последнего запуска
	};

	double Now() const;

	TMap<FGameplayTag, FCooldownEntry> Cooldowns;
};
