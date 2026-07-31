#include "GA_PhysicalSkill.h"
#include "Clanhall.h"
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
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
#include "Engine/Engine.h"

UGA_PhysicalSkill::UGA_PhysicalSkill()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	// Тег коммита намеренно НЕ в ActivationOwnedTags, а вешается loose'ом в ActivateAbility —
	// иначе для не-контактных (пустых по монтажу) активаций он не повесился бы вовсе.
	// Решение разработчика (закрывает бывший «Открытый вопрос 1» Раздела 7): начатую игроком
	// активку нельзя прервать вручную — она доигрывает каст-монтаж целиком (а с рывком —
	// и сам рывок, см. TryFinishAbility), единственный способ её сбить — контратака противника
	// своей активкой. Тег снимается только в EndAbility.
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

	// КД проверяется здесь (тег вешается на активации, см. ActivateAbility), а Charges —
	// на активацию (combat_system.md §1: "проверяется количество зарядов на активацию").
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

	// Поиск цели нужен фолбэк-пути (урон мгновенно, без ожидания контакта). Контрнавык цель
	// отсюда НЕ берёт — он резолвится по контакту, в ResolveHitOn: зона атакующего касается
	// тела защищающегося, состояние защищающегося проверяется в момент касания, тем же
	// паттерном, что и парирование (CheckAndHandleParry).
	AActor* Target = FindMeleeTarget(Avatar);

	// Активка коммитится в момент нажатия: заряды уходят всегда, включая промах и контр —
	// цена решения «придержать навык на контратаку» и есть эти заряды (ability_system.md §2).
	if (Data->ChargeCost > 0)
	{
		ClanhallGameplayEffects::ApplyModifyEffect(SourceASC, SourceASC, UGE_ModifyCharges::StaticClass(), -static_cast<float>(Data->ChargeCost));
	}

	// КД уходит НА АКТИВАЦИИ безусловно, без исключения для контра — тот больше не бесплатен,
	// он лишь побочный эффект попадания. Промах стоит зарядов и КД (combat_system.md §3).
	if (Data->CooldownTag.IsValid())
	{
		ClanhallGameplayEffects::ApplyTimedTag(SourceASC, Data->CooldownTag, Data->Cooldown);
	}

	// Порядок критичен: серию гасим и монтаж запускаем ДО подписки на Event.Hitbox.*.
	// CancelSequenceForExternalMontage() зовёт ForceEndHitboxes(), а тот при непустом списке
	// зон шлёт Event.Hitbox.Closed — подпишись мы раньше, активка получила бы его сама и
	// закончилась до открытия собственной зоны (тот же баг, что D2 закрыл для чейна).
	UAnimInstance* AnimInst = nullptr;
	float MontagePlayLength = 0.0f;
	if (Data->CastMontage)
	{
		if (ACharacter* Char = Cast<ACharacter>(Avatar))
		{
			AnimInst = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
			if (AnimInst)
			{
				if (UClanhallComboComponent* Combo = Char->FindComponentByClass<UClanhallComboComponent>())
				{
					Combo->CancelSequenceForExternalMontage();
				}
				MontagePlayLength = AnimInst->Montage_Play(Data->CastMontage);
			}
		}
	}

	// Задача 1 (task_dash_and_counter_window.md): рывок — данные навыка, не свойство клипа.
	// Запускается ПОСЛЕ списания ресурсов и ПОСЛЕ Montage_Play, ДО ветвления на контактный/
	// фолбэк путь — обязан работать на обоих путях.
	StartDashIfNeeded(Data, Avatar);

	const bool bResolveOnContact = UAnimNotifyState_Hitbox::MontageHasHitbox(Data->CastMontage);
	const bool bMontageStarted = (AnimInst != nullptr && MontagePlayLength > 0.0f);

	if (!bResolveOnContact || !bMontageStarted)
	{
		// Мгновенный фолбэк. Два случая ведут сюда: (1) нет монтажа или на монтаже не расставлены
		// зоны — осознанный фолбэк, он позволяет проверять навык и его фрагменты до нарезки анимаций
		// (инвариант Раздела 7), все четыре ассета Knight сейчас идут именно этим путём; (2) есть
		// зона, но Montage_Play не смог стартовать монтаж (несовместимый скелет, невалидный слот) —
		// тогда AnimNotifyState_Hitbox никогда не откроется и Event.Hitbox.Closed не придёт, а зона
		// была объявлена — резолвим той же сферой, что и в (1), отдельной ветки поведения не нужно.
		if (bResolveOnContact && !bMontageStarted)
		{
			UE_LOG(LogClanhall, Warning, TEXT("ActivateAbility: Montage_Play failed to start %s for %s, falling back to instant resolve"),
				Data->CastMontage ? *Data->CastMontage->GetName() : TEXT("null"), *Avatar->GetName());
		}
		ResolveHitOn(Target);

		// Тег коммита имеет смысл на фолбэке только если запущен рывок: без него способность
		// заканчивается в этом же кадре, и вешать тег было бы бессмысленно.
		if (bDashPending)
		{
			SourceASC->AddLooseGameplayTag(ClanhallGameplayTags::State_SkillCommitted.GetTag());
		}

		bPrimaryTerminatorFired = true;
		TryFinishAbility();
		return;
	}

	// Тег коммита живёт весь каст-монтаж (и, если запущен, весь рывок — см. TryFinishAbility) —
	// вешается loose'ом только здесь, на контактном пути (на фолбэке выше коммититься не во что
	// без рывка, способность уже закончилась). Снимается только в EndAbility.
	SourceASC->AddLooseGameplayTag(ClanhallGameplayTags::State_SkillCommitted.GetTag());

	// Первичный терминатор способности — конец каст-монтажа, а не Event.Hitbox.Closed (тот
	// означает «сейчас нет открытых зон», а не «удар закончился»: между двумя последовательными
	// зонами список зон пустеет, и способность на контактном Closed умерла бы в промежутке между
	// фазами).
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UGA_PhysicalSkill::OnCastMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndDelegate, Data->CastMontage);

	// Event.Hitbox.Hit по-прежнему резолвит попадания. Event.Hitbox.Closed больше НЕ заканчивает
	// способность — только снимает тег коммита, поэтому OnlyTriggerOnce стал false: мультизонный
	// удар шлёт Closed на каждом переходе «были зоны -> зон не осталось».
	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, ClanhallGameplayTags::Event_Hitbox_Hit.GetTag(), nullptr, /*OnlyTriggerOnce*/ false, /*OnlyMatchExact*/ true);
	HitTask->EventReceived.AddDynamic(this, &UGA_PhysicalSkill::OnHitboxHitReceived);
	HitTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* ClosedTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, ClanhallGameplayTags::Event_Hitbox_Closed.GetTag(), nullptr, /*OnlyTriggerOnce*/ false, /*OnlyMatchExact*/ true);
	ClosedTask->EventReceived.AddDynamic(this, &UGA_PhysicalSkill::OnHitboxClosedReceived);
	ClosedTask->ReadyForActivation();
}

