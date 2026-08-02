#include "AbilitySystem/ClanhallComboComponent.h"
#include "Clanhall.h"
#include "AbilitySystem/Fragments/ComboData.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "AbilitySystem/ClanhallParryComponent.h"
#include "ClanhallHumanoidCombatant.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"

namespace
{
	/** main_dev_plan.md §8, Блок D: направление своего шага -> тег, который на себя вешает
	 *  атакующий на время удара (симметричный телеграф). Не путать со свитчем в
	 *  UClanhallParryComponent::TryParry — там маппинг ОБРАТНЫЙ (своё направление -> тег,
	 *  который парируется), это разные вопросы. */
	FGameplayTag DirectionToOwnIncomingTag(EClanhallAttackDirection Direction)
	{
		switch (Direction)
		{
		case EClanhallAttackDirection::Overhead:    return ClanhallGameplayTags::Parry_Incoming_W.GetTag();
		case EClanhallAttackDirection::RightSlash:  return ClanhallGameplayTags::Parry_Incoming_D.GetTag();
		case EClanhallAttackDirection::LeftSlash:   return ClanhallGameplayTags::Parry_Incoming_A.GetTag();
		case EClanhallAttackDirection::LowSweep:    return ClanhallGameplayTags::Parry_Incoming_S.GetTag();
		default:                                    return FGameplayTag();
		}
	}
}

void UClanhallComboComponent::HandleAttackInput(EClanhallAttackDirection Direction)
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || ASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_ComboRecovery.GetTag()))
	{
		// Лок-аут после Recovery (см. EndSequenceWithRecovery) — вход игнорируется, пока тег не
		// спадёт. Дублирует ActivationBlockedTags на GA_ClanhallAbilityBase для WASD-пути конкретно
		// (тот гейтит саму активацию GA, этот — вход в компонент раньше, до попытки активации).
		return;
	}

	if (ASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_SkillCommitted.GetTag()))
	{
		// E2.4: активка в фазе коммита — начатую активку нельзя оборвать (combat_system.md §3).
		// Отбрасываем ввод здесь же, до ActivateStep и его ForceEndHitboxes(), иначе WASD сразу
		// после Q обрывал бы активке окно контакта, хоть заряды и КД уже потрачены.
		return;
	}

	if (ASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_Stunned.GetTag()))
	{
		// main_dev_plan.md §8, Блок D: оглушённый (в т.ч. полным парированием своей серии) не
		// бьёт — дублирует ActivationBlockedTags на GA_ClanhallAbilityBase для WASD-пути
		// конкретно, тем же паттерном, что и два тега выше.
		return;
	}

	// Стойка наземная (см. DoMove в ClanhallCharacter.cpp) — держим ЛКМ в воздухе, тег
	// State.InStance висит, но удар-монтаж посреди падения играть нельзя.
	if (const ACharacter* Char = Cast<ACharacter>(GetOwner());
		Char && Char->GetCharacterMovement() && Char->GetCharacterMovement()->IsFalling())
	{
		return;
	}

	if (StepCount == 0)
	{
		TryStartSequence(Direction);
		return;
	}

	if (bReadWindowOpen)
	{
		// В окне действует "последнее нажатие решает" — перезаписываем, не копим.
		LatestInWindow = Direction;
		return;
	}

	// Окно закрыто (фаза замаха) — ввод отбрасывается целиком: не опенер, не буфер.
}

void UClanhallComboComponent::TryStartSequence(EClanhallAttackDirection Direction)
{
	const UComboData* Data = GetComboData();
	UAnimMontage* Montage = Data ? Data->FindOpenerMontage(Direction) : nullptr;
	if (!Montage)
	{
		// Нейтраль этим направлением не начать (пустой слот FromStance) — ничего не начали.
		return;
	}

	if (ActivateStep(Direction, Montage))
	{
		LastDirection = Direction;
		StepCount = 1;
	}
}

void UClanhallComboComponent::OnComboWindowOpen()
{
	bReadWindowOpen = true;
	LatestInWindow.Reset();

	// main_dev_plan.md §8, Блок B: единственный сигнал для AI о том, что можно подавать
	// направление в HandleAttackInput — сама очерёдность решает AI/BT, компонент не подсказывает.
	OnComboWindowOpened.Broadcast();
}

