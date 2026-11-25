// Камера из-за плеча (`Implementation/Camera.md`). Компонент водит штангу и камеру владельца:
// обычный кадр - герой левее центра; прицеливание (зажата ЛКМ или ПКМ) - камера ближе и правее,
// чтобы открыть противника; колесо мыши - плавный зум в пределах доли от обычной длины штанги.
// Все параметры - здесь, правятся в Details Blueprint-наследника персонажа.

#pragma once

#include "Components/ActorComponent.h"
#include "ClanhallCameraRigComponent.generated.h"

class USpringArmComponent;
class UCameraComponent;

/** Кадр камеры: длина штанги, смещение камеры на её конце и поле зрения. */
USTRUCT(BlueprintType)
struct FClanhallCameraFraming
{
	GENERATED_BODY()

	/** Длина штанги от героя до камеры. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0", Units = "cm"))
	float ArmLength = 400.0f;

	/** Смещение камеры на конце штанги: Y > 0 - камера правее, герой левее центра; Z - выше. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector SocketOffset = FVector(0.0f, 60.0f, 40.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "20", ClampMax = "170", Units = "deg"))
	float FieldOfView = 90.0f;
};

UCLASS(ClassGroup = (Clanhall), meta = (BlueprintSpawnableComponent))
class CLANHALL_API UClanhallCameraRigComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClanhallCameraRigComponent();

	/** Зажата кнопка мыши (любой слой ввода) - камера уходит в кадр прицеливания. */
	void SetAiming(bool bInAiming);

	/** Колесо мыши: положительное - ближе. Шаг - ZoomStep за деление. */
	void AddZoomInput(float WheelDelta);

	/** Обычный кадр. Длина штанги - база зума: колесо масштабирует её. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clanhall|Camera")
	FClanhallCameraFraming Explore;

	/** Кадр прицеливания. Длина штанги - абсолютная, а не доля зума: камера подходит к ней, но если
	 *  колесом уже придвинута ближе, остаётся там и не отъезжает. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clanhall|Camera")
	FClanhallCameraFraming Aim;

	/** Предел зума колесом: доля от длины штанги обычного кадра (0.5 - вдвое ближе). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clanhall|Camera|Zoom", meta = (ClampMin = "0.1", ClampMax = "1"))
	float ZoomMinScale = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clanhall|Camera|Zoom", meta = (ClampMin = "1", ClampMax = "4"))
	float ZoomMaxScale = 1.5f;

	/** Доля длины штанги за одно деление колеса. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clanhall|Camera|Zoom", meta = (ClampMin = "0.01", ClampMax = "0.5"))
	float ZoomStep = 0.1f;

	/** Скорость, с которой зум догоняет колесо (больше - резче). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clanhall|Camera|Zoom", meta = (ClampMin = "0.1"))
	float ZoomInterpSpeed = 8.0f;

	/** Скорость перехода в прицеливание и обратно (больше - резче). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clanhall|Camera", meta = (ClampMin = "0.1"))
	float AimInterpSpeed = 10.0f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	TWeakObjectPtr<USpringArmComponent> Boom;
	TWeakObjectPtr<UCameraComponent> Camera;

	bool bAiming = false;
	/** 0 - обычный кадр, 1 - прицеливание. */
	float AimAlpha = 0.0f;
	float TargetZoomScale = 1.0f;
	float ZoomScale = 1.0f;

	void Apply();
};