void UGA_PhysicalSkill::OnHitboxHitReceived(FGameplayEventData Payload)
{
	// Зона может задеть несколько целей за одно применение (Shield Charge — капсула на пелвисе
	// на весь рывок). Способность здесь НЕ заканчивается, ждём терминатор монтажа/страховку Closed.
	ResolveHitOn(const_cast<AActor*>(Payload.Target.Get()));
}

void UGA_PhysicalSkill::OnHitboxClosedReceived(FGameplayEventData Payload)
{
	// Задача 2 (task_dash_and_counter_window.md): State.SkillCommitted больше НЕ снимается
	// здесь — тег живёт до EndAbility (весь каст-монтаж, а с Задачей 1 — и весь рывок).
	// Подписка на Event.Hitbox.Closed остаётся нужна: OnHitboxHitReceived один резолвит
	// попадания, а закрытие зоны само по себе сигнала не несёт помимо этого.
}

void UGA_PhysicalSkill::OnCastMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// Срабатывает и на нормальном завершении, и на прерывании монтажа. Способность
	// заканчивается только когда отработали оба терминатора (см. TryFinishAbility).
	bPrimaryTerminatorFired = true;
	TryFinishAbility();
}

void UGA_PhysicalSkill::OnDashFinished()
{
	bDashPending = false;
	TryFinishAbility();
}

