#include "AbilitySystem/ClanhallComboComponent.h"
#include "AbilitySystem/Fragments/ComboFragment.h"
#include "AbilitySystem/AbilityData.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "ClanhallCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

void UClanhallComboComponent::HandleAttackInput(EClanhallAttackDirection Direction)
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || ASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_ComboRecovery.GetTag()))
	{
		// Лок-аут после максимальной длины комбо — вход игнорируется, пока тег не спадёт.
		return;
	}

	if (ActiveSequence.IsEmpty())
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
	const UComboFragment* Fragment = GetActiveFragment();
	const FComboSequence* Entry = Fragment ? Fragment->FindExact({ Direction }) : nullptr;
	if (!Entry)
	{
		return;
	}

	if (ActivateDirection(Direction, Entry->Montage))
	{
		ActiveSequence = { Direction };
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

	TArray<EClanhallAttackDirection> Candidate = ActiveSequence;
	Candidate.Add(Direction);

	const UComboFragment* Fragment = GetActiveFragment();
	const FComboSequence* Entry = Fragment ? Fragment->FindExact(Candidate) : nullptr;
	if (!Entry)
	{
		// Невалидное продолжение (Часть B1): без урона, без сдвига шкал — тот же терминальный путь.
		EndSequenceWithRecovery();
		return;
	}

	if (!ActivateDirection(Direction, Entry->Montage))
	{
		// Активация не прошла (например, стойку успели снять между вводом и разрешением окна) —
		// состояние не фиксируем, тот же терминальный путь, что и для невалидного продолжения.
		EndSequenceWithRecovery();
		return;
	}

	ActiveSequence = Candidate;

	if (Fragment && ActiveSequence.Num() >= Fragment->MaxComboLength)
	{
		ApplyComboRecovery();
	}
}

bool UClanhallComboComponent::ActivateDirection(EClanhallAttackDirection Direction, UAnimMontage* Montage)
{
	AClanhallCharacter* Character = Cast<AClanhallCharacter>(GetOwner());
	UAbilitySystemComponent* ASC = GetASC();
	if (!Character || !ASC)
	{
		return false;
	}

	// Часть B1: точка вызова инвертирована — GA_DirectionalAttackBase::ActivateAbility
	// (формулы урона/MP/Balance не тронуты) срабатывает, только если валидатор дошёл до этого вызова.
	if (!ASC->TryActivateAbility(Character->GetAttackHandle(Direction)))
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

	// Правка 2: подстраховка от залипания ActiveSequence, если на монтаже нет/не сработал
	// AnimNotifyState_ComboWindow — доигрывание монтажа всё равно снимет состояние через делегат.
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UClanhallComboComponent::OnAttackMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndDelegate, Montage);
}

void UClanhallComboComponent::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted)
	{
		// Чейн прервал монтаж новым Montage_Play, либо OnStanceExit остановил стойку — в обоих
		// случаях соответствующий путь уже отработал сам, здесь трогать нечего.
		return;
	}

	EndSequenceWithRecovery();
}

void UClanhallComboComponent::EndSequenceWithRecovery()
{
	if (ActiveSequence.IsEmpty())
	{
		// Уже нейтраль — серия уже завершена другим путём (fall-through/делегат). Guard от
		// двойного Recovery на одну серию (combo_system_redesign.md).
		return;
	}

	ResetCombo();

	const UComboFragment* Fragment = GetActiveFragment();
	UAnimMontage* Recovery = Fragment ? Fragment->RecoveryMontage : nullptr;
	if (!Recovery)
	{
		// nullptr: хвост восстановления запечён в сам удар-монтаж, отдельно играть нечего.
		return;
	}

	if (UAnimInstance* AnimInst = GetAnimInstance())
	{
		// Без делегата конца удар-монтажа — иначе собственный конец Recovery снова вызвал бы
		// EndSequenceWithRecovery() (ловушка-петля).
		AnimInst->Montage_Play(Recovery);
		LastPlayedMontage = Recovery;
	}
}

void UClanhallComboComponent::ResetCombo()
{
	ActiveSequence.Empty();
	LatestInWindow.Reset();
}

void UClanhallComboComponent::ApplyComboRecovery()
{
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ClanhallGameplayEffects::ApplyTimedTag(ASC, ClanhallGameplayTags::State_ComboRecovery.GetTag(), ComboRecoveryDuration);
	}

	// ResetCombo НЕ здесь: сброс состояния и Recovery-анимация — по завершении терминального
	// удар-монтажа через EndSequenceWithRecovery (fall-through этого же окна или делегат конца
	// монтажа). Тег живёт своим таймером параллельно, ResetCombo его не снимает.
}

void UClanhallComboComponent::OnStanceExit()
{
	if (UAnimInstance* AnimInst = GetAnimInstance())
	{
		AnimInst->Montage_Stop(StanceExitBlendOutTime, LastPlayedMontage.Get());
	}

	ResetCombo();
}

const UComboFragment* UClanhallComboComponent::GetActiveFragment() const
{
	const AClanhallCharacter* Character = Cast<AClanhallCharacter>(GetOwner());
	const UAbilityData* Data = Character ? Character->GetComboData() : nullptr;
	return Data ? Data->FindFragment<UComboFragment>() : nullptr;
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