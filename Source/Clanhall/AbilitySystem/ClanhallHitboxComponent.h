// Диспетчер активных зон поражения на Character (игрок / будущий враг). Собственной геометрии
// не имеет — форма, кость, смещение и роль каждой зоны приходят из FClanhallHitboxDesc, заданного
// на конкретном AnimNotifyState_Hitbox (main_dev_plan.md §7). Одновременно может быть открыто
// несколько зон, у каждой свой набор задетых целей (AlreadyHit) — поэтому Shield Charge получает
// «каждый враг на пути рывка — ровно один раз» бесплатно, без отдельного сбора целей по траектории.
// НЕСКОЛЬКО ЗОН = НЕСКОЛЬКО УРОНОВ, ВСЕГДА (main_dev_plan.md §4): развилки «две формы одного
// контакта» не существует. Капсула клинка + сфера щита на одном монтаже нанесли бы два урона одним
// взмахом — для покрытия геометрии одного взмаха увеличивать ФОРМУ существующей зоны, а не
// добавлять вторую. Несколько зон оправданы только для последовательных фаз контакта одного
// удара (щит закрылся — меч открылся), а не для одновременного покрытия разной геометрии.
//
// Жизненный цикл зоны:
//   AnimNotifyState_Hitbox: NotifyBegin → BeginHitbox(Desc, this)
//                            NotifyEnd   → EndHitbox(this)
// Пара Begin/End связывается по указателю на нотифай (Source), а не по хендлу: UAnimNotifyState —
// разделяемый const-объект, хендл конкретного проигрывания на нём храниться не может.
// Страховка: UClanhallComboComponent::ForceEndHitboxes() зовёт EndAllHitboxes() на прерванном
// удар-монтаже (чейн/OnStanceExit) — на случай, если NotifyEnd не успел прийти.
//
// Клэш парирования (ability_system.md §2): проверяется только у зон с bParryable == true.
// WASD-удары парируемы по умолчанию (дефолт Desc воспроизводит прежнее захардкоженное крепление),
// активки Q/E/R/F ставят bParryable = false — протухшее CurrentDirection последнего WASD-удара до
// проверки парирования не доходит вовсе, т.к. проверка для таких зон не вызывается.
//
// SetCurrentDirection() должен быть вызван до BeginHitbox() из GA_DirectionalAttackBase::ActivateAbility
// — читается только зонами с bParryable == true.
//
// Помимо OnHitboxHit (BlueprintAssignable, для VFX/SFX) компонент шлёт GameplayEvent'ы
// живой способности — Event.Hitbox.Hit на каждую задетую цель, Event.Hitbox.Closed один раз при
// переходе «были зоны -> зон не осталось» (main_dev_plan.md §7).

#pragma once

#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallHitboxTypes.h"
#include "ClanhallHitboxComponent.generated.h"

class USoundBase;
class UClanhallParryComponent;
class USkeletalMeshComponent;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnClanhallHitboxHit, AActor*, HitActor, FVector, HitLocation, int32, HitboxHandle);

