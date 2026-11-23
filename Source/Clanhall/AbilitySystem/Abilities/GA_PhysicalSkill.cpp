#include "GA_PhysicalSkill.h"
#include "Clanhall.h"
#include "AbilitySystem/AbilityData.h"
#include "AbilitySystem/Fragments/GameplayFragments.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/ClanhallMarkTypes.h"
#include "ClanhallCombatTypes.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallCounterComponent.h"
#include "AbilitySystem/ClanhallSkillCooldownComponent.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystem/WeaponData.h"
#include "ClanhallHumanoidBase.h"
#include "Animation/AnimNotifyState_Hitbox.h"
#include "Animation/AnimNotifyState_CounterWindow.h"
#include "Animation/AnimMontage.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "ClanhallCombatSettings.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
#include "Engine/Engine.h"

namespace
{
	/** Открытый вопрос: если зона поражения открывается позже
	 *  конца окна контра, контр физически не мог сработать раньше, чем удар нанёс урон — разметчик
	 *  посадил Hitbox не в ту фазу замаха. Рантайм-варн один раз за монтаж; полноценная редакторная
	 *  проверка — в бэклог. */
	void WarnIfHitboxOutlivesCounterWindow(const UAnimMontage* Montage, const FString& AbilityName)
	{
		if (!Montage)
		{
			return;
		}

		static TSet<const UAnimMontage*> WarnedMontages;
		if (WarnedMontages.Contains(Montage))
		{
			return;
		}

		float CounterWindowEnd = -1.0f;
		for (const FAnimNotifyEvent& Event : Montage->Notifies)
		{
			if (Cast<UAnimNotifyState_CounterWindow>(Event.NotifyStateClass))
			{
				CounterWindowEnd = FMath::Max(CounterWindowEnd, Event.GetEndTriggerTime());
			}
		}

		if (CounterWindowEnd < 0.0f)
		{
			return; // на монтаже вовсе нет окна контра — сравнивать не с чем
		}

		for (const FAnimNotifyEvent& Event : Montage->Notifies)
		{
			if (Cast<UAnimNotifyState_Hitbox>(Event.NotifyStateClass) && Event.GetTriggerTime() > CounterWindowEnd)
			{
				UE_LOG(LogClanhall, Warning, TEXT("%s: Hitbox on %s opens at %.2fs, after CounterWindow closes at %.2fs — counter cannot land before the hit resolves"),
					*AbilityName, *Montage->GetName(), Event.GetTriggerTime(), CounterWindowEnd);
				WarnedMontages.Add(Montage);
				return;
			}
		}
	}
}

UGA_PhysicalSkill::UGA_PhysicalSkill()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	// Тег коммита намеренно НЕ в ActivationOwnedTags, а вешается loose'ом в ActivateAbility —
	// иначе для не-контактных (пустых по монтажу) активаций он не повесился бы вовсе.
	// Решение разработчика: начатую игроком
	// активку нельзя прервать вручную — она доигрывает каст-монтаж целиком, единственный способ
	// её сбить — контратака противника своей активкой. Тег снимается только в EndAbility.
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

FGameplayTag UGA_PhysicalSkill::GetAbilitySlotTag(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return FGameplayTag();
	}

	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
	if (!Spec)
	{
		return FGameplayTag();
	}

	for (const FGameplayTag& Tag : Spec->GetDynamicSpecSourceTags())
	{
		if (Tag.MatchesTag(ClanhallGameplayTags::Slot.GetTag()))
		{
			return Tag;
		}
	}

	return FGameplayTag();
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

	// Гейт применения — собственная перезарядка приёма (`EconomySystem.md`, «Перезарядка приёмов»).
	// Ключ - идентичность приёма, не спек: тот же приём в двух оружиях перезаряжается один раз.
	const AActor* Avatar = ActorInfo->AvatarActor.Get();
	const UClanhallSkillCooldownComponent* Cooldowns = Avatar ? Avatar->FindComponentByClass<UClanhallSkillCooldownComponent>() : nullptr;
	if (Cooldowns && Cooldowns->IsOnCooldown(Data->CounterTag))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(ClanhallGameplayTags::Denied_Cooldown.GetTag());
		}
		return false;
	}

	return true;
}

