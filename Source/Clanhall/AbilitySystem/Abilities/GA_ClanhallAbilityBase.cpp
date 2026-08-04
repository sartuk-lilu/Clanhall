#include "GA_ClanhallAbilityBase.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"

UGA_ClanhallAbilityBase::UGA_ClanhallAbilityBase()
{
	// Все физические действия (WASD и активные навыки) работают только в боевой стойке.
	// Вне стойки те же клавиши двигают персонажа — TryActivateAbility просто откажет.
	ActivationRequiredTags.AddTag(ClanhallGameplayTags::State_InStance.GetTag());

	// Блокирует новые атаки на время лок-аута после Recovery — накрывает и WASD-удары
	// (GA_DirectionalAttack_*), и физактивки Q/E/R/F (GA_PhysicalSkill), обе наследуются отсюда.
	// UGA_CombatStance наследуется от UGameplayAbility напрямую, поэтому этот тег она берёт
	// себе явно, в собственном конструкторе: вход в стойку на время Recovery заблокирован,
	// ВЫХОД остаётся свободным всегда (идёт через CancelAbilityHandle).
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_ComboRecovery.GetTag());

	// E2.4: активка в фазе коммита (State.SkillCommitted, висит на UGA_PhysicalSkill от активации
	// до Event.Hitbox.Closed) блокирует и вторую активку, и WASD-способности — «начатую активку
	// нельзя оборвать» (combat_system.md §3). Выход из стойки тег не трогает: тот идёт через
	// CancelAbilityHandle на способности стойки, не через TryActivateAbility.
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_SkillCommitted.GetTag());

	// main_dev_plan.md §8, Блок D: оглушение (полное парирование серии — ClanhallComboComponent,
	// либо успешный контрнавык — ClanhallCounterComponent) блокирует новые физдействия. Раньше
	// это гейтила только умирающая GA_EnemyWASDSeries, то есть только у врага; тег теперь
	// симметричен (вешается и на игрока), и блокировка обязана быть тоже.
	ActivationBlockedTags.AddTag(ClanhallGameplayTags::State_Stunned.GetTag());
}

AActor* UGA_ClanhallAbilityBase::FindMeleeTarget(AActor* Avatar) const
{
	if (!Avatar)
	{
		return nullptr;
	}

	const FVector Start = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * TraceRange;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Avatar);

	TArray<AActor*> OverlappedActors;
	UKismetSystemLibrary::SphereOverlapActors(Avatar, Start, TraceRadius, ObjectTypes, nullptr, ActorsToIgnore, OverlappedActors);

	for (AActor* Candidate : OverlappedActors)
	{
		if (Candidate && Candidate->Implements<UAbilitySystemInterface>())
		{
			return Candidate;
		}
	}

	return nullptr;
}

bool UGA_ClanhallAbilityBase::ResolveStandardDamage(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float RawDamage) const
{
	if (!SourceASC || !TargetASC || RawDamage <= 0.0f)
	{
		return false;
	}

	const UClanhallAttributeSet* TargetAttributes = TargetASC->GetSet<UClanhallAttributeSet>();
	if (!TargetAttributes)
	{
		return false;
	}

	const float TargetCurrentAP = TargetAttributes->GetAP();
	const float APRemoved = FMath::Min(RawDamage, TargetCurrentAP);
	const float Overflow = RawDamage - APRemoved;

	if (APRemoved > 0.0f)
	{
		ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, TargetASC, UGE_ModifyAP::StaticClass(), -APRemoved);
		ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, SourceASC, UGE_ModifyAP::StaticClass(), APRemoved * 0.5f);
	}

	if (Overflow > 0.0f)
	{
		ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, TargetASC, UGE_ModifyHP::StaticClass(), -Overflow);
	}

	// task_parry_rework.md §1.4/§1.5: подтверждённый (не парированный) удар — второй сквозной
	// принцип «кто первым нанёс контакт, сбивает другого». Цель получила контакт по себе — её
	// собственная зона (если есть/скоро откроется) гасится до конца текущего шага, симметрично
	// клэшу в UClanhallParryComponent::TryParry. Атакующий — тот, чья зона нанесла контакт —
	// получает хитстоп; роль (игрок/AI) нигде не проверяется.
	if (AActor* TargetAvatar = TargetASC->GetAvatarActor())
	{
		if (UClanhallHitboxComponent* TargetHitbox = TargetAvatar->FindComponentByClass<UClanhallHitboxComponent>())
		{
			TargetHitbox->SuppressHitboxes();
		}
	}

	if (AActor* SourceAvatar = SourceASC->GetAvatarActor())
	{
		if (UClanhallHitboxComponent* SourceHitbox = SourceAvatar->FindComponentByClass<UClanhallHitboxComponent>())
		{
			SourceHitbox->ApplyHitstop(SourceHitbox->HitstopDurationOnDamage);
		}
	}

	return true;
}

float UGA_ClanhallAbilityBase::GetBalanceSign(const UAbilitySystemComponent* SourceASC) const
{
	const bool bIsSTR = SourceASC && SourceASC->HasMatchingGameplayTag(ClanhallGameplayTags::Weapon_Type_STR.GetTag());
	return bIsSTR ? 1.0f : -1.0f;
}
