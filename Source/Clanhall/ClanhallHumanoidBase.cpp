#include "ClanhallHumanoidBase.h"
#include "Clanhall.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CharacterSheetData.h"
#include "AbilitySystem/WeaponData.h"
#include "AbilitySystem/WeaponTypeData.h"
#include "AbilitySystem/Fragments/WeaponFragments.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/AbilityData.h"
#include "AbilitySystem/Abilities/GA_PhysicalSkill.h"
#include "ClanhallWeaponActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

AClanhallHumanoidBase::AClanhallHumanoidBase()
{
}

void AClanhallHumanoidBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// PostInitializeComponents, а не BeginPlay — AActor::BeginPlay диспатчит BeginPlay
	// компонентам раньше, чем выполнится тело переопределения BeginPlay этого актора; любой
	// текущий или будущий компонент, которому CurrentWeapon нужен уже в своём BeginPlay, застал
	// бы его null, поставь инициализацию туда. PostInitializeComponents для акторов, размещённых
	// на уровне, отрабатывает у ВСЕХ акторов до того, как хоть у одного стартует BeginPlay.
	// Пустой Loadout — законное состояние, CurrentWeapon остаётся null, фолбэки в BeginPlay ниже
	// отрабатывают как и раньше.
	CurrentWeapon = (CharacterSheet && CharacterSheet->Loadout.IsValidIndex(0))
		? CharacterSheet->Loadout[0] : nullptr;

	// Множитель скорости оружия применяется при экипировке ко всем трём базовым скоростям
	// разом (`WeaponSystem.md`) - тяжёлое оружие медленное всегда. Считаем от базового JogSpeed бойца, не от текущего MaxWalkSpeed:
	// повторный вызов при будущем свапе оружия иначе умножил бы второй раз.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = GetJogSpeed() * GetWeaponSpeedMultiplier();
	}
}

void AClanhallHumanoidBase::BeginPlay()
{
	Super::BeginPlay();

	if (!AbilitySystemComponent)
	{
		return;
	}

	// Отсутствие данных не должно ломать бой (`WeaponSystem.md`, «Шаблон и живой лист») — боец
	// без листа живёт на фолбэках. Один варнинг на бойца, здесь и только здесь: варнить в местах
	// чтения означало бы залить лог на каждом ударе.
	if (!CurrentWeapon || !CurrentWeapon->Type)
	{
		UE_LOG(LogClanhall, Warning, TEXT("%s: цепочка CurrentWeapon -> Type неполна — "
			"приёмов оружия нет, скорость на фолбэке."), *GetName());
	}

	// Спавн и крепление визуала текущего оружия и оффхенда, если он есть у экземпляра
	// (`WeaponSystem.md`, «Оружие как актор»). Свапа в этом задании нет — акторы ставятся
	// один раз здесь.
	if (CurrentWeapon)
	{
		SpawnedWeapon = SpawnAndAttachWeapon(CurrentWeapon->WeaponClass);

		if (const UWeaponOffhandFragment* Offhand = CurrentWeapon->FindFragment<UWeaponOffhandFragment>())
		{
			SpawnedOffhand = SpawnAndAttachWeapon(Offhand->OffhandClass);
		}
	}

	// Один класс GA_PhysicalSkill гранится по числу записей в
	// GetWeaponType()->Skills (`CharacterHierarchy.md`, «Грант в BeginPlay»; `WeaponSystem.md`,
	// «Класс ↔ лист персонажа») — набор активок принадлежит оружию, не листу. Каждая запись
	// проходит гейт «выучен ли навык» (CharacterSheet->LearnedSkills). Тот же цикл обслуживает и
	// игрока, и AClanhallHumanoidBoss — DataAsset'ы назначаются в Blueprint-наследнике.
	const UWeaponTypeData* WeaponType = GetWeaponType();
	if (WeaponType)
	{
		int32 NumGranted = 0;
		int32 NumRejectedByLearned = 0;

		for (const TPair<FGameplayTag, TObjectPtr<UAbilityData>>& Skill : WeaponType->Skills)
		{
			// Невалидный ключ ИЛИ сам корень Slot (а не лист Q/E/R/F/...) грантится, но
			// GetActiveSkillHandle(Slot_Q) его никогда не найдёт — ключ карты другой.
			// Корень стал выбираемым значением поля, как только завели native-тег Slot
			// под фильтр GetAbilitySlotTag: meta=(Categories="Slot") пропускает и его
			// самого, не только листья. Симптом без этой проверки — "Q не нажимается", причина
			// не ищется.
			if (!Skill.Key.IsValid() || Skill.Key == ClanhallGameplayTags::Slot.GetTag())
			{
				UE_LOG(LogClanhall, Warning, TEXT("%s: запись в WeaponType->Skills с невалидным ключом или корнем Slot вместо листа (Q/E/R/F/...) — навык не будет вызываем."), *GetName());
				continue;
			}

			if (!Skill.Value)
			{
				UE_LOG(LogClanhall, Warning, TEXT("%s: слот %s в WeaponType->Skills не заполнен — навык не грантится."), *GetName(), *Skill.Key.ToString());
				continue;
			}

			if (!Skill.Value->CounterTag.IsValid())
			{
				UE_LOG(LogClanhall, Warning, TEXT("%s: навык в слоте %s без CounterTag — выучить его нечем, LearnedSkills не может на него сослаться."), *GetName(), *Skill.Key.ToString());
				continue;
			}

			// Не выучен — штатное состояние, не ошибка данных (`WeaponSystem.md`, «Класс ↔ лист
			// персонажа»). Verbose, не Warning — иначе лог заливается на каждом бойце.
			if (!CharacterSheet || !CharacterSheet->LearnedSkills.HasTag(Skill.Value->CounterTag))
			{
				UE_LOG(LogClanhall, Verbose, TEXT("%s: навык в слоте %s не выучен (LearnedSkills) — не грантится."), *GetName(), *Skill.Key.ToString());
				++NumRejectedByLearned;
				continue;
			}

			// Слот доносится до способности штатным путём GAS — динамическим тегом спека
			// (не полем в UAbilityData, `CharacterHierarchy.md`, «Ключ по слоту, а не по имени навыка»): один и тот же
			// UAbilityData может лежать сразу в двух китах, слот же принадлежит гранту.
			FGameplayAbilitySpec Spec(UGA_PhysicalSkill::StaticClass(), 1, INDEX_NONE, Skill.Value);
			Spec.GetDynamicSpecSourceTags().AddTag(Skill.Key);
			ActiveSkillHandles.Add(Skill.Key, AbilitySystemComponent->GiveAbility(Spec));
			++NumGranted;
		}

		// Итоговая строка, чтобы гейт не отлаживался вслепую: игрок жмёт Q, ничего не
		// происходит, и без этой строки причина (изученность) не ищется.
		UE_LOG(LogClanhall, Log, TEXT("%s: активок гранто %d, отсеяно изученностью %d."),
			*GetName(), NumGranted, NumRejectedByLearned);
	}
}

void AClanhallHumanoidBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Без явного Destroy оружие переживает своего носителя — PIE, запущенный дважды,
	// оставляет мечи на уровне.
	if (SpawnedWeapon)
	{
		SpawnedWeapon->Destroy();
		SpawnedWeapon = nullptr;
	}
	if (SpawnedOffhand)
	{
		SpawnedOffhand->Destroy();
		SpawnedOffhand = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

AClanhallWeaponActor* AClanhallHumanoidBase::SpawnAndAttachWeapon(TSubclassOf<AClanhallWeaponActor> WeaponClass)
{
	if (!WeaponClass)
	{
		// Законное состояние, не ошибка — оружие без визуала тестируется (`WeaponSystem.md`,
		// «Оружие как актор»). Verbose, не Warning, но не молчит: иначе "меча нет" ищется
		// глазами так же долго, как и "меч торчит из живота".
		UE_LOG(LogClanhall, Verbose, TEXT("%s: WeaponClass не задан — оружие без визуала."), *GetName());
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	AClanhallWeaponActor* SpawnedActor = GetWorld()->SpawnActor<AClanhallWeaponActor>(WeaponClass, SpawnParams);
	if (!SpawnedActor)
	{
		// Реальный случай, не теоретический — WeaponClass указывает сам AClanhallWeaponActor:
		// он Abstract, но пикер TSubclassOf абстрактные классы всё равно показывает. Симптом
		// без этой строки — оружия нет, лог чист.
		UE_LOG(LogClanhall, Warning, TEXT("%s: SpawnActor(%s) вернул nullptr — оружия не будет."),
			*GetName(), *WeaponClass->GetName());
		return nullptr;
	}

	USkeletalMeshComponent* OwnerMesh = GetMesh();
	if (!OwnerMesh)
	{
		UE_LOG(LogClanhall, Warning, TEXT("%s: GetMesh() вернул nullptr — крепить %s некуда, оружие остаётся в мире рут-трансформом."),
			*GetName(), *SpawnedActor->GetClass()->GetName());
		return SpawnedActor;
	}

	const bool bSocketNamed = !SpawnedActor->AttachSocketName.IsNone();
	const bool bSocketExists = bSocketNamed && OwnerMesh->DoesSocketExist(SpawnedActor->AttachSocketName);

	if (!bSocketNamed)
	{
		// Молча ронять оружие в центр персонажа нельзя: симптом «меч торчит из живота»
		// ищется глазами полчаса.
		UE_LOG(LogClanhall, Warning, TEXT("%s: у %s не задан AttachSocketName — крепление в корень меша."),
			*GetName(), *SpawnedActor->GetClass()->GetName());
	}
	else if (!bSocketExists)
	{
		UE_LOG(LogClanhall, Warning, TEXT("%s: сокет %s (заданный в %s) не найден на скелете — крепление в корень меша."),
			*GetName(), *SpawnedActor->AttachSocketName.ToString(), *SpawnedActor->GetClass()->GetName());
	}

	const FName SocketToUse = bSocketExists ? SpawnedActor->AttachSocketName : NAME_None;
	const FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, false);
	SpawnedActor->AttachToComponent(OwnerMesh, AttachRules, SocketToUse);
	SpawnedActor->SetActorRelativeTransform(SpawnedActor->AttachRelativeTransform);

	return SpawnedActor;
}

const UWeaponTypeData* AClanhallHumanoidBase::GetWeaponType() const
{
	return CurrentWeapon ? CurrentWeapon->Type : nullptr;
}

float AClanhallHumanoidBase::GetWeaponSpeedMultiplier() const
{
	const UWeaponTypeData* WeaponType = GetWeaponType();
	return WeaponType ? WeaponType->WeaponSpeedMultiplier : ClanhallWeaponDefaults::WeaponSpeedMultiplier;
}

FGameplayAbilitySpecHandle AClanhallHumanoidBase::GetActiveSkillHandle(FGameplayTag AbilitySlotTag) const
{
	return ActiveSkillHandles.FindRef(AbilitySlotTag);
}

