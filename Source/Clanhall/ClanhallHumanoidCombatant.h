// Второй слой иерархии бойцов (`Combatant Hierarchy.md`, «Три слоя», «Граница слоёв») — те, кто дерётся на данных игрока: комбо-дерево WASD,
// парирование и активные навыки. Не для мобов/монстров — у
// тех будут свои навыки и свои монтажи, их точка входа —
// AClanhallCombatantBase напрямую.
//
// Грант WASD-ударов и активок живёт здесь, а не в AClanhallCharacter: и игрок, и
// AClanhallHumanoidBoss дерутся одним и тем же набором данных (UCharacterSheetData),
// поэтому обслуживающий их код общий. Ввод (Enhanced Input / AI) остаётся снаружи —
// HandleAttackInput на UClanhallComboComponent уже нейтрален к источнику.
//
// Класс бойца — один ассет (`weapon_system.md`, «Ассеты вместо `UClassKitData`»): лист персонажа и слоты
// активных навыков раньше были шестью отдельными именованными полями на бойце (по одному
// на каждый скилл Knight), из-за чего забыть заполнить/перенести одно поле означало
// молчаливый рассинхрон — ровно так однажды остались нулевыми дефолты WASD-классов
// у AClanhallHumanoidBoss. Теперь на бойце одно поле CharacterSheet, GetComboData()/GetWeaponType()
// читают его.

#pragma once

#include "CoreMinimal.h"
#include "ClanhallCombatantBase.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallHumanoidCombatant.generated.h"

class UClanhallComboComponent;
class UClanhallParryComponent;
class UCharacterSheetData;
class UComboData;
class UWeaponTypeData;
class UGA_DirectionalAttackBase;

UCLASS(abstract)
class AClanhallHumanoidCombatant : public AClanhallCombatantBase
{
	GENERATED_BODY()

protected:
	/** Ворота ввода, не буфер (`combat_system.md`, «Направления атаки (WASD)»): сам владеет активацией WASD-ударов и решает,
	 *  когда вызвать TryActivateAbility на GA_DirectionalAttack_*. И Enhanced Input (игрок),
	 *  и AI зовут один и тот же HandleAttackInput. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallComboComponent> ComboComponent;

	/** Флаг bStepParried, TryParry(). Стороне-нейтрален (`Parrying.md`, «`UClanhallParryComponent`») —
	 *  работает одинаково у игрока и у AI. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallParryComponent> ParryComponent;

	/** Лист персонажа одним ассетом (`weapon_system.md`, «Ассеты вместо `UClassKitData`») — оружие и
	 *  карта активных навыков по слоту (Ability.Slot.*) вместо шести отдельных полей.
	 *  Назначается в Blueprint-наследнике (и игрока, и AClanhallHumanoidBoss — один и тот
	 *  же лист на класс). */
	UPROPERTY(EditAnywhere, Category = "Combat|Sheet")
	TObjectPtr<UCharacterSheetData> CharacterSheet;

	/** Не UPROPERTY (`Combatant Hierarchy.md`, «Грант в BeginPlay»): значение одинаково у всех китов — это
	 *  плумбинг GAS, не контент класса. Не видно ни редактору, ни Blueprint — переопределить
	 *  дефолт может только C++-наследник в своём конструкторе, если когда-то понадобится. */
	TSubclassOf<UGA_DirectionalAttackBase> AttackOverheadClass;
	TSubclassOf<UGA_DirectionalAttackBase> AttackRightSlashClass;
	TSubclassOf<UGA_DirectionalAttackBase> AttackLeftSlashClass;
	TSubclassOf<UGA_DirectionalAttackBase> AttackLowSweepClass;

	FGameplayAbilitySpecHandle AttackOverheadHandle;
	FGameplayAbilitySpecHandle AttackRightSlashHandle;
	FGameplayAbilitySpecHandle AttackLeftSlashHandle;
	FGameplayAbilitySpecHandle AttackLowSweepHandle;

