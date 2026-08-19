#include "GA_CombatStance.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/WeaponTypeData.h"
#include "ClanhallCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_CombatStance::UGA_CombatStance()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationOwnedTags.AddTag(ClanhallGameplayTags::State_InStance.GetTag());
	// Нельзя войти в стойку второй раз, пока уже в ней — TryActivateAbility просто откажет.
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_InStance.GetTag());

	// Пока доигрывает Recovery — в стойку не войти. Это и есть видимая причина лок-аута:
	// руки ещё доводят возврат. ВЫХОД из стойки при этом свободен всегда — он идёт через
	// CancelAbilityHandle и ActivationBlockedTags не касается.
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_ComboRecovery.GetTag());
}

void UGA_CombatStance::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AClanhallCharacter* Character = Cast<AClanhallCharacter>(GetAvatarActorFromActorInfo());
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Character || !Movement)
	{
		return;
	}

	// Гасит бег и таймеры Пробела ДО чтения MaxWalkSpeed ниже — вызывается отсюда, а не из
	// AClanhallCharacter::OnStancePressed: тот выполняется каждый кадр удержания ЛКМ (ретрай
	// на Triggered при State.ComboRecovery), а этот метод — ровно один раз на реальный вход
	// (`task_stage4_code_fixes.md`, п.5).
	Character->CancelSpaceHoldAndSprint();

	bSavedOrientRotationToMovement = Movement->bOrientRotationToMovement;
	bSavedUseControllerRotationYaw = Character->bUseControllerRotationYaw;
	SavedMaxWalkSpeed = Movement->MaxWalkSpeed;

	// В стойке разворот — за камерой (мышь), не за направлением движения: ход спиной
	// не должен разворачивать персонаж лицом по ходу (`locomotion_structure.md`, «Локомоция стойки»).
	Movement->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;

	// Оружия нет или множитель не прочитался — берётся StanceBaseSpeed как есть, множитель 1.0
	// (`weapon_system.md`: множитель применяется к отдельной базовой скорости стойки, не к бегу).
	const UWeaponTypeData* WeaponType = Character->GetWeaponType();
	const float SpeedMultiplier = WeaponType ? WeaponType->StanceSpeedMultiplier : ClanhallWeaponDefaults::StanceSpeedMultiplier;
	Movement->MaxWalkSpeed = Character->GetStanceBaseSpeed() * SpeedMultiplier;

	// Вход в стойку на бегу гасит разгон — дальше игрок двигается уже по правилам стойки.
	Movement->StopMovementImmediately();
}

void UGA_CombatStance::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	AClanhallCharacter* Character = ActorInfo ? Cast<AClanhallCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (Character && Movement)
	{
		Movement->bOrientRotationToMovement = bSavedOrientRotationToMovement;
		Character->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
		Movement->MaxWalkSpeed = SavedMaxWalkSpeed;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