void UClanhallComboComponent::OnComboWindowClose()
{
	bReadWindowOpen = false;
	OnComboWindowClosed.Broadcast();

	if (!LatestInWindow.IsSet())
	{
		// Нет ввода к концу окна — терминальный удар без чейна, единый путь завершения серии.
		EndSequenceWithRecovery();
		return;
	}

	const EClanhallAttackDirection Direction = LatestInWindow.GetValue();
	LatestInWindow.Reset();

	// Потолок ранга: длина серии = ClassRank + 1 (ранг 0 → 1 удар, ранг 4 → 5 ударов).
	// Запрещено, даже если слот перехода занят.
	if (StepCount > GetClassRank())
	{
		EndSequenceWithRecovery();
		return;
	}

	if (!LastDirection.IsSet())
	{
		// Защитный случай: сюда не должны попадать при StepCount > 0 — LastDirection и StepCount
		// меняются вместе. Раз рассинхрон всё же случился, безопаснее уйти в Recovery.
		EndSequenceWithRecovery();
		return;
	}

	const UComboData* Data = GetComboData();
	UAnimMontage* Montage = Data ? Data->FindTransitionMontage(LastDirection.GetValue(), Direction) : nullptr;
	if (!Montage)
	{
		// Невалидное продолжение (Часть B1): без урона, без сдвига шкал — тот же терминальный путь.
		EndSequenceWithRecovery();
		return;
	}

	if (!ActivateStep(Direction, Montage))
	{
		// Активация не прошла (например, стойку успели снять между вводом и разрешением окна) —
		// состояние не фиксируем, тот же терминальный путь, что и для невалидного продолжения.
		EndSequenceWithRecovery();
		return;
	}

	LastDirection = Direction;
	++StepCount;
}

bool UClanhallComboComponent::ActivateStep(EClanhallAttackDirection Direction, UAnimMontage* Montage)
{
	AClanhallHumanoidCombatant* Character = Cast<AClanhallHumanoidCombatant>(GetOwner());
	UAbilitySystemComponent* ASC = GetASC();
	const UComboData* Data = GetComboData();
	if (!Character || !ASC || !Data || !Montage)
	{
		return false;
	}

	// Зоны предыдущего шага закрываем ДО активации следующей способности.
	// Иначе Event.Hitbox.Closed от прерванного монтажа (Montage_Play ниже -> bInterrupted)
	// прилетит уже НОВОЙ способности и оборвёт её до открытия собственной зоны.
	ForceEndHitboxes();

	const FDirectionalDamage& Damage = Data->FindDamageByDirection(Direction);

	// Точка вызова инвертирована — GA_DirectionalAttackBase::ActivateAbility
	// (формулы урона/MP/Balance не тронуты) срабатывает, только если валидатор дошёл до этого
	// вызова. BaseDamage профиля идёт в EventMagnitude — GA больше не хранит RawDamage сам.
	FGameplayEventData EventData;
	EventData.EventMagnitude = Damage.BaseDamage;
	if (Damage.DamageType.IsValid())
	{
		// Задел: тип урона в InstigatorTags события, в расчёте пока не читается.
		EventData.InstigatorTags.AddTag(Damage.DamageType);
	}
	// Способность по монтажу определяет режим резолва (контакт или мгновенный фолбэк).
	EventData.OptionalObject = Montage;

	const FGameplayAbilitySpecHandle Handle = Character->GetAttackHandle(Direction);
	if (!ASC->TriggerAbilityFromGameplayEvent(Handle, ASC->AbilityActorInfo.Get(), ClanhallGameplayTags::Event_DirectionalAttack.GetTag(), &EventData, *ASC))
	{
		return false;
	}

	// main_dev_plan.md §8, Блок D (хвост, ревью): снять тег ПРЕДЫДУЩЕГО шага и повесить тег
	// НОВОГО — только здесь, по одну сторону от раннего выхода выше. Раньше Clear стоял ДО
	// попытки активации: на провале тег предыдущего шага снимался, LastDirection не
	// перезаписывался (это делает вызывающий только на успехе), и ResetCombo() снимал тот же
	// тег ещё раз — рефкаунт расходился (сейчас безвредно из-за клампа на нуле в
	// UpdateTagCount, но не в общем случае).
	// State.Parrying по-прежнему вешает разметка монтажа (AnimNotifyState_ParryWindow, на
	// владельца, кто бы им ни был); Parry.Incoming.* — код, т.к. направление в анимации не
	// читается. Оба тега — на СЕБЯ.
	ClearIncomingParryTag();
	ApplyIncomingParryTag(Direction);

	// Свой дедуп «я только что кого-то распарировал» сбрасываем перед КАЖДЫМ собственным
	// шагом — раньше это делала GA_EnemyWASDSeries::PrepareHit, теперь единственный источник
	// шагов — этот метод, у обеих сторон.
	if (UClanhallParryComponent* OwnParry = Character->FindComponentByClass<UClanhallParryComponent>())
	{
		OwnParry->ResetParry();
	}

	PlayMontage(Montage);
	return true;
}

