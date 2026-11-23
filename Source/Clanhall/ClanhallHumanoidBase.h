// Второй слой иерархии бойцов (`CharacterHierarchy.md`, «Три слоя», «Граница слоёв») — те, кто
// дерётся на данных игрока: лист персонажа, оружие и его приёмы. Не для мобов/монстров — у
// тех будут свои навыки и свои монтажи, их точка входа — AClanhallCharacterBase напрямую.
//
// Грант приёмов живёт здесь, а не в AClanhallCharacter: и игрок, и AClanhallHumanoidBoss
// дерутся одним и тем же набором данных (UCharacterSheetData), поэтому обслуживающий их код
// общий. Ввод (Enhanced Input / AI) остаётся снаружи.
//
// Класс бойца — один ассет (`WeaponSystem.md`, «Ассеты вместо `UClassKitData`»): на бойце одно
// поле CharacterSheet, GetWeaponType() читает его.

#pragma once

#include "CoreMinimal.h"
#include "ClanhallCharacterBase.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "ClanhallHumanoidBase.generated.h"

class UClanhallParryComponent;
class UCharacterSheetData;
class UWeaponTypeData;
class UWeaponData;
class AClanhallWeaponActor;

UCLASS(abstract)
class AClanhallHumanoidBase : public AClanhallCharacterBase
{
	GENERATED_BODY()

protected:
	/** Флаг bStepParried, TryParry(). Стороне-нейтрален (`Parrying.md`, «`UClanhallParryComponent`») —
	 *  работает одинаково у игрока и у AI. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallParryComponent> ParryComponent;

	/** Лист персонажа одним ассетом (`WeaponSystem.md`, «Ассеты вместо `UClassKitData`») — оружие и
	 *  карта активных навыков по слоту (Slot.*) вместо шести отдельных полей.
	 *  Назначается в Blueprint-наследнике (и игрока, и AClanhallHumanoidBoss — один и тот
	 *  же лист на класс). */
	UPROPERTY(EditAnywhere, Category = "Combat|Sheet")
	TObjectPtr<UCharacterSheetData> CharacterSheet;

	/** Оружие в руках сейчас. Рантайм-состояние, не шаблон: боец свапает оружие в бою, и
	 *  шаблон описывает только то, с чем он вышел (`WeaponSystem.md`, «Что в лист входит»).
	 *  Транзиентно — в ассете не сериализуется. Выставляется в PostInitializeComponents,
	 *  не в BeginPlay — см. комментарий у объявления. */
	UPROPERTY(Transient)
	TObjectPtr<UWeaponData> CurrentWeapon;

	/** Спавненные акторы текущего оружия — хранить полями, иначе будущий свап не сможет
	 *  убрать старое оружие. Уничтожаются в EndPlay. */
	UPROPERTY(Transient)
	TObjectPtr<AClanhallWeaponActor> SpawnedWeapon;

	UPROPERTY(Transient)
	TObjectPtr<AClanhallWeaponActor> SpawnedOffhand;

	/** Хэндлы активок по слоту (Slot.*), гранятся
	 *  в BeginPlay из GetWeaponType()->Skills через два гейта владения — один цикл на любой
	 *  класс, а не четыре именованных поля (`CharacterHierarchy.md`, «Грант в BeginPlay»;
	 *  `WeaponSystem.md`, «Владение оружием»). */
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> ActiveSkillHandles;

public:
	AClanhallHumanoidBase();

	/** Тип оружия в руках: CurrentWeapon -> Type. nullptr на любом разрыве цепочки —
	 *  вызывающий обязан иметь фолбэк, а не разыменовывать. */
	const UWeaponTypeData* GetWeaponType() const;

	/** Лист персонажа как есть, без разворачивания цепочки — нужен читам вроде
	 *  Clanhall.Player.ShowWeaponEconomy, которым важно различать, ГДЕ именно цепочка
	 *  CurrentWeapon -> Type оборвалась, а не только факт разрыва (GetWeaponType()
	 *  это различие стирает, возвращая nullptr на любом звене). */
	const UCharacterSheetData* GetCharacterSheet() const { return CharacterSheet; }

	/** Оружие в руках сейчас, как есть — тот же смысл, что у GetCharacterSheet(): читам вроде
	 *  Clanhall.Player.ShowWeaponEconomy важно различать, ГДЕ цепочка оборвалась. */
	const UWeaponData* GetCurrentWeapon() const { return CurrentWeapon; }

	/** Множитель скорости активного оружия - GetWeaponType()->WeaponSpeedMultiplier, фолбэк
	 *  ClanhallWeaponDefaults::WeaponSpeedMultiplier на разрыве цепочки (`WeaponSystem.md`).
	 *  Читает PostInitializeComponents (применение к базовым скоростям при экипировке) и
	 *  AClanhallCharacter (BeginPlay, StartSprint/StopSprint). */
	float GetWeaponSpeedMultiplier() const;

	/** Хэндл активного навыка по слоту (Slot.Q/E/R/F/...) — не найден на момент
	 *  BeginPlay (слот закрыт рангом, навык не выучен, или в GetWeaponType()->Skills вовсе
	 *  нет записи) = невалидный хэндл, TryActivateAbility просто откажет. */
	FGameplayAbilitySpecHandle GetActiveSkillHandle(FGameplayTag AbilitySlotTag) const;

protected:
	/** Выставляет CurrentWeapon из CharacterSheet->Loadout[0]. Не BeginPlay — ловушка:
	 *  AActor::BeginPlay диспатчит BeginPlay компонентам раньше, чем выполняется тело
	 *  переопределения BeginPlay этого актора; любой компонент, которому CurrentWeapon нужен
	 *  уже в своём BeginPlay, застал бы его null, поставь инициализацию туда. Ни ошибки, ни
	 *  варнинга. PostInitializeComponents для акторов, размещённых на уровне, отрабатывает
	 *  у ВСЕХ акторов до того, как хоть у одного стартует BeginPlay. Пустой Loadout — законное
	 *  состояние, фолбэки работают как раньше. */
	virtual void PostInitializeComponents() override;

	/** Грант активок из GetWeaponType()->Skills, через два
	 *  гейта владения — общий для игрока и AClanhallHumanoidBoss (`CharacterHierarchy.md`,
	 *  «Грант в BeginPlay»; `WeaponSystem.md`, «Владение оружием»). Спавнит и крепит акторы
	 *  текущего оружия и оффхенда. */
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Спавнит актор оружия и крепит к сокету, который назвал сам актор. Возвращает nullptr,
	 *  если класс не задан — это законное состояние (оружие без визуала тестируется). */
	AClanhallWeaponActor* SpawnAndAttachWeapon(TSubclassOf<AClanhallWeaponActor> WeaponClass);

};
