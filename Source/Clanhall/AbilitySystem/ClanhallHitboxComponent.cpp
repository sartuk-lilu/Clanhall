#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "Clanhall.h"
#include "Animation/AnimNotifyState_Hitbox.h"
#include "Animation/AnimMontage.h"
#include "AbilitySystem/ClanhallParryComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

UClanhallHitboxComponent::UClanhallHitboxComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UClanhallHitboxComponent::SetCurrentDirection(EClanhallAttackDirection Dir)
{
	CurrentDirection = Dir;
}

int32 UClanhallHitboxComponent::BeginHitbox(const FClanhallHitboxDesc& Desc, const UObject* Source)
{
	if (!Source)
	{
		return 0;
	}

	const int32 Handle = NextHitboxHandle++;

	FActiveHitbox NewBox;
	NewBox.Handle = Handle;
	NewBox.Desc = Desc;
	NewBox.Source = Source;
	ActiveHitboxes.Add(MoveTemp(NewBox));

	SetComponentTickEnabled(true);
	return Handle;
}

void UClanhallHitboxComponent::EndHitbox(const UObject* Source)
{
	const bool bWasActive = ActiveHitboxes.Num() > 0;

	ActiveHitboxes.RemoveAll([Source](const FActiveHitbox& Box)
	{
		return Box.Source.Get() == Source;
	});

	if (bWasActive && ActiveHitboxes.Num() == 0)
	{
		SetComponentTickEnabled(false);
		NotifyAllHitboxesClosed();
	}
}

void UClanhallHitboxComponent::EndHitboxesFromMontage(const UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	const bool bWasActive = ActiveHitboxes.Num() > 0;

	const int32 Removed = ActiveHitboxes.RemoveAll([Montage](const FActiveHitbox& Box)
	{
		return UAnimNotifyState_Hitbox::MontageOwnsNotify(Montage, Box.Source.Get());
	});

	if (Removed > 0)
	{
		// Ненулевой Removed означает, что NotifyEnd не пришёл — зона дожила до конца
		// способности. Это не норма, а диагностируемая ошибка разметки/блендаута: без варна
		// она была бы невидимой, и «удар бьёт после конца анимации» искали бы в логике урона.
		UE_LOG(LogClanhall, Warning, TEXT("EndHitboxesFromMontage: %d leaked hitbox(es) from %s closed by ability EndAbility — NotifyEnd was likely eaten by montage blend-out, move the notify end 1-2 frames earlier"),
			Removed, *Montage->GetName());
	}

	if (bWasActive && ActiveHitboxes.Num() == 0)
	{
		SetComponentTickEnabled(false);
		NotifyAllHitboxesClosed();
	}
}

void UClanhallHitboxComponent::EndAllHitboxes()
{
	const bool bWasActive = ActiveHitboxes.Num() > 0;

	ActiveHitboxes.Empty();
	SetComponentTickEnabled(false);

	if (bWasActive)
	{
		NotifyAllHitboxesClosed();
	}
}

void UClanhallHitboxComponent::NotifyAllHitboxesClosed()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag   = ClanhallGameplayTags::Event_Hitbox_Closed.GetTag();
	Payload.Instigator = Owner;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, Payload.EventTag, Payload);
}

void UClanhallHitboxComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ActiveHitboxes.Num() == 0)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	ACharacter* Char = Cast<ACharacter>(Owner);
	USkeletalMeshComponent* Mesh = Char ? Char->GetMesh() : Owner->FindComponentByClass<USkeletalMeshComponent>();
	if (!Mesh)
	{
		return;
	}

	// Снапшот хендлов: колбэк одной зоны может закрыть/открыть другие, поэтому итератор
	// по ActiveHitboxes через вызов TickHitbox держать нельзя.
	TArray<int32, TInlineAllocator<4>> HandlesThisTick;
	HandlesThisTick.Reserve(ActiveHitboxes.Num());
	for (const FActiveHitbox& Box : ActiveHitboxes)
	{
		HandlesThisTick.Add(Box.Handle);
	}

	for (const int32 Handle : HandlesThisTick)
	{
		const int32 Index = ActiveHitboxes.IndexOfByPredicate([Handle](const FActiveHitbox& B)
		{
			return B.Handle == Handle;
		});
		if (Index == INDEX_NONE)
		{
			continue;   // зону закрыли из колбэка соседней зоны в этом же тике
		}
		TickHitbox(ActiveHitboxes[Index], Mesh);
	}
}

