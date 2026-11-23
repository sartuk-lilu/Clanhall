// Фрагменты, которые реально читает GA_PhysicalSkill (CLAUDE.md: "механика раньше
// визуала" — это механика). Презентационные
// фрагменты (Animation/VFX/SFX) — в PresentationFragments.h.

#pragma once

#include "AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "ClanhallCombatTypes.h"
#include "GameplayFragments.generated.h"

/** У навыков, наносящих урон. AP/HP-обмен по стандартной формуле (`CombatSystem.md`, «AP — Armor Points») —
 *  считает ClanhallDamage::ResolveHit, этот фрагмент только хранит число. */
UCLASS()
class CLANHALL_API UDamageFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Damage")
	float BaseDamage = 0.0f;
};

class UMarkData;
class UClanhallMarkComponent;

/** Один уровень навыка (`MarksSystem.md`, «Уровни навыка»): навык уровня N съедает метку уровня
 *  N-1, названную в условии, и кладёт следующую. */
USTRUCT(BlueprintType)
struct CLANHALL_API FClanhallSkillLevel
{
	GENERATED_BODY()

	/** 1 - всегда; 2 - на держателе лежит метка уровня 1 из Reads; 3 - метка уровня 2 из Reads. */
	UPROPERTY(EditAnywhere, Category = "Level", meta = (ClampMin = "1", ClampMax = "3"))
	int32 Level = 1;

	/** Метка условия - ровно одна («или» в условии не бывает). У уровня 1 пусто. */
	UPROPERTY(EditAnywhere, Category = "Level", meta = (Categories = "Mark"))
	FGameplayTag Reads;

	/** Что кладёт на подтверждённом попадании: уровень 1 - метку уровня 1, уровень 2 - метку уровня 2,
	 *  уровень 3 - травму (на цель) или преимущество (на себя), только если съеденную метку положила
	 *  другая школа; своя школа - только сильная версия, ничего не кладётся. */
	UPROPERTY(EditAnywhere, Category = "Level")
	TObjectPtr<UMarkData> Places;

	/** Сила версии - множитель урона приёма. */
	UPROPERTY(EditAnywhere, Category = "Level", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;

	/** Мгновенные эффекты версии - на попадании, один раз. */
	UPROPERTY(EditAnywhere, Category = "Level")
	TArray<EClanhallInstantEffect> InstantEffects;

	/** Длина версии в долях; 0 - как у приёма (UAbilityData::DurationBeats). */
	UPROPERTY(EditAnywhere, Category = "Level", meta = (ClampMin = "0.0"))
	float DurationBeatsOverride = 0.0f;
};

/** Уровни навыка по меткам (`MarksSystem.md`, «Уровни навыка»). Отсутствие фрагмента - приём без
 *  меток, законно: идентичность приёма важнее его места в чужой связке. */
UCLASS(meta = (DisplayName = "Skill Levels"))
class CLANHALL_API USkillLevelsFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	/** Приём кладёт и читает метки либо на цели, либо на себе. */
	UPROPERTY(EditAnywhere, Category = "Levels")
	EClanhallMarkTarget Target = EClanhallMarkTarget::Enemy;

	UPROPERTY(EditAnywhere, Category = "Levels")
	TArray<FClanhallSkillLevel> Levels;

	/** Уровень, которым приём выйдет сейчас, на этом держателе (`MarksSystem.md`, «Уровни навыка»):
	 *  от старшего к младшему; выполнены условия двух - старший. Индекс в Levels, INDEX_NONE - у
	 *  приёма нет уровня 1 (ошибка данных, ловит валидатор). Одна функция для способности и панели:
	 *  подсказка и удар не могут разойтись. */
	int32 ChooseLevelIndex(const UClanhallMarkComponent* Holder) const;
};
