#include "ClanhallWeaponActor.h"
#include "Components/StaticMeshComponent.h"

AClanhallWeaponActor::AClanhallWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	// Зоны поражения живут на нотифаях монтажа (ClanhallHitboxComponent) — физическая
	// коллизия меча в руке им только помешает.
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
