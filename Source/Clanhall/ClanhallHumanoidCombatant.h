// Второй слой иерархии бойцов (`Combatant Hierarchy.md`, «Три слоя», «Граница слоёв») — те, кто дерётся на данных игрока: комбо-дерево WASD,
// парирование и активные навыки. Не для мобов/монстров — у
// тех будут свои навыки и свои монтажи, их точка входа —
// AClanhallCombatantBase напрямую.
//
// Грант WASD-ударов и активок живёт здесь, а не в AClanhallCharacter: и игрок, и
// AClanhallHumanoidBoss дерутся одним и тем же набором данных (UClassKitData),
// поэтому обслуживающий их код общий. Ввод (Enhanced Input / AI) остаётся снаружи —
// HandleAttackInput на UClanhallComboComponent уже нейтрален к источнику.
//
// Класс бойца — один ассет (`Combatant Hierarchy.md`, «UClassKitData — класс одним ассетом»): ComboData, ClassTag и слоты
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
	/** Ворота ввода, не буфер (`combat_system.md`, «Направления атаки (WASD)»): сам владеет активацией WASD-ударов и решает,
	 *  когда вызвать TryActivateAbility на GA_DirectionalAttack_*. И Enhanced Input (игрок),
	 *  и AI зовут один и тот же HandleAttackInput. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallComboComponent> ComboComponent;

	/** Флаг bStepParried, TryParry(). Стороне-нейтрален (`Parrying.md`, «`UClanhallParryComponent`») —
	 *  работает одинаково у игрока и у AI. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallParryComponent> ParryComponent;

	/** Класс бойца одним ассетом (`Combatant Hierarchy.md`, «UClassKitData — класс одним ассетом») — ComboData, ClassTag и
	 *  карта активных навыков по слоту (Ability.Slot.*) вместо шести отдельных полей.
	 *  Назначается в Blueprint-наследнике (и игрока, и AClanhallHumanoidBoss — один и тот
	 *  же кит на класс). */
	UPROPERTY(EditAnywhere, Category = "Combat|Class")
	TObjectPtr<UClassKitData> ClassKit;

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

	/** Хэндлы активок кита по слоту (Ability.Slot.*), гранятся
	 *  в BeginPlay из ClassKit->Skills — один цикл на любой класс, а не четыре именованных поля
	 *  (`Combatant Hierarchy.md`, «Грант в BeginPlay»). */
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> ActiveSkillHandles;

public:
	AClanhallHumanoidCombatant();

	/** Потолок длины серии WASD-комбо (1-4). Плейсхолдер до системы прокачки —
	 *  см. `Combat Stance and WASD Attacks.md`, ранг / потолок длины комбо. Свойство ЭКЗЕМПЛЯРА, не кита:
	 *  один кит обслуживает и рядового бойца, и босса
	 *  того же класса, у них разный ранг (`Combatant Hierarchy.md`, «ClassRank — свойство экземпляра»). Ранга 0 не существует (`economy_system.md`, «Ранги и длина серии»):
	 *  на ранге 0 нет ни одного активного навыка, то есть нет ни одного потребителя
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
	 *  активировать (`Combat Stance and WASD Attacks.md`: инверсия потока активации). */
	FGameplayAbilitySpecHandle GetAttackHandle(EClanhallAttackDirection Direction) const;

	/** Хэндл активного навыка по слоту (Ability.Slot.Q/E/R/F/...) — не найден в
	 *  ClassKit->Skills на момент BeginPlay = невалидный хэндл, TryActivateAbility просто
	 *  откажет. */
	FGameplayAbilitySpecHandle GetActiveSkillHandle(FGameplayTag AbilitySlotTag) const;

	/** (`combat_system.md`, «Stagger — усталость»; `Combatant Hierarchy.md`, «Прототипный поиск противника»): есть ли у ПРОТИВНИКА этого бойца (см. FindPrototypeOpponent)
	 *  навык с синергией на RequiredMark в ClassKit->Skills. Читает UClanhallParryComponent в
	 *  BeginPlay, чтобы решить, копится ли Stagger владельца вообще. */
	bool HasOpponentWithMarkSynergy(FGameplayTag RequiredMark) const;

protected:
	/** Грант WASD-ударов и активок из ClassKit — общий для игрока и AClanhallHumanoidBoss
	 *  (`Combatant Hierarchy.md`, «Грант в BeginPlay»). */
	virtual void BeginPlay() override;

private:
	/** Прототип 1v1: единственный ДРУГОЙ AClanhallHumanoidCombatant в мире. Полноценного
	 *  таргетинга (кто чей противник) в проекте ещё нет — AIController/BT тоже нет
	 *  (`Combatant Hierarchy.md`, «Прототипный поиск противника»).
	 *  Заменить, когда одновременно появится больше одного противника. */
	AClanhallHumanoidCombatant* FindPrototypeOpponent() const;

	/** Есть ли у ЭТОГО бойца (не противника) в ClassKit->Skills навык с синергией RequiredMark. */
	bool HasAbilityWithMarkSynergy(FGameplayTag RequiredMark) const;
};
