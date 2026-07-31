// Фрагменты, которые реально читает GA_PhysicalSkill (CLAUDE.md: "механика раньше
// визуала" — это механика). Презентационные
// фрагменты (Animation/VFX/SFX) — в PresentationFragments.h.

#pragma once

#include "AbilityFragment.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/ClanhallMarkTypes.h"
#include "GameplayFragments.generated.h"

/** У навыков, наносящих урон. AP/HP-обмен по стандартной формуле (combat_system.md §AP) —
 *  считает GA_ClanhallAbilityBase::ResolveStandardDamage, этот фрагмент только хранит число. */
UCLASS()
class CLANHALL_API UDamageFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Damage")
	float BaseDamage = 0.0f;
};

/** У навыков, накладывающих метку на цель после попадания (mark_system.md §2, Правила 1 и 3). */
UCLASS()
class CLANHALL_API UMarkApplyFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Mark", meta = (Categories = "Mark"))
	FGameplayTag MarkTag;
};

/** У навыков, потребляющих метки на цели (mark_system.md §2, Правило 2). */
UCLASS()
class CLANHALL_API UMarkTriggerFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	/** У навыков, потребляющих метки на цели (mark_system.md §2, Правило 2).
	 *  Первое совпадение выигрывает. Матч по MatchesTag, поэтому конкретные метки
	 *  ставить ВЫШЕ широких: запись с корневым тегом Mark («любая метка») перекроет
	 *  всё, что стоит после неё. */
	UPROPERTY(EditAnywhere, Category = "Mark")
	TArray<FMarkSynergy> Synergies;
};

/** У навыков, перемещающих владельца рывком вперёд (Shield Charge и подобные). Перемещение —
 *  данные навыка, а не свойство клипа: запекать дистанцию в анимацию нельзя (правка потребовала
 *  бы реэкспорта), и это сломало бы инвариант «механика работает без единой анимации» —
 *  при CastMontage == nullptr рывка не было бы вовсе (task_dash_and_counter_window.md, Задача 1).
 *  Фрагмент, а не поле заголовка: его отсутствие несёт смысл «навык не двигает персонажа»,
 *  невыразимый через Distance = 0 (CLAUDE.md, критерий «заголовок или фрагмент»). */
UCLASS(meta = (DisplayName = "Dash"))
class CLANHALL_API UDashFragment : public UAbilityFragment
{
	GENERATED_BODY()
public:
	/** Дистанция рывка вперёд, см. */
	UPROPERTY(EditAnywhere, Category = "Dash", meta = (ClampMin = "0.0"))
	float Distance = 300.0f;

	/** За сколько секунд проходится дистанция. Держать близким к длине каст-монтажа. */
	UPROPERTY(EditAnywhere, Category = "Dash", meta = (ClampMin = "0.01"))
	float Duration = 0.35f;
};
