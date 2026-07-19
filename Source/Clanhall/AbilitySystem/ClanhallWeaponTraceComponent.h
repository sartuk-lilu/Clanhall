// Тикающий компонент на Character (игрок / будущий враг).
// Когда weapon trace активен, каждый тик делает sphere sweep от LastSocketPosition до текущей
// позиции сокета оружия. Попадание: сначала проверка парирования (UClanhallParryComponent),
// затем — OnWeaponHit для обработки урона от способностей.
//
// Жизненный цикл:
//   AnimNotifyState_WeaponTrace: NotifyBegin → BeginTrace()   (≈20% монтажа)
//                                 NotifyEnd   → EndTrace()     (≈80% монтажа)
// Страховка: UClanhallComboComponent::ForceEndWeaponTrace() зовёт EndTrace() ещё раз на
// прерванном удар-монтаже (чейн/OnStanceExit) — на случай, если NotifyEnd не успел прийти.
//
// SetCurrentDirection() должен быть вызван до BeginTrace() из GA_DirectionalAttackBase::ActivateAbility.

#pragma once

#include "Components/ActorComponent.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallWeaponTraceComponent.generated.h"

class USoundBase;
class UClanhallParryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnClanhallWeaponHit, AActor*, HitActor, FVector, HitLocation);

UCLASS(ClassGroup="Clanhall", meta=(BlueprintSpawnableComponent))
class CLANHALL_API UClanhallWeaponTraceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClanhallWeaponTraceComponent();

	/** Имя сокета на скелете оружия. Задаётся в редакторе. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trace")
	FName WeaponSocketName = "WeaponSocket";

	/** Радиус sphere sweep (см). */
	UPROPERTY(EditAnywhere, Category="Trace")
	float TraceRadius = 20.0f;

	/** Звук столкновения оружий при успешном парировании (опционально). */
	UPROPERTY(EditAnywhere, Category="VFX")
	TObjectPtr<USoundBase> ClashSound;

	/** Установить направление текущего удара — вызывается из GA_DirectionalAttackBase до BeginTrace. */
	void SetCurrentDirection(EClanhallAttackDirection Dir);

	/** Вызывается из AnimNotifyState_WeaponTrace::NotifyBegin. */
	void BeginTrace();

	/** Вызывается из AnimNotifyState_WeaponTrace::NotifyEnd (и из страховки
	 *  UClanhallComboComponent::ForceEndWeaponTrace — повторный вызов безвреден). */
	void EndTrace();

	bool IsTracing() const { return bTraceActive; }

	/** Обычный хит (не парирование) — способность слушает этот делегат для нанесения урона. */
	UPROPERTY(BlueprintAssignable, Category="Trace")
	FOnClanhallWeaponHit OnWeaponHit;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool bTraceActive = false;
	FVector LastSocketPosition = FVector::ZeroVector;
	EClanhallAttackDirection CurrentDirection = EClanhallAttackDirection::Overhead;
	TArray<AActor*> AlreadyHit;

	void DoTrace();

	/** Проверяет, является ли хит парированием. Если да — вызывает TryParry на ParryComponent.
	 *  Возвращает true, если парирование обработано (следующий шаг OnWeaponHit не нужен). */
	bool CheckAndHandleParry(AActor* HitActor, const FHitResult& Hit);
};
