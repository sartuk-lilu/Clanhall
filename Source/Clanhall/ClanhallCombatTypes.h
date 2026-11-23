// Общие типы боевой системы, не привязанные к конкретному классу абилок — вынесены сюда, чтобы
// избежать взаимозависимостей заголовков.

#pragma once

#include "UObject/ObjectMacros.h"
#include "ClanhallCombatTypes.generated.h"

/** Трек метки (`MarksSystem.md`, «Концепция»): физический трек кладут и снимают только
 *  физические активки, магический — только заклинания. Один боец несёт по одной метке
 *  на каждый трек одновременно, независимо друг от друга. */
UENUM(BlueprintType)
enum class EClanhallMarkTrack : uint8
{
	Physical,
	Magic
};

/** Направление уклонения (`CombatSystem.md`, «Уклонение»). Смещение
 *  капсулы, не направление ввода - уводит тело от корпуса, а не от камеры (см.
 *  AClanhallCharacter::TriggerEvade). */
UENUM(BlueprintType)
enum class EClanhallEvadeDirection : uint8
{
	Left,
	Right,
	ForwardDash
};

/** Может ли приём стать смертельным ударом (`CombatSystem.md`, «Случайность») - свойство приёма.
 *  У атакующих приёмов кинжала - только со спины (`AbilitySystem.md`, «Механика школы оружия»). */
UENUM(BlueprintType)
enum class EClanhallLethalMode : uint8
{
	None          UMETA(DisplayName = "Never"),
	Always        UMETA(DisplayName = "Always"),
	FromBackOnly  UMETA(DisplayName = "From Back Only")
};
