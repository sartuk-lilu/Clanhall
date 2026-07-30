#include "GA_PhysicalSkill.h"
#include "AbilitySystem/AbilityData.h"
#include "AbilitySystem/Fragments/GameplayFragments.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/ClanhallMarkTypes.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallCounterComponent.h"
#include "AbilitySystem/ClanhallComboComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "Animation/AnimNotifyState_Hitbox.h"
#include "Animation/AnimMontage.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
#include "Engine/Engine.h"

UGA_PhysicalSkill::UGA_PhysicalSkill()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	// E2.4: тег коммита живёт ровно окно контакта (ActivationOwnedTags снимается автоматически
	// на EndAbility). На контактном пути висит от активации до Event.Hitbox.Closed; на
	// фолбэк-пути способность заканчивается сразу же, тега фактически нет — без анимации
	// коммититься не во что.
	ActivationOwnedTags.AddTag(ClanhallGameplayTags::State_SkillCommitted.GetTag());
}

const UAbilityData* UGA_PhysicalSkill::GetAbilityData(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return nullptr;
	}

	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
	return Spec ? Cast<UAbilityData>(Spec->SourceObject.Get()) : nullptr;
}

bool UGA_PhysicalSkill::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UAbilityData* Data = GetAbilityData(Handle, ActorInfo);
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!Data || !ASC)
	{
		return false;
	}

	// КД проверяется здесь (тег вешается на активации, см. ActivateAbility — исключение: успешный
	// контрнавык КД не вешает вовсе), а Charges — на активацию (combat_system.md §1: "проверяется
	// количество зарядов на активацию").
	if (Data->CooldownTag.IsValid() && ASC->HasMatchingGameplayTag(Data->CooldownTag))
	{
		return false;
	}

	if (Data->ChargeCost > 0)
	{
		const UClanhallAttributeSet* Attributes = ASC->GetSet<UClanhallAttributeSet>();
		if (!Attributes || Attributes->GetCharges() < static_cast<float>(Data->ChargeCost))
		{
			return false;
		}
	}

	return true;
}

void UGA_PhysicalSkill::ResolveMarkLogic(const UAbilityData* Data, UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, UClanhallMarkComponent* TargetMarkComponent)
{
	if (!TargetMarkComponent)
	{
		return;
	}

	if (const UMarkTriggerFragment* Trigger = Data->FindFragment<UMarkTriggerFragment>())
	{
		const FGameplayTag CurrentMark = TargetMarkComponent->GetCurrentMark();
		if (CurrentMark.IsValid())
		{
			for (const FMarkSynergy& Synergy : Trigger->Synergies)
			{
				// MatchesTag, а не ==: RequiredMark задаёт КРУПНОСТЬ записи. Конкретный тег
				// (Mark.BrokenGuard) матчит только себя; корневой Mark матчит любую метку — так
				// выражается «активирует независимо от типа метки» (Knight Retribution,
				// physical_abilities.md). Тот же приём, что у CounteredBy в контрнавыке.
				if (!Synergy.RequiredMark.IsValid() || !CurrentMark.MatchesTag(Synergy.RequiredMark))
				{
					continue;
				}

				// mark_system.md §2 Правило 3: метка сгорает -> бафф на себя ИЛИ дебафф на цель, никогда оба.
				TargetMarkComponent->ClearMark();

				if (Synergy.EffectOnTarget)
				{
					// Состояние на цель — на каждой задетой цели.
					ClanhallGameplayEffects::ApplyEffect(SourceASC, TargetASC, Synergy.EffectOnTarget);
				}
				else if (Synergy.EffectOnSelf && !bSelfSynergySpent)
				{
					// Состояние на себя — один раз за применение, сколько бы целей ни задело:
					// трёхкратно наложенный баф либо бессмыслен, либо стакается непредсказуемо.
					ClanhallGameplayEffects::ApplyEffect(SourceASC, SourceASC, Synergy.EffectOnSelf);
					bSelfSynergySpent = true;
				}

				// Заряды — РЕСУРС, а не состояние: начисляются ЗА КАЖДУЮ задетую цель и НЕ
				// гатятся bSelfSynergySpent. Мультицель — редкая ситуация повышенного риска, и она
				// должна награждать. От переполнения защищает кламп MaxCharges в UClanhallAttributeSet.
				if (Synergy.ChargeGain > 0)
				{
					ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, SourceASC, UGE_ModifyCharges::StaticClass(), static_cast<float>(Synergy.ChargeGain));
				}
				break;
			}
		}
	}

	if (const UMarkApplyFragment* MarkApply = Data->FindFragment<UMarkApplyFragment>())
	{
		TargetMarkComponent->ApplyMark(MarkApply->MarkTag, SourceASC);
	}
}

