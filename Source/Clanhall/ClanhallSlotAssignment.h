// Назначение клавиши приёму игроком (`Controls.md`, «Панель и клавиши приёмов»): класс оружия,
// слот, идентичность приёма. Отдельный заголовок - его читают и гуманоид (переопределения), и
// сохранение HUD (UClanhallHUDSaveGame).

#pragma once

#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPath.h"
#include "ClanhallSlotAssignment.generated.h"

USTRUCT(BlueprintType)
struct CLANHALL_API FClanhallSlotAssignment
{
	GENERATED_BODY()

	/** Класс оружия (UWeaponTypeData) - путь, а не указатель: запись переживает рестарт. */
	UPROPERTY(BlueprintReadOnly, Category = "Slots")
	FSoftObjectPath WeaponType;

	UPROPERTY(BlueprintReadOnly, Category = "Slots")
	FGameplayTag Slot;

	/** Идентичность приёма - UAbilityData::CounterTag. */
	UPROPERTY(BlueprintReadOnly, Category = "Slots")
	FGameplayTag Skill;
};
