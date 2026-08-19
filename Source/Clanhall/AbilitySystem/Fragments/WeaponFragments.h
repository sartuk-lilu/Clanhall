// Фрагменты оружия. Единственный сейчас — Offhand: меч+щит держит в левой руке щит, дуалы —
// второй клинок, двуруч и копьё — ничего. Отсутствие фрагмента и есть ответ «в левой руке
// пусто» — ровно тот смысл, который не выражается пустым полем (CLAUDE.md, критерий
// «заголовок или фрагмент»).

#pragma once

#include "WeaponFragment.h"
#include "WeaponFragments.generated.h"

class AClanhallWeaponActor;

/** Живёт на экземпляре (UWeaponData::Fragments), не на типе. Класс актора — контент
 *  конкретного предмета, а тип задаёт форму, а не величину. Положить оффхенд на тип —
 *  ошибка, из-за которой все щиты этого типа станут одним щитом. */
UCLASS(meta = (DisplayName = "Offhand"))
class CLANHALL_API UWeaponOffhandFragment : public UWeaponFragment
{
	GENERATED_BODY()
public:
	/** Класс актора в левой руке: щит, второй клинок. Тот же критерий, что у основной руки
	 *  (`weapon_system.md`, «Оружие как актор») — сокет и трансформ несёт сам актор. */
	UPROPERTY(EditAnywhere, Category = "Offhand")
	TSubclassOf<AClanhallWeaponActor> OffhandClass;
};
