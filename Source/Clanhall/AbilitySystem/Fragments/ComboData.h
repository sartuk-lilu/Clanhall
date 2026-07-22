// Данные комбо WASD (combo_transition_model_task.md, ранее combo_datatable_picker_task.md,
// combo_fragments_redesign_task.md, development_plan.md Раздел 6.5). Профиль урона — 4 обязательных
// именованных поля (у игрока направления есть всегда, "не задал" не бывает). Модель ходов — пары
// переходов, а не пути: пара направлений «предыдущее -> новое» выбирает клип, история серии до
// предыдущего шага не участвует. Опенер из стойки (FromStance) — все 4 направления валидны
// (FComboTransitionSet). Переходы между ударами (From*) — по 3 направления: повтор направления
// запрещён по дизайну, и слот-владелец исключён из типа набора (FComboTransitionsFrom*), а не просто
// оставлен пустым — невозможное состояние непредставимо. Резолв — простой switch в UComboData
// (FindOpenerMontage/FindTransitionMontage/FindRecoveryAnimation), без поиска и без логов конфликтов:
// слот либо заполнен, либо пуст, конфликтовать нечему.

#pragma once

#include "Engine/DataAsset.h"
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

/** Набор опенеров из боевой стойки — все 4 направления валидны (повтора направления тут нет,
 *  предыдущего удара не было). Направление выводится из ИМЕНИ ПОЛЯ, отдельного поля Direction нет:
 *  дублировать его значило бы завести источник рассинхрона с профилем урона.
 *  nullptr = переход запрещён. */
USTRUCT(BlueprintType)
struct FComboTransitionSet
{
	GENERATED_BODY()

	/** W */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToOverhead;

	/** A */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToLeftSlash;

	/** D */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToRightSlash;

	/** S */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToLowSweep;
};

/** Продолжения после удара W. Слота ToOverhead нет: повтор направления запрещён по дизайну
 *  (нужен новый полноценный замах = Recovery). Невозможное состояние непредставимо. */
USTRUCT(BlueprintType)
struct FComboTransitionsFromOverhead
{
	GENERATED_BODY()

	/** A */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToLeftSlash;

	/** D */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToRightSlash;

	/** S */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToLowSweep;
};

/** Продолжения после удара A. Слота ToLeftSlash нет: повтор направления запрещён по дизайну
 *  (нужен новый полноценный замах = Recovery). Невозможное состояние непредставимо. */
USTRUCT(BlueprintType)
struct FComboTransitionsFromLeftSlash
{
	GENERATED_BODY()

	/** W */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToOverhead;

	/** D */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToRightSlash;

	/** S */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToLowSweep;
};

/** Продолжения после удара D. Слота ToRightSlash нет: повтор направления запрещён по дизайну
 *  (нужен новый полноценный замах = Recovery). Невозможное состояние непредставимо. */
USTRUCT(BlueprintType)
struct FComboTransitionsFromRightSlash
{
	GENERATED_BODY()

	/** W */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToOverhead;

	/** A */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToLeftSlash;

	/** S */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToLowSweep;
};

/** Продолжения после удара S. Слота ToLowSweep нет: повтор направления запрещён по дизайну
 *  (нужен новый полноценный замах = Recovery). Невозможное состояние непредставимо. */
USTRUCT(BlueprintType)
struct FComboTransitionsFromLowSweep
{
	GENERATED_BODY()

	/** W */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToOverhead;

	/** A */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToLeftSlash;

	/** D */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> ToRightSlash;
};

/** Возврат в стойку после терминального удара, по ПОСЛЕДНЕМУ направлению серии. Один struct-филд
 *  вместо 4 плоских — сворачивается в Details одной строкой, как FromStance/FromOverhead/... .
 *  Animation Sequence, не Montage: в возврате пока нет notify-окон и секций, монтаж был бы лишней
 *  сущностью — тот же принцип, что у StanceAnim. Играется через
 *  UAnimInstance::PlaySlotAnimationAsDynamicMontage (UClanhallComboComponent::EndSequenceWithRecovery),
 *  слот задаёт UClanhallComboComponent::RecoverySlotName. nullptr = хвост запечён в сам удар-монтаж,
 *  отдельно не играется. */
