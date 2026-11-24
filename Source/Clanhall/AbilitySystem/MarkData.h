// Метка - один ассет (`MarksSystem.md`): тег-идентичность, уровень, вид и эффект на параметры.
// Уровень - горизонт времени, а не сила: время жизни берётся из настроек боя по уровню
// (MarkL1Beats / MarkL2Beats / MarkL3Beats), поля-переопределения на метке нет.
//
// Что метка меняет - Modifiers: атрибут, операция, значение. Компонент собирает из них Infinite
// GameplayEffect на лету (один на ассет метки), время жизни держит он же. Так метка - ровно один
// ассет, без парного Blueprint-эффекта на каждую. Effect - запасной путь для того, что модификатором
// не выразить (Infinite GE, тоже снимается вместе с меткой). Бонусы меток складываются, а
// не перемножаются: модификаторы множителей - операция MultiplyAdditive
// (`MarksSystem.md`, «Слоты меток»; ×1.15 и ×1.4 дают ×1.55). MultiplyCompound метками не
// используется.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "ClanhallCombatTypes.h"
#include "MarkData.generated.h"

class UGameplayEffect;
class UTexture2D;
class UAnimMontage;

/** Одна правка атрибута меткой. Множители - Multiply (Additive): бонусы складываются. */
USTRUCT(BlueprintType)
struct CLANHALL_API FClanhallMarkModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Mark")
	FGameplayAttribute Attribute;

	/** Multiply (Additive) для множителей (1.15 - плюс 15%), Add (Base) для крита и снижения порога. */
	UPROPERTY(EditAnywhere, Category = "Mark")
	TEnumAsByte<EGameplayModOp::Type> Op = EGameplayModOp::MultiplyAdditive;

	UPROPERTY(EditAnywhere, Category = "Mark")
	float Value = 1.0f;
};

UCLASS()
class CLANHALL_API UMarkData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Идентичность метки - её называет условие уровня навыка. Пока метка лежит, тег висит на ASC
	 *  держателя loose-тегом. */
	UPROPERTY(EditAnywhere, Category = "Mark", meta = (Categories = "Mark"))
	FGameplayTag MarkTag;

	/** Уровень 1/2/3 - горизонт времени. */
	UPROPERTY(EditAnywhere, Category = "Mark", meta = (ClampMin = "1", ClampMax = "3"))
	int32 Level = 1;

	/** Дебафф/бафф на уровнях 1-2, травма/преимущество на уровне 3. */
	UPROPERTY(EditAnywhere, Category = "Mark")
	EClanhallMarkKind Kind = EClanhallMarkKind::Debuff;

	/** Что метка меняет - правки атрибутов; компонент собирает из них Infinite-эффект. */
	UPROPERTY(EditAnywhere, Category = "Mark")
	TArray<FClanhallMarkModifier> Modifiers;

	/** Запасной путь - готовый GameplayEffect (Infinite) для того, что модификатором не выразить. */
	UPROPERTY(EditAnywhere, Category = "Mark")
	TSubclassOf<UGameplayEffect> Effect;

	/** Мгновенная визуальная реакция травмы или преимущества (подброс, заморозка) - косметика,
	 *  nullptr законен (`MarksSystem.md`, «Травмы и преимущества»). */
	UPROPERTY(EditAnywhere, Category = "Mark|Presentation")
	TObjectPtr<UAnimMontage> Reaction;

	UPROPERTY(EditAnywhere, Category = "Mark|Presentation")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, Category = "Mark|Presentation")
	FText DisplayName;
};
