#include "AbilitySystem/ClanhallComboComponent.h"
#include "Clanhall.h"
#include "AbilitySystem/Fragments/ComboData.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "ClanhallCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
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

	if (ActiveDirections.IsEmpty())
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
	const FComboChain* Chain = ResolveChain({ Direction });
	if (!Chain)
	{
		return;
	}

	if (ActivateStep(Direction, Chain, /*StepIndex=*/ 0))
	{
		ActiveDirections = { Direction };
		CurrentChain = Chain;
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

	// Потолок ранга (combo_fragments_redesign_task.md, "Резолв шага" п.2): если кандидат длиннее
	// ClassRank — продолжение запрещено, даже если запись длиннее в дереве есть.
	const int32 CandidateLength = ActiveDirections.Num() + 1;
	if (CandidateLength > GetClassRank())
	{
		EndSequenceWithRecovery();
		return;
	}

	TArray<EClanhallAttackDirection> Candidate = ActiveDirections;
	Candidate.Add(Direction);

	const FComboChain* Chain = ResolveChain(Candidate);
	if (!Chain)
	{
		// Невалидное продолжение (Часть B1): без урона, без сдвига шкал — тот же терминальный путь.
		EndSequenceWithRecovery();
		return;
	}

	if (!ActivateStep(Direction, Chain, Candidate.Num() - 1))
	{
		// Активация не прошла (например, стойку успели снять между вводом и разрешением окна) —
		// состояние не фиксируем, тот же терминальный путь, что и для невалидного продолжения.
		EndSequenceWithRecovery();
		return;
	}

	ActiveDirections = Candidate;
	CurrentChain = Chain;

	if (ActiveDirections.Num() >= GetClassRank())
	{
		ApplyComboRecovery();
	}
}

const FComboChain* UClanhallComboComponent::ResolveChain(const TArray<EClanhallAttackDirection>& CandidateDirections) const
{
	const UComboData* Data = GetComboData();
	if (!Data)
	{
		return nullptr;
	}

	// Разбивка по длине (не по коллизии!): ExactMatches — кандидат совпадает с цепочкой целиком
	// (она терминируется здесь). PrefixMatches — кандидат её собственный префикс, ветка длиннее.
	// Ветвление дерева (несколько PrefixMatches, расходящихся дальше) — это норма, не конфликт;
	// путать их с коллизией одинаковых по направлениям ExactMatches нельзя (иначе Warning на
	// каждом шаге ветвления и случайный выбор Recovery от чужого хвоста).
	TArray<const FComboChain*> ExactMatches;
	TArray<const FComboChain*> PrefixMatches;
	for (const FComboChain& Chain : Data->Chains)
	{
		if (Chain.Steps.Num() < CandidateDirections.Num())
		{
			continue;
		}

		bool bPrefixMatches = true;
		for (int32 Index = 0; Index < CandidateDirections.Num(); ++Index)
		{
			const FComboMove* Move = Data->FindMoveById(Chain.Steps[Index].MoveId);
			if (!Move || Move->Direction != CandidateDirections[Index])
			{
				bPrefixMatches = false;
				break;
			}
		}

		if (!bPrefixMatches)
		{
			continue;
		}

		if (Chain.Steps.Num() == CandidateDirections.Num())
		{
			ExactMatches.Add(&Chain);
		}
		else
		{
			PrefixMatches.Add(&Chain);
		}
	}

	if (!ExactMatches.IsEmpty())
	{
		// Кандидат терминируется здесь. Несколько ExactMatches с одинаковой длиной/направлениями —
		// ошибка данных (разрешить нечем): берётся первая + UE_LOG Warning вне shipping.
		if (ExactMatches.Num() > 1)
		{
#if !UE_BUILD_SHIPPING
			FString ConflictingMoveIds;
			for (const FComboChain* Match : ExactMatches)
			{
				const FName FinalMoveId = Match->Steps.IsValidIndex(CandidateDirections.Num() - 1)
					? Match->Steps[CandidateDirections.Num() - 1].MoveId
					: NAME_None;
				ConflictingMoveIds += ConflictingMoveIds.IsEmpty() ? FinalMoveId.ToString() : FString::Printf(TEXT(", %s"), *FinalMoveId.ToString());
			}
			UE_LOG(LogClanhall, Warning, TEXT("UClanhallComboComponent: combo tree data conflict at depth %d, taking first — conflicting MoveId: %s"),
				CandidateDirections.Num(), *ConflictingMoveIds);
#endif
		}

		return ExactMatches[0];
	}

	if (!PrefixMatches.IsEmpty())
	{
		// Кандидат — чистый промежуточный узел без собственной терминальной записи: несколько
		// PrefixMatches здесь — обычное ветвление дальше по дереву, не коллизия. Первая запись
		// нужна только чтобы взять ход текущего шага (Steps[N-1]) и не мешать продолжению.
#if !UE_BUILD_SHIPPING
		const FName FirstStepMoveId = PrefixMatches[0]->Steps.IsValidIndex(CandidateDirections.Num() - 1)
			? PrefixMatches[0]->Steps[CandidateDirections.Num() - 1].MoveId
			: NAME_None;
		for (const FComboChain* Branch : PrefixMatches)
		{
			const FName StepMoveId = Branch->Steps.IsValidIndex(CandidateDirections.Num() - 1)
				? Branch->Steps[CandidateDirections.Num() - 1].MoveId
				: NAME_None;
			if (StepMoveId != FirstStepMoveId)
			{
				// Реальная несогласованность данных: один и тот же шаг (тот же индекс, то же
				// направление) отыгрывается разными ходами в зависимости от ветки, которая ещё
				// не выбрана игроком — так быть не должно.
				UE_LOG(LogClanhall, Warning, TEXT("UClanhallComboComponent: combo tree branches diverge on MoveId at shared depth %d: %s vs %s"),
					CandidateDirections.Num(), *FirstStepMoveId.ToString(), *StepMoveId.ToString());
				break;
			}
		}
#endif
		return PrefixMatches[0];
	}

	return nullptr;
}

bool UClanhallComboComponent::ActivateStep(EClanhallAttackDirection Direction, const FComboChain* Chain, int32 StepIndex)
{
	AClanhallCharacter* Character = Cast<AClanhallCharacter>(GetOwner());
	UAbilitySystemComponent* ASC = GetASC();
	const UComboData* Data = GetComboData();
	if (!Character || !ASC || !Data || !Chain || !Chain->Steps.IsValidIndex(StepIndex))
	{
		return false;
	}

	const FComboMove* Move = Data->FindMoveById(Chain->Steps[StepIndex].MoveId);
	if (!Move)
	{
		return false;
	}

	const FDirectionalDamage& Damage = Data->FindDamageByDirection(Move->Direction);

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

	PlayMontage(Move->Montage);
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

	// Подстраховка от залипания ActiveDirections, если на монтаже нет/не сработал
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
	if (ActiveDirections.IsEmpty())
	{
		// Уже нейтраль — серия уже завершена другим путём (fall-through/делегат). Guard от
		// двойного Recovery на одну серию.
		return;
	}

	UAnimMontage* Recovery = CurrentChain ? CurrentChain->RecoveryMontage : nullptr;

	ResetCombo();

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
	ActiveDirections.Empty();
	LatestInWindow.Reset();
	CurrentChain = nullptr;
	// Упрочнение: гасим ворота даже если сброс пришёл при открытом окне (напр. OnStanceExit
	// посреди чтения ввода) — не даём следующему нажатию попасть в уже мёртвое окно.
	bReadWindowOpen = false;
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
