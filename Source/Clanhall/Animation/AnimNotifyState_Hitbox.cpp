#include "AnimNotifyState_Hitbox.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimMontage.h"

#if WITH_EDITOR
// Сами DrawWire*-хелперы живут в PrimitiveDrawingUtils.h (UE 5.8) и тянутся сюда транзитивно.
// Если после апгрейда движка они перестанут находиться — инклюдить PrimitiveDrawingUtils.h напрямую.
#include "SceneManagement.h"
#endif

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
		ShapeName = TEXT("Capsule");
		break;
	case EClanhallHitboxShape::Box:
		ShapeName = TEXT("Box");
		break;
	case EClanhallHitboxShape::Sphere:
	default:
		ShapeName = TEXT("Sphere");
		break;
	}

	if (!Hitbox.bParryable)
	{
		return FString::Printf(TEXT("Hitbox [%s, no clash]"), *ShapeName);
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

bool UAnimNotifyState_Hitbox::MontageOwnsNotify(const UAnimMontage* Montage, const UObject* Notify)
{
	if (!Montage || !Notify)
	{
		return false;
	}

	for (const FAnimNotifyEvent& Event : Montage->Notifies)
	{
		if (Event.NotifyStateClass == Notify)
		{
			return true;
		}
	}

	return false;
}

#if WITH_EDITOR
void UAnimNotifyState_Hitbox::DrawInEditor(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* MeshComp,
                                           const UAnimSequenceBase* Animation, const FAnimNotifyEvent& NotifyEvent) const
{
	if (!PDI || !MeshComp) return;

	FVector Location;
	FQuat   Rotation;
	if (!Hitbox.GetWorldTransform(MeshComp, Location, Rotation))
	{
		return;   // кости нет — молчим: вьюпорт зовёт это каждый кадр, лог бы залило
	}

	// Тот же код цвета, что у рантайм-дебага: cyan — зона участвует в клэше, orange — нет.
	const FLinearColor Color = Hitbox.bParryable ? FLinearColor(0.f, 1.f, 1.f) : FLinearColor(1.f, 0.5f, 0.f);

	const FMatrix Basis = FQuatRotationTranslationMatrix(Rotation, Location);
	const FVector X = Basis.GetScaledAxis(EAxis::X);
	const FVector Y = Basis.GetScaledAxis(EAxis::Y);
	const FVector Z = Basis.GetScaledAxis(EAxis::Z);

	switch (Hitbox.Shape)
	{
	case EClanhallHitboxShape::Capsule:
		// Ось капсулы — локальная Z, ровно как у FCollisionShape::MakeCapsule в свипе.
		DrawWireCapsule(PDI, Location, X, Y, Z, Color, Hitbox.Radius, Hitbox.GetEffectiveHalfHeight(), 16, SDPG_Foreground, 1.0f);
		break;
	case EClanhallHitboxShape::Box:
		DrawOrientedWireBox(PDI, Location, X, Y, Z, Hitbox.BoxExtent, Color, SDPG_Foreground, 1.0f);
		break;
	case EClanhallHitboxShape::Sphere:
	default:
		DrawWireSphere(PDI, FTransform(Rotation, Location), Color, Hitbox.Radius, 16, SDPG_Foreground, 1.0f);
		break;
	}
}
#endif