void UGA_PhysicalSkill::StartDashIfNeeded(const UAbilityData* Data, AActor* Avatar)
{
	const UDashFragment* Dash = Data ? Data->FindFragment<UDashFragment>() : nullptr;
	if (!Dash)
	{
		return;
	}

	if (!Cast<ACharacter>(Avatar))
	{
		UE_LOG(LogClanhall, Warning, TEXT("StartDashIfNeeded: %s has UDashFragment but Avatar %s is not ACharacter (no CharacterMovementComponent), dash skipped"),
			*Data->DisplayName.ToString(), *Avatar->GetName());
		return;
	}

	// Направление фиксируется на активации, наведения на цель нет.
	const FVector Forward = Avatar->GetActorForwardVector();
	const FVector TargetLocation = Avatar->GetActorLocation() + Forward * Dash->Distance;

	UAbilityTask_ApplyRootMotionMoveToForce* DashTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this, TEXT("ClanhallDash"), TargetLocation, Dash->Duration,
		/*bSetNewMovementMode*/ false, EMovementMode::MOVE_Walking,
		/*bRestrictSpeedToExpected*/ false, /*PathOffsetCurve*/ nullptr,
		ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, /*ClampVelocityOnFinish*/ 0.0f);
	DashTask->OnTimedOut.AddDynamic(this, &UGA_PhysicalSkill::OnDashFinished);
	DashTask->OnTimedOutAndDestinationReached.AddDynamic(this, &UGA_PhysicalSkill::OnDashFinished);
	DashTask->ReadyForActivation();

	bDashPending = true;
}

void UGA_PhysicalSkill::TryFinishAbility()
{
	if (bPrimaryTerminatorFired && !bDashPending)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_PhysicalSkill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Страховка от залипания State.SkillCommitted, если Event.Hitbox.Closed так и не пришёл
	// (прерванный монтаж, ForceEndHitboxes и т.п.) — безопасно, даже если тег уже снят выше.
	if (UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		SourceASC->RemoveLooseGameplayTag(ClanhallGameplayTags::State_SkillCommitted.GetTag());
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
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

	// Контр резолвится ПО КОНТАКТУ, как и парирование: зона дотянулась до тела врага, и в этот
	// момент проверяется его открытое окно. Промах и недостаточная дальность = контра не было.
	// Двойное срабатывание невозможно: ConsumeCounter закрывает окно, второй вызов вернёт false.
	UClanhallCounterComponent::TryResolveCounter(Target, Data->CounterTag);

	// Метка/синергия/ChargeGain — один раз НА ЦЕЛЬ за применение, а не на каждый контакт: два
	// хитбокса на монтаже дают два урона, но вторая фаза не должна видеть метку, которую только
	// что положила первая (синергия с корневым RequiredMark сработала бы на собственной метке).
	const bool bMarkAlreadyResolved = MarkResolvedTargets.ContainsByPredicate([Target](const TWeakObjectPtr<AActor>& Weak)
	{
		return Weak.Get() == Target;
	});
	if (!bMarkAlreadyResolved)
	{
		MarkResolvedTargets.Add(Target);

		if (UClanhallMarkComponent* TargetMarkComponent = Target->FindComponentByClass<UClanhallMarkComponent>())
		{
			ResolveMarkLogic(Data, SourceASC, TargetASC, TargetMarkComponent);
		}
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
