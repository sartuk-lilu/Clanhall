// Описание активной зоны поражения. Задаётся на нотифае монтажа, передаётся в
// UClanhallHitboxComponent::BeginHitbox. Компонент собственной геометрии не имеет —
// вся форма приходит из данных анимации (main_dev_plan.md §7).

#pragma once

#include "CoreMinimal.h"
#include "CollisionShape.h"
#include "ClanhallHitboxTypes.generated.h"

class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class EClanhallHitboxShape : uint8
{
	Sphere,
	Capsule,
	Box
};

USTRUCT(BlueprintType)
struct FClanhallHitboxDesc
{
	GENERATED_BODY()

	/** Кость или сокет крепления зоны. Дефолт воспроизводит прежнее захардкоженное крепление
	 *  (WeaponSocket). */
	UPROPERTY(EditAnywhere, Category = "Hitbox")
	FName Bone = FName("WeaponSocket");

	/** Смещение зоны относительно кости, в пространстве кости. */
	UPROPERTY(EditAnywhere, Category = "Hitbox")
	FVector LocationOffset = FVector::ZeroVector;

	/** Поворот зоны относительно кости. Для капсулы задаёт направление оси (ось капсулы —
	 *  локальная Z результирующего поворота): капсула вдоль клинка требует поворота. */
	UPROPERTY(EditAnywhere, Category = "Hitbox")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	EClanhallHitboxShape Shape = EClanhallHitboxShape::Sphere;

	/** Радиус сферы и капсулы. */
	UPROPERTY(EditAnywhere, Category = "Hitbox", meta = (ClampMin = "0.0", EditCondition = "Shape != EClanhallHitboxShape::Box", EditConditionHides))
	float Radius = 20.0f;

	/** Половина высоты капсулы (полная высота = 2 * HalfHeight, включая полусферы). */
	UPROPERTY(EditAnywhere, Category = "Hitbox", meta = (ClampMin = "0.0", EditCondition = "Shape == EClanhallHitboxShape::Capsule", EditConditionHides))
	float HalfHeight = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Hitbox", meta = (EditCondition = "Shape == EClanhallHitboxShape::Box", EditConditionHides))
	FVector BoxExtent = FVector(20.0f, 20.0f, 20.0f);

	/** Участвует ли этот взмах в клэше парирования (ability_system.md §2: WASD парируют WASD,
	 *  активки контрят активки — активка в клэше не участвует вообще).
	 *  true  — WASD-удары: попадание по актору со State.Parrying идёт в UClanhallParryComponent.
	 *  false — активки Q/E/R/F: зона бьёт, но парирование не проверяет. Так протухшее
	 *          CurrentDirection от последнего WASD-удара до проверки не доходит вовсе —
	 *          для таких зон проверка парирования не вызывается. */
	UPROPERTY(EditAnywhere, Category = "Hitbox")
	bool bParryable = true;

	/** Мировой трансформ зоны на текущей позе меша. Единая точка для свипа в рантайме и для
	 *  превью в редакторе монтажей: разойдись они — превью показывало бы не то, что бьёт.
	 *  false = кости/сокета с именем Bone на меше нет. Логировать или молчать решает
	 *  вызывающий: компонент логирует один раз, вьюпорт молчит (он рисует каждый кадр). */
	bool GetWorldTransform(const USkeletalMeshComponent* Mesh, FVector& OutLocation, FQuat& OutRotation) const;

	/** HalfHeight < Radius — вырожденная капсула; поля независимы, кросс-полевой ClampMin
	 *  в метаданных не выражается, поэтому зажимаем здесь. */
	float GetEffectiveHalfHeight() const { return FMath::Max(HalfHeight, Radius); }

	/** Форма для свипа. Sphere — дефолт и фолбэк. */
	FCollisionShape MakeCollisionShape() const;
};