void UClanhallHitboxComponent::TickHitbox(FActiveHitbox& Box, USkeletalMeshComponent* Mesh)
{
	FVector CurrentLocation;
	FQuat CurrentRotation;
	if (!Box.Desc.GetWorldTransform(Mesh, CurrentLocation, CurrentRotation))
	{
		if (!Box.bLoggedMissingBone)
		{
			UE_LOG(LogClanhall, Warning, TEXT("UClanhallHitboxComponent: bone/socket '%s' not found on %s, hitbox skipped."), *Box.Desc.Bone.ToString(), *GetNameSafe(GetOwner()));
			Box.bLoggedMissingBone = true;
		}
		return;
	}

	if (!Box.bHasPrevTransform)
	{
		// Первый тик зоны — свип от несуществующей предыдущей позиции дал бы ложное попадание.
		Box.LastLocation = CurrentLocation;
		Box.LastRotation = CurrentRotation;
		Box.bHasPrevTransform = true;
		return;
	}

	// Копии для фазы оповещения ниже — Box станет небезопасен для чтения после неё.
	const bool bParryable = Box.Desc.bParryable;
	const int32 HitboxHandle = Box.Handle;

	const FCollisionShape Shape = Box.Desc.MakeCollisionShape();

	AActor* Owner = GetOwner();
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ClanhallHitbox), false, Owner);

	GetWorld()->SweepMultiByObjectType(
		Hits,
		Box.LastLocation,
		CurrentLocation,
		CurrentRotation,
		FCollisionObjectQueryParams(ECC_Pawn),
		Shape,
		Params
	);

	struct FPendingHit
	{
		AActor* Actor = nullptr;
		FVector ImpactPoint = FVector::ZeroVector;
	};
	TArray<FPendingHit, TInlineAllocator<4>> PendingHits;

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == Owner)
		{
			continue;
		}

		const bool bAlreadyHit = Box.AlreadyHit.ContainsByPredicate([HitActor](const TWeakObjectPtr<AActor>& Weak)
		{
			return Weak.Get() == HitActor;
		});
		if (bAlreadyHit)
		{
			continue;
		}

		Box.AlreadyHit.Add(HitActor);
		PendingHits.Add(FPendingHit{HitActor, Hit.ImpactPoint});
	}

#if !UE_BUILD_SHIPPING
	if (bDrawDebugHitboxes)
	{
		const FColor Color = Box.Desc.bParryable ? FColor::Cyan : FColor::Orange;
		DrawDebugLine(GetWorld(), Box.LastLocation, CurrentLocation, Color, false, 0.0f, SDPG_Foreground, 1.0f);
		switch (Box.Desc.Shape)
		{
		case EClanhallHitboxShape::Capsule:
			DrawDebugCapsule(GetWorld(), CurrentLocation, Box.Desc.GetEffectiveHalfHeight(), Box.Desc.Radius, CurrentRotation, Color, false, 0.0f, SDPG_Foreground);
			break;
		case EClanhallHitboxShape::Box:
			DrawDebugBox(GetWorld(), CurrentLocation, Box.Desc.BoxExtent, CurrentRotation, Color, false, 0.0f, SDPG_Foreground);
			break;
		case EClanhallHitboxShape::Sphere:
		default:
			DrawDebugSphere(GetWorld(), CurrentLocation, Box.Desc.Radius, 16, Color, false, 0.0f, SDPG_Foreground);
			break;
		}
		for (const FPendingHit& Pending : PendingHits)
		{
			DrawDebugPoint(GetWorld(), Pending.ImpactPoint, 10.0f, FColor::Red, false, 1.0f);
		}
	}
#endif

	Box.LastLocation = CurrentLocation;
	Box.LastRotation = CurrentRotation;

	// Ниже этой точки Box трогать НЕЛЬЗЯ: подписчик может изнутри колбэка закрыть зону
	// (EndHitbox/EndAllHitboxes) или открыть новую (BeginHitbox), а это RemoveAll/Empty/Add
	// по ActiveHitboxes — ссылка Box протухнет. Всё состояние зоны уже записано выше,
	// здесь работаем только с копиями bParryable/HitboxHandle и локальным PendingHits.
	for (const FPendingHit& Pending : PendingHits)
	{
		if (bParryable && CheckAndHandleParry(Pending.Actor, Pending.ImpactPoint))
		{
			continue;   // парирование обработано, обычный хит не нужен
		}

		OnHitboxHit.Broadcast(Pending.Actor, Pending.ImpactPoint, HitboxHandle);

		FGameplayEventData Payload;
		Payload.EventTag       = ClanhallGameplayTags::Event_Hitbox_Hit.GetTag();
		Payload.Instigator     = Owner;
		Payload.Target         = Pending.Actor;
		Payload.EventMagnitude = static_cast<float>(HitboxHandle);
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, Payload.EventTag, Payload);
	}
}

bool UClanhallHitboxComponent::CheckAndHandleParry(AActor* HitActor, FVector ImpactPoint)
{
	// Враг должен быть в паррируемом состоянии
	IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(HitActor);
	if (!Interface) return false;

	UAbilitySystemComponent* HitASC = Interface->GetAbilitySystemComponent();
	if (!HitASC) return false;

	if (!HitASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_Parrying.GetTag()))
	{
		return false;
	}

	// Передаём в ParryComponent: он проверяет совпадение направления и ставит флаг
	UClanhallParryComponent* ParryComp = GetOwner()->FindComponentByClass<UClanhallParryComponent>();
	if (!ParryComp) return false;

	return ParryComp->TryParry(HitActor, CurrentDirection, ImpactPoint);
}
