// Компонент на бойце (игрок и враг, симметрично), который держит окно контрнавыка.
// Пока висит окно, на владельце State.CounterWindow и запомнен набор навыков, которыми эту
// активку можно прервать (CounteredByTags), + её хендл. Совпадение (через HasTag, с учётом
// иерархии тегов) идентичности входящего навыка с этим набором = контр: активка сбивается,
// сбитому хитстоп, окно закрывается.
//
// (`AbilitySystem.md`, «Контрнавык»).

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "ClanhallCounterComponent.generated.h"

class UAbilitySystemComponent;

/** Владельца окна сбили контрнавыком. Точка подключения для получателя (флинч/VFX/звук) —
 *  сама реакция сюда не входит, строится отдельной системой (открытый вопрос). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClanhallCounterConsumed);

/** Оба выпустили приём кадр в кадр - редкий VFX-взрыв (`AbilitySystem.md`, «Одновременная
 *  активация»). Вещается на обоих. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClanhallPerfectClash, AActor*, Other);

UCLASS(ClassGroup="Clanhall", meta=(BlueprintSpawnableComponent))
class CLANHALL_API UClanhallCounterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Открывает окно: запоминает набор навыков, которыми контримую активку можно прервать
	 *  (CounteredByTags) и её хендл, вешает State.CounterWindow на ASC владельца. */
	void OpenWindow(const FGameplayTagContainer& InCounteredBy, FGameplayAbilitySpecHandle InCounteredHandle);

	/** Приём владельца стартовал (UGA_PhysicalSkill::ActivateAbility): запомнить его набор
	 *  CounteredBy, хендл и время - для одновременной активации, когда окно ещё не открылось. */
	void NotifySkillActivated(const FGameplayTagContainer& InCounteredBy, FGameplayAbilitySpecHandle InHandle);

	/** Приём владельца закончился - встык с ним уже не перехватить. */
	void NotifySkillEnded(FGameplayAbilitySpecHandle InHandle);

	/** Оба выпустили приём кадр в кадр. */
	UPROPERTY(BlueprintAssignable, Category = "Counter")
	FOnClanhallPerfectClash OnPerfectClash;

	/** Транслирует получателям, что владельца этого окна сбили контром (флинч/VFX/звук). */
	UPROPERTY(BlueprintAssignable, Category = "Counter")
	FOnClanhallCounterConsumed OnCounterConsumed;

	/** Закрывает окно без контра (истекло время / активка доиграла). */
	void CloseWindow();

	/** true, если сейчас открыто окно и IncomingTag входит в CounteredByTags (HasTag — матчит и родительские теги). */
	bool IsCounterableBy(FGameplayTag IncomingTag) const;

	/** Отменяет контримую активку (CancelAbilityHandle), вешает сбитому короткий стан, начисляет
	 *  хитстоп, закрывает окно (`EconomySystem.md`, «Наказание сбитого контром»). Сбитый приём уже
	 *  на перезарядке: она стартует на активации. */
	void ConsumeCounter();

	/** Общий резолвер для навыков. Контр - если у Target открыто окно с тем же CounterTag, или
	 *  встык: окно ещё не открылось, но контримый приём Target стартовал не раньше чем за
	 *  SimultaneousBeats до активации контрящего (SourceActivationTime). Резолвится ПО КОНТАКТУ и
	 *  только на подтверждённом попадании (GA_PhysicalSkill::ResolveHitOn). */
	static bool TryResolveCounter(AActor* Target, FGameplayTag IncomingCounterTag, AActor* Source, double SourceActivationTime);

private:
	bool bWindowOpen = false;
	FGameplayTagContainer CounteredByTags;
	FGameplayAbilitySpecHandle CounteredHandle;

	/** Идущий приём владельца - для одновременной активации. */
	FGameplayTagContainer ActiveSkillCounteredBy;
	FGameplayAbilitySpecHandle ActiveSkillHandle;
	double ActiveSkillStartTime = -1.0;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
	UAbilitySystemComponent* GetASC();
};
