// Второй слой иерархии бойцов (`CharacterHierarchy.md`, «Три слоя», «Граница слоёв») — те, кто
// дерётся на данных игрока: лист персонажа, оружие и его приёмы. Не для мобов/монстров — у
// тех будут свои навыки и свои монтажи, их точка входа — AClanhallCharacterBase напрямую.
//
// Грант приёмов живёт здесь, а не в AClanhallCharacter: и игрок, и AClanhallHumanoidBoss
// дерутся одним и тем же набором данных (UCharacterSheetData), поэтому обслуживающий их код
// общий. Ввод (Enhanced Input / AI) остаётся снаружи.
//
// Оружие (`WeaponSystem.md`, «Свап оружия»): весь лоадаут носится на теле; выбрано одно - его
// ряд приёмов активен; приём выхватывает выбранное оружие в руки и бьёт одним движением; после
// приёма оружие опущено в руках и через несколько долей без дела само уходит на тело.
// Противник (bKeepsWeaponInHands) оружие не убирает никогда.

#pragma once

#include "CoreMinimal.h"
#include "ClanhallCharacterBase.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "Engine/TimerHandle.h"
#include "ClanhallHumanoidBase.generated.h"

class UCharacterSheetData;
class UWeaponTypeData;
class UWeaponData;
class AClanhallWeaponActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClanhallWeaponSelected, int32, LoadoutIndex);

/** Спавненные акторы одного оружия лоадаута: основная рука и оффхенд (может не быть). */
USTRUCT()
struct FClanhallSpawnedWeapon
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<AClanhallWeaponActor> Main;

	UPROPERTY(Transient)
	TObjectPtr<AClanhallWeaponActor> Offhand;
};

UCLASS(abstract)
class AClanhallHumanoidBase : public AClanhallCharacterBase
{
	GENERATED_BODY()

protected:
	/** Лист персонажа одним ассетом (`WeaponSystem.md`, «Ассеты») — лоадаут и изученные приёмы.
	 *  Назначается в Blueprint-наследнике (и игрока, и AClanhallHumanoidBoss). */
	UPROPERTY(EditAnywhere, Category = "Combat|Sheet")
	TObjectPtr<UCharacterSheetData> CharacterSheet;

	/** Противник оружие не убирает никогда (`WeaponSystem.md`, «Свап оружия»): оружие сразу
	 *  в руках, авто-возврата на тело нет, смена - старое убрал, новое сразу достал. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Sheet")
	bool bKeepsWeaponInHands = false;

	/** Выбранное оружие - индекс в CharacterSheet->Loadout. Его ряд приёмов активен, от него тип
	 *  урона и скорость. Рантайм, выставляется в PostInitializeComponents. */
	int32 SelectedWeaponIndex = INDEX_NONE;

	/** Выбранное оружие, как есть (CharacterSheet->Loadout[SelectedWeaponIndex]). Транзиентно. */
	UPROPERTY(Transient)
	TObjectPtr<UWeaponData> CurrentWeapon;

	/** Выбранное оружие в руках (true) или на теле (false). */
	bool bWeaponInHands = false;

	/** Акторы всего лоадаута, по индексу лоадаута. Уничтожаются в EndPlay. */
	UPROPERTY(Transient)
	TArray<FClanhallSpawnedWeapon> SpawnedWeapons;

	/** Хэндлы приёмов по классу оружия и слоту: приёмы всего лоадаута гранятся разом, и
	 *  GetActiveSkillHandle читает карту выбранного оружия. Класс оружия - ключ, потому что
	 *  набор приёмов принадлежит классу (`WeaponSystem.md`, «Класс ↔ лист персонажа»). */
	TMap<const UWeaponTypeData*, TMap<FGameplayTag, FGameplayAbilitySpecHandle>> SkillHandlesByWeaponType;

	/** Время мира последнего приёма каждого оружия лоадаута - для удара из ножен
	 *  (`AbilitySystem.md`, «Механика школы оружия»). Отрицательное - ни разу. */
	TArray<double> LastSkillTimeByWeapon;

	FTimerHandle AutoHolsterTimer;

public:
	AClanhallHumanoidBase();

	/** Тип выбранного оружия: CurrentWeapon -> Type. nullptr на любом разрыве цепочки —
	 *  вызывающий обязан иметь фолбэк, а не разыменовывать. */
	const UWeaponTypeData* GetWeaponType() const;

	/** Выбранное оружие, как есть — без разворачивания до типа. Читает
	 *  GA_PhysicalSkill::ResolveHitOn (тип урона экземпляра). */
	const UWeaponData* GetCurrentWeapon() const { return CurrentWeapon; }

