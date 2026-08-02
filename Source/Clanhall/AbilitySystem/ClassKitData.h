// Класс бойца одним ассетом (main_dev_plan.md §8, Блок A2). До этого блока ComboData,
// ClassTag и четыре именованных слота UAbilityData (Q/E/R/F) были шестью отдельными полями
// на бойце, размазанными по BP defaults — забыть заполнить одно означало молчаливый
// рассинхрон (ровно так сломались WASD-удары у AClanhallHumanoidBoss: конструктор, где жили
// дефолты классов, не совпадал с конструктором, откуда шёл грант).
//
// Фрагментов у кита нет: состав фиксирован — комбо-данные и слоты активок есть у ЛЮБОГО
// класса без исключения. Это прецедент UComboData («состав комбо-данных фиксирован, в
// отличие от опциональных Fragments у UAbilityData»), а не прецедент UAbilityData.
//
// Направление связи: кит -> тег, не тег -> кит. Реестр Ability.Class.* -> UClassKitData
// сюда сознательно не входит — он окупится только там, где кит нужно выдать без актора
// (смена класса/оружия в рантайме, Раздел 10; экран выбора класса), а раньше будет угадан.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ClassKitData.generated.h"

class UComboData;
class UAbilityData;

UCLASS()
class CLANHALL_API UClassKitData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Ability.Class.Knight и т.д. — идентичность кита. */
	UPROPERTY(EditAnywhere, Category = "ClassKit", meta = (Categories = "Ability.Class"))
	FGameplayTag ClassTag;

	/** Дерево ходов и профиль урона WASD-серии — общий на класс. */
	UPROPERTY(EditAnywhere, Category = "ClassKit")
	TObjectPtr<UComboData> ComboData;

	/** Активные навыки класса. Ключ — Cooldown.Slot.* (Q/E/R/F и далее по канону восьми
	 *  слотов, ability_system.md §3), а не имя скилла: слот один и тот же для всех оружий,
	 *  а именованные поля-на-скилл зашивали бы имя класса в поле, которое обязано
	 *  обслужить все классы, и их пришлось бы переписывать на восемь. */
	UPROPERTY(EditAnywhere, Category = "ClassKit", meta = (Categories = "Cooldown.Slot"))
	TMap<FGameplayTag, TObjectPtr<UAbilityData>> Skills;
};
