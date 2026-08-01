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
	StartDashIfNeeded(Data, Avatar, MontagePlayLength);

	const bool bMontageStarted = (AnimInst != nullptr && MontagePlayLength > 0.0f);

	// Первичный терминатор — конец каст-монтажа всегда, когда монтаж стартовал, независимо от
	// режима резолва урона: длительность отыгрыша анимации не зависит от того, размечены ли на
	// монтаже зоны контакта. Обе строки стоят до ветвления по bResolveOnContact ниже, иначе навык
	// с назначенным, но ещё не размеченным монтажом (текущее состояние всех четырёх Knight,
	// разметка — Блок D п.10) заканчивался бы мгновенно, и WASD-удар обрывал бы его анимацию
	// на любом кадре (task_primary_terminator_fix.md).
	if (bMontageStarted)
	{
		SourceASC->AddLooseGameplayTag(ClanhallGameplayTags::State_SkillCommitted.GetTag());

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UGA_PhysicalSkill::OnCastMontageEnded);
		AnimInst->Montage_SetEndDelegate(EndDelegate, Data->CastMontage);
	}

	const bool bResolveOnContact = UAnimNotifyState_Hitbox::MontageHasHitbox(Data->CastMontage);

	if (!bResolveOnContact || !bMontageStarted)
	{
		// Резолв урона сферой — два случая с разной причиной и разным временем жизни способности:
		// (1) !bMontageStarted — монтажа нет, либо Montage_Play не смог его стартовать
		//     (несовместимый скелет, невалидный слот): отыгрывать нечего, терминатором служит сам
		//     этот мгновенный резолв.
		// (2) bMontageStarted && !bResolveOnContact — монтаж есть и играет, но на нём не расставлен
		//     AnimNotifyState_Hitbox: тег и делегат для этого случая уже поставлены выше,
		//     первичный терминатор придёт из OnCastMontageEnded — здесь его не трогаем.
		if (bResolveOnContact && !bMontageStarted)
		{
			UE_LOG(LogClanhall, Warning, TEXT("ActivateAbility: Montage_Play failed to start %s for %s, falling back to instant resolve"),
				Data->CastMontage ? *Data->CastMontage->GetName() : TEXT("null"), *Avatar->GetName());
		}
		ResolveHitOn(Target);

		if (!bMontageStarted)
		{
			// Монтажа нет — тег коммита нужен только если запущен рывок: без него способность
			// обязана заканчиваться в этом же кадре, а не позже.
			if (bDashPending)
			{
				SourceASC->AddLooseGameplayTag(ClanhallGameplayTags::State_SkillCommitted.GetTag());
			}
			bPrimaryTerminatorFired = true;
		}

		// Подписка на Event.Hitbox.Hit при неразмеченных зонах бессмысленна — урон уже
		// отрезолвлен сферой выше, независимо от того, какой из двух случаев сработал.
		TryFinishAbility();
		return;
	}

	// Event.Hitbox.Hit резолвит попадания. Event.Hitbox.Closed никто из активки не слушает —
	// он не несёт для неё сигнала: способность живёт до терминаторов (см. TryFinishAbility),
	// а не до опустевшего списка зон. Тег продолжает существовать и слаться компонентом,
	// просто активка на него не подписывается.
	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, ClanhallGameplayTags::Event_Hitbox_Hit.GetTag(), nullptr, /*OnlyTriggerOnce*/ false, /*OnlyMatchExact*/ true);
	HitTask->EventReceived.AddDynamic(this, &UGA_PhysicalSkill::OnHitboxHitReceived);
	HitTask->ReadyForActivation();
}

void UGA_PhysicalSkill::OnHitboxHitReceived(FGameplayEventData Payload)
{
	// Зона может задеть несколько целей за одно применение (Shield Charge — капсула на пелвисе
	// на весь рывок). Способность здесь НЕ заканчивается, ждём терминаторы (см. TryFinishAbility).
	ResolveHitOn(const_cast<AActor*>(Payload.Target.Get()));
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

void UGA_PhysicalSkill::StartDashIfNeeded(const UAbilityData* Data, AActor* Avatar, float MontagePlayLength)
{
	const UDashFragment* Dash = Data ? Data->FindFragment<UDashFragment>() : nullptr;
	if (!Dash)
	{
		return;
	}

	// Грубая проверка, не точная: код не знает, где именно в монтаже кончается фаза движения
	// (у Shield Charge — кадр 13 из 33), точную границу держит разметчик. Но самый заметный
	// случай — рывок длиннее самого монтажа целиком — она ловит: без варна причина
	// «проскальзывания по земле» после конца анимации неочевидна (task_counterwindow_symmetry.md,
	// Задача 5).
	if (MontagePlayLength > 0.0f && Dash->Duration > MontagePlayLength)
	{
		UE_LOG(LogClanhall, Warning, TEXT("StartDashIfNeeded: %s dash Duration %.2f exceeds cast montage length %.2f — character will keep sliding after the animation ends"),
			*Data->DisplayName.ToString(), Dash->Duration, MontagePlayLength);
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

	// bDashPending выставлен ДО ReadyForActivation(): если задача когда-нибудь завершится
	// синхронно внутри неё, OnDashFinished должен увидеть уже true и снять его сам, а не быть
	// перезаписанным следующей строкой — иначе способность и State.SkillCommitted повисают
	// навсегда. Сейчас ApplyRootMotionMoveToForce финиширует в TickTask, не в Activate, так что
	// порядок ещё не стрелял, но полагаться на эту деталь реализации движка не стоит.
	bDashPending = true;
	DashTask->ReadyForActivation();
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
	// Единственная точка снятия State.SkillCommitted (не подстраховка — Closed-путь тег больше
	// не трогает вовсе). Момент снятия — конец каст-монтажа и рывка одновременно: EndAbility
	// вызывается только из TryFinishAbility, когда отработали оба терминатора.
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
