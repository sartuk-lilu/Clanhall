#include "AbilitySystem/ClanhallComboComponent.h"
#include "Clanhall.h"
#include "AbilitySystem/Fragments/ComboData.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystem/ClanhallWeaponTraceComponent.h"
#include "ClanhallCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UClanhallComboComponent::HandleAttackInput(EClanhallAttackDirection Direction)
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || ASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_ComboRecovery.GetTag()))
	{
		// Лок-аут после Recovery (см. EndSequenceWithRecovery) — вход игнорируется, пока тег не
		// спадёт. Дублирует ActivationBlockedTags на GA_ClanhallAbilityBase для WASD-пути конкретно
		// (тот гейтит саму активацию GA, этот — вход в компонент раньше, до попытки активации).
		return;
	}

	// Стойка наземная (см. DoMove в ClanhallCharacter.cpp) — держим ЛКМ в воздухе, тег
	// State.InStance висит, но удар-монтаж посреди падения играть нельзя.
	if (const ACharacter* Char = Cast<ACharacter>(GetOwner());
		Char && Char->GetCharacterMovement() && Char->GetCharacterMovement()->IsFalling())
	{
		return;
	}

	if (StepCount == 0)
	{
		TryStartSequence(Direction);
		return;
	}

	if (bReadWindowOpen)
	{
		// В окне действует "последнее нажатие решает" — перезаписываем, не копим.
		LatestInWindow = Direction;
		return;
	}

	// Окно закрыто (фаза замаха) — ввод отбрасывается целиком: не опенер, не буфер.
}

void UClanhallComboComponent::TryStartSequence(EClanhallAttackDirection Direction)
{
	const UComboData* Data = GetComboData();
	UAnimMontage* Montage = Data ? Data->FindOpenerMontage(Direction) : nullptr;
	if (!Montage)
	{
		// Нейтраль этим направлением не начать (пустой слот FromStance) — ничего не начали.
		return;
	}

	if (ActivateStep(Direction, Montage))
	{
		LastDirection = Direction;
		StepCount = 1;
	}
}

void UClanhallComboComponent::OnComboWindowOpen()
{
	bReadWindowOpen = true;
	LatestInWindow.Reset();
}

void UClanhallComboComponent::OnComboWindowClose()
{
	bReadWindowOpen = false;

	if (!LatestInWindow.IsSet())
	{
		// Нет ввода к концу окна — терминальный удар без чейна, единый путь завершения серии.
		EndSequenceWithRecovery();
		return;
	}

	const EClanhallAttackDirection Direction = LatestInWindow.GetValue();
	LatestInWindow.Reset();

	// Потолок ранга: если продолжение длиннее ClassRank — запрещено, даже если слот перехода занят.
	if (StepCount + 1 > GetClassRank())
	{
		EndSequenceWithRecovery();
		return;
	}

	if (!LastDirection.IsSet())
	{
		// Защитный случай: сюда не должны попадать при StepCount > 0 — LastDirection и StepCount
		// меняются вместе. Раз рассинхрон всё же случился, безопаснее уйти в Recovery.
		EndSequenceWithRecovery();
		return;
	}

	const UComboData* Data = GetComboData();
	UAnimMontage* Montage = Data ? Data->FindTransitionMontage(LastDirection.GetValue(), Direction) : nullptr;
	if (!Montage)
	{
		// Невалидное продолжение (Часть B1): без урона, без сдвига шкал — тот же терминальный путь.
		EndSequenceWithRecovery();
		return;
	}

	if (!ActivateStep(Direction, Montage))
	{
		// Активация не прошла (например, стойку успели снять между вводом и разрешением окна) —
		// состояние не фиксируем, тот же терминальный путь, что и для невалидного продолжения.
		EndSequenceWithRecovery();
		return;
	}

	LastDirection = Direction;
	++StepCount;

	if (StepCount >= GetClassRank())
	{
		ApplyComboRecovery();
	}
}

