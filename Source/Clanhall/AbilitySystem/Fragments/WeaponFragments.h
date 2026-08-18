// Фрагменты оружия. Единственный сейчас — Offhand: меч+щит держит в левой руке щит, дуалы —
// второй клинок, двуруч и копьё — ничего. Отсутствие фрагмента и есть ответ «в левой руке
// пусто» — ровно тот смысл, который не выражается пустым полем (CLAUDE.md, критерий
// «заголовок или фрагмент»).

#pragma once

#include "WeaponFragment.h"
#include "WeaponFragments.generated.h"

class UStaticMesh;

/** Живёт на экземпляре (UWeaponData::Fragments), не на типе. Меш — контент конкретного
 *  предмета, а тип задаёт форму, а не величину. Положить оффхенд на тип — ошибка, из-за
 *  которой все щиты этого типа станут одним щитом. */
UCLASS(meta = (DisplayName = "Offhand"))
class CLANHALL_API UWeaponOffhandFragment : public UWeaponFragment
{
	GENERATED_BODY()
public:
	/** Меш в левой руке: щит, второй клинок. Крепит Blueprint — потребителя в C++ нет. */
	UPROPERTY(EditAnywhere, Category = "Offhand")
	TObjectPtr<UStaticMesh> Mesh;
};
