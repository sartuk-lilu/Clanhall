#include "GA_PhysicalSkill.h"
#include "Debug/ClanhallDebugMessage.h"
#include "Clanhall.h"
#include "AbilitySystem/AbilityData.h"
#include "AbilitySystem/Fragments/GameplayFragments.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "AbilitySystem/MarkData.h"
#include "AbilitySystem/ClanhallTargetingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ClanhallCombatTypes.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallCounterComponent.h"
#include "AbilitySystem/ClanhallSkillCooldownComponent.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "AbilitySystem/ClanhallDamage.h"
#include "AbilitySystem/WeaponTypeData.h"
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
#include "ClanhallCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
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

namespace
{
	/** Окно контра не должно быть узким (`AbilitySystem.md`, «Контрнавык»): контрнавык проверяет
	 *  знание, а не реакцию. Ширина - разметка CounterWindow с учётом play rate под длину приёма.
	 *  Варн один раз за монтаж. */
	void WarnIfCounterWindowNarrow(const UAnimMontage* Montage, float PlayRate, const FString& AbilityName)
	{
		if (!Montage || PlayRate <= 0.0f)
		{
			return;
		}
		static TSet<const UAnimMontage*> WarnedMontages;
		if (WarnedMontages.Contains(Montage))
		{
			return;
		}
		for (const FAnimNotifyEvent& Event : Montage->Notifies)
		{
			if (Cast<UAnimNotifyState_CounterWindow>(Event.NotifyStateClass))
			{
				const float WindowSeconds = Event.GetDuration() / PlayRate;
				const float MinSeconds = ClanhallBeats::ToSeconds(UClanhallCombatSettings::Get().MinCounterWindowBeats);
				if (WindowSeconds < MinSeconds)
				{
					UE_LOG(LogClanhall, Warning, TEXT("%s: CounterWindow on %s lasts %.2fs at this skill length, narrower than %.2fs - the counter becomes a reaction test"),
						*AbilityName, *Montage->GetName(), WindowSeconds, MinSeconds);
					WarnedMontages.Add(Montage);
				}
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

	// В прыжке не бьют (`CombatSystem.md`, «Общие действия») - правило общее для игрока и противника.
	if (const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
	{
		if (Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsFalling())
		{
			return false;
		}
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

UClanhallMarkComponent* UGA_PhysicalSkill::FindLevelHolder(AActor* Avatar, EClanhallMarkTarget Target) const
{
	if (!Avatar)
	{
		return nullptr;
	}
	if (Target == EClanhallMarkTarget::Self)
	{
		return Avatar->FindComponentByClass<UClanhallMarkComponent>();
	}

	// Уровень выбирается в момент нажатия (`MarksSystem.md`, «Уровни навыка»), когда контакта ещё
	// нет: держатель - тот, по кому приём направлен. Игрок - мягкая цель прицела, противник - игрок
	// (прототипное допущение 1v1), иначе ближайший перед собой.
	AActor* Opponent = nullptr;
	if (const UClanhallTargetingComponent* Targeting = Avatar->FindComponentByClass<UClanhallTargetingComponent>())
	{
		Opponent = Targeting->CurrentTarget;
	}
	if (!Opponent)
	{
		const APawn* AvatarPawn = Cast<APawn>(Avatar);
		if (AvatarPawn && !AvatarPawn->IsPlayerControlled())
		{
			Opponent = UGameplayStatics::GetPlayerPawn(Avatar, 0);
		}
	}
	if (!Opponent)
	{
		Opponent = FindMeleeTarget(Avatar);
	}
	return Opponent ? Opponent->FindComponentByClass<UClanhallMarkComponent>() : nullptr;
}

void UGA_PhysicalSkill::ApplyChosenLevel(UClanhallMarkComponent* Holder, AActor* Avatar)
{
	const UAbilityData* Data = GetAbilityData(CurrentSpecHandle, CurrentActorInfo);
	const USkillLevelsFragment* Levels = Data ? Data->FindFragment<USkillLevelsFragment>() : nullptr;
	if (!Levels || !Levels->Levels.IsValidIndex(ChosenLevelIndex) || !Holder)
	{
		return;
	}
	const FClanhallSkillLevel& Level = Levels->Levels[ChosenLevelIndex];

	// Навык уровня N съедает метку уровня N-1, названную в условии, и кладёт следующую
	// (`MarksSystem.md`, «Уровни навыка»). Метка съедается на попадании: истекла между нажатием и
	// попаданием - версия бьёт как выбрана, но ничего не кладёт.
	if (Level.Level <= 1)
	{
		if (Level.Places)
		{
			Holder->PlaceMark(Level.Places, SkillSchool, Avatar);
		}
	}
	else
	{
		FGameplayTag EatenSchool;
		if (Holder->ConsumeMark(Level.Level - 1, Level.Reads, EatenSchool))
		{
			if (Level.Level == 2)
			{
				if (Level.Places)
				{
					Holder->PlaceMark(Level.Places, SkillSchool, Avatar);
				}
			}
			else if (EatenSchool != SkillSchool)
			{
				// Третий уровень на метке другой школы - травма цели или преимущество себе
				// (`MarksSystem.md`, «Травмы и преимущества»).
				if (Level.Places)
				{
					UClanhallMarkComponent* LongHolder = Level.Places->Kind == EClanhallMarkKind::Advantage
						? Avatar->FindComponentByClass<UClanhallMarkComponent>() : Holder;
					if (LongHolder)
					{
						LongHolder->PlaceMark(Level.Places, SkillSchool, Avatar);
					}
				}
			}
#if !UE_BUILD_SHIPPING
			else
			{
				ClanhallDebug::Screen(FColor::Yellow, FString::Printf(
					TEXT("%s: %s placed by own school - strong version only"), *Data->DisplayName.ToString(), *Level.Reads.ToString()));
			}
#endif
		}
	}

	// Мгновенные эффекты версии - один раз в момент попадания (`MarksSystem.md`, «Мгновенные
	// эффекты»). PierceThreshold уже учтён в расчёте урона (bHitIgnoresThreshold).
	for (const EClanhallInstantEffect Effect : Level.InstantEffects)
	{
		switch (Effect)
		{
		case EClanhallInstantEffect::RemoveOwnTrauma:
			if (UClanhallMarkComponent* Own = Avatar->FindComponentByClass<UClanhallMarkComponent>())
			{
				Own->RemoveOldest(EClanhallMarkKind::Trauma);
			}
			break;
		case EClanhallInstantEffect::RemoveTargetAdvantage:
			if (Levels->Target == EClanhallMarkTarget::Enemy)
			{
				Holder->RemoveOldest(EClanhallMarkKind::Advantage);
			}
			break;
		default:
			break;
		}
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

	// Контримый приём запоминается с момента активации - для одновременной активации, когда окно
	// контра ещё не открылось (`AbilitySystem.md`, «Контрнавык»).
	ActivationTime = Avatar->GetWorld() ? Avatar->GetWorld()->GetTimeSeconds() : 0.0;
	if (UClanhallCounterComponent* OwnCounter = Avatar->FindComponentByClass<UClanhallCounterComponent>())
	{
		if (!Data->CounteredBy.IsEmpty())
		{
			OwnCounter->NotifySkillActivated(Data->CounteredBy, Handle);
		}
	}

	// Приём выхватывает выбранное оружие и бьёт одним движением - лишней доли нет
	// (`WeaponSystem.md`, «Свап оружия»). Время приёма этого оружия - для ножен.
	if (AClanhallHumanoidBase* Humanoid = Cast<AClanhallHumanoidBase>(Avatar))
	{
		// Ножны - по времени с последнего приёма этого оружия, а не по тому, на теле ли меч: у
		// противника, который оружие не убирает, ножны работают так же. Читать до NotifySkillStarted,
		// он перепишет время.
		const UWeaponTypeData* Type = Humanoid->GetWeaponType();
		bFromScabbard = Type && Type->SchoolMechanic == EClanhallSchoolMechanic::Scabbard
			&& Humanoid->GetSecondsSinceSelectedWeaponUsed() >= ClanhallBeats::ToSeconds(UClanhallCombatSettings::Get().ScabbardBeats);

		bDrawnByThisSkill = Humanoid->DrawSelectedWeapon();
		Humanoid->NotifySkillStarted();
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

	// Школа приёма - школа класса выбранного оружия: метка помнит её, третий уровень сравнивает
	// школы (`MarksSystem.md`, «Слоты меток»).
	if (const AClanhallHumanoidBase* SchoolOwner = Cast<AClanhallHumanoidBase>(Avatar))
	{
		SkillSchool = SchoolOwner->GetWeaponType() ? SchoolOwner->GetWeaponType()->School : FGameplayTag();
	}

	// Уровень навыка решает метка в момент нажатия (`MarksSystem.md`, «Уровни навыка»): от
	// старшего к младшему, выполнены оба условия - старший. Версия задаёт силу, длину и мгновенные
	// эффекты.
	float DurationBeats = Data->DurationBeats;
	const USkillLevelsFragment* LevelsFragment = Data->FindFragment<USkillLevelsFragment>();
	if (LevelsFragment)
	{
		ChosenLevelIndex = LevelsFragment->ChooseLevelIndex(FindLevelHolder(Avatar, LevelsFragment->Target));
		if (LevelsFragment->Levels.IsValidIndex(ChosenLevelIndex))
		{
			const FClanhallSkillLevel& Level = LevelsFragment->Levels[ChosenLevelIndex];
			HitDamageMultiplier *= Level.DamageMultiplier;
			bHitIgnoresThreshold = Level.InstantEffects.Contains(EClanhallInstantEffect::PierceThreshold);
			if (Level.DurationBeatsOverride > 0.0f)
			{
				DurationBeats = Level.DurationBeatsOverride;
			}
#if !UE_BUILD_SHIPPING
			ClanhallDebug::Screen(FColor::White, FString::Printf(TEXT("%s - level %d"), *Data->DisplayName.ToString(), Level.Level));
#endif
		}

		// Приём на себя кладёт метку на активации: цели нет, попадания ждать не по кому.
		if (LevelsFragment->Target == EClanhallMarkTarget::Self)
		{
			ApplyChosenLevel(Avatar->FindComponentByClass<UClanhallMarkComponent>(), Avatar);
		}
	}

	// Удар из ножен - мощность приёма, а не уровень: короче и сильнее поверх выбранной версии
	// (`AbilitySystem.md`, «Механика школы оружия»).
	if (bFromScabbard)
	{
		const UClanhallCombatSettings& Settings = UClanhallCombatSettings::Get();
		DurationBeats *= Settings.ScabbardDurationFactor;
		HitDamageMultiplier *= Settings.ScabbardDamageMultiplier;
#if !UE_BUILD_SHIPPING
		ClanhallDebug::Screen(FColor::Cyan, FString::Printf(TEXT("%s - from scabbard"), *Data->DisplayName.ToString()));
#endif
	}

	// Длина приёма - данные, а не клип (`AbilitySystem.md`, «Приёмы оружия»): монтаж подгоняется
	// под длину версии play rate'ом, нотифаи сохраняют пропорции разметки.
	const float DurationSeconds = ClanhallBeats::ToSeconds(DurationBeats);

	// Монтаж запускаем ДО подписки на Event.Hitbox.*.
	UAnimInstance* AnimInst = nullptr;
	float MontagePlayLength = 0.0f;
	float MontagePlayRate = 1.0f;
	if (Data->CastMontage)
	{
		if (ACharacter* Char = Cast<ACharacter>(Avatar))
		{
			AnimInst = Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
			if (AnimInst)
			{
				const float ClipLength = Data->CastMontage->GetPlayLength();
				MontagePlayRate = (ClipLength > 0.0f && DurationSeconds > 0.0f) ? ClipLength / DurationSeconds : 1.0f;
				MontagePlayLength = AnimInst->Montage_Play(Data->CastMontage, MontagePlayRate);
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
	if (bMontageStarted)
	{
		WarnIfCounterWindowNarrow(Data->CastMontage, MontagePlayRate, Data->DisplayName.ToString());
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

	if (AActor* EndingAvatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr)
	{
		if (UClanhallCounterComponent* OwnCounter = EndingAvatar->FindComponentByClass<UClanhallCounterComponent>())
		{
			OwnCounter->NotifySkillEnded(Handle);
		}
	}

	// После приёма оружие остаётся в руках и само уходит на тело через несколько долей без дела.
	if (AClanhallHumanoidBase* Humanoid = ActorInfo ? Cast<AClanhallHumanoidBase>(ActorInfo->AvatarActor.Get()) : nullptr)
	{
		Humanoid->NotifySkillEnded();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_PhysicalSkill::TryTauntParry(AActor* Avatar, AActor* Target, UAbilitySystemComponent* TargetASC)
{
	UAbilitySystemComponent* SourceASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	const AClanhallCharacterBase* SourceFighter = Cast<AClanhallCharacterBase>(Avatar);
	const AClanhallCharacterBase* TargetFighter = Cast<AClanhallCharacterBase>(Target);
	if (!SourceASC || !TargetASC || !SourceFighter || !TargetFighter
		|| !TargetASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_Taunting.GetTag()))
	{
		return false;
	}

	const UClanhallCombatSettings& Settings = UClanhallCombatSettings::Get();
	if (TargetFighter->GetPower() < Settings.TauntParryRatio * SourceFighter->GetPower())
	{
		// Против равных и сильных насмешка-парирования нет: удар идёт по обычным правилам и,
		// если подтвердится, сорвёт насмешку (ClanhallDamage::ResolveHit).
		return false;
	}

	// Анимация, а не окно под урон: оба без управления на одно и то же время.
	const float Seconds = ClanhallBeats::ToSeconds(Settings.TauntParryBeats);
	ClanhallGameplayEffects::ApplyTimedTagToTarget(TargetASC, SourceASC, ClanhallGameplayTags::State_TauntParried.GetTag(), Seconds);
	ClanhallGameplayEffects::ApplyTimedTag(TargetASC, ClanhallGameplayTags::State_TauntParrying.GetTag(), Seconds);

	// Насмешка закончилась парированием, приём атакующего отменён без урона и меток.
	FGameplayTagContainer TauntTags;
	TauntTags.AddTag(ClanhallGameplayTags::Ability_Common_Taunt.GetTag());
	TargetASC->CancelAbilities(&TauntTags);

#if !UE_BUILD_SHIPPING
	ClanhallDebug::Screen(FColor::Cyan, FString::Printf(TEXT("%s stopped %s with a taunt"),
		*GetNameSafe(Target), *GetNameSafe(Avatar)));
#endif

	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	return true;
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

	AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;
	const AClanhallHumanoidBase* SourceCharacter = Cast<AClanhallHumanoidBase>(Avatar);
	const UWeaponData* SourceWeapon = SourceCharacter ? SourceCharacter->GetCurrentWeapon() : nullptr;
	const UWeaponTypeData* SourceWeaponType = SourceCharacter ? SourceCharacter->GetWeaponType() : nullptr;

	IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(Target);
	UAbilitySystemComponent* TargetASC = TargetInterface ? TargetInterface->GetAbilitySystemComponent() : nullptr;

	// Насмешка против гораздо более слабого - парирование (`CombatSystem.md`, «Насмешка»): удар
	// останавливается до урона, меток и контра, оба уходят в анимацию без окна под урон. Работает в
	// обе стороны: сила сравнивается, а не роль.
	if (TryTauntParry(Avatar, Target, TargetASC))
	{
		return;
	}

	// Один расчёт попадания на все источники урона (`Implementation/Damage.md`): порог по типу
	// урона выбранного оружия минус его пробитие, спина, крит, смертельный удар.
	const UDamageFragment* DamageFragment = Data->FindFragment<UDamageFragment>();
	FClanhallHitContext Hit;
	Hit.SourceActor = Avatar;
	Hit.SourceASC = SourceASC;
	Hit.TargetActor = Target;
	Hit.TargetASC = TargetASC;
	Hit.RawDamage = DamageFragment ? DamageFragment->BaseDamage : 0.0f;
	// Утилитарный навык без урона — попадание подтверждено самим фактом найденной цели,
	// иначе метки у таких навыков никогда бы не срабатывали.
	Hit.bUtility = (DamageFragment == nullptr);
	Hit.DamageType = SourceWeapon ? SourceWeapon->DamageType : FGameplayTag();
	Hit.ArmorPenetration = (SourceWeaponType ? SourceWeaponType->ArmorPenetration : 0.0f)
		+ (SourceWeapon ? SourceWeapon->ArmorPenetrationBonus : 0.0f);
	Hit.DamageMultiplier = HitDamageMultiplier;
	Hit.bBackstab = Data->bBackstab;
	Hit.LethalMode = Data->LethalMode;
	Hit.bIgnoreThreshold = bHitIgnoresThreshold;

	const FClanhallHitResult HitResult = ClanhallDamage::ResolveHit(Hit);

#if !UE_BUILD_SHIPPING
	// На каждое попадание, без гейта: при мультицели полезно видеть, сколько целей задело.
	ClanhallDebug::Screen(HitResult.bConfirmed ? FColor::Orange : FColor::Silver, FString::Printf(
		TEXT("%s -> %s | AP -%.0f HP -%.0f%s%s%s%s"),
		*Data->DisplayName.ToString(), *GetNameSafe(Target), HitResult.DamageToAP, HitResult.DamageToHP,
		HitResult.bAbsorbed ? TEXT(" | absorbed by threshold") : TEXT(""),
		HitResult.bCrit ? TEXT(" | CRIT") : TEXT(""),
		HitResult.bBackstab ? TEXT(" | backstab") : TEXT(""),
		HitResult.bLethal ? TEXT(" | LETHAL") : TEXT("")));
#endif

	// Всё «на попадании» читает только bConfirmed: удар, целиком поглощённый порогом, - не
	// попадание (`CombatSystem.md`, «Damage Threshold (DT)»): ни контра, ни меток.
	if (!HitResult.bConfirmed)
	{
		return;
	}

	// Контр резолвится ПО КОНТАКТУ: зона дотянулась до тела врага, и в этот
	// момент проверяется его открытое окно. Промах и недостаточная дальность = контра не было.
	// Двойное срабатывание невозможно: ConsumeCounter закрывает окно, второй вызов вернёт false.
	// Только подтверждённое попадание: удар, целиком поглощённый порогом, не контрит (выше - return).
	UClanhallCounterComponent::TryResolveCounter(Target, Data->CounterTag, Avatar, ActivationTime);

	// Метки - один раз НА ЦЕЛЬ за применение, а не на каждый контакт: два хитбокса на монтаже
	// дают два урона, но вторая фаза не должна съесть метку, которую только что положила первая.
	// Навык по площади кладёт метку каждой цели отдельно (`MarksSystem.md`, «Слоты меток»).
	const USkillLevelsFragment* LevelsFragment = Data->FindFragment<USkillLevelsFragment>();
	const bool bMarkAlreadyResolved = MarkResolvedTargets.ContainsByPredicate([Target](const TWeakObjectPtr<AActor>& Weak)
	{
		return Weak.Get() == Target;
	});
	if (LevelsFragment && LevelsFragment->Target == EClanhallMarkTarget::Enemy && !bMarkAlreadyResolved)
	{
		MarkResolvedTargets.Add(Target);
		ApplyChosenLevel(Target->FindComponentByClass<UClanhallMarkComponent>(), Avatar);
	}
}
