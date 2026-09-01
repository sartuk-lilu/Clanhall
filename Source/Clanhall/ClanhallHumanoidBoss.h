// Первый AI-боец на данных игрока (`Character Hierarchy.md`, «AClanhallHumanoidBoss»). Сам класс — только
// иерархия: ASC/атрибуты/метки/зоны/контр — от AClanhallCharacterBase, комбо-дерево/
// парирование/слоты навыков — от AClanhallHumanoidBase. AIController, Behavior Tree
// и исполнитель боевых фаз приедут позже — здесь их сознательно ещё нет.

#pragma once

#include "CoreMinimal.h"
#include "ClanhallHumanoidBase.h"
#include "ClanhallHumanoidBoss.generated.h"

UCLASS()
class AClanhallHumanoidBoss : public AClanhallHumanoidBase
{
	GENERATED_BODY()

public:
	AClanhallHumanoidBoss();
};
