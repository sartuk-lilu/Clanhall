// Общие типы боевой системы, не привязанные к конкретному классу абилок.
// EClanhallAttackDirection используется в GA_DirectionalAttackBase, ClanhallParryComponent
// и ClanhallHitboxComponent — вынесен сюда, чтобы избежать взаимозависимостей заголовков.

#pragma once

#include "UObject/ObjectMacros.h"
#include "ClanhallCombatTypes.generated.h"

UENUM(BlueprintType)
enum class EClanhallAttackDirection : uint8
{
	Overhead,	// W
	RightSlash,	// D
	LeftSlash,	// A
	LowSweep	// S
};
