// Данные комбо для текущего оружия (combo_system_redesign.md, development_plan.md Раздел 6.5).
// Полная последовательность направлений от опенера -> монтаж именно этого шага (префиксное
// дерево), а не (индекс, направление) — различает A->D и W->D без спецкода.

#pragma once

#include "AbilityFragment.h"
#include "ClanhallCombatTypes.h"
#include "ComboFragment.generated.h"

class UAnimMontage;

/** Один узел дерева: полный путь направлений от опенера + монтаж именно этого (последнего) удара.
 *  [Left] = одиночный A; [Left, Right] = A->D; [Left, Right, Overhead] = A->D->W. Меток не несёт —
 *  WASD меток не кладёт (mark_system.md §6). */
USTRUCT(BlueprintType)
struct FComboSequence
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Combo")
	TArray<EClanhallAttackDirection> Sequence;

	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> Montage;
};

UCLASS()
class CLANHALL_API UComboFragment : public UAbilityFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Combo")
	TArray<FComboSequence> Sequences;

	/** Длина комбо = ранг оружия (1-4, development_plan.md). После удара этой длины —
	 *  State.ComboRecovery (лок-аут продолжения). */
	UPROPERTY(EditAnywhere, Category = "Combo")
	int32 MaxComboLength = 1;

	/** Общий хвост восстановления — играет после ЛЮБОГО терминального удара серии (нет чейна),
	 *  независимо от её длины. НЕ то же самое, что State.ComboRecovery (тот — лок-аут ввода,
	 *  вешается только по достижении MaxComboLength). nullptr допустим: тогда хвост запечён в
	 *  сам удар-монтаж, отдельно ничего не играется (combo_system_redesign.md). */
	UPROPERTY(EditAnywhere, Category = "Combo")
	TObjectPtr<UAnimMontage> RecoveryMontage;

	/** Точное совпадение полного пути -> запись, или nullptr если такого узла нет в дереве. */
	const FComboSequence* FindExact(const TArray<EClanhallAttackDirection>& Path) const;
};