	const UCharacterSheetData* GetCharacterSheet() const { return CharacterSheet; }

	/** Лоадаут листа - для панели. Пустой массив, если листа нет. */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	TArray<UWeaponData*> GetLoadout() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	int32 GetSelectedWeaponIndex() const { return SelectedWeaponIndex; }

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
	bool IsWeaponInHands() const { return bWeaponInHands; }

	UPROPERTY(BlueprintAssignable, Category = "Combat|Weapon")
	FOnClanhallWeaponSelected OnWeaponSelected;

	/** Множитель скорости выбранного оружия - GetWeaponType()->WeaponSpeedMultiplier, фолбэк
	 *  ClanhallWeaponDefaults::WeaponSpeedMultiplier на разрыве цепочки (`WeaponSystem.md`). */
	float GetWeaponSpeedMultiplier() const;

	/** Хэндл приёма выбранного оружия по слоту (Slot.Q/E/...). Невалидный - приёма нет (не выучен,
	 *  нет записи, нет оружия), TryActivateAbility просто откажет. */
	FGameplayAbilitySpecHandle GetActiveSkillHandle(FGameplayTag AbilitySlotTag) const;

	/** Хэндл приёма конкретного оружия лоадаута по слоту - для панели (все ряды видны сразу). */
	FGameplayAbilitySpecHandle GetSkillHandleForWeapon(int32 LoadoutIndex, FGameplayTag AbilitySlotTag) const;

	/** Клавиша оружия - одно действие: убрать то, что в руках, и выбрать новое
	 *  (`WeaponSystem.md`, «Свап оружия»). В руках было оружие - доля State.WeaponSwap; руки пусты -
	 *  бесплатно. Во время начатого приёма - отказ. Возвращает, принят ли выбор. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	bool SelectWeapon(int32 LoadoutIndex);

	/** Приём выхватывает выбранное оружие: перекрепляет его в руки, если оно на теле. Возвращает
	 *  true, если выхватил именно сейчас. Зовёт UGA_PhysicalSkill на активации. */
	bool DrawSelectedWeapon();

	/** Приём выбранного оружия начался - сбросить авто-возврат на тело, запомнить время для ножен. */
	void NotifySkillStarted();

	/** Приём закончился - оружие остаётся в руках, авто-возврат на тело через AutoHolsterBeats. */
	void NotifySkillEnded();

	/** Секунды с последнего приёма выбранного оружия; ни разу - очень большое число. */
	double GetSecondsSinceSelectedWeaponUsed() const;

protected:
	/** Выставляет выбранное оружие (индекс 0). Не BeginPlay — ловушка: AActor::BeginPlay
	 *  диспатчит BeginPlay компонентам раньше, чем выполняется тело переопределения BeginPlay
	 *  этого актора; любой компонент, которому CurrentWeapon нужен уже в своём BeginPlay, застал
	 *  бы его null. PostInitializeComponents для акторов на уровне отрабатывает у ВСЕХ акторов до
	 *  того, как хоть у одного стартует BeginPlay. Пустой Loadout — законное состояние. */
	virtual void PostInitializeComponents() override;

	/** Спавнит акторы всего лоадаута на тело (противник - выбранное сразу в руки) и гранит приёмы
	 *  всех классов оружия лоадаута через гейт «выучен ли» (`CharacterHierarchy.md`, «Грант в
	 *  BeginPlay»; `WeaponSystem.md`, «Класс ↔ лист персонажа»). */
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Скорость сменилась вместе с выбранным оружием. База - шаг (JogSpeed × множитель);
	 *  игрок переопределяет, чтобы учесть бег. */
	virtual void ApplyWeaponSpeed();

private:
	/** Спавнит актор оружия. nullptr, если класс не задан — законное состояние (оружие без визуала). */
	AClanhallWeaponActor* SpawnWeaponActor(TSubclassOf<AClanhallWeaponActor> WeaponClass);

	/** Крепит актор к сокету руки (bHands) или тела. Порядок SnapToTarget → SetActorRelativeTransform
	 *  не переставляется (`Implementation/Weapons.md`, «Спавн и крепление»). */
	void AttachWeaponActor(AClanhallWeaponActor* Actor, bool bHands);

	/** Перекрепить оба актора оружия лоадаута в руки или на тело. */
	void AttachLoadoutWeapon(int32 LoadoutIndex, bool bHands);

	/** Гранит приёмы одного класса оружия, если они ещё не гранчены. */
	void GrantSkillsOfWeaponType(const UWeaponTypeData* WeaponType);

	void SetSelectedWeaponIndex(int32 LoadoutIndex);

	void HolsterByTimer();
};