void UGA_PhysicalSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const UAbilityData* Data = GetAbilityData(Handle, ActorInfo);

	if (!SourceASC || !Avatar || !Data)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Поиск цели нужен и для урона (фолбэк-путь), и для контр-запроса. Контр — это вопрос
	// «кому я отвечаю», а не проверка попадания.
	AActor* Target = FindMeleeTarget(Avatar);

	// Резолвер контрнавыка: до списания Charges (ability_system.md §2).
	// Совпал CounterTag этого навыка с открытым окном цели → навык цели сбит + получает полный КД.
	// Контр обязан сбить навык врага немедленно, до анимации — резолвится на активации, не на контакте.
	const bool bWasCounter = Target && UClanhallCounterComponent::TryResolveCounter(Target, Data->CounterTag);

	// Активка коммитится в момент нажатия: заряды уходят всегда, включая контр и промах.
	// Контр тратит ресурс так же, как обычное применение (ability_system.md §2) — цена
	// решения «придержать навык на контратаку» и есть эти заряды.
	if (Data->ChargeCost > 0)
	{
		ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, SourceASC, UGE_ModifyCharges::StaticClass(), -static_cast<float>(Data->ChargeCost));
	}

	if (bWasCounter)
	{
		// Навык врага сбит и получил полный КД. Навык игрока в КД НЕ уходит и готов сразу —
		// ограничен только зарядами (ability_system.md §2, «Успешная контратака»).
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// КД уходит НА АКТИВАЦИИ, а не на подтверждённом попадании: активку нельзя оборвать,
	// промах стоит зарядов и КД. Это цена коммита (combat_system.md §3).
	if (Data->CooldownTag.IsValid())
	{
		ClanhallGameplayEffects::ApplyTimedTag(SourceASC, Data->CooldownTag, Data->Cooldown);
	}

	// Порядок критичен: серию гасим и монтаж запускаем ДО подписки на Event.Hitbox.*.
	// CancelSequenceForExternalMontage() зовёт ForceEndHitboxes(), а тот при непустом списке
	// зон шлёт Event.Hitbox.Closed — подпишись мы раньше, активка получила бы его сама и
	// закончилась до открытия собственной зоны (тот же баг, что D2 закрыл для чейна).
	if (Data->CastMontage)
	{
		if (ACharacter* Char = Cast<ACharacter>(Avatar))
		{
			if (UAnimInstance* AnimInst = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr)
			{
				if (UClanhallComboComponent* Combo = Char->FindComponentByClass<UClanhallComboComponent>())
				{
					Combo->CancelSequenceForExternalMontage();
				}
				AnimInst->Montage_Play(Data->CastMontage);
			}
		}
	}

	const bool bResolveOnContact = UAnimNotifyState_Hitbox::MontageHasHitbox(Data->CastMontage);

	if (!bResolveOnContact)
	{
		// Нет монтажа или на монтаже не расставлены зоны — мгновенный резолв по цели, найденной
		// на активации, без ожидания контакта. Осознанный фолбэк: он позволяет проверять навык и его
		// фрагменты до нарезки анимаций (инвариант Раздела 7). Все четыре ассета Knight сейчас
		// идут именно этим путём — CastMontage у них пока nullptr.
		ResolveHitOn(Target);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Контактный путь: EndAbility НЕ вызывается здесь — ждём Event.Hitbox.Hit/Closed.
	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, ClanhallGameplayTags::Event_Hitbox_Hit.GetTag(), nullptr, /*OnlyTriggerOnce*/ false, /*OnlyMatchExact*/ true);
	HitTask->EventReceived.AddDynamic(this, &UGA_PhysicalSkill::OnHitboxHitReceived);
	HitTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* ClosedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, ClanhallGameplayTags::Event_Hitbox_Closed.GetTag(), nullptr, /*OnlyTriggerOnce*/ true, /*OnlyMatchExact*/ true);
	ClosedTask->EventReceived.AddDynamic(this, &UGA_PhysicalSkill::OnHitboxClosedReceived);
	ClosedTask->ReadyForActivation();
}

