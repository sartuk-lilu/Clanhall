// Общий базовый класс для WASD-ударов (GA_DirectionalAttackBase) и активных навыков
// (GA_PhysicalSkill). Канон: все они работают только в боевой стойке (combat_system.md §3)
// и пока (до анимации в Разделе 6.5) находят цель одинаково — сферой перед персонажем.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "GA_ClanhallAbilityBase.generated.h"

class UAbilitySystemComponent;

UCLASS(Abstract)
class CLANHALL_API UGA_ClanhallAbilityBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ClanhallAbilityBase();

protected:
	/** Раздел 2-3: мгновенный поиск цели сферой перед персонажем вместо анимации/трейса по сокету оружия. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float TraceRange = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float TraceRadius = 75.0f;

	AActor* FindMeleeTarget(AActor* Avatar) const;

	/** combat_system.md §AP: снимает AP цели, 50% снятого возвращается атакующему в AP,
	 *  переполнение (урон больше остатка AP) идёт прямиком в HP. Возвращает true, если урон
	 *  был нанесён — это и есть "confirmed hit" для КД и синергий меток. */
	bool ResolveStandardDamage(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float RawDamage) const;
};