UCLASS(ClassGroup="Clanhall", meta=(BlueprintSpawnableComponent))
class CLANHALL_API UClanhallHitboxComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClanhallHitboxComponent();

	/** Звук столкновения оружий при успешном парировании (опционально). */
	UPROPERTY(EditAnywhere, Category="VFX")
	TObjectPtr<USoundBase> ClashSound;

	/** Рисовать активные зоны в мире. Нужно при разметке монтажей: «не дотянулся — не попал»
	 *  должно быть видно глазом, а не выясняться по логам. */
	UPROPERTY(EditAnywhere, Category = "Hitbox|Debug")
	bool bDrawDebugHitboxes = false;

	/** Открыть зону. Source — объект-владелец окна (нотифай), по нему же зона закрывается.
	 *  Возвращает хендл зоны (>0) или 0 при отказе — в т.ч. если зоны сейчас подавлены
	 *  (см. IsSuppressed). */
	int32 BeginHitbox(const FClanhallHitboxDesc& Desc, const UObject* Source);

	/** Закрыть все зоны, открытые этим Source. */
	void EndHitbox(const UObject* Source);

	/** Закрыть все зоны без разбора. Страховка от залипания при прерывании монтажа —
	 *  зовётся из UClanhallComboComponent. */
	void EndAllHitboxes();

	/** Закрыть только те зоны, чей Source-нотифай принадлежит указанному монтажу. Уборка за
	 *  съеденным NotifyEnd (Branching Point), зовётся из EndAbility активки. Точечно, а не
	 *  EndAllHitboxes(): EndAbility приходит асинхронно (может прилететь из OnDashFinished много
	 *  позже конца монтажа), и к этому моменту зоны может владеть уже следующий шаг комбо. */
	void EndHitboxesFromMontage(const UAnimMontage* Montage);

	bool IsAnyHitboxActive() const { return ActiveHitboxes.Num() > 0; }

	/** Установить направление текущего WASD-удара — вызывается из GA_DirectionalAttackBase до
	 *  открытия зоны. Читается ТОЛЬКО зонами с bParryable == true. */
	void SetCurrentDirection(EClanhallAttackDirection Dir);

	/** task_parry_rework.md §1.4: по владельцу пришёл любой контакт (клэш или пропущенный
	 *  урон) — зоны глушатся до конца текущего шага. Закрывает уже открытые зоны (как
	 *  EndAllHitboxes) и взводит флаг, из-за которого BeginHitbox() дальше не открывает новые.
	 *  Если открытых зон не было (типичный случай для клэша — своя зона ещё не успела
	 *  открыться), Event.Hitbox.Closed шлётся здесь явно: иначе ждущая способность
	 *  (GA_DirectionalAttackBase/GA_PhysicalSkill) не получит терминатор, а State.SkillCommitted
	 *  зависнет до конца каст-монтажа. */
	void SuppressHitboxes();

	/** Снимает подавление — зовётся на СЛЕДУЮЩЕМ шаге/активации владельца
	 *  (UClanhallComboComponent::ActivateStep, UGA_PhysicalSkill::ActivateAbility). */
	void ResetSuppression() { bHitboxesSuppressed = false; }

	bool IsSuppressed() const { return bHitboxesSuppressed; }

	/** task_parry_rework.md §1.5: кратковременно замедляет текущий проигрываемый монтаж
	 *  владельца — вызывается у того, ЧЬЯ зона нанесла контакт (не у обоих, не у получившего).
	 *  Play rate восстанавливается таймером через Duration секунд. Duration <= 0 — no-op. */
	void ApplyHitstop(float Duration);

	/** Скорость проигрывания монтажа на время хитстопа. */
	UPROPERTY(EditDefaultsOnly, Category = "Hitbox|Hitstop")
	float HitstopPlayRate = 0.15f;

	/** Длительность хитстопа при подтверждённом уроне (~3-5 кадров @ 30fps). */
	UPROPERTY(EditDefaultsOnly, Category = "Hitbox|Hitstop")
	float HitstopDurationOnDamage = 0.1f;

	/** Длительность хитстопа при клэше (~5-8 кадров @ 30fps). Держать ≤ 5 кадров (0.166с) —
	 *  на игроке хитстоп сдвигает вправо его собственное освобождение, а у босса нет
	 *  симметричного сдвига; при 8+ кадрах цепочка парирований по расчёту перестаёт сходиться
	 *  (task_parry_rework.md §1.5). */
	UPROPERTY(EditDefaultsOnly, Category = "Hitbox|Hitstop")
	float HitstopDurationOnClash = 0.166f;

	/** Обычный хит (не парирование). HitboxHandle позволяет подписчику отличить свою зону от
	 *  чужой — нужно способности, ждущей своё окно контакта (Event.Hitbox.Hit несёт тот же
	 *  хендл в EventMagnitude). */
	UPROPERTY(BlueprintAssignable, Category = "Hitbox")
	FOnClanhallHitboxHit OnHitboxHit;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	struct FActiveHitbox
	{
		int32 Handle = 0;
		FClanhallHitboxDesc Desc;
		TWeakObjectPtr<const UObject> Source;

		/** Предыдущая позиция/поворот зоны — начало свипа этого тика. */
		FVector  LastLocation = FVector::ZeroVector;
		FQuat    LastRotation = FQuat::Identity;
		bool     bHasPrevTransform = false;

		/** Свой набор задетых целей НА ЗОНУ, а не на компонент. Именно это даёт
		 *  Shield Charge «каждый враг на пути получает ровно один раз за рывок». */
		TArray<TWeakObjectPtr<AActor>> AlreadyHit;

		/** Кость/сокет не нашлись — залогировано один раз, дальше зона молча пропускается. */
		bool bLoggedMissingBone = false;
	};

	TArray<FActiveHitbox> ActiveHitboxes;
	int32 NextHitboxHandle = 1;
	EClanhallAttackDirection CurrentDirection = EClanhallAttackDirection::Overhead;

	/** task_parry_rework.md §1.4: взведено — BeginHitbox() отказывает, до сброса на
	 *  следующем шаге/активации владельца. */
	bool bHitboxesSuppressed = false;

	/** Хендл восстановления play rate после ApplyHitstop — переиспользуется, повторный вызов
	 *  просто отодвигает восстановление, двух параллельных хитстопов быть не должно. */
	FTimerHandle HitstopRestoreHandle;

	void TickHitbox(FActiveHitbox& Box, USkeletalMeshComponent* Mesh);

	/** Проверяет, является ли хит парированием. Если да — вызывает TryParry на ParryComponent.
	 *  Возвращает true, если парирование обработано (следующий шаг OnHitboxHit не нужен). */
	bool CheckAndHandleParry(AActor* HitActor, FVector ImpactPoint);

	/** Фаза контакта окончена: закрылась последняя зона. Шлётся ОДИН раз на переходе
	 *  «были зоны -> зон не осталось». Слать на каждый вызов End* нельзя: вызов по пустому
	 *  списку оборвал бы способность, которая активировалась, но зону ещё не открыла. */
	void NotifyAllHitboxesClosed();
};
