// Описание активной зоны поражения. Задаётся на нотифае монтажа, передаётся в
// UClanhallHitboxComponent::BeginHitbox. Компонент собственной геометрии не имеет —
// вся форма приходит из данных анимации (main_dev_plan.md §7, Порция C).

#pragma once

#include "ClanhallHitboxTypes.generated.h"

UENUM(BlueprintType)
enum class EClanhallHitboxShape : uint8
{
	Sphere  UMETA(DisplayName = "Сфера"),
	Capsule UMETA(DisplayName = "Капсула"),
	Box     UMETA(DisplayName = "Бокс")
};

USTRUCT(BlueprintType)
struct FClanhallHitboxDesc
{
	GENERATED_BODY()

	/** Кость или сокет крепления зоны. Дефолт воспроизводит поведение до Порции C. */
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
	 *  false — активки Q/E/R/F: зона бьёт, но парирование не проверяет. Это и закрывает п.31:
	 *          протухшее CurrentDirection от последнего WASD-удара до проверки не доходит. */
	UPROPERTY(EditAnywhere, Category = "Hitbox")
	bool bParryable = true;
};
