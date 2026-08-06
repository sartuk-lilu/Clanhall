// Второй слой иерархии бойцов — те, кто дерётся на данных игрока: комбо-дерево WASD,
// парирование и активные навыки (main_dev_plan.md §8, Блок A). Не для мобов/монстров — у
// тех будут свои навыки и свои монтажи («Принятые решения» п.7), их точка входа —
// AClanhallCombatantBase напрямую.
//
// Грант WASD-ударов и активок живёт здесь, а не в AClanhallCharacter: и игрок, и
// AClanhallHumanoidBoss дерутся одним и тем же набором данных (UClassKitData),
// поэтому обслуживающий их код общий. Ввод (Enhanced Input / AI) остаётся снаружи —
// HandleAttackInput на UClanhallComboComponent уже нейтрален к источнику (Раздел 7).
//
// Класс бойца — один ассет (main_dev_plan.md §8, Блок A2): ComboData, ClassTag и слоты
// активных навыков раньше были шестью отдельными именованными полями на бойце (по одному
// на каждый скилл Knight), из-за чего забыть заполнить/перенести одно поле означало
// молчаливый рассинхрон — ровно так однажды остались нулевыми дефолты WASD-классов
// у AClanhallHumanoidBoss. Теперь на бойце одно поле ClassKit, GetComboData()/GetClassTag()
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
class UClassKitData;
class UComboData;
class UGA_DirectionalAttackBase;

UCLASS(abstract)
class AClanhallHumanoidCombatant : public AClanhallCombatantBase
{
	GENERATED_BODY()

protected:
	/** Ворота ввода, не буфер (combo_system.md): сам владеет активацией WASD-ударов и решает,
	 *  когда вызвать TryActivateAbility на GA_DirectionalAttack_*. И Enhanced Input (игрок),
	 *  и AI (main_dev_plan.md §8, Блок B) зовут один и тот же HandleAttackInput. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallComboComponent> ComboComponent;

	/** Раздел 5: флаг bStepParried, TryParry(). Стороне-нейтрален (task_parry_rework.md) —
	 *  работает одинаково у игрока и у AI. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallParryComponent> ParryComponent;

	/** main_dev_plan.md §8, Блок A2: класс бойца одним ассетом — ComboData, ClassTag и
	 *  карта активных навыков по слоту (Ability.Slot.*) вместо шести отдельных полей.
	 *  Назначается в Blueprint-наследнике (и игрока, и AClanhallHumanoidBoss — один и тот
	 *  же кит на класс). */
	UPROPERTY(EditAnywhere, Category = "Combat|Class")
	TObjectPtr<UClassKitData> ClassKit;

	/** Не UPROPERTY (main_dev_plan.md §8, Блок A2): значение одинаково у всех китов — это
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

	/** main_dev_plan.md §8, Блок A2: хэндлы активок кита по слоту (Ability.Slot.*), гранятся
	 *  в BeginPlay из ClassKit->Skills — один цикл на любой класс, а не четыре именованных поля. */
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> ActiveSkillHandles;

public:
	AClanhallHumanoidCombatant();

	/** Потолок длины серии WASD-комбо (1-4). Плейсхолдер до системы прокачки —
	 *  см. combo_system.md, ранг / потолок длины комбо. Свойство ЭКЗЕМПЛЯРА, не кита
	 *  (main_dev_plan.md §8, Блок A2, DO NOT): один кит обслуживает и рядового бойца, и босса
	 *  того же класса, у них разный ранг. Ранга 0 не существует (task_skill_economy_loops.md
	 *  §3): на ранге 0 нет ни одного активного навыка, то есть нет ни одного потребителя
	 *  зарядов — класс либо не взят вовсе, либо взят и сразу ранга 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Class", meta = (ClampMin = "1", ClampMax = "4"))
	int32 ClassRank = 1;

	/** Ability.Class.Knight и т.д. — читает ClassKit->ClassTag. BlueprintPure: старое поле
	 *  ClassTag было BlueprintReadWrite, и хотя в C++ его сейчас не читает ничто (GA_ClanhallAbilityBase
	 *  и GA_PhysicalSkill его не используют — чистый задел), нода в ABP/виджете, если она есть,
	 *  не должна остаться без замены при перекомпиляции BP. */
	UFUNCTION(BlueprintPure, Category = "Combat|Class")
	FGameplayTag GetClassTag() const;

	/** Данные комбо текущего оружия — читает UClanhallComboComponent через GetComboData(). */
	const UComboData* GetComboData() const;

	/** Хэндл направленного удара по W/A/S/D — UClanhallComboComponent сам решает, когда его
	 *  активировать (combo_system.md: инверсия потока активации). */
	FGameplayAbilitySpecHandle GetAttackHandle(EClanhallAttackDirection Direction) const;

	/** Хэндл активного навыка по слоту (Ability.Slot.Q/E/R/F/...) — не найден в
	 *  ClassKit->Skills на момент BeginPlay = невалидный хэндл, TryActivateAbility просто
	 *  откажет (main_dev_plan.md §8, Блок A2). */
	FGameplayAbilitySpecHandle GetActiveSkillHandle(FGameplayTag AbilitySlotTag) const;

	/** task_stagger_control_code.md §7: есть ли у ПРОТИВНИКА этого бойца (см. FindPrototypeOpponent)
	 *  навык с синергией на RequiredMark в ClassKit->Skills. Читает UClanhallParryComponent в
	 *  BeginPlay, чтобы решить, копится ли Stagger владельца вообще. */
	bool HasOpponentWithMarkSynergy(FGameplayTag RequiredMark) const;

protected:
	/** Грант WASD-ударов и активок из ClassKit — общий для игрока и AClanhallHumanoidBoss. */
	virtual void BeginPlay() override;

private:
	/** Прототип 1v1: единственный ДРУГОЙ AClanhallHumanoidCombatant в мире. Полноценного
	 *  таргетинга (кто чей противник) в проекте ещё нет — AIController/BT тоже нет (main_dev_plan.md
	 *  §8, Блок G). Заменить, когда одновременно появится больше одного противника. */
	AClanhallHumanoidCombatant* FindPrototypeOpponent() const;

	/** Есть ли у ЭТОГО бойца (не противника) в ClassKit->Skills навык с синергией RequiredMark. */
	bool HasAbilityWithMarkSynergy(FGameplayTag RequiredMark) const;
};