void UClanhallComboComponent::ApplyIncomingParryTag(EClanhallAttackDirection Direction)
{
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->AddLooseGameplayTag(DirectionToOwnIncomingTag(Direction));
	}
}

void UClanhallComboComponent::ClearIncomingParryTag()
{
	if (!LastDirection.IsSet())
	{
		return;
	}
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->RemoveLooseGameplayTag(DirectionToOwnIncomingTag(LastDirection.GetValue()));
	}
}

void UClanhallComboComponent::NotifyStepParried(AActor* ParrierActor)
{
	if (LastParriedStepIndex == StepCount)
	{
		// main_dev_plan.md §8, Блок D (хвост, ревью): этот шаг уже засчитан — второй актор,
		// задевший его в том же окне, не должен задвоить счёт (иначе ParriedStepsThisSeries
		// обгонит StepCount, и ResolveParryOutcome() != тихо не увидит полное парирование).
		return;
	}

	LastParriedStepIndex = StepCount;
	++ParriedStepsThisSeries;
	LastParrierActor = ParrierActor;
}

void UClanhallComboComponent::ResolveParryOutcome()
{
	if (ParriedStepsThisSeries != StepCount)
	{
		return;
	}

	// main_dev_plan.md §8, Блок D (было GA_EnemyWASDSeries::FinalizeSeries): все шаги серии
	// отпарированы -> владелец серии оглушён, парировавший получает снижение КД. Симметрично —
	// роль (игрок/AI) не участвует, решает только факт полного парирования.
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ClanhallGameplayEffects::ApplyTimedTag(ASC, ClanhallGameplayTags::State_Stunned.GetTag(), FullParryStunDuration);
	}

	if (AActor* Parrier = LastParrierActor.Get())
	{
		if (const IAbilitySystemInterface* ParrierInterface = Cast<IAbilitySystemInterface>(Parrier))
		{
			if (UAbilitySystemComponent* ParrierASC = ParrierInterface->GetAbilitySystemComponent())
			{
				ReduceCooldowns(ParrierASC, FullParryCooldownReduction);
			}
		}
	}
}

void UClanhallComboComponent::ReduceCooldowns(UAbilitySystemComponent* TargetASC, float ReductionSeconds) const
{
	if (!TargetASC) return;

	// MakeQuery_MatchAnyOwningTags в UE 5.3+ проверяет и asset-теги, и granted-теги (DynamicGrantedTags).
	// Наш GE_ApplyTimedTag хранит тег в DynamicGrantedTags → запрос с родительским тегом Cooldown его найдёт.
	FGameplayTagContainer CooldownRoot;
	CooldownRoot.AddTag(FGameplayTag::RequestGameplayTag("Cooldown"));
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownRoot);
	TArray<FActiveGameplayEffectHandle> Handles = TargetASC->GetActiveEffects(Query);

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	// Собрать теги и новые длительности до изменения контейнера.
	TArray<FGameplayTag> TagsToReapply;
	TArray<float> NewDurations;

	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		const FActiveGameplayEffect* ActiveEffect = TargetASC->GetActiveGameplayEffect(Handle);
		if (!ActiveEffect) continue;

		FGameplayTag CooldownTag;
		for (const FGameplayTag& Tag : ActiveEffect->Spec.DynamicGrantedTags)
		{
			if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag("Cooldown")))
			{
				CooldownTag = Tag;
				break;
			}
		}
		if (!CooldownTag.IsValid()) continue;

		float StartTime, TotalDuration;
		TargetASC->GetGameplayEffectStartTimeAndDuration(Handle, StartTime, TotalDuration);
		const float Remaining = (StartTime + TotalDuration) - CurrentTime;
		const float NewRemaining = Remaining - ReductionSeconds;

		if (NewRemaining > 0.0f)
		{
			TagsToReapply.Add(CooldownTag);
			NewDurations.Add(NewRemaining);
		}
		// NewRemaining <= 0 → КД истёк сразу, не перевешиваем
	}

	// Снять все найденные КД-эффекты, затем перевесить те, что ещё не истекли.
	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		TargetASC->RemoveActiveGameplayEffect(Handle);
	}
	for (int32 i = 0; i < TagsToReapply.Num(); ++i)
	{
		ClanhallGameplayEffects::ApplyTimedTag(TargetASC, TagsToReapply[i], NewDurations[i]);
	}
}

