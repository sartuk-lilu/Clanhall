#include "AnimNotifyState_Hitbox.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimMontage.h"

void UAnimNotifyState_Hitbox::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner) return;

	if (UClanhallHitboxComponent* HitboxComp = Owner->FindComponentByClass<UClanhallHitboxComponent>())
	{
		HitboxComp->BeginHitbox(Hitbox, this);
	}
}

void UAnimNotifyState_Hitbox::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner) return;

	if (UClanhallHitboxComponent* HitboxComp = Owner->FindComponentByClass<UClanhallHitboxComponent>())
	{
		HitboxComp->EndHitbox(this);
	}
}

FString UAnimNotifyState_Hitbox::GetNotifyName_Implementation() const
{
	FString ShapeName;
	switch (Hitbox.Shape)
	{
	case EClanhallHitboxShape::Capsule:
		ShapeName = TEXT("Капсула");
		break;
	case EClanhallHitboxShape::Box:
		ShapeName = TEXT("Бокс");
		break;
	case EClanhallHitboxShape::Sphere:
	default:
		ShapeName = TEXT("Сфера");
		break;
	}

	if (!Hitbox.bParryable)
	{
		return FString::Printf(TEXT("Hitbox [%s, без клэша]"), *ShapeName);
	}

	return FString::Printf(TEXT("Hitbox [%s]"), *ShapeName);
}

bool UAnimNotifyState_Hitbox::MontageHasHitbox(const UAnimMontage* Montage)
{
	if (!Montage)
	{
		return false;
	}

	for (const FAnimNotifyEvent& Event : Montage->Notifies)
	{
		if (Cast<UAnimNotifyState_Hitbox>(Event.NotifyStateClass))
		{
			return true;
		}
	}

	return false;
}
