// Компонент на бойце (игрок и враг, симметрично), который держит окно контрнавыка.
// Пока висит окно, на владельце State.CounterWindow и запомнена идентичность контримой
// активки (CounterTag) + её хендл. Совпадение CounterTag входящего навыка с активным
// окном = контр: активка сбивается и уходит на полный КД, окно закрывается.
//
// clanhall_claude_code_counter.md, Раздел 6.

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
	/** Открывает окно: запоминает CounterTag контримой активки, её хендл и КД (чтобы наложить
	 *  на владельца при успешном контре), вешает State.CounterWindow на ASC владельца. */
	void OpenWindow(FGameplayTag InCounterTag, FGameplayAbilitySpecHandle InCounteredHandle, FGameplayTag InCooldownTag, float InCooldownDuration);

	/** Закрывает окно без контра (истекло время / активка доиграла). */
	void CloseWindow();

	/** true, если сейчас открыто окно и IncomingTag совпадает с CounterTag контримой активки. */
	bool IsCounterableBy(FGameplayTag IncomingTag) const;

	/** Отменяет контримую активку (CancelAbilityHandle), навешивает ей полный КД, закрывает окно. */
	void ConsumeCounter();

	/** Общий резолвер для навыков: если у Target открыто окно с тем же CounterTag — сбивает его
	 *  активку и возвращает true (вызывающий навык не коммитится). Иначе false — штатный путь. */
	static bool TryResolveCounter(AActor* Target, FGameplayTag IncomingCounterTag);

private:
	bool bWindowOpen = false;
	FGameplayTag ActiveCounterTag;
	FGameplayAbilitySpecHandle CounteredHandle;
	FGameplayTag CounteredCooldownTag;
	float CounteredCooldownDuration = 0.0f;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	UAbilitySystemComponent* GetASC();
};