void UClanhallComboComponent::PlayMontage(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	UAnimInstance* AnimInst = GetAnimInstance();
	if (!AnimInst)
	{
		return;
	}

	AnimInst->Montage_Play(Montage);
	LastPlayedMontage = Montage;

	// Подстраховка от залипания состояния, если на монтаже нет/не сработал
	// AnimNotifyState_ComboWindow — доигрывание монтажа всё равно снимет состояние через делегат.
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UClanhallComboComponent::OnAttackMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndDelegate, Montage);
}

void UClanhallComboComponent::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != LastPlayedMontage.Get())
	{
		// Делегат от уже смещённого монтажа: чейн (или активка) успел стартовать следующий
		// монтаж, а зоны предыдущего закрыты в ActivateStep / CancelSequenceForExternalMontage.
		// Трогать зоны ТЕКУЩЕГО владельца нельзя: EndAllHitboxes послал бы Event.Hitbox.Closed
		// и оборвал живую способность до окна контакта. Делегат приходит асинхронно (после
		// блендаута), поэтому синхронного порядка в ActivateStep недостаточно.
		return;
	}

	// Прерванный монтаж — основной кейс залипшей зоны (нотифай NotifyEnd не успел
	// сработать): страховка идёт до раннего return по bInterrupted.
	ForceEndHitboxes();

	if (bInterrupted)
	{
		// Чейн прервал монтаж новым Montage_Play, либо OnStanceExit остановил стойку — в обоих
		// случаях соответствующий путь уже отработал сам, здесь трогать нечего.
		return;
	}

	EndSequenceWithRecovery();
}

void UClanhallComboComponent::ForceEndHitboxes()
{
	if (UClanhallHitboxComponent* HitboxComp = GetOwner() ? GetOwner()->FindComponentByClass<UClanhallHitboxComponent>() : nullptr)
	{
		HitboxComp->EndAllHitboxes();
	}
}

void UClanhallComboComponent::EndSequenceWithRecovery()
{
	if (StepCount == 0)
	{
		// Уже нейтраль — серия завершена другим путём (fall-through/делегат).
		// Guard от двойного Recovery на одну серию.
		return;
	}

	// main_dev_plan.md §8, Блок D: исход серии — читает StepCount/ParriedStepsThisSeries,
	// поэтому ДО ResetCombo() ниже (тот их обнулит).
	ResolveParryOutcome();

	const UComboData* Data = GetComboData();
	// Резолв Recovery-анимации ДО ResetCombo(): сброс очищает LastDirection.
	UAnimMontage* RecoveryMontage = (Data && LastDirection.IsSet()) ? Data->FindRecoveryMontage(LastDirection.GetValue()) : nullptr;

	ResetCombo();

	if (!RecoveryMontage)
	{
		// nullptr: хвост восстановления запечён в сам удар-монтаж, отдельно играть нечего.
		// Лок-аут НЕ вешается: показывать его нечем, а невидимого лока в системе больше нет.
		return;
	}

	UAnimInstance* AnimInst = GetAnimInstance();
	if (!AnimInst)
	{
		return;
	}

	// НЕ вызывать PlayMontage() здесь: тот хелпер вешает Montage_SetEndDelegate на
	// OnAttackMontageEnded, и для Recovery это ловушка-петля (конец Recovery ->
	// OnAttackMontageEnded(bInterrupted=false) -> EndSequenceWithRecovery -> снова Recovery).
	// Recovery играется без делегата конца — не "оптимизировать" обратно на PlayMontage().
	const float PlayedDuration = AnimInst->Montage_Play(RecoveryMontage);
	if (PlayedDuration == 0.f)
	{
		UE_LOG(LogClanhall, Warning, TEXT("EndSequenceWithRecovery: Montage_Play failed to start %s"), *RecoveryMontage->GetName());
		// Монтаж не стартовал — лок не вешаем, иначе он был бы невидимым.
		return;
	}
	LastPlayedMontage = RecoveryMontage;

	// Лок-аут ровно на длительность Recovery-анимации: тег спадает в тот же кадр, в котором
	// заканчивается анимация. Никаких долей и добавок — лок обязан быть виден целиком.
	// Блокирует новые атаки (ActivationBlockedTags на UGA_ClanhallAbilityBase и ранний гейт
	// в HandleAttackInput) И вход в стойку (UGA_CombatStance). Выход из стойки — свободен.
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ClanhallGameplayEffects::ApplyTimedTag(ASC, ClanhallGameplayTags::State_ComboRecovery.GetTag(), RecoveryMontage->GetPlayLength());
	}
}

