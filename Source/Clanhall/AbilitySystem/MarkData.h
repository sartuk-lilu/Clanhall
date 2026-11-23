// Метка - один ассет (`MarksSystem.md`): тег-идентичность, уровень, вид и эффект на параметры.
// Уровень - горизонт времени, а не сила: время жизни берётся из настроек боя по уровню
// (MarkL1Beats / MarkL2Beats / MarkL3Beats), поля-переопределения на метке нет.
//
// Эффект - GameplayEffect с модификаторами атрибутов БЕЗ собственной длительности (Infinite):
// время жизни держит UClanhallMarkComponent, он же снимает эффект. Бонусы меток складываются, а
// не перемножаются: модификаторы множителей - операция MultiplyAdditive
// (`MarksSystem.md`, «Слоты меток»; ×1.15 и ×1.4 дают ×1.55). MultiplyCompound метками не
// используется.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ClanhallCombatTypes.h"
#include "MarkData.generated.h"

class UGameplayEffect;
class UTexture2D;
class UAnimMontage;

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

	/** Что метка меняет - модификаторы атрибутов (MultiplyAdditive для множителей), Infinite. */
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