void UGA_PhysicalSkill::ResolveMarkLogic(const UAbilityData* Data, UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, UClanhallMarkComponent* TargetMarkComponent)
{
	if (!TargetMarkComponent)
	{
		return;
	}

	// Владелец этой активки — для слота баффа EffectOnSelf (`MarksSystem.md`, «Слоты эффектов
	// от активаций»), собственный UClanhallMarkComponent атакующего, не путать с TargetMarkComponent.
	AActor* SourceAvatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;

	if (const UMarkTriggerFragment* Trigger = Data->FindFragment<UMarkTriggerFragment>())
	{
		// Физическая активка читает и сжигает ТОЛЬКО физический трек — магический трек
		// (заклинания) она не трогает ни в каком случае (`MarksSystem.md`, «Концепция»).
		const FGameplayTag CurrentMark = TargetMarkComponent->GetCurrentMark(EClanhallMarkTrack::Physical);
		if (CurrentMark.IsValid())
		{
			for (const FMarkSynergy& Synergy : Trigger->Synergies)
			{
				// MatchesTag, а не ==: RequiredMark задаёт КРУПНОСТЬ записи. Конкретный тег
				// (Mark.BrokenGuard) матчит только себя; корневой Mark матчит любую метку — так
				// выражается «активирует независимо от типа метки» (Knight Retribution,
				// `PhysicalAbilities.md`). Тот же приём, что у CounteredBy в контрнавыке.
				if (!Synergy.RequiredMark.IsValid() || !CurrentMark.MatchesTag(Synergy.RequiredMark))
				{
					continue;
				}

				// (`MarksSystem.md`, «Активация синергии»): метка сгорает -> бафф на себя ИЛИ дебафф на цель, никогда оба.
				TargetMarkComponent->ClearMark(EClanhallMarkTrack::Physical);

				if (Synergy.EffectOnTarget)
				{
					// Состояние на цель — на каждой задетой цели, через слот дебаффа ЭТОГО
					// создателя на цели (`MarksSystem.md`, «Слоты эффектов от активаций»): новый
					// дебафф того же атакующего снимает свой прежний, дебаффы других не трогает.
					// TargetMarkComponent гарантирован не-null — единственный вызывающий,
					// ResolveHitOn, находит его до вызова ResolveMarkLogic (см. ранний return выше).
					TargetMarkComponent->ApplyDebuffFrom(SourceASC, Synergy.EffectOnTarget);
				}
				else if (Synergy.EffectOnSelf && !bSelfSynergySpent)
				{
					// Состояние на себя — один раз за применение, сколько бы целей ни задело,
					// через слот баффа атакующего (`MarksSystem.md`, «Слоты эффектов от активаций»).
					if (UClanhallMarkComponent* SourceMarkComponent = SourceAvatar ? SourceAvatar->FindComponentByClass<UClanhallMarkComponent>() : nullptr)
					{
						SourceMarkComponent->ApplySelfBuff(Synergy.EffectOnSelf);
					}
					else
					{
						UE_LOG(LogClanhall, Warning, TEXT("ResolveMarkLogic: source has no UClanhallMarkComponent, applying %s without a buff slot"), *Synergy.EffectOnSelf->GetName());
						ClanhallGameplayEffects::ApplyEffect(SourceASC, SourceASC, Synergy.EffectOnSelf);
					}
					bSelfSynergySpent = true;
				}

				break;
			}
		}
	}

	if (const UMarkApplyFragment* MarkApply = Data->FindFragment<UMarkApplyFragment>())
	{
		// Порядок «сначала потребить, потом положить» (см. выше) сохранён: положить метку
		// физического трека — последний шаг резолва, после того как прежняя уже могла сгореть.
		TargetMarkComponent->ApplyMark(EClanhallMarkTrack::Physical, MarkApply->MarkTag, SourceASC);
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

	// Перезарядка стартует на коммите приёма, включая промах и сбитый контром приём: он уходит
	// на перезарядку как использованный (`EconomySystem.md`, «Наказание сбитого контром»).
	if (UClanhallSkillCooldownComponent* Cooldowns = Avatar->FindComponentByClass<UClanhallSkillCooldownComponent>())
	{
		Cooldowns->StartCooldown(Data->CounterTag, Data->CooldownBeats);
	}

	// (`CombatSystem.md`, «Сквозной принцип: контакт сбивает зону получателя»): новая активка — снимаем подавление зон прошлого шага/промаха,
	// независимо от того, есть ли у навыка каст-монтаж (фолбэк-путь тоже открывает зону сферой).
	if (UClanhallHitboxComponent* HitboxComp = Avatar->FindComponentByClass<UClanhallHitboxComponent>())
	{
		HitboxComp->ResetSuppression();
	}

	// Поиск цели нужен фолбэк-пути (урон мгновенно, без ожидания контакта). Контрнавык цель
	// отсюда НЕ берёт — он резолвится по контакту, в ResolveHitOn: зона атакующего касается
	// тела защищающегося, состояние защищающегося проверяется в момент касания.
	AActor* Target = FindMeleeTarget(Avatar);

	// Длина приёма - данные, а не клип (`AbilitySystem.md`, «Приёмы оружия»): монтаж подгоняется
	// под DurationBeats play rate'ом, нотифаи сохраняют пропорции разметки.
	const float DurationSeconds = ClanhallBeats::ToSeconds(Data->DurationBeats);

	// Монтаж запускаем ДО подписки на Event.Hitbox.*.
	UAnimInstance* AnimInst = nullptr;
	float MontagePlayLength = 0.0f;
	if (Data->CastMontage)
	{
		if (ACharacter* Char = Cast<ACharacter>(Avatar))
		{
			AnimInst = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
			if (AnimInst)
			{
				const float ClipLength = Data->CastMontage->GetPlayLength();
				const float PlayRate = (ClipLength > 0.0f && DurationSeconds > 0.0f) ? ClipLength / DurationSeconds : 1.0f;
				MontagePlayLength = AnimInst->Montage_Play(Data->CastMontage, PlayRate);
			}
		}
	}

	const bool bMontageStarted = (AnimInst != nullptr && MontagePlayLength > 0.0f);

	// Терминатор — конец каст-монтажа всегда, когда монтаж стартовал, независимо от
	// режима резолва урона: длительность отыгрыша анимации не зависит от того, размечены ли на
	// монтаже зоны контакта. Обе строки стоят до ветвления по bResolveOnContact ниже, иначе навык
	// с назначенным, но ещё не размеченным монтажом заканчивался бы мгновенно, и следующее
	// действие обрывало бы его анимацию на любом кадре.
	if (bMontageStarted)
	{
		SourceASC->AddLooseGameplayTag(ClanhallGameplayTags::State_SkillCommitted.GetTag());
		PlayedCastMontage = Data->CastMontage;

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UGA_PhysicalSkill::OnCastMontageEnded);
		AnimInst->Montage_SetEndDelegate(EndDelegate, Data->CastMontage);
	}

	const bool bResolveOnContact = UAnimNotifyState_Hitbox::MontageHasHitbox(Data->CastMontage);

	if (bResolveOnContact)
	{
		WarnIfHitboxOutlivesCounterWindow(Data->CastMontage, Data->DisplayName.ToString());
	}

	if (!bResolveOnContact || !bMontageStarted)
	{
		// Резолв урона сферой — два случая с разной причиной и разным временем жизни способности:
		// (1) !bMontageStarted — монтажа нет, либо Montage_Play не смог его стартовать
		//     (несовместимый скелет, невалидный слот): урон резолвится сразу, а терминатор -
		//     истечение DurationBeats (OnDurationElapsed).
		// (2) bMontageStarted && !bResolveOnContact — монтаж есть и играет, но на нём не расставлен
		//     AnimNotifyState_Hitbox: тег и делегат для этого случая уже поставлены выше,
		//     терминатор придёт из OnCastMontageEnded — здесь его не трогаем.
		if (bResolveOnContact && !bMontageStarted)
		{
			UE_LOG(LogClanhall, Warning, TEXT("ActivateAbility: Montage_Play failed to start %s for %s, falling back to instant resolve"),
				Data->CastMontage ? *Data->CastMontage->GetName() : TEXT("null"), *Avatar->GetName());
		}
		// Монтажа нет — приём всё равно длится DurationBeats: тег коммита вешается до резолва, чтобы
		// весь приём, включая кадр попадания, был закрыт для второго действия. Подписка на
		// Event.Hitbox.Hit при неразмеченных зонах бессмысленна — урон отрезолвлен сферой ниже.
		if (!bMontageStarted)
		{
			SourceASC->AddLooseGameplayTag(ClanhallGameplayTags::State_SkillCommitted.GetTag());
		}

		ResolveHitOn(Target);

		if (!bMontageStarted)
		{
			UAbilityTask_WaitDelay* DurationTask = UAbilityTask_WaitDelay::WaitDelay(this, DurationSeconds);
			DurationTask->OnFinish.AddDynamic(this, &UGA_PhysicalSkill::OnDurationElapsed);
			DurationTask->ReadyForActivation();
		}
		return;
	}

	// Event.Hitbox.Hit резолвит попадания. Event.Hitbox.Closed никто из активки не слушает —
	// он не несёт для неё сигнала: способность живёт до конца каст-монтажа (OnCastMontageEnded),
	// а не до опустевшего списка зон. Тег продолжает существовать и слаться компонентом,
	// просто активка на него не подписывается.
	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, ClanhallGameplayTags::Event_Hitbox_Hit.GetTag(), nullptr, /*OnlyTriggerOnce*/ false, /*OnlyMatchExact*/ true);
	HitTask->EventReceived.AddDynamic(this, &UGA_PhysicalSkill::OnHitboxHitReceived);
	HitTask->ReadyForActivation();
}