void UClanhallComboComponent::CancelSequenceForExternalMontage()
{
	if (StepCount == 0)
	{
		// Нейтраль — прерывать нечего.
		return;
	}

	ForceEndHitboxes();
	ResetCombo();
	// Монтаж НЕ останавливаем: Montage_Play вызывающего сам его перебьёт.
	// LastPlayedMontage обнуляем: (1) прежний трюк с no-op Montage_Stop в OnStanceExit стал
	// избыточен — после Порции B тот зовётся только при StepCount > 0, а ResetCombo() выше
	// уже поставил 0; (2) иначе гард в OnAttackMontageEnded пропустит делегат прерванного
	// удар-монтажа и закроет зоны уже живой активки, оборвав её до контакта.
	LastPlayedMontage.Reset();
}

void UClanhallComboComponent::ResetCombo()
{
	// main_dev_plan.md §8, Блок D: снять Parry.Incoming.* последнего шага ДО LastDirection.Reset()
	// ниже — ClearIncomingParryTag читает LastDirection, чтобы знать, какой тег снимать.
	ClearIncomingParryTag();

	LastDirection.Reset();
	StepCount = 0;
	LatestInWindow.Reset();
	// Упрочнение: гасим ворота даже если сброс пришёл при открытом окне (напр. OnStanceExit
	// посреди чтения ввода) — не даём следующему нажатию попасть в уже мёртвое окно.
	bReadWindowOpen = false;
	// Исход серии уже прочитан (если серия завершилась штатно, ResolveParryOutcome отработал
	// до этого вызова) — обнуляем безусловно, чтобы ни один путь завершения (внешнее
	// прерывание, выход из стойки) не протащил чужой счёт в следующую серию.
	ParriedStepsThisSeries = 0;
	LastParrierActor.Reset();
	LastParriedStepIndex = 0;
	// Тег State.ComboRecovery (если уже был повешен) НЕ снимается здесь — он живёт своим таймером
	// независимо от состояния серии. Именно это не даёт связке "выйти из стойки и сразу войти
	// обратно" бесплатно отменить лок-аут (см. OnStanceExit).
}

void UClanhallComboComponent::OnStanceExit()
{
	// Прерываем ТОЛЬКО живой удар-монтаж (StepCount > 0). При StepCount == 0 это либо
	// нейтраль (гасить нечего), либо уже играет Recovery — и он обязан доиграть: именно
	// он показывает игроку, почему новые атаки и вход в стойку пока не работают.
	// Recovery живёт в слоте upperbody, низ уходит в локомоцию по bInStance — персонаж
	// бежит, руки доводят возврат к стойке.
	if (StepCount > 0)
	{
		if (UAnimInstance* AnimInst = GetAnimInstance())
		{
			AnimInst->Montage_Stop(StanceExitBlendOutTime, LastPlayedMontage.Get());
		}

		// ForceEndHitboxes переехал СЮДА (E2.6): при StepCount == 0 живого удар-монтажа нет, а
		// значит нет и своей зоны — всё открытое принадлежит активке в фазе коммита (State.
		// SkillCommitted). Закрывать чужую зону нельзя: Event.Hitbox.Closed оборвал бы её до
		// контакта, хотя выход из стойки по канону блокирует новые действия, а не отменяет
		// начатое (combat_system.md §3) — тот же принцип, что уже защищает каст-монтаж строкой выше.
		ForceEndHitboxes();
	}

	ResetCombo();
}

const UComboData* UClanhallComboComponent::GetComboData() const
{
	const AClanhallHumanoidCombatant* Character = Cast<AClanhallHumanoidCombatant>(GetOwner());
	return Character ? Character->GetComboData() : nullptr;
}

int32 UClanhallComboComponent::GetClassRank() const
{
	const AClanhallHumanoidCombatant* Character = Cast<AClanhallHumanoidCombatant>(GetOwner());
	return Character ? Character->ClassRank : 0;
}

UAbilitySystemComponent* UClanhallComboComponent::GetASC() const
{
	if (const IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return Interface->GetAbilitySystemComponent();
	}
	return nullptr;
}

UAnimInstance* UClanhallComboComponent::GetAnimInstance() const
{
	const ACharacter* Char = Cast<ACharacter>(GetOwner());
	return Char && Char->GetMesh() ? Char->GetMesh()->GetAnimInstance() : nullptr;
}
