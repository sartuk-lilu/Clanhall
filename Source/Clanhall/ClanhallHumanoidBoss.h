// Первый AI-боец на данных игрока (main_dev_plan.md §8, Блок A). Сам класс — только
// иерархия: ASC/атрибуты/метки/зоны/контр — от AClanhallCombatantBase, комбо-дерево/
// парирование/слоты навыков — от AClanhallHumanoidCombatant. AIController, Behavior Tree
// и исполнитель боевых фаз приезжают в Блоке G — здесь их сознательно ещё нет.

#pragma once

#include "CoreMinimal.h"
#include "ClanhallHumanoidCombatant.h"
#include "ClanhallHumanoidBoss.generated.h"

UCLASS()
class AClanhallHumanoidBoss : public AClanhallHumanoidCombatant
{
	GENERATED_BODY()

public:
	AClanhallHumanoidBoss();
};
