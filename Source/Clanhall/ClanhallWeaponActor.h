// Оружие как актор, не меш (`weapon_system.md`, «Оружие как актор»). Голой геометрии
// некуда повесить горящий клинок, кровь на лезвии, звук или трейл — всё это компоненты,
// а компоненты живут на акторе.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ClanhallWeaponActor.generated.h"

class UStaticMeshComponent;

UCLASS(Abstract, Blueprintable)
class CLANHALL_API AClanhallWeaponActor : public AActor
{
	GENERATED_BODY()
public:
	AClanhallWeaponActor();

	/** Визуал оружия целиком. Blueprint-наследник вешает сюда VFX, звук, трейлы —
	 *  ради этого оружие и стало актором, а не мешом в поле ассета. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	/** Сокет на скелете носителя. Свойство актора, а не типа оружия: рукояти различаются
	 *  даже внутри одного типа (`weapon_system.md`, «Оружие как актор»). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName AttachSocketName;

	/** Доводка посадки в руке. Подгоняется глазами в Blueprint оружия. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FTransform AttachRelativeTransform;
};
