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

/** Режим ввода: одна и та же клавиша значит разное в зависимости от зажатой кнопки мыши
 *  (`combat_system.md`). Cast зарезервирован под магию на ПКМ и сегодня
 *  недостижим - ПКМ ни к чему не привязана. Заведён сразу, чтобы приход магии не потребовал
 *  переразводки всех гейтов ввода. */
UENUM(BlueprintType)
enum class EClanhallInputMode : uint8
{
	Free    UMETA(DisplayName = "Free"),
	Attack  UMETA(DisplayName = "Attack"),
	Cast    UMETA(DisplayName = "Cast")
};

/** Направление защитного действия вне стойки (`combat_system.md`, «Отскок»). Смещение
 *  капсулы, не направление ввода - уводит тело от корпуса, а не от камеры (см.
 *  AClanhallCharacter::TriggerEvade). */
UENUM(BlueprintType)
enum class EClanhallEvadeDirection : uint8
{
	Left,
	Right,
	ForwardDash
};