void UGA_PhysicalSkill::OnHitboxHitReceived(FGameplayEventData Payload)
{
	// Зона может задеть несколько целей за одно применение (Shield Charge — капсула на пелвисе
	// на весь рывок). Способность здесь НЕ заканчивается, ждём Event.Hitbox.Closed.
	ResolveHitOn(const_cast<AActor*>(Payload.Target.Get()));
}

void UGA_PhysicalSkill::OnHitboxClosedReceived(FGameplayEventData Payload)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_PhysicalSkill::ResolveHitOn(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	const UAbilityData* Data = GetAbilityData(CurrentSpecHandle, CurrentActorInfo);
	if (!SourceASC || !Data)
	{
		return;
	}

	IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(Target);
	UAbilitySystemComponent* TargetASC = TargetInterface ? TargetInterface->GetAbilitySystemComponent() : nullptr;

	bool bConfirmedHit = false;
	if (const UDamageFragment* Damage = Data->FindFragment<UDamageFragment>())
	{
		bConfirmedHit = ResolveStandardDamage(SourceASC, TargetASC, Damage->BaseDamage);
	}
	else
	{
		// Утилитарный навык без урона — попадание подтверждено самим фактом найденной цели,
		// иначе КД и метки у таких навыков никогда бы не срабатывали.
		bConfirmedHit = (TargetASC != nullptr);
	}

	if (!bConfirmedHit)
	{
		return;
	}

	if (UClanhallMarkComponent* TargetMarkComponent = Target->FindComponentByClass<UClanhallMarkComponent>())
	{
		ResolveMarkLogic(Data, SourceASC, TargetASC, TargetMarkComponent);
	}

	// Balance — СОСТОЯНИЕ, а не ресурс: сдвигается один раз за применение навыка, сколько бы
	// целей ни задело (mark_system.md §2, правило «ресурс/состояние»). Тройной сдвиг сломал бы шкалу.
	if (!bBalanceApplied && !FMath::IsNearlyZero(Data->BalanceShift))
	{
		const float Shift = GetBalanceSign(SourceASC) * Data->BalanceShift;
		ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, SourceASC, UGE_ModifyBalance::StaticClass(), Shift);
		bBalanceApplied = true;
	}

#if !UE_BUILD_SHIPPING
	// На каждое попадание, без гейта: при мультицели полезно видеть, сколько целей задело.
	if (const UClanhallAttributeSet* SelfAttributes = SourceASC->GetSet<UClanhallAttributeSet>())
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Orange, FString::Printf(
			TEXT("%s hit | AP %.0f/%.0f  Charges %.0f/%.0f  Balance %.1f"),
			*Data->DisplayName.ToString(), SelfAttributes->GetAP(), SelfAttributes->GetMaxAP(),
			SelfAttributes->GetCharges(), SelfAttributes->GetMaxCharges(), SelfAttributes->GetBalance()));
	}
#endif
}
