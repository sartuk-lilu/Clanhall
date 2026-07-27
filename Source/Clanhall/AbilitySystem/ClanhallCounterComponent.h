// Компонент на бойце (игрок и враг, симметрично), который держит окно контрнавыка.
// Пока висит окно, на владельце State.CounterWindow и запомнен набор навыков, которыми эту
// активку можно прервать (CounteredByTags), + её хендл. Совпадение (через HasTag, с учётом
// иерархии тегов) идентичности входящего навыка с этим набором = контр: активка сбивается
// и уходит на полный КД, окно закрывается.
//
// ability_system.md §2.

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "ClanhallCounterComponent.generated.h"

class UAbilitySystemComponent;

UCLASS(ClassGroup="Clanhall", meta=(BlueprintSpawnableComponent))
class CLANHALL_API UClanhallCounterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Открывает окно: запоминает набор навыков, которыми контримую активку можно прервать
	 *  (CounteredByTags), её хендл и КД (чтобы наложить на владельца при успешном контре),
	 *  вешает State.CounterWindow на ASC владельца. */
	void OpenWindow(const FGameplayTagContainer& InCounteredBy, FGameplayAbilitySpecHandle InCounteredHandle, FGameplayTag InCooldownTag, float InCooldownDuration);

	/** Закрывает окно без контра (истекло время / активка доиграла). */
	void CloseWindow();

	/** true, если сейчас открыто окно и IncomingTag входит в CounteredByTags (HasTag — матчит и родительские теги). */
	bool IsCounterableBy(FGameplayTag IncomingTag) const;

	/** Отменяет контримую активку (CancelAbilityHandle), навешивает ей полный КД, закрывает окно. */
	void ConsumeCounter();

	/** Общий резолвер для навыков: если у Target открыто окно с тем же CounterTag — сбивает его
	 *  активку и возвращает true (вызывающий навык не коммитится). Иначе false — штатный путь. */
	static bool TryResolveCounter(AActor* Target, FGameplayTag IncomingCounterTag);

private:
	bool bWindowOpen = false;
	FGameplayTagContainer CounteredByTags;
	FGameplayAbilitySpecHandle CounteredHandle;
	FGameplayTag CounteredCooldownTag;
	float CounteredCooldownDuration = 0.0f;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	UAbilitySystemComponent* GetASC();
};
