#include "Camera/ClanhallCameraRigComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SpringArmComponent.h"

UClanhallCameraRigComponent::UClanhallCameraRigComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	Aim.ArmLength = 250.0f;
	Aim.SocketOffset = FVector(0.0f, 90.0f, 50.0f);
	Aim.FieldOfView = 85.0f;
}

void UClanhallCameraRigComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* Owner = GetOwner())
	{
		Boom = Owner->FindComponentByClass<USpringArmComponent>();
		Camera = Owner->FindComponentByClass<UCameraComponent>();
	}
	// Штанга считает своё положение в своём тике - кадр должен быть выставлен до него.
	if (USpringArmComponent* BoomPtr = Boom.Get())
	{
		BoomPtr->AddTickPrerequisiteComponent(this);
	}
	TargetZoomScale = ZoomScale = 1.0f;
	AimAlpha = bAiming ? 1.0f : 0.0f;
	Apply();
}

void UClanhallCameraRigComponent::SetAiming(bool bInAiming)
{
	bAiming = bInAiming;
}

void UClanhallCameraRigComponent::AddZoomInput(float WheelDelta)
{
	TargetZoomScale = FMath::Clamp(TargetZoomScale - WheelDelta * ZoomStep, ZoomMinScale, ZoomMaxScale);
}

void UClanhallCameraRigComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ZoomScale = FMath::FInterpTo(ZoomScale, TargetZoomScale, DeltaTime, ZoomInterpSpeed);
	AimAlpha = FMath::FInterpTo(AimAlpha, bAiming ? 1.0f : 0.0f, DeltaTime, AimInterpSpeed);
	Apply();
}

void UClanhallCameraRigComponent::Apply()
{
	const float ExploreArm = Explore.ArmLength * ZoomScale;
	// Прицеливание подводит камеру к своей длине, но не отодвигает уже придвинутую колесом.
	const float AimArm = FMath::Min(ExploreArm, Aim.ArmLength);

	if (USpringArmComponent* BoomPtr = Boom.Get())
	{
		BoomPtr->TargetArmLength = FMath::Lerp(ExploreArm, AimArm, AimAlpha);
		BoomPtr->SocketOffset = FMath::Lerp(Explore.SocketOffset, Aim.SocketOffset, AimAlpha);
	}
	if (UCameraComponent* CameraPtr = Camera.Get())
	{
		CameraPtr->SetFieldOfView(FMath::Lerp(Explore.FieldOfView, Aim.FieldOfView, AimAlpha));
	}
}