bool UClanhallComboComponent::ActivateStep(EClanhallAttackDirection Direction, UAnimMontage* Montage)
{
	AClanhallCharacter* Character = Cast<AClanhallCharacter>(GetOwner());
	UAbilitySystemComponent* ASC = GetASC();
	const UComboData* Data = GetComboData();
	if (!Character || !ASC || !Data || !Montage)
	{
		return false;
	}

	const FDirectionalDamage& Damage = Data->FindDamageByDirection(Direction);

	// Часть B1: точка вызова инвертирована — GA_DirectionalAttackBase::ActivateAbility
	// (формулы урона/MP/Balance не тронуты) срабатывает, только если валидатор дошёл до этого
	// вызова. BaseDamage профиля идёт в EventMagnitude — GA больше не хранит RawDamage сам.
	FGameplayEventData EventData;
	EventData.EventMagnitude = Damage.BaseDamage;
	if (Damage.DamageType.IsValid())
	{
		// Задел: тип урона в InstigatorTags события, в расчёте пока не читается.
		EventData.InstigatorTags.AddTag(Damage.DamageType);
	}

	const FGameplayAbilitySpecHandle Handle = Character->GetAttackHandle(Direction);
	if (!ASC->TriggerAbilityFromGameplayEvent(Handle, ASC->AbilityActorInfo.Get(), ClanhallGameplayTags::Event_DirectionalAttack.GetTag(), &EventData, *ASC))
	{
		return false;
	}

	PlayMontage(Montage);
	return true;
}

void UClanhallComboComponent::PlayMontage(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	UAnimInstance* AnimInst = GetAnimInstance();
	if (!AnimInst)
	{
		return;
	}

	AnimInst->Montage_Play(Montage);
	LastPlayedMontage = Montage;

	// Подстраховка от залипания состояния, если на монтаже нет/не сработал
	// AnimNotifyState_ComboWindow — доигрывание монтажа всё равно снимет состояние через делегат.
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UClanhallComboComponent::OnAttackMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndDelegate, Montage);
}

void UClanhallComboComponent::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// Прерванный монтаж — основной кейс залипшего трейса (нотифай WeaponTraceEnd не успел
	// сработать): страховка идёт до раннего return по bInterrupted.
	ForceEndWeaponTrace();

	if (bInterrupted)
	{
		// Чейн прервал монтаж новым Montage_Play, либо OnStanceExit остановил стойку — в обоих
		// случаях соответствующий путь уже отработал сам, здесь трогать нечего.
		return;
	}

	EndSequenceWithRecovery();
}

void UClanhallComboComponent::ForceEndWeaponTrace()
{
	if (UClanhallWeaponTraceComponent* TraceComp = GetOwner() ? GetOwner()->FindComponentByClass<UClanhallWeaponTraceComponent>() : nullptr)
	{
		TraceComp->EndTrace();
	}
}

void UClanhallComboComponent::EndSequenceWithRecovery()
{
	if (StepCount == 0)
	{
		// Уже нейтраль — серия уже завершена другим путём (fall-through/делегат). Guard от
		// двойного Recovery на одну серию.
		return;
	}

	const UComboData* Data = GetComboData();
	// Резолв Recovery-анимации и чтение bCeilingReached — оба ДО ResetCombo(): сброс очищает
	// LastDirection и гасит флаг.
	UAnimMontage* RecoveryMontage = (Data && LastDirection.IsSet()) ? Data->FindRecoveryMontage(LastDirection.GetValue()) : nullptr;
	const bool bWasCeilingReached = bCeilingReached;

	ResetCombo();

	// Лок-аут новых атак (State.ComboRecovery) реально стартует здесь, не в ApplyComboRecovery —
	// таймер должен покрывать время ПОСЛЕ возврата в стойку, а не время, пока ещё доигрывает
	// терминальный удар-монтаж (дефект «а» из задачи). База — доля длительности Recovery-анимации;
	// если серии некуда было расти (потолок ClassRank), сверху добавляется ComboRecoveryDuration.
	float LockDuration = RecoveryMontage ? RecoveryMontage->GetPlayLength() * RecoveryLockFraction : 0.f;
	if (bWasCeilingReached)
	{
		LockDuration += ComboRecoveryDuration;
	}
	if (LockDuration > 0.f)
	{
		if (UAbilitySystemComponent* ASC = GetASC())
		{
			ClanhallGameplayEffects::ApplyTimedTag(ASC, ClanhallGameplayTags::State_ComboRecovery.GetTag(), LockDuration);
		}
	}

	if (!RecoveryMontage)
	{
		// nullptr: хвост восстановления запечён в сам удар-монтаж, отдельно играть нечего. Штраф за
		// потолок ClassRank (если был) уже применён выше.
		return;
	}

	if (UAnimInstance* AnimInst = GetAnimInstance())
	{
		// НЕ вызывать PlayMontage() здесь: тот хелпер вешает Montage_SetEndDelegate на
		// OnAttackMontageEnded, и для Recovery это ловушка-петля (конец Recovery ->
		// OnAttackMontageEnded(bInterrupted=false) -> EndSequenceWithRecovery -> снова Recovery).
		// Recovery играется без делегата конца, как и было в текущей реализации — не "оптимизировать"
		// обратно на PlayMontage().
		const float PlayedDuration = AnimInst->Montage_Play(RecoveryMontage);
		if (PlayedDuration == 0.f)
		{
			UE_LOG(LogClanhall, Warning, TEXT("EndSequenceWithRecovery: Montage_Play failed to start %s"), *RecoveryMontage->GetName());
		}
		LastPlayedMontage = RecoveryMontage;
	}
}

