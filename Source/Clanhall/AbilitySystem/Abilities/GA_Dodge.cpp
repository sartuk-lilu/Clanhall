#include "GA_Dodge.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallDamage.h"
#include "AbilitySystem/WeaponData.h"
#include "AbilitySystem/WeaponTypeData.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "ClanhallCombatSettings.h"
#include "ClanhallHumanoidBase.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Dodge::UGA_Dodge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Пока отскок идёт, новые действия закрыты: цена общего действия - его длина в долях
	// (`CombatSystem.md`, «Общие действия»). Тот же тег вешает хвост восстановления после.
	ActivationOwnedTags.AddTag(ClanhallGameplayTags::State_EvadeRecovery.GetTag());

	// ActivationBlockedTags блокирует ЛЮБУЮ активацию, пока отскок или хвост доигрывает - общий
	// для уклонения и приседа.
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_EvadeRecovery.GetTag());

	// Начатый приём доигрывает целиком (`CombatSystem.md`) - уход посреди него закрыт.
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_SkillCommitted.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_WeaponSwap.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_Dead.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_Stunned.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_TauntParried.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_TauntParrying.GetTag());
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_UsingCrystal.GetTag());
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

	// Анимация, дальность и длина в долях - свои у оружия (`CombatSystem.md`, «Уклонение»).
	const AClanhallHumanoidBase* Humanoid = Cast<AClanhallHumanoidBase>(Character);
	const UWeaponTypeData* WeaponType = Humanoid ? Humanoid->GetWeaponType() : nullptr;
	const float Distance = WeaponType ? WeaponType->DodgeDistance : ClanhallWeaponDefaults::DodgeDistance;
	const float DurationSeconds = ClanhallBeats::ToSeconds(WeaponType ? WeaponType->DodgeBeats : ClanhallWeaponDefaults::DodgeBeats);
	TouchDamage = WeaponType ? WeaponType->DodgeTouchDamage : 0.0f;

	// Направление берётся от корпуса, а не от камеры и не от вектора ввода (`CombatSystem.md`):
	// в страйфе, чтобы уйти влево, вектор ввода потребовал бы сначала поехать влево - то есть
	// занять плохую позицию, прежде чем получить право из неё уйти.
	FVector WorldDirection = FVector::ZeroVector;
	UAnimMontage* TravelMontage = nullptr;
	switch (Direction)
	{
	case EClanhallEvadeDirection::Left:
		WorldDirection = -Character->GetActorRightVector();
		TravelMontage = WeaponType ? WeaponType->DodgeMontageLeft.Get() : nullptr;
		break;
	case EClanhallEvadeDirection::Right:
		WorldDirection = Character->GetActorRightVector();
		TravelMontage = WeaponType ? WeaponType->DodgeMontageRight.Get() : nullptr;
		break;
	case EClanhallEvadeDirection::Back:
		WorldDirection = -Character->GetActorForwardVector();
		TravelMontage = WeaponType ? WeaponType->DodgeMontageBack.Get() : nullptr;
		break;
	case EClanhallEvadeDirection::Forward:
	default:
		WorldDirection = Character->GetActorForwardVector();
		TravelMontage = WeaponType ? WeaponType->DodgeMontageForward.Get() : nullptr;
		break;
	}

	// Косметика — механика не зависит от того, стартовал ли монтаж. Монтаж подгоняется под длину
	// отскока в долях, как монтаж приёма.
	if (TravelMontage)
	{
		if (UAnimInstance* AnimInst = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
		{
			const float ClipLength = TravelMontage->GetPlayLength();
			const float PlayRate = (ClipLength > 0.0f && DurationSeconds > 0.0f) ? ClipLength / DurationSeconds : 1.0f;
			AnimInst->Montage_Play(TravelMontage, PlayRate);
		}
	}

	// Отклик по касанию: Hitbox-нотифай на монтаже отскока шлёт Event.Hitbox.Hit. Приём в это время
	// идти не может (State.EvadeRecovery закрывает активки), поэтому инвариант «один резолвер на
	// актора» не нарушается.
	if (TouchDamage > 0.0f)
	{
		UAbilityTask_WaitGameplayEvent* TouchTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, ClanhallGameplayTags::Event_Hitbox_Hit.GetTag(), nullptr, /*OnlyTriggerOnce*/ false, /*OnlyMatchExact*/ true);
		TouchTask->EventReceived.AddDynamic(this, &UGA_Dodge::OnTouchHit);
		TouchTask->ReadyForActivation();
	}

	const FVector TargetLocation = Character->GetActorLocation() + WorldDirection * Distance;

	// Корпус на уходе не доворачивается - смещение и только. Кадров неуязвимости нет: перемещение
	// капсулы Root Motion Source'ом - не LaunchCharacter и не телепорт SetActorLocation, иначе свип
	// противника прошёл бы сквозь то место, где игрока уже нет, но коллизия ещё есть.
	UAbilityTask_ApplyRootMotionMoveToForce* DodgeTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this, TEXT("ClanhallDodge"), TargetLocation, FMath::Max(DurationSeconds, 0.01f),
		/*bSetNewMovementMode*/ false, EMovementMode::MOVE_Walking,
		/*bRestrictSpeedToExpected*/ false, /*PathOffsetCurve*/ nullptr,
		ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, /*ClampVelocityOnFinish*/ 0.0f);
	DodgeTask->OnTimedOut.AddDynamic(this, &UGA_Dodge::OnDodgeFinished);
	DodgeTask->OnTimedOutAndDestinationReached.AddDynamic(this, &UGA_Dodge::OnDodgeFinished);
	DodgeTask->ReadyForActivation();
}

void UGA_Dodge::OnTouchHit(FGameplayEventData Payload)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	AActor* Target = const_cast<AActor*>(Payload.Target.Get());
	IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(Target);
	if (!Avatar || !TargetInterface)
	{
		return;
	}

	// Отклик - просто удар, без меток (`CombatSystem.md`, «Уклонение»): уклонение - защита, а не
	// нападение. Контр, метки и перезарядки он не трогает; порог и спина - по общим правилам.
	const AClanhallHumanoidBase* Humanoid = Cast<AClanhallHumanoidBase>(Avatar);
	const UWeaponData* Weapon = Humanoid ? Humanoid->GetCurrentWeapon() : nullptr;
	const UWeaponTypeData* WeaponType = Humanoid ? Humanoid->GetWeaponType() : nullptr;

	FClanhallHitContext Hit;
	Hit.SourceActor = Avatar;
	Hit.SourceASC = GetAbilitySystemComponentFromActorInfo();
	Hit.TargetActor = Target;
	Hit.TargetASC = TargetInterface->GetAbilitySystemComponent();
	Hit.RawDamage = TouchDamage;
	Hit.DamageType = Weapon ? Weapon->DamageType : FGameplayTag();
	Hit.ArmorPenetration = (WeaponType ? WeaponType->ArmorPenetration : 0.0f) + (Weapon ? Weapon->ArmorPenetrationBonus : 0.0f);
	ClanhallDamage::ResolveHit(Hit);
}

void UGA_Dodge::OnDodgeFinished()
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAnimInstance* AnimInst = (Character && Character->GetMesh()) ? Character->GetMesh()->GetAnimInstance() : nullptr;
	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;

	// Монтаж хвоста не стартовал - лок не продлеваем: невидимого лока в системе не бывает.
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
