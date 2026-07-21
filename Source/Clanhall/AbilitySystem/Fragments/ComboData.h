// Данные комбо WASD (combo_datatable_picker_task.md, combo_fragments_redesign_task.md,
// development_plan.md Раздел 6.5). Профиль урона — 4 обязательных именованных поля (у игрока
// направления есть всегда, "не задал" не бывает). База ходов — Data Table на класс (FComboMove —
// строка таблицы), шаги цепочек ссылаются на строки по имени через FComboStep (обёртка нужна для
// editor-пикера в ClanhallEditor — кастомизация по типу структуры затронула бы все голые FName).
// Направление ВАЛИДИРУЕТ путь; MoveId ВЫБИРАЕТ клип — резолв в UClanhallComboComponent, не здесь.

#pragma once

#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "ClanhallCombatTypes.h"
#include "GameplayTagContainer.h"
#include "ComboData.generated.h"

class UAnimMontage;
class UAnimSequence;

/** Базовый урон и тип на направление — направление задаётся именем поля-владельца в UComboData,
 *  в самой структуре не хранится. */
USTRUCT(BlueprintType)
struct FDirectionalDamage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Combo")
	float BaseDamage = 10.0f;

	/** Заглушка: тег типа урона. В расчёте пока НЕ используется — задел. */
	UPROPERTY(EditAnywhere, Category = "Combo", meta = (Categories = "Damage.Type"))
	FGameplayTag DamageType;
};

/** Один ход = клип одного удара, строка Data Table класса (MovesTable). Идентификатором служит
 *  ИМЯ СТРОКИ таблицы (Stance_D, D_A, A_W — без префикса класса, класс задан самой таблицей),
 *  отдельного поля MoveId больше нет. Одному направлению может соответствовать несколько ходов
 *  (общий и уникальный) — различаются именем строки, не Direction. */
USTRUCT(BlueprintType)
struct FComboMove : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Combo")
	EClanhallAttackDirection Direction = EClanhallAttackDirection::Overhead;

	/** Клип одного удара — окна парирования/трейса зашиты в самом монтаже. */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> Montage;
};

/** Обёртка одного шага цепочки — хранит имя строки в MovesTable. Отдельная структура (не голый
 *  FName), чтобы editor-пикер (Фаза 3, ClanhallEditor) можно было навесить кастомизацией по типу
 *  структуры, не затрагивая все FName-поля в редакторе. */
USTRUCT(BlueprintType)
struct FComboStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Combo")
	FName MoveId;
};

/** Одна цепочка комбо — шаги ссылаются на строки MovesTable по имени. Последовательность
 *  направлений выводится из Move.Direction каждого шага, а не хранится отдельно. Steps[0] по
 *  соглашению — ход "из стойки" (опенер = цепочка с одним шагом). */
USTRUCT(BlueprintType)
struct FComboChain
{
	GENERATED_BODY()

	/** Только для читаемости в редакторе (не участвует в резолве) — заголовок свёрнутого элемента
	 *  массива Chains вместо "Index [N]" (UComboData::Chains, meta=TitleProperty="Description").
	 *  Впиши название всей последовательности целиком, напр. "D-A-W Finisher". */
	UPROPERTY(EditAnywhere, Category = "Combo")
	FString Description;

	UPROPERTY(EditAnywhere, Category = "Combo")
	TArray<FComboStep> Steps;

	/** Recovery на финал именно этой цепочки. nullptr = хвост запечён в удар-монтаж, отдельно
	 *  не играется. */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> RecoveryMontage;
};

/** Данные комбо класса/оружия — профиль урона (4 обязательных поля), Data Table ходов и дерево
 *  цепочек одним ассетом. */
UCLASS()
class CLANHALL_API UComboData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Category = "Combo" (не "Combo|Damage"): подкатегории Details-панели рисуются ПОСЛЕ обычных
	// полей родительской категории независимо от порядка объявления в C++ — вложенность увела бы
	// урон вниз, под MovesTable/Chains. Плоская категория + порядок объявления держат урон сверху.
	UPROPERTY(EditAnywhere, Category = "Combo")
	FDirectionalDamage Overhead;

	UPROPERTY(EditAnywhere, Category = "Combo")
	FDirectionalDamage RightSlash;

	UPROPERTY(EditAnywhere, Category = "Combo")
	FDirectionalDamage LeftSlash;

	UPROPERTY(EditAnywhere, Category = "Combo")
	FDirectionalDamage LowSweep;

	/** Loop-поза боевой стойки класса. ABP тянет её в Sequence Player состояния CombatStance
	 *  (Main States) через переменную CurrentStanceAnim. Именно UAnimSequence, не UAnimMontage:
	 *  стойка — поза в state machine, а не монтаж через слот. */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimSequence> StanceAnim;

	/** База ходов класса — одна Data Table на ассет, строки типа FComboMove. Фильтр пикера
	 *  таблиц по типу строки (RowStructure — Engine/Private/DataTable.cpp, формат подтверждён
	 *  живыми примерами в движке: RichTextBlock.h/RichTextBlockImageDecorator.h). */
	UPROPERTY(EditAnywhere, Category = "Combo", meta = (RequiredAssetDataTags = "RowStructure=/Script/Clanhall.ComboMove"))
	TObjectPtr<UDataTable> MovesTable;

	/** TitleProperty — встроенный механизм Details-панели (не наш код): заголовок свёрнутого
	 *  элемента массива берётся из FComboChain::Description вместо "Index [N]". */
	UPROPERTY(EditAnywhere, Category = "Combo", meta = (TitleProperty = "Description"))
	TArray<FComboChain> Chains;

	/** Урон всегда есть (ссылка, не указатель) — свитч по направлению возвращает нужное поле. */
	const FDirectionalDamage& FindDamageByDirection(EClanhallAttackDirection Direction) const;

	/** Ход по имени строки — из MovesTable. */
	const FComboMove* FindMoveById(FName MoveId) const;
};