USTRUCT(BlueprintType)
struct FComboRecoveryAnimations
{
	GENERATED_BODY()

	/** Ассет: W_Recovery. */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimSequence> AfterOverhead;

	/** Ассет: A_Recovery. */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimSequence> AfterLeftSlash;

	/** Ассет: D_Recovery. */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimSequence> AfterRightSlash;

	/** Ассет: S_Recovery. */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimSequence> AfterLowSweep;
};

/** Данные комбо класса/оружия — профиль урона (4 обязательных поля), опенеры из стойки, переходы
 *  между ударами и Recovery одним ассетом. */
UCLASS()
class CLANHALL_API UComboData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Category = "Combo" (не "Combo|Damage"): подкатегории Details-панели рисуются ПОСЛЕ обычных
	// полей родительской категории независимо от порядка объявления в C++ — вложенность увела бы
	// урон вниз, под переходами. Плоская категория + порядок объявления держат урон сверху.
	UPROPERTY(EditAnywhere, Category = "Combo")
	FDirectionalDamage Overhead;

	UPROPERTY(EditAnywhere, Category = "Combo")
	FDirectionalDamage RightSlash;

	UPROPERTY(EditAnywhere, Category = "Combo")
	FDirectionalDamage LeftSlash;

	UPROPERTY(EditAnywhere, Category = "Combo")
	FDirectionalDamage LowSweep;

	/** Loop-поза боевой стойки класса. ABP забирает её через статичный
	 *  AClanhallCharacter::GetStanceAnim(Character) (BlueprintPure, берёт ACharacter, каст на
	 *  AClanhallCharacter внутри) в Event Blueprint Update Animation и кладёт в переменную состояния
	 *  CombatStance (Main States), которую читает Sequence Player. Именно UAnimSequence, не
	 *  UAnimMontage: стойка — поза в state machine, а не монтаж через слот. */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimSequence> StanceAnim;

	/** Опенеры из боевой стойки. Ассеты: Stance_W / Stance_A / Stance_D / Stance_S. */
	UPROPERTY(EditAnywhere, Category = "Combo")
	FComboTransitionSet FromStance;

	/** Продолжения после удара W. Ассеты: W_A / W_D / W_S. Повтор направления (W_W) непредставим
	 *  на уровне типа — см. FComboTransitionsFromOverhead. */
	UPROPERTY(EditAnywhere, Category = "Combo")
	FComboTransitionsFromOverhead FromOverhead;

	/** Продолжения после удара A. Ассеты: A_D / A_S / A_W. */
	UPROPERTY(EditAnywhere, Category = "Combo")
	FComboTransitionsFromLeftSlash FromLeftSlash;

	/** Продолжения после удара D. Ассеты: D_A / D_S / D_W. */
	UPROPERTY(EditAnywhere, Category = "Combo")
	FComboTransitionsFromRightSlash FromRightSlash;

	/** Продолжения после удара S. Ассеты: S_A / S_D / S_W. */
	UPROPERTY(EditAnywhere, Category = "Combo")
	FComboTransitionsFromLowSweep FromLowSweep;

	/** Возврат в стойку после терминального удара. См. FComboRecoveryAnimations. */
	UPROPERTY(EditAnywhere, Category = "Combo")
	FComboRecoveryAnimations Recovery;

	/** Урон всегда есть (ссылка, не указатель) — свитч по направлению возвращает нужное поле. */
	const FDirectionalDamage& FindDamageByDirection(EClanhallAttackDirection Direction) const;

	/** Опенер из стойки. nullptr = этим направлением серию начать нельзя. */
	UAnimMontage* FindOpenerMontage(EClanhallAttackDirection To) const;

	/** Переход между ударами. nullptr = продолжение запрещено -> Recovery. */
	UAnimMontage* FindTransitionMontage(EClanhallAttackDirection From, EClanhallAttackDirection To) const;

	/** Возврат в стойку по последнему направлению серии. nullptr = хвост запечён в удар-монтаж. */
	UAnimSequence* FindRecoveryAnimation(EClanhallAttackDirection LastDirection) const;
};
