// Общий базовый класс для WASD-ударов (GA_DirectionalAttackBase) и активных навыков
// (GA_PhysicalSkill). Канон: все они работают только в боевой стойке (combat_system.md §3).
// Поиск цели сферой перед персонажем (FindMeleeTarget) — основной путь урона у активок
// (Q/E/R/F резолвят на активации) и у WASD-ударов без Hitbox-разметки; у размеченных
// WASD-ударов основной путь — резолв по контакту зоны (main_dev_plan.md §7, Порция D).

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
	/** Раздел 2-3: мгновенный поиск цели сферой перед персонажем. После Порции D — уже не
	 *  основной путь урона у WASD-ударов (тот резолвится по контакту зоны, см.
	 *  GA_DirectionalAttackBase), а геометрия ДВУХ оставшихся путей: мгновенный фолбэк, когда на
	 *  шаге нет монтажа/Hitbox-нотифая, и поиск цели для контрнавыка в GA_PhysicalSkill (активки
	 *  Q/E/R/F урон резолвят на активации, как раньше). Поле не мёртвое. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float TraceRange = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float TraceRadius = 75.0f;

	AActor* FindMeleeTarget(AActor* Avatar) const;

	/** combat_system.md §AP: снимает AP цели, 50% снятого возвращается атакующему в AP,
	 *  переполнение (урон больше остатка AP) идёт прямиком в HP. Возвращает true, если урон
	 *  был нанесён — это и есть "confirmed hit", который гейтит метки/синергии/Balance у активок
	 *  (GA_PhysicalSkill). КД активок больше НЕ зависит от confirmed hit — уходит на активации. */
	bool ResolveStandardDamage(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float RawDamage) const;

	/** combat_system.md §2: STR-оружие двигает шкалу вправо, DEX — влево.
	 *  Знак определяется ТОЛЬКО типом оружия, в данных навыка не хранится. */
	float GetBalanceSign(const UAbilitySystemComponent* SourceASC) const;
};