void UClanhallComboComponent::CancelSequenceForExternalMontage()
{
	if (StepCount == 0)
	{
		// Нейтраль — прерывать нечего.
		return;
	}

	ForceEndWeaponTrace();
	ResetCombo();
	// Монтаж НЕ останавливаем: Montage_Play вызывающего сам его перебьёт.
	// Прилетит OnAttackMontageEnded(bInterrupted=true) -> ранний return -> состояние уже чистое.
	// LastPlayedMontage намеренно не трогаем: если игрок отпустит ЛКМ во время каста, OnStanceExit
	// вызовет Montage_Stop по уже мёртвому удар-монтажу (no-op) вместо живого каста.
}

void UClanhallComboComponent::ResetCombo()
{
	LastDirection.Reset();
	StepCount = 0;
	LatestInWindow.Reset();
	// Упрочнение: гасим ворота даже если сброс пришёл при открытом окне (напр. OnStanceExit
	// посреди чтения ввода) — не даём следующему нажатию попасть в уже мёртвое окно.
	bReadWindowOpen = false;
	// Тег State.ComboRecovery (если уже был повешен) НЕ снимается здесь — он живёт своим таймером
	// независимо от состояния серии. Именно это не даёт связке "выйти из стойки и сразу войти
	// обратно" бесплатно отменить лок-аут (см. OnStanceExit).
	bCeilingReached = false;
}

void UClanhallComboComponent::ApplyComboRecovery()
{
	// Тег State.ComboRecovery здесь НЕ вешается: терминальный удар-монтаж ещё доигрывает, и таймер
	// стартовал бы раньше, чем реально начинается Recovery — к моменту возврата в стойку лок-аут
	// почти истёк бы. Тег вешает EndSequenceWithRecovery, когда Recovery реально стартует; здесь
	// только фиксируем факт для добавочного штрафа.
	bCeilingReached = true;

	// ResetCombo НЕ здесь: сброс состояния и Recovery-анимация — по завершении терминального
	// удар-монтажа через EndSequenceWithRecovery (fall-through этого же окна или делегат конца
	// монтажа).
}

void UClanhallComboComponent::OnStanceExit()
{
	// Выход из стойки во время Recovery остаётся бесплатным: гасит монтаж с блендом и сбрасывает
	// серию как обычно. State.ComboRecovery намеренно НЕ снимается — тег продолжает тикать своим
	// таймером (см. ResetCombo), поэтому "выйти из стойки и сразу войти обратно" не даёт бесплатной
	// отмены лок-аута на новые атаки.
	if (UAnimInstance* AnimInst = GetAnimInstance())
	{
		AnimInst->Montage_Stop(StanceExitBlendOutTime, LastPlayedMontage.Get());
	}

	ForceEndWeaponTrace();
	ResetCombo();
}

const UComboData* UClanhallComboComponent::GetComboData() const
{
	const AClanhallCharacter* Character = Cast<AClanhallCharacter>(GetOwner());
	return Character ? Character->GetComboData() : nullptr;
}

int32 UClanhallComboComponent::GetClassRank() const
{
	const AClanhallCharacter* Character = Cast<AClanhallCharacter>(GetOwner());
	return Character ? Character->ClassRank : 1;
}

UAbilitySystemComponent* UClanhallComboComponent::GetASC() const
{
	if (const IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return Interface->GetAbilitySystemComponent();
	}
	return nullptr;
}

UAnimInstance* UClanhallComboComponent::GetAnimInstance() const
{
	const ACharacter* Char = Cast<ACharacter>(GetOwner());
	return Char && Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
}
