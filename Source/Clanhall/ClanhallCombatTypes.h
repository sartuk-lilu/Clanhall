// Общие типы боевой системы, не привязанные к конкретному классу абилок — вынесены сюда, чтобы
// избежать взаимозависимостей заголовков.

#pragma once

#include "UObject/ObjectMacros.h"
#include "ClanhallCombatTypes.generated.h"

/** Вид метки (`MarksSystem.md`, «Уровни меток»): на цели - дебафф, на себе - бафф; на третьем
 *  уровне дебафф называется травмой, бафф - преимуществом. */
UENUM(BlueprintType)
enum class EClanhallMarkKind : uint8
{
	Debuff     UMETA(DisplayName = "Debuff"),
	Buff       UMETA(DisplayName = "Buff"),
	Trauma     UMETA(DisplayName = "Trauma"),
	Advantage  UMETA(DisplayName = "Advantage")
};

/** На кого приём кладёт метку (`MarksSystem.md`, «Слоты меток»: либо на цель, либо на себя, не на
 *  обоих сразу). На этом же держателе приём читает метку своего условия. */
UENUM(BlueprintType)
enum class EClanhallMarkTarget : uint8
{
	Enemy  UMETA(DisplayName = "Enemy"),
	Self   UMETA(DisplayName = "Self")
};

/** Мгновенный эффект версии навыка (`MarksSystem.md`, «Мгновенные эффекты»): срабатывает один раз
 *  в момент попадания и ничего не оставляет висеть. Набор расширяется по мере приёмов. */
UENUM(BlueprintType)
enum class EClanhallInstantEffect : uint8
{
	RemoveOwnTrauma        UMETA(DisplayName = "Remove Own Trauma"),
	RemoveTargetAdvantage  UMETA(DisplayName = "Remove Target Advantage"),
	PierceThreshold        UMETA(DisplayName = "Pierce Threshold")
};

/** Направление уклонения (`CombatSystem.md`, «Уклонение»): «Пробел + направление». Смещение
 *  капсулы от корпуса, а не от камеры и не по вектору ввода (см. AClanhallCharacter::TriggerEvade).
 *  Отскока «назад по умолчанию» нет: без направления уклонения нет. */
UENUM(BlueprintType)
enum class EClanhallEvadeDirection : uint8
{
	Left,
	Right,
	Forward,
	Back
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

/** Слой ввода (`Controls.md`, «Ввод»): зажатая кнопка мыши выбирает, кто действует, клавиша - что.
 *  ЛКМ - тело (приёмы), ПКМ - рот (слоги); обе зажаты - главная последняя зажатая. */
UENUM(BlueprintType)
enum class EClanhallInputLayer : uint8
{
	None   UMETA(DisplayName = "None"),
	Body   UMETA(DisplayName = "Body (skills)"),
	Mouth  UMETA(DisplayName = "Mouth (syllables)")
};

/** Механика школы оружия (`AbilitySystem.md`, «Механика школы оружия») - поле класса оружия, а не
 *  приёма. В прототипе две: ножны длинного меча и спина кинжала; остальные школы добавятся с
 *  классами. */
UENUM(BlueprintType)
enum class EClanhallSchoolMechanic : uint8
{
	None      UMETA(DisplayName = "None"),
	Scabbard  UMETA(DisplayName = "Scabbard Strike"),
	Backstab  UMETA(DisplayName = "Backstab")
};
