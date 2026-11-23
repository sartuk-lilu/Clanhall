// Метки бойца - три уровня (`MarksSystem.md`, «Уровни меток», «Слоты меток»). Один экземпляр на
// каждого бойца, симметрично: противник кладёт метки на игрока по той же лестнице.
//
// - Первый уровень - один слот дебаффа и один слот баффа. Новая метка заменяет лежащую в своём
//   слоте, та же самая обновляет таймер.
// - Второй уровень и третий (травмы, преимущества) - списки без потолка.
// - Копий нет: повтор той же метки обновляет таймер. Уровни лежат отдельно: подъём съедает метку
//   только читаемого уровня. Истёкшая метка пропадает целиком, ниже не опускается.
// - Каждая метка помнит школу и бойца, который её положил: третий уровень сравнивает школы.
//
// Время жизни - по уровню из настроек боя; держит его компонент (тик раз в 0.1 с), а не
// длительность GameplayEffect: обновление таймера - просто новое время, без переприменения.
// Эффект метки (UMarkData::Effect, Infinite) снимается вместе с меткой. Пока метка лежит, её тег
// висит на ASC loose-тегом.

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ActiveGameplayEffectHandle.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallMarkComponent.generated.h"

class UAbilitySystemComponent;
class UMarkData;

/** Лежащая на бойце метка - для HUD и отладки. */
USTRUCT(BlueprintType)
struct CLANHALL_API FClanhallMarkView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mark")
	TObjectPtr<UMarkData> Mark = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Mark")
	FGameplayTag Tag;

	UPROPERTY(BlueprintReadOnly, Category = "Mark")
	int32 Level = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Mark")
	EClanhallMarkKind Kind = EClanhallMarkKind::Debuff;

	/** Школа, которая положила метку (`School.*`); пусто - отладка. */
	UPROPERTY(BlueprintReadOnly, Category = "Mark")
	FGameplayTag SourceSchool;

	UPROPERTY(BlueprintReadOnly, Category = "Mark")
	float RemainingBeats = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClanhallMarksChanged);

UCLASS(ClassGroup = "Clanhall", meta = (BlueprintSpawnableComponent))
class CLANHALL_API UClanhallMarkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClanhallMarkComponent();

	/** Положить метку. Слот и время - по уровню и виду метки. Та же метка уже лежит - обновить
	 *  таймер и источник. Возвращает, легла ли. */
	bool PlaceMark(UMarkData* Mark, FGameplayTag SourceSchool, AActor* SourceActor);

	/** Съесть названную метку уровня, если она ещё лежит (метка могла истечь между нажатием и
	 *  попаданием). OutSchool - школа съеденной: третий уровень сравнивает её со своей. */
	bool ConsumeMark(int32 Level, FGameplayTag MarkTag, FGameplayTag& OutSchool);

	/** Лежит ли метка с этим тегом на этом уровне. */
	bool HasMark(int32 Level, FGameplayTag MarkTag) const;

	/** Снять одну, самую старую, травму или преимущество (`MarksSystem.md`, «Травмы и
	 *  преимущества»: снятие - мгновенный эффект отдельного навыка, по одной). */
	bool RemoveOldest(EClanhallMarkKind Kind);

	/** Все лежащие метки: слоты первого уровня, потом второй, потом третий. */
	UFUNCTION(BlueprintPure, Category = "Clanhall|Marks")
	TArray<FClanhallMarkView> GetMarks() const;

	UPROPERTY(BlueprintAssignable, Category = "Clanhall|Marks")
	FOnClanhallMarksChanged OnMarksChanged;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Ассет метки держит не эта структура (она не UPROPERTY), а его владельцы - UAbilityData
	 *  тестового набора; ассеты данных в бою не выгружаются. */
	struct FPlacedMark
	{
		TObjectPtr<UMarkData> Mark;
		FGameplayTag SourceSchool;
		TWeakObjectPtr<AActor> SourceActor;
		double PlacedAt = 0.0;
		double ExpiresAt = 0.0;
		FActiveGameplayEffectHandle EffectHandle;
	};

	/** Первый уровень: слот дебаффа [0] и слот баффа [1]. */
	TOptional<FPlacedMark> LevelOneSlots[2];

	/** Второй уровень - без потолка. */
	TArray<FPlacedMark> LevelTwo;

	/** Третий уровень - травмы и преимущества, без потолка. */
	TArray<FPlacedMark> LevelThree;

	UAbilitySystemComponent* GetOwnerASC() const;
	double Now() const;
	double LifetimeSeconds(int32 Level) const;

	void Apply(FPlacedMark& Placed);
	void Remove(FPlacedMark& Placed);

	FClanhallMarkView MakeView(const FPlacedMark& Placed) const;
};
