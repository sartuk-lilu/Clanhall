#include "ClanhallHitboxTypes.h"
#include "Components/SkeletalMeshComponent.h"

bool FClanhallHitboxDesc::GetWorldTransform(const USkeletalMeshComponent* Mesh, FVector& OutLocation, FQuat& OutRotation) const
{
	if (!Mesh) return false;

	const bool bBoneExists = Mesh->GetBoneIndex(Bone) != INDEX_NONE || Mesh->DoesSocketExist(Bone);
	if (!bBoneExists) return false;

	const FTransform BoneTransform = Mesh->GetSocketTransform(Bone, RTS_World);
	// Смещение поворачивается трансформом кости, поворот домножается справа.
	OutLocation = BoneTransform.GetLocation() + BoneTransform.GetRotation().RotateVector(LocationOffset);
	OutRotation = BoneTransform.GetRotation() * FQuat(RotationOffset);
	return true;
}

FCollisionShape FClanhallHitboxDesc::MakeCollisionShape() const
{
	switch (Shape)
	{
	case EClanhallHitboxShape::Capsule:
		return FCollisionShape::MakeCapsule(Radius, GetEffectiveHalfHeight());
	case EClanhallHitboxShape::Box:
		return FCollisionShape::MakeBox(BoxExtent);
	case EClanhallHitboxShape::Sphere:
	default:
		return FCollisionShape::MakeSphere(Radius);
	}
}
