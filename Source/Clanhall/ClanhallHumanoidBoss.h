// Первый AI-боец на данных игрока. Сам класс — только
// иерархия: ASC/атрибуты/метки/зоны/контр — от AClanhallCombatantBase, комбо-дерево/
// парирование/слоты навыков — от AClanhallHumanoidCombatant. AIController, Behavior Tree
// и исполнитель боевых фаз приедут позже — здесь их сознательно ещё нет.

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
