// Второй слой иерархии бойцов — те, кто дерётся на данных игрока: комбо-дерево WASD,
// парирование и слоты активных навыков Q/E/R/F (main_dev_plan.md §8, Блок A). Не для
// мобов/монстров — у тех будут свои навыки и свои монтажи («Принятые решения» п.7), их
// точка входа — AClanhallCombatantBase напрямую.
//
// Грант WASD-ударов и активок Q/E/R/F живёт здесь, а не в AClanhallCharacter: и игрок, и
// AClanhallHumanoidBoss дерутся одним и тем же набором данных (UComboData, UAbilityData),
// поэтому обслуживающий их код общий. Ввод (Enhanced Input / AI) остаётся снаружи —
// HandleAttackInput на UClanhallComboComponent уже нейтрален к источнику (Раздел 7).

#pragma once

#include "CoreMinimal.h"
#include "ClanhallCombatantBase.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallHumanoidCombatant.generated.h"

class UClanhallComboComponent;
class UClanhallParryComponent;
class UAbilityData;
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

	/** Раздел 5: флаг bParrySuccessful, TryParry(). Стороне-нейтрален с Блока D — работает
	 *  одинаково у игрока и у врага. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UClanhallParryComponent> ParryComponent;

	/** «Данные оружия» для комбо (combo_system.md): профиль урона, база ходов и дерево
	 *  цепочек одним ассетом — без фрагментов-обёрток. Назначается в Blueprint-наследнике. */
	UPROPERTY(EditAnywhere, Category = "Combat|WASD")
	TObjectPtr<UComboData> ComboData;

	/** Blueprint-наследники позволяют переопределить TraceRange/TraceRadius и т.п. в редакторе.
	 *  Монтажи и урон — в UComboData, не на этих классах. По умолчанию — соответствующий
	 *  C++ класс (работает без Blueprint). */
	UPROPERTY(EditAnywhere, Category = "Combat|WASD")
	TSubclassOf<UGA_DirectionalAttackBase> AttackOverheadClass;

	UPROPERTY(EditAnywhere, Category = "Combat|WASD")
	TSubclassOf<UGA_DirectionalAttackBase> AttackRightSlashClass;

	UPROPERTY(EditAnywhere, Category = "Combat|WASD")
	TSubclassOf<UGA_DirectionalAttackBase> AttackLeftSlashClass;

	UPROPERTY(EditAnywhere, Category = "Combat|WASD")
	TSubclassOf<UGA_DirectionalAttackBase> AttackLowSweepClass;

	FGameplayAbilitySpecHandle AttackOverheadHandle;
	FGameplayAbilitySpecHandle AttackRightSlashHandle;
	FGameplayAbilitySpecHandle AttackLeftSlashHandle;
	FGameplayAbilitySpecHandle AttackLowSweepHandle;

	// --- Раздел 4: активные навыки Knight (Q/E/R/F) через GA_PhysicalSkill + DataAsset ---
	// Отдельных «вражеских» ассетов не существует (main_dev_plan.md §8) — те же слоты и тот
	// же грант обслужат AClanhallHumanoidBoss, когда Блок C назначит их в его Blueprint.

	/** DataAsset'ы Knight привязываются в Blueprint-наследнике (Content/...) */
	UPROPERTY(EditAnywhere, Category = "Combat|Knight")
	TObjectPtr<UAbilityData> KnightSkillQ_ShieldSlam;

	UPROPERTY(EditAnywhere, Category = "Combat|Knight")
	TObjectPtr<UAbilityData> KnightSkillE_PowerStrike;

	UPROPERTY(EditAnywhere, Category = "Combat|Knight")
	TObjectPtr<UAbilityData> KnightSkillR_ShieldCharge;

	UPROPERTY(EditAnywhere, Category = "Combat|Knight")
	TObjectPtr<UAbilityData> KnightSkillF_Retribution;

	FGameplayAbilitySpecHandle ActiveSkillQHandle;
	FGameplayAbilitySpecHandle ActiveSkillEHandle;
	FGameplayAbilitySpecHandle ActiveSkillRHandle;
	FGameplayAbilitySpecHandle ActiveSkillFHandle;

public:
	AClanhallHumanoidCombatant();

	/** Тег класса персонажа (Ability.Class.Knight и т.д.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|WASD", meta = (Categories = "Ability.Class"))
	FGameplayTag ClassTag;

	/** Потолок длины серии WASD-комбо (0-4). Плейсхолдер до системы прокачки —
	 *  см. combo_system.md, ранг / потолок длины комбо. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|WASD", meta = (ClampMin = "0", ClampMax = "4"))
	int32 ClassRank = 1;

	/** Данные комбо текущего оружия — читает UClanhallComboComponent через GetComboData(). */
	FORCEINLINE const UComboData* GetComboData() const { return ComboData; }

	/** Хэндл направленного удара по W/A/S/D — UClanhallComboComponent сам решает, когда его
	 *  активировать (combo_system.md: инверсия потока активации). */
	FGameplayAbilitySpecHandle GetAttackHandle(EClanhallAttackDirection Direction) const;

protected:
	/** Грант WASD-ударов и активок Q/E/R/F — общий для игрока и AClanhallHumanoidBoss. */
	virtual void BeginPlay() override;
};
