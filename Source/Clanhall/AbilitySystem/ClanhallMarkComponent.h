// Метка участника боя (mark_system.md §1-4). Канон: "Метки = GameplayTags на компоненте"
// (CLAUDE.md / technical_context.md). Один экземпляр на каждого боеспособного актора —
// у игрока и у каждого врага свой, независимый (mark_system.md §3, §5: "два независимых трека").

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ActiveGameplayEffectHandle.h"
#include "ClanhallMarkComponent.generated.h"

class UAbilitySystemComponent;

UCLASS()
class CLANHALL_API UClanhallMarkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClanhallMarkComponent();

	/** Снимает текущую метку (если есть) и накладывает новую на 5 сек.
	 *  Правило максимума (mark_system.md §2, Правило 3): на участнике всегда не больше одной метки. */
	void ApplyMark(FGameplayTag NewMark);

	/** Снимает текущую метку без замены — используется когда метка сгорает в синергии (Правило 2). */
	void ClearMark();

	/** Текущая метка, если её время ещё не истекло; невалидный тег иначе. */
	FGameplayTag GetCurrentMark() const;

	bool HasMark(FGameplayTag MarkTag) const { return MarkTag.IsValid() && GetCurrentMark() == MarkTag; }

private:
	UAbilitySystemComponent* GetOwnerASC() const;

	// Подсказка "какой именно тег проверять" — реальная истина всегда в теге на ASC,
	// см. GetCurrentMark(): по истечении 5 сек GE сам снимает тег, кэш мог не узнать об этом сразу.
	FGameplayTag CachedMarkTag;

	FActiveGameplayEffectHandle ActiveMarkEffectHandle;
};
