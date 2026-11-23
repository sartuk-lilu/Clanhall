// Расчёт попадания - одно место для всех источников физического урона: приём, отклик уклонения.
// Знает порог «противник × тип урона», удар в спину, крит, смертельный удар и что такое
// подтверждённое попадание. Каноны: `CombatSystem.md`, «Damage Threshold (DT)», «Удар в спину»,
// «Случайность», «AP», «Пробивший контакт сбивает зону получателя».
//
// Всё, что «срабатывает на попадании» (метки, контр, срыв насмешки), читает bConfirmed из
// результата - своей проверки попадания ни у кого больше нет.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallDamage.generated.h"

class AActor;
class UAbilitySystemComponent;

/** Что известно о попадании до расчёта. */
struct FClanhallHitContext
{
	AActor* SourceActor = nullptr;
	UAbilitySystemComponent* SourceASC = nullptr;
	AActor* TargetActor = nullptr;
	UAbilitySystemComponent* TargetASC = nullptr;

	/** Сырой урон приёма. Ноль и меньше при bUtility - утилита, попадание подтверждается целью. */
	float RawDamage = 0.0f;

	/** Утилита без урона (`CombatSystem.md`: «Утилита без урона подтверждает попадание фактом
	 *  цели, порог её не касается»). */
	bool bUtility = false;

	/** Тип урона экземпляра оружия (`Damage.Type.*`) - выбирает порог цели. */
	FGameplayTag DamageType;

	/** Пробитие: база класса + надбавка экземпляра, вычитается из порога плоско. */
	float ArmorPenetration = 0.0f;

	/** Множитель версии приёма: сильная версия по метке, удар из ножен. */
	float DamageMultiplier = 1.0f;

	/** Приём бьёт в спину мимо AP и порога (`CombatSystem.md`, «Удар в спину») - свойство приёма. */
	bool bBackstab = false;

	EClanhallLethalMode LethalMode = EClanhallLethalMode::None;

	/** Мгновенный эффект «удар пробивает порог» (`MarksSystem.md`, «Мгновенные эффекты»). */
	bool bIgnoreThreshold = false;
};

/** Что вышло из попадания. BlueprintType - уходит в делегат получателя OnHitReceived. */
USTRUCT(BlueprintType)
struct CLANHALL_API FClanhallHitResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Hit")
	float DamageToAP = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Hit")
	float DamageToHP = 0.0f;

	/** Подтверждённое попадание: нанесён ненулевой урон, или утилита нашла цель. */
	UPROPERTY(BlueprintReadOnly, Category = "Hit")
	bool bConfirmed = false;

	/** Урон целиком ушёл под порог - ноль, не попадание. */
	UPROPERTY(BlueprintReadOnly, Category = "Hit")
	bool bAbsorbed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Hit")
	bool bCrit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Hit")
	bool bLethal = false;

	/** Источник был за спиной цели (в секторе спины). */
	UPROPERTY(BlueprintReadOnly, Category = "Hit")
	bool bFromBack = false;

	/** Удар в спину сработал - мимо AP и порога. */
	UPROPERTY(BlueprintReadOnly, Category = "Hit")
	bool bBackstab = false;

	/** AP цели был больше нуля и стал нулём (`CombatSystem.md`, «AP»: сигнал пробития). */
	UPROPERTY(BlueprintReadOnly, Category = "Hit")
	bool bArmorBroken = false;

	UPROPERTY(BlueprintReadOnly, Category = "Hit")
	TObjectPtr<AActor> Source = nullptr;
};

namespace ClanhallDamage
{
	/** Расчёт и применение попадания. Порядок: спина → смертельный → множители и крит → удар в
	 *  спину → порог (пока у цели есть AP) → AP/HP. Потом - на получателе: сбитие его зон, если
	 *  попадание подтверждено; делегаты OnHitReceived и OnArmorBroken. */
	CLANHALL_API FClanhallHitResult ResolveHit(const FClanhallHitContext& Context);

	/** Источник в секторе спины цели (BackSectorDegrees из настроек боя). */
	CLANHALL_API bool IsBehind(const AActor* Target, const AActor* Source);
}
