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

/** Владельца окна сбили контрнавыком. Точка подключения для получателя (флинч/VFX/звук) —
 *  сама реакция сюда не входит, строится отдельной системой (main_dev_plan.md §7,
 *  «Открытые вопросы»). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClanhallCounterConsumed);

UCLASS(ClassGroup="Clanhall", meta=(BlueprintSpawnableComponent))
class CLANHALL_API UClanhallCounterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Открывает окно: запоминает набор навыков, которыми контримую активку можно прервать
	 *  (CounteredByTags), её хендл, КД (чтобы наложить на владельца при успешном контре) и
	 *  длительность стана (свойство СБИТОГО навыка — см. AbilityData::CounterStunDuration),
	 *  вешает State.CounterWindow на ASC владельца. */
	void OpenWindow(const FGameplayTagContainer& InCounteredBy, FGameplayAbilitySpecHandle InCounteredHandle, FGameplayTag InCooldownTag, float InCooldownDuration, float InStunDuration);

	/** Транслирует получателям, что владельца этого окна сбили контром (флинч/VFX/звук). */
	UPROPERTY(BlueprintAssignable, Category = "Counter")
	FOnClanhallCounterConsumed OnCounterConsumed;

	/** Закрывает окно без контра (истекло время / активка доиграла). */
	void CloseWindow();

	/** true, если сейчас открыто окно и IncomingTag входит в CounteredByTags (HasTag — матчит и родительские теги). */
	bool IsCounterableBy(FGameplayTag IncomingTag) const;

	/** Отменяет контримую активку (CancelAbilityHandle), навешивает ей полный КД, закрывает окно. */
	void ConsumeCounter();

	/** Общий резолвер для навыков: если у Target открыто окно с тем же CounterTag — сбивает его
	 *  активку и возвращает true. Резолвится ПО КОНТАКТУ (GA_PhysicalSkill::ResolveHitOn), не на
	 *  активации — возвращаемое значение сейчас не влияет на коммит вызывающего навыка, тот
	 *  списывает Charges/КД безусловно (combat_system.md §3). Иначе false — штатный путь. */
	static bool TryResolveCounter(AActor* Target, FGameplayTag IncomingCounterTag);

private:
	bool bWindowOpen = false;
	FGameplayTagContainer CounteredByTags;
	FGameplayAbilitySpecHandle CounteredHandle;
	FGameplayTag CounteredCooldownTag;
	float CounteredCooldownDuration = 0.0f;
	float CounteredStunDuration = 0.0f;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	UAbilitySystemComponent* GetASC();
};
