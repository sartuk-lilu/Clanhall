#include "GA_Dodge.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

UGA_Dodge::UGA_Dodge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Вешает State.DodgeRecovery только КОРОТКАЯ форма (см. OnDodgeFinished), но
	// ActivationBlockedTags блокирует ЛЮБУЮ активацию, пока тег висит — короткий отскок
	// в стойке → выход из стойки → дальний отскок тоже заблокирован до конца хвоста.
	// Это осознанно: тег защищает не WASD-серию, а сам отскок от спама.
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_DodgeRecovery.GetTag());
}

bool UGA_Dodge::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return false;
	}

	// Короткий отскок в стойке — чистое репозиционирование, зарядов не стоит никогда.
	if (ASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_InStance.GetTag()))
	{
		return true;
	}

	// Дальний вне боя — цена не проверяется вовсе: не "цена ноль", а "шага списания нет"
	// (`combat_system.md`, «Боевое состояние»).
	if (!ASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_InCombat.GetTag()))
	{
		return true;
	}

	// Дальний в бою — единственный случай, где отскок вообще стоит зарядов.
	const UClanhallAttributeSet* Attributes = ASC->GetSet<UClanhallAttributeSet>();
	if (!Attributes || Attributes->GetCharges() < static_cast<float>(DodgeChargeCost))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(ClanhallGameplayTags::Denied_Charges.GetTag());
		}
		return false;
	}

	return true;
}

void UGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Character || !ASC || !Movement)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const bool bInStance = ASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_InStance.GetTag());
	const bool bInCombat = ASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_InCombat.GetTag());
	bShortFormPending = bInStance;

	const float Distance = bInStance ? ShortDodgeDistance : LongDodgeDistance;

	// Цена — только у дальнего отскока в бою, списывается тем же механизмом, что
	// в UGA_PhysicalSkill: на активации и безвозвратно (`economy_system.md`, «Почему кулдаунов нет»).
	if (!bInStance && bInCombat && DodgeChargeCost > 0)
	{
		ClanhallGameplayEffects::ApplyModifyEffect(ASC, ASC, UGE_ModifyCharges::StaticClass(), -static_cast<float>(DodgeChargeCost));
	}

	// Направление — вектор ввода перемещения ИЗ УЖЕ СВЕДЁННОГО прошлого кадра
	// (GetLastMovementInputVector(), не Pending): порядок обработки MoveAction (Triggered)
	// и SpaceAction (Started) внутри одного кадра — свойство Enhanced Input, не кода,
	// и Pending-вектор в кадре нажатия Пробела мог ещё не накопиться. В стойке вектор
	// существует только при Shift+WASD (DoMove не регистрирует AddMovementInput без него —
	// то же правило "в стойке без Shift назад" получаем бесплатно), вне стойки всегда, если
	// ввод не заблокирован State.SkillCommitted. Ввода нет — назад от камеры, не от форварда актора.
	FVector DodgeDirection = Character->GetLastMovementInputVector();
	if (!DodgeDirection.IsNearlyZero())
	{
		DodgeDirection = DodgeDirection.GetSafeNormal();
	}
	else if (AController* Controller = Character->GetController())
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		DodgeDirection = -FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	}
	else
	{
		DodgeDirection = -Character->GetActorForwardVector();
	}

	// Косметика — механика не зависит от того, стартовал ли монтаж (`CLAUDE.md`, «Механика
	// работает без анимационных ассетов»).
	UAnimMontage* TravelMontage = bInStance ? ShortDodgeMontage : LongDodgeMontage;
	if (TravelMontage)
	{
		if (UAnimInstance* AnimInst = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInst->Montage_Play(TravelMontage);
		}
	}

	const FVector TargetLocation = Character->GetActorLocation() + DodgeDirection * Distance;

	// Кадров неуязвимости нет: перемещение капсулы Root Motion Source'ом, тем же способом,
	// что рывок в UGA_PhysicalSkill/UDashFragment — не LaunchCharacter и не телепорт
	// SetActorLocation, иначе свип противника прошёл бы сквозь то место, где игрока уже нет,
	// но коллизия ещё есть.
	UAbilityTask_ApplyRootMotionMoveToForce* DodgeTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this, TEXT("ClanhallDodge"), TargetLocation, DodgeDuration,
		/*bSetNewMovementMode*/ false, EMovementMode::MOVE_Walking,
		/*bRestrictSpeedToExpected*/ false, /*PathOffsetCurve*/ nullptr,
		ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, /*ClampVelocityOnFinish*/ 0.0f);
	DodgeTask->OnTimedOut.AddDynamic(this, &UGA_Dodge::OnDodgeFinished);
	DodgeTask->OnTimedOutAndDestinationReached.AddDynamic(this, &UGA_Dodge::OnDodgeFinished);
	DodgeTask->ReadyForActivation();
}

void UGA_Dodge::OnDodgeFinished()
{
	if (bShortFormPending)
	{
		ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
		UAnimInstance* AnimInst = (Character && Character->GetMesh()) ? Character->GetMesh()->GetAnimInstance() : nullptr;
		UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;

		// Монтаж не стартовал — лок не вешаем, невидимого лока в системе не бывает
		// ни при каких условиях (по образцу UClanhallComboComponent::EndSequenceWithRecovery).
		if (AnimInst && DodgeRecoveryMontage && ASC)
		{
			const float PlayedDuration = AnimInst->Montage_Play(DodgeRecoveryMontage);
			if (PlayedDuration > 0.0f)
			{
				ClanhallGameplayEffects::ApplyTimedTag(ASC, ClanhallGameplayTags::State_DodgeRecovery.GetTag(), DodgeRecoveryMontage->GetPlayLength());
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
