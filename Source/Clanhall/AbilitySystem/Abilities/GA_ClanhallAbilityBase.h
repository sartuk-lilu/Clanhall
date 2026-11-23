// Общий базовый класс физических способностей, наносящих урон (GA_PhysicalSkill).
// Основной путь урона — резолв по контакту зоны (`DataAssetAndFragments.md`, «Два режима
// резолва урона»). Поиск цели сферой перед персонажем (FindMeleeTarget) — геометрия
// мгновенного фолбэка: он работает, когда у навыка нет монтажа либо на монтаже не расставлен
// Hitbox-нотифай.

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
	/** Мгновенный поиск цели сферой перед персонажем. Не основной путь урона — активки
	 *  резолвят по контакту зоны, — а геометрия мгновенного фолбэка: у навыка нет монтажа
	 *  либо на монтаже не расставлен Hitbox-нотифай. Контр цель
	 *  отсюда не берёт: он резолвится по контакту, в GA_PhysicalSkill::ResolveHitOn. Поле не мёртвое. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float TraceRange = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float TraceRadius = 75.0f;

	AActor* FindMeleeTarget(AActor* Avatar) const;
};
