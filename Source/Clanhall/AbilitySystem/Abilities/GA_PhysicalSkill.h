// Раздел 4: один навык-класс для всех физических активных способностей (Q/E/R/F/...).
// Канон CLAUDE.md: "Логика абилки не меняется при правке данных — меняется только DataAsset".
// Конкретное содержание (урон/метка/синергия/баланс/КД/стоимость) приходит из UAbilityData,
// который привязывается к каждому гранту через FGameplayAbilitySpec::SourceObject —
// то есть один и тот же класс граннтится 4 раза (Shield Slam/Power Strike/Shield Charge/
// Retribution), и каждый раз с другим SourceObject.
//
// Два режима резолва, выбираются на активации по Data->CastMontage (см.
// UAnimNotifyState_Hitbox::MontageHasHitbox):
//   - Контактный (монтаж есть и на нём расставлен AnimNotifyState_Hitbox): способность ждёт
//     Event.Hitbox.Hit/Event.Hitbox.Closed от UClanhallHitboxComponent и резолвит цель(и) в
//     момент реального касания зоны. Одна зона может задеть несколько целей за применение
//     (Shield Charge — капсула на пелвисе на весь рывок, п.25) — см. правило мультицели ниже.
//   - Мгновенный фолбэк (CastMontage == nullptr ИЛИ на нём не расставлена зона): резолв по
//     FindMeleeTarget на активации, как до перехода на контактный резолв. Все четыре ассета
//     Knight сейчас идут этим путём — разметка зон на монтажах ещё не сделана (редакторная
//     работа, не код).
//
// Инвариант «один резолвер на актора»: Event.Hitbox.Hit и Event.Hitbox.Closed летят на весь
// ASC владельца без адресации. Система корректна ровно потому, что в любой момент времени
// урон по контакту резолвит не более одной способности на актора. Это обеспечивается двумя
// правилами: (1) чейн WASD — ActivateStep закрывает зоны предыдущего шага ДО активации
// следующей способности; (2) активка — CancelSequenceForExternalMontage() гасит
// серию ДО того, как активка подпишется на события (см. ActivateAbility.cpp). Payload.EventMagnitude
// несёт хендл зоны и остаётся неиспользованным заделом: если инвариант когда-нибудь придётся
// ослабить (две зоны от разных владельцев одновременно), фильтр по хендлу — готовая точка
// расширения. Сейчас фильтр не добавлять — он создал бы вид защиты там, где защищает порядок
// вызовов.

#pragma once

#include "GA_ClanhallAbilityBase.h"
#include "GA_PhysicalSkill.generated.h"

class UAbilityData;
class UClanhallMarkComponent;
struct FMarkSynergy;

UCLASS()
class CLANHALL_API UGA_PhysicalSkill : public UGA_ClanhallAbilityBase
{
	GENERATED_BODY()

public:
	UGA_PhysicalSkill();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION()
	void OnHitboxHitReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnHitboxClosedReceived(FGameplayEventData Payload);

private:
	/** SourceObject ищется через Handle, а не GetCurrentSourceObject() — у InstancedPerExecution
	 *  абилки на момент CanActivateAbility ещё нет персистентного инстанса (см. движок,
	 *  AbilitySystemComponent_Abilities.cpp: CanActivateAbility может вызываться на CDO). */
	const UAbilityData* GetAbilityData(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;

	void ResolveMarkLogic(const UAbilityData* Data, UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, UClanhallMarkComponent* TargetMarkComponent);

	/** Общая часть контактного и фолбэк-путей: урон + метки/синергия + Balance + debug.
	 *  Вызывается ОДИН раз на каждую задетую цель. */
	void ResolveHitOn(AActor* Target);

	/** Баф на себя из синергии (EffectOnSelf) уже применён за это применение навыка.
	 *  ChargeGain этим флагом НЕ гатится: заряды — ресурс и начисляются за каждую
	 *  задетую цель (mark_system.md §2, правило мультицели). */
	bool bSelfSynergySpent = false;

	/** Сдвиг Balance уже применён за это применение навыка. */
	bool bBalanceApplied = false;

	// Оба флага выше — обычные поля без сброса: InstancingPolicy == InstancedPerExecution,
	// на каждое применение создаётся свежий инстанс. Не превращать в статики и не сбрасывать
	// вручную — именно политика инстансирования делает их корректными.
};
