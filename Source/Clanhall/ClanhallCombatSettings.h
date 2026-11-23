// Глобальные параметры боя - одно место вместо россыпи констант. Project Settings → Game →
// Clanhall Combat, значения пишутся в DefaultGame.ini (`config = Game, defaultconfig`).
//
// Время боя идёт долями такта (`MagicSystem.md`, «Доли»): все длительности в данных задаются
// в долях, а в секунды переводятся только через ClanhallBeats::ToSeconds. Длина доли -
// параметр (`Gameloop/Parameters.md`, «Перезарядка и такт»), а не число в коде.
//
// Сетки долей в P5 нет: доля - единица времени, ввод к ней не привязан. Метроном и сетка -
// вместе с ритмом каста.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ClanhallCombatSettings.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Clanhall Combat"))
class CLANHALL_API UClanhallCombatSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UClanhallCombatSettings();

	/** Длина одной доли такта в секундах. Плейсхолдер 0.5 с - 120 долей в минуту. */
	UPROPERTY(config, EditAnywhere, Category = "Beats", meta = (ClampMin = "0.05", Units = "s"))
	float BeatSeconds = 0.5f;

	static const UClanhallCombatSettings& Get() { return *GetDefault<UClanhallCombatSettings>(); }
};

namespace ClanhallBeats
{
	/** Единственная точка перевода долей в секунды. Отрицательное время не бывает - ноль. */
	inline float ToSeconds(float Beats)
	{
		return FMath::Max(Beats, 0.0f) * UClanhallCombatSettings::Get().BeatSeconds;
	}

	/** Обратный перевод - для отладки и HUD (остаток перезарядки в долях). */
	inline float FromSeconds(float Seconds)
	{
		const float Beat = UClanhallCombatSettings::Get().BeatSeconds;
		return Beat > 0.0f ? Seconds / Beat : 0.0f;
	}
}