void UGA_PhysicalSkill::OnHitboxHitReceived(FGameplayEventData Payload)
{
	// Зона может задеть несколько целей за одно применение. Способность здесь НЕ заканчивается,
	// ждём конец каст-монтажа (OnCastMontageEnded).
	ResolveHitOn(const_cast<AActor*>(Payload.Target.Get()));
}

void UGA_PhysicalSkill::OnCastMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// Срабатывает и на нормальном завершении, и на прерывании монтажа.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_PhysicalSkill::OnDurationElapsed()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_PhysicalSkill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Уборка за съеденным NotifyEnd: зона, чей NotifyEnd не пришёл, продолжила бы свипаться и
	// слать Event.Hitbox.Hit после конца способности. Точечно по своему монтажу, а не
	// EndAllHitboxes(): EndAbility приходит асинхронно (из делегата конца монтажа), и зонами к
	// этому моменту может владеть уже следующее действие.
	// Нормальный путь (NotifyEnd пришёл) уже закрыл зону — вызов будет холостым.
	if (AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr)
	{
		if (UClanhallHitboxComponent* Hitboxes = Avatar->FindComponentByClass<UClanhallHitboxComponent>())
		{
			Hitboxes->EndHitboxesFromMontage(PlayedCastMontage.Get());
		}
	}

	// Единственная точка снятия State.SkillCommitted (не подстраховка — Closed-путь тег больше
	// не трогает вовсе). Момент снятия — конец каст-монтажа.
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

	// Тип урона оружия в руке — донесён до резолва попадания, потребитель (порог зоны DT)
	// появится в P3a (`WeaponSystem.md`, «Ассеты вместо `UClassKitData`»); пока не используется.
	const AClanhallHumanoidBase* SourceCharacter = CurrentActorInfo ? Cast<AClanhallHumanoidBase>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
	const UWeaponData* SourceWeapon = SourceCharacter ? SourceCharacter->GetCurrentWeapon() : nullptr;
	const FGameplayTag SourceWeaponDamageType = SourceWeapon ? SourceWeapon->DamageType : FGameplayTag();

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
		// иначе метки у таких навыков никогда бы не срабатывали.
		bConfirmedHit = (TargetASC != nullptr);
	}

	if (!bConfirmedHit)
	{
		return;
	}

	// Контр резолвится ПО КОНТАКТУ: зона дотянулась до тела врага, и в этот
	// момент проверяется его открытое окно. Промах и недостаточная дальность = контра не было.
	// Двойное срабатывание невозможно: ConsumeCounter закрывает окно, второй вызов вернёт false.
	UClanhallCounterComponent::TryResolveCounter(Target, Data->CounterTag);

	// Метка/синергия — один раз НА ЦЕЛЬ за применение, а не на каждый контакт: два
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

#if !UE_BUILD_SHIPPING
	// На каждое попадание, без гейта: при мультицели полезно видеть, сколько целей задело.
	if (const UClanhallAttributeSet* SelfAttributes = SourceASC->GetSet<UClanhallAttributeSet>())
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Orange, FString::Printf(
			TEXT("%s hit | AP %.0f/%.0f"),
			*Data->DisplayName.ToString(), SelfAttributes->GetAP(), SelfAttributes->GetMaxAP()));
	}
#endif
}
