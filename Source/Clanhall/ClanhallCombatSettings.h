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

	/** Цена смены оружия, когда в руках было оружие (`WeaponSystem.md`, «Свап оружия»): одна доля. */
	UPROPERTY(config, EditAnywhere, Category = "Weapons", meta = (ClampMin = "0.0"))
	float WeaponSwapBeats = 1.0f;

	/** Сколько долей без приёма оружие остаётся в руках, прежде чем уйти на тело. Плейсхолдер. */
	UPROPERTY(config, EditAnywhere, Category = "Weapons", meta = (ClampMin = "0.0"))
	float AutoHolsterBeats = 6.0f;

	/** Окно двойного нажатия клавиши оружия (как «11» в песочнице - парные мечи), секунды:
	 *  это свойство ввода, а не боевого времени. */
	UPROPERTY(config, EditAnywhere, Category = "Weapons", meta = (ClampMin = "0.05", Units = "s"))
	float WeaponDoubleTapSeconds = 0.3f;

	/** Сектор спины для удара в спину, градусы (`Gameloop/Parameters.md`: 120°). */
	UPROPERTY(config, EditAnywhere, Category = "Damage", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float BackSectorDegrees = 120.0f;

	/** Шанс смертельного удара у приёмов, которые им бывают (1%). */
	UPROPERTY(config, EditAnywhere, Category = "Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LethalChance = 0.01f;

	/** Доля MaxHP, которую теряют босс и игрок вместо смерти от смертельного удара (35%). */
	UPROPERTY(config, EditAnywhere, Category = "Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LethalHPFraction = 0.35f;

	/** Потолок шанса крита, одинаково у игрока и противника (20%). */
	UPROPERTY(config, EditAnywhere, Category = "Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CritChanceCap = 0.2f;

	/** Множитель крита. Не назначен в Gameloop - плейсхолдер. */
	UPROPERTY(config, EditAnywhere, Category = "Damage", meta = (ClampMin = "1.0"))
	float CritMultiplier = 1.5f;

	/** Длина насмешки в долях - всё это время боец открыт (`CombatSystem.md`, «Насмешка»). */
	UPROPERTY(config, EditAnywhere, Category = "Taunt", meta = (ClampMin = "0.25"))
	float TauntBeats = 2.0f;

	/** Своя перезарядка насмешки в долях. Плейсхолдер. */
	UPROPERTY(config, EditAnywhere, Category = "Taunt", meta = (ClampMin = "0.0"))
	float TauntCooldownBeats = 10.0f;

	/** Во сколько раз насмехающийся должен быть сильнее (Power), чтобы насмешка стала
	 *  парированием. Открытый пункт дизайна «чем мерить» - пока поле Power, как в симуляторе. */
	UPROPERTY(config, EditAnywhere, Category = "Taunt", meta = (ClampMin = "1.0"))
	float TauntParryRatio = 3.0f;

	/** Длина анимации насмешки-парирования в долях - на это время оба без управления. */
	UPROPERTY(config, EditAnywhere, Category = "Taunt", meta = (ClampMin = "0.0"))
	float TauntParryBeats = 2.0f;

	/** Короткий стан с отменой приёма от контрнавыка, в долях, одинаковый для всех
	 *  (`CombatSystem.md`, «Контроль»). Верх - «потолок обычного контроля на игроке» 0.5 с
	 *  (`Gameloop/Parameters.md`). */
	UPROPERTY(config, EditAnywhere, Category = "Counter", meta = (ClampMin = "0.0"))
	float CounterStunBeats = 1.0f;

	/** Одновременная активация (`AbilitySystem.md`, «Контрнавык»): насколько раньше контрящего мог
	 *  стартовать контримый приём, чтобы встык считался перехватом, в долях. */
	UPROPERTY(config, EditAnywhere, Category = "Counter", meta = (ClampMin = "0.0"))
	float SimultaneousBeats = 0.25f;

	/** Окно контра не должно быть узким (`AbilitySystem.md`): уже этого - варн разметки. */
	UPROPERTY(config, EditAnywhere, Category = "Counter", meta = (ClampMin = "0.0"))
	float MinCounterWindowBeats = 1.0f;

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
