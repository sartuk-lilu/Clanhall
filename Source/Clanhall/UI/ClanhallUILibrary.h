// Blueprint Function Library для HUD: сохранение раскладки рамок и клавиш, данные панели приёмов,
// значки меток и кристалл. Вызывается из виджетов Blueprint; ASC и бойца в суб-виджеты задаёт
// родитель (`Implementation/HUD.md`).

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/ClanhallMarkComponent.h"
#include "ClanhallSlotAssignment.h"
#include "ClanhallUILibrary.generated.h"

class AClanhallHumanoidBase;
class UAbilityData;
class UWeaponData;
class UTexture2D;

/** Иконка приёма на панели (`AbilitySystem.md`, «Панель приёмов»). Только то, что игрок мог бы
 *  вывести сам: никакого «нажми сейчас». */
USTRUCT(BlueprintType)
struct CLANHALL_API FClanhallPanelSkill
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Panel")
	TObjectPtr<UAbilityData> Skill = nullptr;

	/** Идентичность приёма (CounterTag) - для AssignSlot. */
	UPROPERTY(BlueprintReadOnly, Category = "Panel")
	FGameplayTag SkillIdentity;

	/** Клавиша; пусто - приём на панели без клавиши (законно). */
	UPROPERTY(BlueprintReadOnly, Category = "Panel")
	FGameplayTag Slot;

	/** Остаток перезарядки в долях; 0 - готов. Идёт и у оружия не в руках. */
	UPROPERTY(BlueprintReadOnly, Category = "Panel")
	float RemainingCooldownBeats = 0.0f;

	/** Полная перезарядка приёма в долях - для доли оверлея. */
	UPROPERTY(BlueprintReadOnly, Category = "Panel")
	float TotalCooldownBeats = 0.0f;

	/** Клавиша приёма зажата на активном ряду - следующий клик ЛКМ запустит его. */
	UPROPERTY(BlueprintReadOnly, Category = "Panel")
	bool bSelected = false;

	/** Каким уровнем приём выйдет сейчас (`MarksSystem.md`, «Уровни навыка»). */
	UPROPERTY(BlueprintReadOnly, Category = "Panel")
	int32 Level = 1;
};

/** Ряд панели - одно оружие лоадаута. Ряды всех оружий видны сразу. */
USTRUCT(BlueprintType)
struct CLANHALL_API FClanhallPanelRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Panel")
	TObjectPtr<UWeaponData> Weapon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Panel")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Panel")
	int32 LoadoutIndex = INDEX_NONE;

	/** Выбранное оружие - его ряд активен. */
	UPROPERTY(BlueprintReadOnly, Category = "Panel")
	bool bSelected = false;

	/** Выбранное оружие в руках, а не на теле. */
	UPROPERTY(BlueprintReadOnly, Category = "Panel")
	bool bInHands = false;

	UPROPERTY(BlueprintReadOnly, Category = "Panel")
	TArray<FClanhallPanelSkill> Skills;
};

UCLASS()
class CLANHALL_API UClanhallUILibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Сохраняет позиции рамок в слот "HUDLayout". Вызывать в OnDragEnd каждой рамки. */
	UFUNCTION(BlueprintCallable, Category="Clanhall|UI")
	static void SaveHUDLayout(FVector2D PlayerFramePosition, FVector2D EnemyFramePosition);

	/** Загружает позиции рамок. Возвращает false при первом запуске (используй дефолты).
	 *  Вызывать в Event Construct главного HUD-виджета. */
	UFUNCTION(BlueprintCallable, Category="Clanhall|UI")
	static bool LoadHUDLayout(FVector2D& PlayerFramePosition, FVector2D& EnemyFramePosition);

	/** Клавиши приёмов игрока - тот же слот сохранения, раскладка рамок не трогается. */
	static void SaveSkillSlots(const TArray<FClanhallSlotAssignment>& Assignments);
	static bool LoadSkillSlots(TArray<FClanhallSlotAssignment>& OutAssignments);

	/** Всё, что рисует панель приёмов: ряд на каждое оружие лоадаута, иконки приёмов с клавишей,
	 *  перезарядкой, выбранностью и уровнем. Перезарядка тикает - звать поллингом, как бары. */
	UFUNCTION(BlueprintCallable, Category="Clanhall|UI")
	static TArray<FClanhallPanelRow> GetSkillPanel(AClanhallHumanoidBase* Fighter);

	/** Каким уровнем приём выйдет сейчас - та же USkillLevelsFragment::ChooseLevelIndex, что
	 *  выбирает уровень на активации. Держатель приёма на цель - текущая цель прицела; цели нет -
	 *  уровень 1. Приёма без уровней - 1. */
	UFUNCTION(BlueprintPure, Category="Clanhall|UI")
	static int32 PreviewSkillLevel(const AClanhallHumanoidBase* Fighter, const UAbilityData* Skill);

	/** Метки бойца двумя группами (`Controls.md`, «Значки меток и ресурсов»): первый и второй
	 *  уровни; травмы и преимущества. */
	UFUNCTION(BlueprintCallable, Category="Clanhall|UI")
	static void GetFighterMarks(AActor* Fighter, TArray<FClanhallMarkView>& Marks, TArray<FClanhallMarkView>& TraumasAndAdvantages);

	/** Накопленный кристалл бойца: целые и доля следующего. 0 - компонента нет. */
	UFUNCTION(BlueprintPure, Category="Clanhall|UI")
	static float GetFighterCrystal(const AActor* Fighter);
};