	/** Хэндлы активок по слоту (Ability.Slot.*), гранятся
	 *  в BeginPlay из GetWeaponType()->Skills через два гейта владения — один цикл на любой
	 *  класс, а не четыре именованных поля (`Combatant Hierarchy.md`, «Грант в BeginPlay»;
	 *  `weapon_system.md`, «Владение оружием»). */
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> ActiveSkillHandles;

public:
	AClanhallHumanoidCombatant();

	/** Тип оружия в руках: CharacterSheet -> Weapon -> Type. nullptr на любом разрыве цепочки —
	 *  вызывающий обязан иметь фолбэк, а не разыменовывать. */
	const UWeaponTypeData* GetWeaponType() const;

	/** Лист персонажа как есть, без разворачивания цепочки — нужен читам вроде
	 *  Clanhall.Player.ShowWeaponEconomy, которым важно различать, ГДЕ именно цепочка
	 *  CharacterSheet -> Weapon -> Type оборвалась, а не только факт разрыва (GetWeaponType()
	 *  это различие стирает, возвращая nullptr на любом звене). */
	const UCharacterSheetData* GetCharacterSheet() const { return CharacterSheet; }

	/** Данные комбо текущего оружия — читает UClanhallComboComponent через GetComboData(). */
	const UComboData* GetComboData() const;

	/** Хэндл направленного удара по W/A/S/D — UClanhallComboComponent сам решает, когда его
	 *  активировать (`Combat Stance and WASD Attacks.md`: инверсия потока активации). */
	FGameplayAbilitySpecHandle GetAttackHandle(EClanhallAttackDirection Direction) const;

	/** Хэндл активного навыка по слоту (Ability.Slot.Q/E/R/F/...) — не найден на момент
	 *  BeginPlay (слот закрыт рангом, навык не выучен, или в GetWeaponType()->Skills вовсе
	 *  нет записи) = невалидный хэндл, TryActivateAbility просто откажет. */
	FGameplayAbilitySpecHandle GetActiveSkillHandle(FGameplayTag AbilitySlotTag) const;

	/** (`combat_system.md`, «Stagger — усталость»; `Combatant Hierarchy.md`, «Прототипный поиск противника»): есть ли у ПРОТИВНИКА этого бойца (см. FindPrototypeOpponent)
	 *  навык с синергией на RequiredMark в GetWeaponType()->Skills. Читает UClanhallParryComponent в
	 *  BeginPlay, чтобы решить, копится ли Stagger владельца вообще. Гейтами владения не
	 *  фильтруется — см. комментарий у HasAbilityWithMarkSynergy. */
	bool HasOpponentWithMarkSynergy(FGameplayTag RequiredMark) const;

protected:
	/** Грант WASD-ударов из CharacterSheet и активок из GetWeaponType()->Skills, через два
	 *  гейта владения — общий для игрока и AClanhallHumanoidBoss (`Combatant Hierarchy.md`,
	 *  «Грант в BeginPlay»; `weapon_system.md`, «Владение оружием»). */
	virtual void BeginPlay() override;

private:
	/** Прототип 1v1: единственный ДРУГОЙ AClanhallHumanoidCombatant в мире. Полноценного
	 *  таргетинга (кто чей противник) в проекте ещё нет — AIController/BT тоже нет
	 *  (`Combatant Hierarchy.md`, «Прототипный поиск противника»).
	 *  Заменить, когда одновременно появится больше одного противника. */
	AClanhallHumanoidCombatant* FindPrototypeOpponent() const;

	/** Есть ли у ЭТОГО бойца (не противника) в GetWeaponType()->Skills навык с синергией
	 *  RequiredMark. Гейтами владения намеренно не фильтруется (`weapon_system.md`, «Владение
	 *  оружием») — вопрос не «может ли применить», а «числится ли в наборе оружия вообще». */
	bool HasAbilityWithMarkSynergy(FGameplayTag RequiredMark) const;
};
