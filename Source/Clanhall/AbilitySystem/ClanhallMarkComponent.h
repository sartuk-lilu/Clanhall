// Метка участника боя (`mark_system.md`). Канон: "Метки = GameplayTags на компоненте"
// (CLAUDE.md). Один экземпляр на каждого боеспособного актора —
// у игрока и у каждого врага свой, независимый (`mark_system.md`, «Получение метки от врага»: "два независимых трека").
//
// Два ТРЕКА (`mark_system.md`, «Концепция»): физический — от физических активок, магический —
// от заклинаний. Правило максимума (не больше одной метки) действует ВНУТРИ трека: метка
// физического трека не снимает и не видит метку магического, и наоборот. Один и тот же путь
// кода (шаблон FMarkTrackState) обслуживает оба трека, различаются только индексом.

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ActiveGameplayEffectHandle.h"
#include "ClanhallCombatTypes.h"
#include "ClanhallMarkComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS()
class CLANHALL_API UClanhallMarkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClanhallMarkComponent();

	/** Снимает текущую метку ТРЕКА (если есть) и накладывает новую на 5 сек.
	 *  InSourceASC — кто наложил метку (игрок или враг). Нужно для IsOwnMark().
	 *  Правило максимума (`mark_system.md`, «Перезапись»): на участнике всегда не больше одной
	 *  метки НА ТРЕК. Метка другого трека не трогается. */
	void ApplyMark(EClanhallMarkTrack Track, FGameplayTag NewMark, UAbilitySystemComponent* InSourceASC = nullptr);

	/** Снимает метку ТРЕКА без замены — используется когда метка сгорает в синергии (Правило 2). */
	void ClearMark(EClanhallMarkTrack Track);

	/** Текущая метка ТРЕКА, если её время ещё не истекло; невалидный тег иначе. */
	FGameplayTag GetCurrentMark(EClanhallMarkTrack Track) const;

	bool HasMark(EClanhallMarkTrack Track, FGameplayTag MarkTag) const { return MarkTag.IsValid() && GetCurrentMark(Track) == MarkTag; }

	/** Возвращает true если QueryASC является источником текущей метки ТРЕКА.
	 *  Потребителя сейчас НЕТ: перенос своей метки на цель убран вместе с концепцией
	 *  «горячей картошки» (`mark_system.md`, «Наложение» — метка летит только от
	 *  атакующего к цели и только при попадании). Метод сохранён под HUD («кто повесил
	 *  метку») и синергии врага. Вражеская метка атакой не снимается. */
	bool IsOwnMark(EClanhallMarkTrack Track, const UAbilitySystemComponent* QueryASC) const;

	/** Слот баффа владельца (`mark_system.md`, «Правила метки», «Слоты эффектов от активаций»):
	 *  хендл активного баффа от СОБСТВЕННЫХ активаций владельца. Снимает прежний хендл (если жив)
	 *  и применяет новый — один слот, новый эффект того же вида заменяет прежний. */
	void ApplySelfBuff(TSubclassOf<UGameplayEffect> EffectClass);

	/** Слот дебаффа на владельце, ПО ОДНОМУ НА КАЖДОГО СОЗДАТЕЛЯ (`mark_system.md`, «Правила
	 *  метки», «Слоты эффектов от активаций»): снимает прежний дебафф ЭТОГО ЖЕ SourceASC на
	 *  владельце (если жив) и применяет новый. Дебаффы других создателей не трогаются. */
	void ApplyDebuffFrom(UAbilitySystemComponent* SourceASC, TSubclassOf<UGameplayEffect> EffectClass);

private:
	/** Состояние одного трека — кэш тега, хендл эффекта, источник. Один и тот же путь кода для
	 *  обоих треков, только индекс разный. */
	struct FMarkTrackState
	{
		// Подсказка "какой именно тег проверять" — реальная истина всегда в теге на ASC,
		// см. GetCurrentMark(): по истечении 5 сек GE сам снимает тег, кэш мог не узнать об этом сразу.
		FGameplayTag CachedMarkTag;

		FActiveGameplayEffectHandle ActiveMarkEffectHandle;

		// Кто наложил текущую метку. Валиден пока метка жива.
		TWeakObjectPtr<UAbilitySystemComponent> CurrentMarkSourceASC;
	};

	UAbilitySystemComponent* GetOwnerASC() const;

	FMarkTrackState Tracks[2];

	/** Хендл активного баффа от собственных активаций владельца (см. ApplySelfBuff). Снятие по
	 *  устаревшему/протухшему хендлу безопасно — RemoveActiveGameplayEffect не требует отдельной
	 *  проверки на истечение. */
	FActiveGameplayEffectHandle SelfBuffHandle;

	/** Дебаффы на владельце, по одному хендлу на создателя (см. ApplyDebuffFrom). Протухшие ключи
	 *  (создатель уничтожен) чистятся при вставке новой записи. */
	TMap<TWeakObjectPtr<UAbilitySystemComponent>, FActiveGameplayEffectHandle> DebuffHandlesBySource;
};
