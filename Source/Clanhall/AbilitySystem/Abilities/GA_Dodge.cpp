#include "GA_Dodge.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallComboComponent.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Dodge::UGA_Dodge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// ActivationBlockedTags блокирует ЛЮБУЮ активацию, пока хвост восстановления доигрывает -
	// тег защищает сам класс уход/рывок/присед от спама, общий для всех трёх
	// (`combat_system.md`).
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_EvadeRecovery.GetTag());
}

bool UGA_Dodge::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar)
	{
		return false;
	}

	// Удар коммитится (`combat_system.md`) - живой удар-монтаж блокирует любой уход. Предикат
	// точный: окно чтения серии находится ВНУТРИ удар-монтажа, поэтому «серия активна» и есть
	// «удар живой». Хвост Recovery уже не серия - убежать во время него законно, тег
	// State.ComboRecovery гасит атаки, а не побег.
	if (const UClanhallComboComponent* Combo = Avatar->FindComponentByClass<UClanhallComboComponent>())
	{
		if (Combo->IsSequenceActive())
		{
			return false;
		}
	}

	// Цена (только рывок вперёд, только в бою) зависит от направления, которое известно лишь
	// на активации - CanActivateAbility событие (TriggerEventData) не получает, движковая
	// сигнатура его не несёт. Проверка и списание живут в ActivateAbility, где направление уже
	// известно; отказ там идёт тем же Denied.Charges через ручной NotifyAbilityFailed.
	return true;
}

void UGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Character || !ASC || !Movement || !TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const EClanhallEvadeDirection Direction = static_cast<EClanhallEvadeDirection>(FMath::RoundToInt(TriggerEventData->EventMagnitude));
	const bool bInCombat = ASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_InCombat.GetTag());
	const bool bChargedDash = (Direction == EClanhallEvadeDirection::ForwardDash) && bInCombat;

	// Цена - только у рывка вперёд в бою, списывается тем же механизмом, что в
	// UGA_PhysicalSkill: на активации и безвозвратно (`economy_system.md`, «Почему кулдаунов нет»).
	// Боковой уход не стоит зарядов никогда; рывок вне боя цену не проверяет вовсе - не «цена
	// ноль», а «шага списания нет» (`combat_system.md`, «Боевое состояние»).
	if (bChargedDash && LongDashChargeCost > 0)
	{
		const UClanhallAttributeSet* Attributes = ASC->GetSet<UClanhallAttributeSet>();
		if (!Attributes || Attributes->GetCharges() < static_cast<float>(LongDashChargeCost))
		{
			// Отказ идёт здесь, а не из CanActivateAbility (см. комментарий там) - это уже
			// ПОСЛЕ Super::ActivateAbility, то есть после PreActivate, где движок вешает
			// ActivationOwnedTags. У UGA_Dodge их сегодня нет, поэтому моргания не видно, но
			// если когда-нибудь появятся - тег повесится и снимется в один кадр вместе с этим
			// EndAbility(bWasCancelled=true). Безвредно как есть, учитывать при добавлении тегов.
			FGameplayTagContainer FailureTags;
			FailureTags.AddTag(ClanhallGameplayTags::Denied_Charges.GetTag());
			ASC->NotifyAbilityFailed(Handle, this, FailureTags);
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		ClanhallGameplayEffects::ApplyModifyEffect(ASC, ASC, UGE_ModifyCharges::StaticClass(), -static_cast<float>(LongDashChargeCost));
	}

	float Distance = 0.0f;
	UAnimMontage* TravelMontage = nullptr;
	FVector WorldDirection = FVector::ZeroVector;

	switch (Direction)
	{
	case EClanhallEvadeDirection::Left:
		Distance = SideDodgeDistance;
		TravelMontage = SideDodgeLeftMontage;
		// Корпус в страйфе развёрнут по камере, на бегу - по движению, поэтому направление
		// ухода берётся от корпуса, а не от камеры и не от вектора ввода (`combat_system.md`):
		// в страйфе, чтобы уйти влево, вектор ввода потребовал бы сначала поехать влево - то
		// есть занять плохую позицию, прежде чем получить право из неё уйти.
		WorldDirection = -Character->GetActorRightVector();
		break;
	case EClanhallEvadeDirection::Right:
		Distance = SideDodgeDistance;
		TravelMontage = SideDodgeRightMontage;
		WorldDirection = Character->GetActorRightVector();
		break;
	case EClanhallEvadeDirection::ForwardDash:
	default:
		Distance = LongDashDistance;
		TravelMontage = LongDashMontage;
		WorldDirection = Character->GetActorForwardVector();
		break;
	}

	const float Duration = (Direction == EClanhallEvadeDirection::ForwardDash) ? LongDashDuration : SideDodgeDuration;

	// Косметика — механика не зависит от того, стартовал ли монтаж (`CLAUDE.md`, «Механика
	// работает без анимационных ассетов»).
	if (TravelMontage)
	{
		if (UAnimInstance* AnimInst = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInst->Montage_Play(TravelMontage);
		}
	}

	const FVector TargetLocation = Character->GetActorLocation() + WorldDirection * Distance;

	// Корпус на уходе не доворачивается - смещение и только (`combat_system.md`). Кадров
	// неуязвимости нет: перемещение капсулы Root Motion Source'ом, тем же способом, что рывок
	// в UGA_PhysicalSkill/UDashFragment - не LaunchCharacter и не телепорт SetActorLocation,
	// иначе свип противника прошёл бы сквозь то место, где игрока уже нет, но коллизия ещё есть.
	UAbilityTask_ApplyRootMotionMoveToForce* DodgeTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this, TEXT("ClanhallDodge"), TargetLocation, Duration,
		/*bSetNewMovementMode*/ false, EMovementMode::MOVE_Walking,
		/*bRestrictSpeedToExpected*/ false, /*PathOffsetCurve*/ nullptr,
		ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, /*ClampVelocityOnFinish*/ 0.0f);
	DodgeTask->OnTimedOut.AddDynamic(this, &UGA_Dodge::OnDodgeFinished);
	DodgeTask->OnTimedOutAndDestinationReached.AddDynamic(this, &UGA_Dodge::OnDodgeFinished);
	DodgeTask->ReadyForActivation();
}

void UGA_Dodge::OnDodgeFinished()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAnimInstance* AnimInst = (Character && Character->GetMesh()) ? Character->GetMesh()->GetAnimInstance() : nullptr;
	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;

	// Монтаж не стартовал - лок не вешаем, невидимого лока в системе не бывает ни при каких
	// условиях (по образцу UClanhallComboComponent::EndSequenceWithRecovery).
	if (AnimInst && EvadeRecoveryMontage && ASC)
	{
		const float PlayedDuration = AnimInst->Montage_Play(EvadeRecoveryMontage);
		if (PlayedDuration > 0.0f)
		{
			ClanhallGameplayEffects::ApplyTimedTag(ASC, ClanhallGameplayTags::State_EvadeRecovery.GetTag(), EvadeRecoveryMontage->GetPlayLength());
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
