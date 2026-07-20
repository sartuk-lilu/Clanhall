# Clanhall

# План Разработки Прототипа — Технический документ

## Стек и архитектурные решения

**Engine:** Unreal Engine 5  
**Плагин:** Gameplay Ability System (GAS)  
**Паттерн данных:** DataAsset + Fragments (композиция через данные)  
**Язык:** C++ для ядра, Blueprint для быстрого прототипирования навыков

---
## Design-docs 

![design-doc](https://github.com/user-attachments/assets/fae4721f-fc54-47a4-b6cb-4d64d4d72d8e)

---
## Статус

| #   | Система                                    | Статус      |
| --- | ------------------------------------------ | ----------- |
| 1   | Фундамент GAS и атрибуты                   | ✅ Готово    |
| 2   | Боевая стойка и WASD-удары                 | ✅ Готово    |
| 3   | Система меток                              | ✅ Готово    |
| 4   | DataAsset, Fragments, первые навыки Knight | ✅ Готово    |
| 5   | Парирование (placeholder)                  | ✅ Готово    |
| 6   | Контр-навык                                | ✅ Готово    |
| 6.5 | Animation Setup / Комбо-система WASD       | In Progress |
| 7   | Рядовой противник                          |             |
| 8   | Стартовый босс                             |             |
| 9   | Магическая система                         |             |
| 10  | Смена оружия                               |             |
| 11  | Колесо умений                              |             |

---
## Архитектура DataAsset + Fragments

Центральный паттерн проекта. Каждый навык, заклинание или оружие — это `UPrimaryDataAsset` с заголовком и массивом фрагментов.

### Почему Fragments, а не один большой класс

Без фрагментов: один класс на 50 полей, большинство — null в зависимости от типа навыка.  
С фрагментами: заголовок содержит только то, что есть у каждой абилки. Фрагмент — только то, что нужно конкретной.

### Структура DataAsset (заголовок)

```cpp
UCLASS()
class UAbilityData : public UPrimaryDataAsset
{
    // Заголовок — есть у каждой абилки без исключения
    FText DisplayName;
    UTexture2D* Icon;
    float Cooldown;
    FGameplayTag RequiredClass;   // Ability.Class.Knight и т.д.
    int32 ChargeCost;             // 0 / 2 / 4 / 6

    // Фрагменты — только то что нужно конкретной абилке
    UPROPERTY(EditAnywhere, Instanced)
    TArray<TObjectPtr<UAbilityFragment Fragments;

    template<typename T
    T* FindFragment() const;  // запрос фрагмента по типу → nullptr если нет
};
```

### Ключевые спецификаторы базового класса фрагмента

```cpp
UCLASS(Abstract, DefaultToInstanced, EditInlineNew)
class UAbilityFragment : public UObject {};
// DefaultToInstanced — каждый экземпляр в массиве уникален
// EditInlineNew    — редактор разворачивает содержимое прямо внутри DataAsset
```
### Фрагменты проекта

| Фрагмент | Поля | Когда добавлять |
|---|---|---|
| `UAnimationFragment` | CastMontage, ImpactMontage | У всех видимых навыков |
| `UVFXFragment` | CastEffect, ImpactEffect | У всех с визуалом |
| `USFXFragment` | CastSound, ImpactSound | У всех со звуком |
| `UDamageFragment` | BaseDamage, DamageEffect (GE) | У наносящих урон |
| `UMarkApplyFragment` | MarkTag, MarkEffect (GE 5 сек) | У накладывающих метку |
| `UMarkTriggerFragment` | TArray\<FMarkSynergy\ | У потребляющих метки |
| `UBalanceFragment` | Shift (float) | У всех физических навыков |
### FMarkSynergy — структура внутри UMarkTriggerFragment

```cpp
USTRUCT()
struct FMarkSynergy
{
    FGameplayTag RequiredMark;          // метка-условие на цели
    TSubclassOf<UGameplayEffect EffectOnTarget;  // дебафф на врага
    TSubclassOf<UGameplayEffect EffectOnSelf;    // бафф на себя
    // Только одно из двух заполнено — никогда оба
};
```
### Как GameplayAbility читает фрагменты

Абилка данных не содержит — только запрашивает через `FindFragment<T()`:

```cpp
void UGA_ShieldSlam::ActivateAbility(...)
{
    if (auto* Anim = AbilityData-FindFragment<UAnimationFragment())
        PlayMontage(Anim-CastMontage);

    // При confirmed hit:
    if (auto* Dmg = AbilityData-FindFragment<UDamageFragment())
        ApplyGameplayEffectToTarget(Target, Dmg-DamageEffect);

    if (auto* Mark = AbilityData-FindFragment<UMarkApplyFragment())
        ApplyGameplayEffectToTarget(Target, Mark-MarkEffect);

    if (auto* Trigger = AbilityData-FindFragment<UMarkTriggerFragment())
        CheckAndActivateSynergy(Target, Trigger-Synergies);

    if (auto* Balance = AbilityData-FindFragment<UBalanceFragment())
        ShiftBalance(Balance-Shift);
}
```

Логика абилки не меняется при изменении данных — только DataAsset.
### Порядок добавления фрагментов (не всё сразу)

1. Заголовок + `UDamageFragment` — навык наносит урон
2. `UMarkApplyFragment` + `UMarkTriggerFragment` — система меток работает
3. `UAnimationFragment`, `UVFXFragment`, `USFXFragment` — визуал и звук
4. `UBalanceFragment` — шкала STR/DEX подключена

**Правило:** сначала механика работает, потом она красиво выглядит.

---
## Таксономия GameplayTags (закладывается в Разделе 1)

Теги закладываются **один раз и полностью** — переименование тегов в середине разработки ломает весь GAS-граф.

```
Ability.Skill.Knight.*
Ability.Skill.Warrior.*
Ability.Skill.Assassin.*
Ability.Skill.Lancer.*


Ability.Class.Knight
Ability.Class.Warrior
Ability.Class.Assassin
Ability.Class.Lancer

Mark.*

State.Casting
State.CastingAntimagic
State.Parrying
State.CounterWindow
State.InStance
State.Stunned
State.Knockdown
State.ComboRecovery

Unit.Role.Mob
Unit.Role.Boss.Humanoid
Unit.Role.Boss.Monster

Weapon.Type.STR
Weapon.Type.DEX

Balance.Overload.STR
Balance.Overload.DEX

Magic.School.Elemental.Rank.*
Magic.School.Aether
Magic.School.Materia
Magic.School.Stars

Cooldown.Slot.Q
Cooldown.Slot.E
Cooldown.Slot.R
Cooldown.Slot.F
Cooldown.Slot.Z
Cooldown.Slot.X
Cooldown.Slot.C
Cooldown.Slot.V

```

---
### Симуляция боевой системы

![prototype_fight](https://github.com/user-attachments/assets/efa74915-a4a9-48dc-af5b-1831a61b71c3)

---
### Раздел 1 — Фундамент GAS и атрибуты
**Статус:** ✅ Готово.

**Итог:** GAS подключён, `AbilitySystemComponent` живёт на Character и Enemy, все пять ресурсов существуют как атрибуты и клампятся в каноничных границах, полная таксономия тегов заложена одним файлом.

**Реализация (по факту):**
- `UClanhallAttributeSet` (`AbilitySystem/ClanhallAttributeSet.h/.cpp`) — девять `FGameplayAttributeData`: AP/MaxAP, HP/MaxHP, MP/MaxMP, Charges/MaxCharges и Balance. У Balance намеренно нет MaxBalance — диапазон жёстко зашит −100..+100 прямо в клампе. Комбинированный макрос `ATTRIBUTE_ACCESSORS` (getter + value getter/setter/initter) объявлен в проекте сам, т.к. движок даёт только четыре отдельных «кирпича» (паттерн Lyra / GASDocumentation).
- **Кламп в двух точках, а не в одной.** `PreAttributeChange` ловит мгновенные правки из кода (напр. `SetHP`). Но модификаторы `GameplayEffect` идут через Aggregator и могут на шаг обогнать пересчёт лимита, поэтому `PostGameplayEffectExecute` повторно клампит итоговое значение. Ресурсы → `[0, Max]`, Balance → `[−100, 100]`.
- **Репликация заложена сразу** (задел под мультиплеер, хотя прототип однопользовательский): все девять атрибутов через `DOREPLIFETIME_CONDITION_NOTIFY(COND_None, REPNOTIFY_Always)` + свой `OnRep_*` на каждом (`GAMEPLAYATTRIBUTE_REPNOTIFY`).
- **Таксономия GameplayTags** — `AbilitySystem/ClanhallGameplayTags.h`, нативные теги через `UE_DECLARE_GAMEPLAY_TAG_EXTERN` (не DataTable). Заложены полностью и сразу: `Ability.Class.*`, `Ability.Skill.*` (листья Knight — под Разделы 4/6), `State.*`, `Weapon.Type.*`, `Balance.Overload.*`, `Parry.Incoming.*`, `Cooldown.Slot.*`, `SetByCaller.Magnitude`, `Event.*`, `Damage.Type.*`, `Perk`, `Magic.School.*` (только корни — ранги отложены до Раздела 9), `Unit.Role.*`, `Mark.*` (полные 33 метки + `Compressed`).
- Часть тегов объявлена «на вырост»: листья `Ability.Skill.Knight.*` нужны детектору контрнавыка в Разделе 6, `State.CounterActive` — тоже задел под Раздел 6 (см. примечание там).

**Правило (соблюдается):** дописывать теги можно, переименовывать — нет, это ломает весь GAS-граф.

**Проверка:** атрибуты создаются на обоих ASC, значения держатся в границах при любом источнике изменения, HUD-минимум читает AP/HP/MP.

---
### Раздел 2 — Боевая стойка и WASD-удары
**Статус:** ✅ Готово.

**Итог:** LMB-hold переводит в боевую стойку, в стойке W/A/S/D — четыре направленных удара с AP-обменом, регеном MP и сдвигом Balance; вне стойки те же клавиши просто двигают персонажа.

**Реализация (по факту):**
- **Стойка** — `UGA_CombatStance` (`Abilities/GA_CombatStance.h/.cpp`), `InstancedPerActor`. Пока активна, вешает `State.InStance` (`ActivationOwnedTags`); повторный вход заблокирован тем же тегом в `ActivationBlockedTags`. Привязка LMB-hold/release — на стороне персонажа/контроллера.
- **Гейт на стойку для всех физдействий.** Общий предок `UGA_ClanhallAbilityBase` ставит `ActivationRequiredTags = State.InStance`. Поэтому вне стойки WASD-абилки просто не активируются (`TryActivateAbility` отказывает), и клавиши работают как движение — без отдельной ветки кода.
- **WASD-удары** — база `UGA_DirectionalAttackBase` (`InstancedPerExecution`) + четыре тонких наследника (`GA_DirectionalAttacks.h`), каждый переопределяет только `GetDirection()` → `Overhead(W)/RightSlash(D)/LeftSlash(A)/LowSweep(S)`. Enum `EClanhallAttackDirection` вынесён в `ClanhallCombatTypes.h`, чтобы не плодить взаимозависимости заголовков (его же читают Parry- и WeaponTrace-компоненты).
- **Поиск цели (placeholder до анимации Раздела 6.5)** — `FindMeleeTarget` в базовом классе: `SphereOverlapActors` сферой перед персонажем (`TraceRange 200`, `TraceRadius 75`), берётся первый актор с `IAbilitySystemInterface`.
- **AP-обмен** — `ResolveStandardDamage` (в базовом классе): у цели снимается `min(урон, AP цели)`, 50 % снятого возвращается атакующему в свой AP, переполнение (урон больше остатка AP) идёт прямо в HP. Возврат `true` = «confirmed hit» — на нём завязаны КД и синергии. Всё через generic-эффекты `GE_Modify*` с SetByCaller-магнитудой.
- **MP и Balance на удар** (`GA_DirectionalAttackBase::ActivateAbility`): STR-оружие (`Weapon.Type.STR` на ASC) → MP +10, Balance +5..+15; DEX → MP +5, Balance −5..−15 (сдвиг рандомный в диапазоне). Само число урона теперь приходит из комбо-компонента через `TriggerEventData->EventMagnitude` (редизайн Раздела 6.5) — но STR/DEX-логика MP/Balance осталась здесь.
- **Пассивный дрейф Balance к нулю** — `GE_BalanceDrift` (Infinite, `Period 1.0` сек, без execute-on-application) + `ExecCalc_BalanceDrift`: шаг 2 ед/тик к центру, с клампом `Sign*Drift` в пределах `±|текущее|`, чтобы не проскочить ноль на другую сторону.
- Экранный дебаг AP/MP/Balance на каждый удар (non-shipping).

**Проверка:** в стойке удар по болванчику — AP уходит туда-обратно (−цель/+50 % себе), MP растёт, Balance ползёт от направления оружия и сам возвращается к нулю в простое.

---
### Раздел 3 — Система меток
**Статус:** ✅ Готово.

**Итог:** у каждого бойца свой независимый трек метки — таймер-тег на компоненте, максимум одна метка, с различением «своя/чужая» по источнику.

**Реализация (по факту):**
- **`UClanhallMarkComponent`** (`AbilitySystem/ClanhallMarkComponent.h/.cpp`) — по одному экземпляру на игрока и на каждого врага (два независимых трека, `mark_system.md §3, §5`).
- **Метка = таймер-тег на ASC** через `ClanhallGameplayEffects::ApplyTimedTag`, длительность 5 сек (`MarkDurationSeconds`). Максимум одна: `ApplyMark` сначала зовёт `ClearMark` (`RemoveActiveGameplayEffect` старого хендла), стека нет.
- **Источник метки** — `CurrentMarkSourceASC` (`TWeakObjectPtr`). `IsOwnMark(QueryASC)` различает свою метку игрока (осталась от промаха — переносится на врага следующим попаданием) и чужую (наложена активкой босса — атакой НЕ снимается). Это и есть правило «двух треков».
- **Кэш ≠ истина.** `CachedMarkTag` — только подсказка «какой тег проверять»; истина всегда в теге на ASC. `GetCurrentMark()` каждый раз перепроверяет `HasMatchingGameplayTag`, потому что по истечении 5 сек GE снимает тег сам, а кэш мог не узнать об этом мгновенно.
- **Проверка синергии, активация и генерация заряда** живут в `GA_PhysicalSkill` (Раздел 4), метод `ResolveMarkLogic`: метка на цели сгорает → эффект срабатывает (бафф на себя ИЛИ дебафф на цель, никогда оба) → навык кладёт свою новую метку.
- HUD: цветной индикатор типа метки + таймер 5 сек над врагом.

> ⚠️ **Расхождение план ↔ код:** в плане было «+2 Charge при успешной синергии», но реализовано **+1** (`ApplyModifyEffect(... UGE_ModifyCharges, 1.0f)` в `ResolveMarkLogic`). Нужно решить, какое значение каноничное, и свести план с кодом.

**Проверка:** вручную повесить метку, вторым навыком активировать синергию — эффект срабатывает, заряд добавляется, новая метка ложится.

---
### Раздел 4 — DataAsset, Fragments, первые навыки Knight
**Статус:** ✅ Готово.

**Итог:** один класс-навык `UGA_PhysicalSkill` обслуживает все физические активки; всё их содержание (урон/метка/синергия/баланс/КД/стоимость) — данные в `UAbilityData`, меняются без перекомпиляции.

**Реализация (по факту):**
- **`UAbilityData`** (`AbilitySystem/AbilityData.h`) : `UPrimaryDataAsset`. Заголовок: `DisplayName`, `Icon`, `Cooldown` (по умолч. 10), `CooldownTag` (`Cooldown.Slot.*`), `RequiredClass` (`Ability.Class.*`), `CounterTag` (`Ability.Skill.*` — идентичность для контрнавыка), `ChargeCost` (0), массив `Fragments` (Instanced). Шаблон `FindFragment<T>()` возвращает первый фрагмент типа T или nullptr.
- **Фрагменты.** База `UAbilityFragment` (`Abstract, DefaultToInstanced, EditInlineNew` — массив полиморфных подобъектов, редактируемых прямо внутри ассета). Механические (`GameplayFragments.h`): `UDamageFragment`(BaseDamage), `UMarkApplyFragment`(MarkTag), `UMarkTriggerFragment`(TArray\<FMarkSynergy\>), `UBalanceFragment`(Shift). Презентационные (Animation/VFX/SFX) вынесены в `PresentationFragments.h`.
- **Один класс на все активки.** `UGA_PhysicalSkill` (`InstancedPerExecution`) гранится 4 раза (Shield Slam / Power Strike / Shield Charge / Retribution), каждый раз со своим `SourceObject` (=`UAbilityData`) через `FGameplayAbilitySpec::SourceObject`. `GetAbilityData` достаёт `SourceObject` через `Handle`, а не `GetCurrentSourceObject()` — у `InstancedPerExecution` на момент `CanActivateAbility` персистентного инстанса ещё нет (метод может вызваться на CDO).
- **Стоимость и КД разведены.** `CanActivateAbility` проверяет тег КД (он есть только после подтверждённого попадания) и наличие Charges. Charges списываются на активацию; тег КД вешается только на confirmed hit (`ApplyTimedTag` на `Data->Cooldown`). Это канон «КД только при confirmed hit».
- **Порядок в `ActivateAbility`:** резолвер контрнавыка (до любой стоимости) → списание Charges → поиск цели → урон через `UDamageFragment`/`ResolveStandardDamage` (утил-навык без урона считается попавшим по факту найденной цели) → перенос своей метки с игрока на врага (если висела от промаха) → `ResolveMarkLogic` (синергия + новая метка) → сдвиг Balance → тег КД → опциональный косметический монтаж. Ветка промаха: метка навыка остаётся на игроке 5 сек как «своя», переносимая следующим попаданием.
- **Knight Ранг 1–2:** Q Shield Slam, E Power Strike, R Shield Charge, F Retribution. КД по тиру Q/E=10, R/F=20 (прототип). Charges: Q/E=0, R/F=2. Все числа — в DataAsset'ах.

**Проверка:** нажатие Q → Shield Slam, метка ложится, синергия из Раздела 3 срабатывает; правка цифр в DataAsset меняет поведение без перекомпиляции.

---
### Раздел 5 — Парирование (placeholder)
**Статус:** ✅ Готово.

**Канонический дизайн** (`combat_system.md §5`): Clash Detection — weapon-trace игрока пересекается с weapon-trace врага в активном окне анимации, без UI-индикаторов. **Полная trace-версия — в Разделе 6.5.** Ниже — то, что реализовано в этом разделе как placeholder до готовых монтажей.

**Реализация (по факту):**
- **`UClanhallParryComponent`** (`AbilitySystem/ClanhallParryComponent.h/.cpp`) на Character: флаг `bParrySuccessful`, `ResetParry()`, `TryParry(HitEnemy, PlayerDirection, HitLocation)`. Маппинг направления удара игрока → парируемый тег `Parry.Incoming.*` по правилу «обратное направление»: W парирует входящий S, S→W, D→A, A→D. Проверяет, что нужный `Parry.Incoming.*` висит на ASC игрока, ставит флаг, играет clash-звук.
  - *Placeholder-точка вызова:* в Разделе 5 `TryParry` дёргался из обработчиков ввода WASD; в Разделе 6.5 переносится на коллбэк weapon-trace (`UClanhallWeaponTraceComponent::CheckAndHandleParry`).
- **`UGA_EnemyWASDSeries`** (`Abilities/GA_EnemyWASDSeries.h/.cpp`) — AI-серия ударов, `InstancedPerExecution`, снимает требование стойки, заблокирована `State.Stunned` и `State.ComboRecovery`. Шагает по `AttackDirections` через `AbilityTask_WaitDelay`. На каждый удар (`PrepareHit`): сброс парирования, `State.Parrying` на СЕБЯ (враг — паррируемый актор) на `WindowDuration`, `Parry.Incoming.*` на игрока на то же окно, ожидание. По истечении окна (`OnWindowExpired`): парировано → счётчик; иначе → `ResolveStandardDamage` по игроку (AI получает свои 50 % AP). Между ударами — пауза `DelayBetweenHits`.
  - *Интерим-нюанс:* `State.Parrying` тут вешается кодом — это временная замена `AnimNotify_ParryWindowStart/End`. Когда монтажи врага будут готовы, эту строку убирают (блок F.15 Раздела 6.5), иначе окно откроется дважды.
- **Правило «всё или ничего»** — `FinalizeSeries`: полное парирование (`ParriedCount == число ударов`) → AI оглушён (`State.Stunned` на `StunDuration`) + КД игрока сокращаются на `CDReduction`. Пропустил хоть один — эффекта нет. После серии — `State.ComboRecovery` против мгновенного перезапуска (длительность: при полном парировании `Stun+0.5`, иначе 1.0 сек).
- **Сокращение КД игрока** — `ReducePlayerCooldowns`: запрос активных эффектов по родительскому тегу `Cooldown` (`MakeQuery_MatchAnyOwningTags` — в UE 5.3+ смотрит и asset-, и granted-теги; наш `GE_ApplyTimedTag` кладёт тег в `DynamicGrantedTags`), считает `остаток − CDReduction`, снимает все КД-эффекты и перевешивает ещё не истёкшие с укороченной длительностью.
- **Конкретная серия** — `UGA_Series_Crosscut`:  «Перекрёстный» Часового A→D (ответ игрока D→A). Гранится Training Dummy по таймеру 4 сек.

**Проверка:** болванчик делает A→D, игрок жмёт D→A — парирует и оглушает; пропустил один — эффекта нет.

---
### Раздел 6 — Контрнавык
**Статус:** ✅ Готово.

**Итог:** активка врага, начатая в открытом окне, прерывается тем же навыком игрока (совпадение `CounterTag`) — активка врага уходит на полный КД, а навык игрока при этом не тратит ни стоимости, ни КД.

**Реализация (по факту):**
- **`UClanhallCounterComponent`** (`AbilitySystem/ClanhallCounterComponent.h/.cpp`) — на обоих бойцах, симметрично. `OpenWindow(CounterTag, CounteredHandle, CooldownTag, CooldownDuration)` запоминает идентичность контримой активки, её хендл и КД, вешает `State.CounterWindow` (loose-тег) на владельца. `IsCounterableBy(Tag)` = окно открыто И тег совпал с `ActiveCounterTag`. `ConsumeCounter` → `CancelAbilityHandle` активки врага + полный КД на неё + закрытие окна. Статический `TryResolveCounter(Target, Tag)` — общий резолвер, который дёргают навыки игрока.
- **Сторона игрока** (`GA_PhysicalSkill::ActivateAbility`): ДО списания Charges/КД зовётся `TryResolveCounter(Target, Data->CounterTag)`. Успех → активка врага сбита + полный КД, а навык игрока не коммитится вовсе (ранний `EndAbility` — без стоимости, без КД, без урона). Это и есть «Charges не расходуются, КД не уходит, навык готов сразу».
- **Сторона врага** (`GA_EnemyActiveSkill.h/.cpp`): `InstancedPerExecution`, без требования стойки, заблокирован `Stunned`/`ComboRecovery`. `ActivateAbility` открывает окно контрнавыка на своём CounterComponent и запускает `WaitDelay(CounterWindowDuration)` до удара. Если снаружи `ConsumeCounter` вызвал `CancelAbilityHandle` — задача `WaitDelay` снимается, `OnHitDelayExpired` не вызовется, урона нет. Если окно не прервали — по истечении: враг сам закрывает своё окно, `ResolveStandardDamage` по игроку, наложение `HitMarkTag` (если задан).
  - *Интерим-нюанс:* окно врага открывается кодом — временная замена `AnimNotifyState_CounterWindow` (реальных монтажей пока нет). Убирается в блоке F.16 Раздела 6.5, иначе окно задвоится.
- **Конкретная активка** — `UGA_Enemy_PowerStrike`: и `AbilityTags`, и `CounterTag` = `Ability.Skill.Knight.PowerStrike` (намеренно тот же тег, что у Knight E — иначе контр не сматчится). `Cooldown.Slot.E`, 10 сек, `CounterWindowDuration 1.2`, `HitDamage 40`. (`HitMarkTag = Mark.BrokenGuard` отложен до Раздела 7 вместе с полным Часовым.)

> ⚠️ **Расхождение план/заголовок ↔ код:** тег `State.CounterActive` объявлен с комментарием «вешается на игрока на 0.1 сек и читается `GA_PhysicalSkill` → пропускает Charges/КД», но по факту навык игрока обходит стоимость через ранний `EndAbility` в резолвере контра, а `State.CounterActive` в `GA_PhysicalSkill` **не читается**. Тег сейчас объявлен, но не используется — либо перевести пропуск стоимости на него, либо убрать тег/комментарий.

**Проверка:** враг начинает Power Strike → игрок LMB+E → навык врага прерван, у игрока он остаётся готовым.


---
### Раздел 6.5 — Animation Setup / Комбо-система WASD
**Статус:** In Progress — C++ готов, идут редакторные ассеты (Этапы ниже).

**Итог:** WASD-удары стали направленным деревом комбо: `UClanhallComboComponent` сам валидирует ввод по
данным и активирует шаг, `GA_DirectionalAttackBase` больше не решает ничего, кроме урона/MP/Balance.
Анимационный слой навешивается поверх готовой механики.

**Канон:** механика комбо и нарезка узловыми позами — `combo_system.md`; направления, парирование,
clash detection — `combat_system.md §4/§5`; AnimGraph, слои и слоты — `Advanced/locomotion structure.md`.

**Ключевой инвариант:** **ни один Notify не решает урон/КД/Charges** — всё посчитано в момент нажатия, до
анимации. Механика работает без единого монтажа, поэтому монтажи можно тестировать по отдельности, не
боясь сломать бой, а `Montage` хода может быть nullptr (законный способ проверить дерево до нарезки).

**Реализация (по факту):**
- **`UComboData`** (`Fragments/ComboData.h`) — один Data Asset на класс, три поля: `DamageProfile` (урон на
  НАПРАВЛЕНИЕ, не на комбо-запись), `Moves` (`MoveId` → `Direction` + `Montage`), `Chains` (цепочки из
  `MoveId`, свой `RecoveryMontage` на каждую, `RequiredUnlock` под перки).
- **Инвариант резолва:** направление ВАЛИДИРУЕТ путь, `MoveId` ВЫБИРАЕТ клип. Не сводить обратно к «клип по
  направлению» — иначе теряется возможность держать разные удары одного направления в разных ветках дерева.
- **Ворота ввода, не буфер.** До окна чтения нажатия отбрасываются и не копятся; в окне «последнее решает».
  Нет накопления → нет залипания отзывчивости.
- **Потолок длины серии = `AClanhallCharacter::ClassRank`** (0–4, плейсхолдер до прокачки) — на персонаже,
  не в ассете: длина зависит от бойца, а не от оружия. Дерево может содержать записи длиннее ранга — они
  просто недоступны.
- **Урон — из профиля, не с абилки.** Компонент резолвит `BaseDamage` по направлению шага ДО активации и
  передаёт через `TriggerAbilityFromGameplayEvent` (`EventMagnitude` = урон, `InstigatorTags` несут
  `DamageType` — задел, в расчёте не читается). Формулы MP/Balance в GA не тронуты.
- **Recovery-анимация ≠ тег `State.ComboRecovery`.** Анимация берётся из завершившейся цепочки
  (`RecoveryMontage`, nullptr = хвост запечён в удар) и играет всегда после терминального удара. Тег —
  геймплейный лок-аут ввода, только после удара на потолке ранга. Единая точка завершения —
  `EndSequenceWithRecovery`.
- **Страховки состояния.** Серия сбрасывается и по концу окна, и по концу удар-монтажа (делегат) — даже
  если нотифая на монтаже нет. `ResetCombo` гасит `bReadWindowOpen`. Weapon trace принудительно
  закрывается из компонента при конце/прерывании монтажа и при выходе из стойки.
- **Гейт на данные:** нет цепочки-опенера на направление → соответствующий WASD не бьёт вообще.
- **Класс-нейтральность данных** (задел под ИИ босса): ключ хода `EClanhallAttackDirection` нейтрален к
  источнику ввода, player-специфики в `UComboData` нет. Враг сейчас идёт через `GA_EnemyWASDSeries`.

**Служебные теги раздела:** `State.ComboRecovery` (лок-аут после потолка ранга), `Event.DirectionalAttack`
(несёт `BaseDamage` от компонента к абилке — не notify, выбор способности не гейтит).

**Разметка нотифай-стейтов (общее для всех окон):** Montage Tick Type = **Branching Point** — обычные
нотифаи обрабатываются на границе тика и на низком FPS съедают кадры, что заметно на окнах в 0.2–0.5 сек.
Конец стейта не ставить впритык к концу монтажа — 1–2 кадра запаса, иначе `NotifyEnd` попадёт в блендаут.

**Рабочее правило:** числа в доках — плейсхолдеры, менять свободно. Структура и архитектура — нет. Если
для монтажа нужно то, чего нет в коде (новый Notify, поле, сигнал) — не собирать в BP втихую, а вернуться
с конкретным вопросом. Анимация и код переключаются с интерима на финал одновременно (Блок F).

**Проверка:** в стойке W→A играет цепочку с разными клипами; невалидное продолжение гасит серию без урона
и сдвига шкал; удар на потолке `ClassRank` вешает лок-аут; правка `UComboData` меняет дерево без
перекомпиляции.

#### Этапы

**Блок A — Фундамент**
**Статус:** ✅ Готово.

1. `[Редактор]` Сокет `WeaponSocket` на скелете оружия игрока (и врага, если у него отдельная модель оружия) — его ждёт `WeaponTraceComponent`.
2. `[Редактор]` ABP: Locomotion state machine + слои UpperBody / FullBody / Cast.
3. `[Редактор]` Выставление pelvis кости в root X/Y из motion capture 

**Блок B — Монтажи игрока и базовые нотифаи**
**Статус:** ✅ Готово.

4. `[Редактор]` 4 базовых монтажа WASD-ударов (W=Overhead, D=RightSlash, A=LeftSlash, S=LowSweep) — это опенеры: по ходу (`FComboMove`) в `UComboData.Moves` на каждое направление + по цепочке из одного шага (`FComboChain.Steps = [MoveId]`) в `UComboData.Chains`.  

#### Mocap
https://github.com/user-attachments/assets/1a1860be-d227-4b33-a482-516d13a3bf6f
#### Sequencer
https://github.com/user-attachments/assets/30800fd3-17b4-4992-8a20-a2b77ff6b3f6


##### Статус Комбо Анимаций Knight

| col/row | Stance |  W  |  A  |  S  |  D  |
| :------ | :----: | :-: | :-: | :-: | :-: |
| Stance  |   ❌    |  ✅  |  ✅  |  ✅  |  ✅  |
| W       |   ✅    |  ❌  |  ✅  |  ✅  | 🔜  |
| A       |   ✅    |  ✅  |  ❌  |  ✅  |  ✅  |
| S       |   ✅    |  ✅  |  ✅  |  ❌  | 🔜  |
| D       |   ✅    |  ✅  |  ✅  |  ✅  |  ❌  |

5. `[Редактор]` На каждом из 4: `AnimNotifyState_WeaponTrace` вокруг фазы контакта. Montage Tick Type = **Branching Point**.  
6. `[Редактор]` На каждом из 4: `AnimNotifyState_ComboWindow`
7. `[Редактор]` Монтажи продолжений комбо (2-й удар и далее по направлениям), с теми же двумя наборами нотифаев из п.4–5.

**Блок C — DataAsset комбо-оружия**
**Статус:** 

8. `[Редактор]` Создать Data Asset типа `DataTable UComboMove`, заполнить все 16 вариатов анимаций в `Row_Name, Direction, Montage`.
9. `[Редактор]` Создать Data Asset типа `UComboData`, заполнить три поля: `DamageProfile` (`BaseDamage` на каждое из 4 направлений), `Moves` (`MoveId`+`Direction`+`Montage` на каждый ход), `Chains` (включая опенеры на все 4 направления, без них WASD не бьёт). Назначить ассет в поле `ComboData` на BP-персонаже, там же выставить `ClassRank` (потолок длины серии — больше не поле ассета).

|     | A           | D               | S                 | W                 |
| --- | ----------- | --------------- | ----------------- | ----------------- |
| A   | ❌           | ADA/`ADS`/`ADW` | ASA/ASD/ASW       | AWA/AWD/AWS       |
| D   | DAD/DAS/DAW | ❌               | `DSA`/`DSD`/`DSW` | `DWA`/`DWD`/`DWS` |
| S   | SAD/SAS/SAW | SDA/`SDS`/`SDW` | ❌                 | SWA/SWD/SWS       |
| W   | WAD/WAS/WAW | WDA/`WDS`/`WDW` | WSA/WSD/WSW       | ❌                 |

**Блок D — Активки Q/E/R/F (Knight)**  
**Статус:** 

10. `[Редактор]` `CastMontage` в `UAnimationFragment` каждого из Q/E/R/F.  
11. `[Редактор]` `AnimNotify_ApplyMark` на кадре удара (задел на будущее — эффекта пока не даёт, ставить безопасно).  
12. `[Редактор + дизайн]` Root Motion: **Shield Charge (R)** — рывок 3–4 шага. Для **Retribution (F)** сперва уточнить у геймдизайнера, нужно ли ему перемещение, прежде чем включать.  
13. `[Редактор]` Проставить `CounterTag` в 4 Knight `UAbilityData` (`Ability.Skill.Knight.ShieldSlam / PowerStrike / ShieldCharge / Retribution`) — сейчас пусто во всех четырёх.

**Блок E — Монтажи врага и его нотифаи**  

14. `[Редактор]` Placeholder-анимации Часового (Training Dummy) и Стража.  
15. `[Редактор]` На ударах серий врага: `AnimNotifyState_WeaponTrace` (для клэша парирования через trace игрока).  
16. `[Редактор]` На ударах серий врага: `AnimNotifyState_ParryWindow` (Begin ~20%, End ~80%). Montage Tick Type = **Branching Point** — окно короткое, терять кадры нельзя.  

**Блок F — Переключение интеримов код↔нотифай
17. `[C++]` Когда монтажи врага с Parry-нотифаями из п.14 готовы → убрать интерим-строку `ApplyTimedTag(SelfASC, State_Parrying, WindowDuration)` в `GA_EnemyWASDSeries::PrepareHit`. Иначе окно `State.Parrying` откроется дважды.  
18. `[C++]` Когда появится реальный монтаж Power Strike с `AnimNotifyState_CounterWindow` → убрать интерим `OpenWindow/CloseWindow` из `GA_EnemyActiveSkill.cpp`. Иначе окно контрнавыка задвоится.

**Блок G — Слоты и состояние при прерывании**  
19. `[Редактор]` Выставить слоты в монтажах: `DefaultSlot` → **`upperbody`** на всех WASD-ударах, продолжениях и Recovery-хвостах; **`fullbody`** на R/F. Канон — `Advanced/locomotion structure.md §3`. Типовая ошибка: монтаж остался в `DefaultSlot` → логика работает, анимации не видно.  
20. `[C++]` **Отмена серии чужим монтажом.** `upperbody` и `fullbody` в одной группе → активка Q/E/R/F прерывает удар-монтаж, `OnAttackMontageEnded` на `bInterrupted` делает ранний return, `ActiveDirections` залипает, WASD мёртв до выхода из стойки. Воспроизведение: LMB → W → сразу Q/R до открытия комбо-окна.
    - Новый публичный метод `UClanhallComboComponent::CancelSequenceForExternalMontage()`: ранний `return` при пустом `ActiveDirections` (нейтраль — прерывать нечего), иначе `ForceEndWeaponTrace()` + `ResetCombo()`. Монтаж НЕ останавливать — `Montage_Play` вызывающего сам его перебьёт, прилетит `OnAttackMontageEnded(bInterrupted=true)` → ранний return → состояние уже чистое.
    - Без Recovery-анимации и без `State.ComboRecovery`: чужой монтаж уже занимает слот, Recovery дрался бы с ним; наказания за прерывание нет — тот же принцип, что у невалидного продолжения.
    - Вызов в `GA_PhysicalSkill::ActivateAbility` внутри блока `if (AnimFrag->CastMontage)`, прямо перед `Montage_Play`. Не в начале `ActivateAbility`: способность может выйти раньше (резолвер контрнавыка, промах, пустой `CastMontage`) — тогда чужой монтаж не стартует и гасить серию не за что.
    - `LastPlayedMontage` после отмены НЕ обнулять: он указывает на мёртвый монтаж, и `Montage_Stop` по нему из `OnStanceExit` — безвредный no-op (каст доиграет). `Montage_Stop` с `nullptr` остановил бы всё подряд и убил каст.
    - Проверка: после каста WASD снова бьёт опенером; активка из нейтрали ничего не ломает; контрнавык посреди комбо серию НЕ гасит (чужого монтажа не было).
21. `[C++]` **Правило на будущее — проверять при каждом новом монтаже.** Любой новый источник `Montage_Play` в `DefaultGroup` (реакции на удар, стан, касты) обязан сначала гасить серию тем же вызовом из п.18.
22. `[Редактор + дизайн]` Решить по слоту `Cast`. Собрано два слота (`upperbody`, `fullbody`), Q/E сидят на `upperbody`. Если отдельный приёмник под заклинания нужен — создать в Anim Slot Manager и добавить ноду `Slot 'Cast'` в AnimGraph.
23. `[Редактор]` Боевая стойка отдельным состоянием в Main States, переключение по тегу `State.InStance`. Сейчас низ тела под верхними атаками отдаёт idle/локомоцию — для плейсхолдеров нормально, для финала нет. Структура AnimGraph не меняется, меняется наполнение Main States.
24. `[C++]` **B2 — урон по окну контакта.** Перенести фиксацию урона с активации (frame 0) на окно `AnimNotifyState_WeaponTrace`. Закрывает «ударил мгновенно → тут же увернулся». Требует, чтобы резолвнутый `BaseDamage` дожил от выбора шага до момента контакта.
25. `[C++]` **Shield Charge multi-hit.** По дизайну R задевает всех на пути рывка, но `GA_PhysicalSkill` резолвит одну цель через `FindMeleeTarget`. Нужен сбор целей по траектории рывка вместо одиночного поиска.


---
### Раздел 7 — Рядовой противник (Часовой)

**Характеристики:** AP 150 / HP 300 / MP 0 / Charges 2 / DT 8

- Behavior Tree или простой State Machine
- Две WASD-серии: A→D («Перекрёстный»), W→W («Сверху дважды»)
- «Сверху дважды» появляется при HP < 60%
- Активный навык: **Power Strike** (идентичный тег Ability.Skill.Warrior/Knight → контрнавык работает)
- Накладывает на игрока BROKEN GUARD при попадании
- При HP < 50%: Power Strike после каждой второй серии, агрессия если BROKEN GUARD на игроке
- Пауза 1.5 сек после промаха Power Strike

---
### Раздел 8 — Стартовый босс (Старый Страж)

**Характеристики:** AP 300 / HP 700 / MP 80 / Charges 4 / DT 12

WASD-серии:
- «Вертикаль» W→S (Фаза 1, 2, 3)
- «Тройной удар» D→D→A (Фаза 1, 2, 3)
- «Широкий крест» A→D→W (Фаза 2, 3 — появляется при HP < 70%)

Активные навыки:
- **Overhead Slam** (2 Charges) — тег Ability.Skill.Warrior.OverheadSlam → накладывает STAGGERED на игрока. Контрнавык: Warrior R
- **War Shout** (0 Charges) — утилита, накладывает BROKEN GUARD на игрока. Контрнавыком не блокируется

Заклинание (хардкод для этого раздела, без полной системы магии):
- **Fire Spark** (W→S) — телеграф: иконки слогов + аудио 1.0 сек, конус 3м, накладывает BURNING на игрока
- Антимагия: недоступна (игрок не знает слогов)

Опасные цепочки меток на игроке:
- War Shout (BROKEN GUARD) → Overhead Slam → +50% урон + игнор 50% DT
- Overhead Slam (STAGGERED) → War Shout → Knockdown 1.5 сек
- Fire Spark (BURNING) → War Shout → игрок в панике −30% урон 3 сек

Фазы:
- Фаза 1 (100–70%): серии + Overhead Slam раз в 2 серии + Fire Spark каждые 20 сек
- Фаза 2 (70–40%): добавляется «Широкий крест» + War Shout перед сериями + Fire Spark каждые 12 сек
- Фаза 3 (<40%): темп +20%, Fire Spark каждые 8 сек, Overhead Slam без паузы

После победы: кат-сцена → гримуар открывается → слоги Wîn (W) и Sîl (S) записываются → разблокировано Ранг 1 Стихий.

---
### Прототип магической системы

![spells_prototype](https://github.com/user-attachments/assets/82a408f8-28a7-425d-aa9c-2fc725c807c9)

---
### Раздел 9 — Магическая система

- RMB зажат = режим каста, движение заблокировано
- 8 клавиш как syllable input (Q/A/W/S/E/D/R/F в режиме каста)
- Lookup-таблица: комбо слогов → `GameplayAbility`
- Ранг 1 Стихий (разблокированный Стражем): Fire Spark (W→S), Frost Stomp (S→W)
- Синергии с метками через те же Fragments (MarkTriggerFragment на спеллах)
- Grimoire UI минимум: страница с комбо, описанием, иконкой
- Разблокировка ранга: триггер после победы над боссом → запись слогов в книгу
- Антимагия Режим А: RMB+LMB, те же слоги, прерывает каст босса, мана не тратится, игрок получает +25 MP; у магии игрока КД нет (только MP)
- Антимагия Режим Б: те же слоги на уже активный зональный эффект, тратит MP
- Ранг школы игрока гейтит антимагию

---
### Раздел 10 — Смена оружия и финальная интеграция

- Weapon slot система: клавиши 1–6
- Swap AbilitySet при смене: RemoveAbilities старого набора → GrantAbilities с нового DataAsset
- Эффекты перегруза Balance: при выходе за ±60 → GameplayEffect: навыки перегруженной стороны ×2 Charges
- Добавить второе оружие (Warrior / Assassin) для теста свопа
- Сквозной тест всего цикла: два оружия, два противника, метки в обе стороны, магия

---
### Раздел 11 — Колесо классов и перков

![progression_wheel](https://github.com/user-attachments/assets/e188554d-f339-4724-95f7-a9533642664e)

---
## Бэклог пост-прототипа

Следующие механики **не входят в Разделы 1–10**, реализуются после боевого прототипа:

1. **Боссы контрят спам** (см. ability_system.md §4) — после Раздела 10. Гейт: совпадение сабкласса + порог 3 повтора подряд.
2. **AP-рефилл** (замена боевого хила). Прототип: клавиша V восстанавливает 50% макс. AP,  ограниченные заряды. Полная экономика (возврат зарядов за босса/волны, зеркальный босс-рефилл   по бездействию, отображение на плашках) — дизайн в combat_system §1, доводится после боевого   прототипа. HP лечится только в городе. N зарядов и объём возврата — TBD.Базовую версию (V = +50% AP, счётчик зарядов) можно ставить в прототип рано — простой GameplayAbility + расход атрибута-счётчика. Экономику возврата зарядов оставить на потом.
3. **Магические печати-пины** — замки на дверях/сундуках, открываемые набором слогов (известная последовательность = ключ). Слоги как ключи к исследованию мира.
4. **Аннигиляция кастов** — редкое событие: оба участника выпустили одинаковые слоги в пересекающемся окне → заклинания аннигилируют, взрыв в центре, отброс обоих.
5. **Debug-читы** (можно раньше, полезно для тестов): консольные команды `Clanhall.GrantCharges N`, `Clanhall.ApplyMark Mark.X`, `Clanhall.SetBalance N`, `Clanhall.SetAP N`.
6. **Editor-валидатор DataAssets:** у FMarkSynergy заполнено ровно одно из EffectOnTarget/EffectOnSelf; каждая потребляемая метка кем-то накладывается; теги существуют.
7. **Асимметрия возврата AP** (игрок 50% / босс 30–40%) — проверить на плейтесте.

---


# План Разработки HUD — Рабочий трекер

 Живой документ. Отражает **что сделано, как именно, и что дальше**.
 Стек: Unreal Engine 5.8, GAS. ASC живёт на `AClanhallCharacter`, атрибуты — в `UClanhallAttributeSet` на этом ASC.
 Собираем UMG поверх готового C++

---
## Контекст

- **Движок:** UE 5.8, плагин GAS.
- **Персонаж:** `AClanhallCharacter` (C++ класс, Blueprint-наследник в Content). ASC висит **прямо на персонаже** (не на PlayerState).
- **Атрибуты:** `UClanhallAttributeSet` на ASC. Ноды `Get Gameplay Attribute Value (ASC, Attribute, bFound)`.
- **TargetingComponent:** на Character, `BlueprintReadOnly`. Получать: `GetOwningPlayerPawn → Cast ClanhallCharacter → TargetingComponent`. Это **мягкая цель под удар/метку** — EnemyFrame его больше не слушает
- **BossSensorComponent:** на Character, `BlueprintReadOnly`. Получать: `GetOwningPlayerPawn → Cast ClanhallCharacter → BossSensorComponent`. Это **драйвер Enemy Frame**: держит `Unit.Role.Boss.*` юнитов в радиусе (`EnterRadius`/`ExitRadius` гистерезис), вещает `OnFrameUnitEntered(AActor*)` / `OnFrameUnitExited(AActor*)`. N боссов = N рамок.
- **OnTargetChanged:** объявлен `BlueprintAssignable` на TargetingComponent, для мягкой цели — не для EnemyFrame.
- **SaveHUDLayout / LoadHUDLayout:** API подтверждён, вызывать `SaveHUDLayout` в `On Mouse Button Up` после перетаскивания. `LoadHUDLayout` возвращает bool (false = первый запуск → дефолты).
- **Дефолтные позиции рамок:** Player Frame `(50, 700)`, Enemy Frame `(1870, 700)`.
- **State.InStance:** тег существует в `ClanhallGameplayTags` (для crosshair).
- **Balance:** диапазон −100..+100, центр 0, данные из `GetBalance()`. Рисуем двумя встречными барами.
- **EnemyFrame — важная правка:** `GetAbilitySystemComponent` **не висит напрямую на `AActor`** в Blueprint. Правильная нода: `Ability System Blueprint Library → Get Ability System Component (Actor)`, передать `NewTarget`. TargetingComponent уже фильтрует цели по `IAbilitySystemInterface` перед записью в `CurrentTarget`, поэтому ASC вернётся валидный — но null-check оставить (враг мог умереть между тиками).

---

## Общие технические решения

### Поллинг vs событийная модель
- **WBP_AttributeBar** — на **property binding** (поллинг каждый кадр через `Get Gameplay Attribute Value`). Для прототипа достаточно. Апгрейд на событийную модель (`WaitAttributeChanged` → `SetPercent`/`SetText`) — потом, перед релизом.
- **WBP_ChargesPanel** — на **событийной модели** сразу (`WaitAttributeChanged`), т.к. дискретные иконки удобнее обновлять по событию, а не поллить.

### Гочи GAS в Blueprint (проверено на практике)
1. **`Create Widget` работает только с `UserWidget`.** Голый `Image` / `Spacer` так не создать → оборачивать в маленький UserWidget (`WBP_Diamond`).
2. **`WaitAttributeChanged`** — правильное имя ноды (`UAbilityAsync_WaitAttributeChanged`). Ноды `WaitAttributeChange` (без «d») в 5.8 **не существует** — не искать.
3. **Пин `Target Actor` у `WaitAttributeChanged` хочет `Actor`, не ASC.** ASC — это `ActorComponent`, у него есть `Get Owner`. Раз ASC на персонаже → `TargetASC → Get Owner` даёт нужного актора. **НЕ** использовать `Get Avatar Actor From Actor Info` — это метод `UGameplayAbility`, пины не совместимы с ASC.
4. **`Get Ability System Component (Actor)`** для чужого актора (враг) — через `Ability System Blueprint Library`, а не прямой нодой на `AActor`.
5. Из `WaitAttributeChanged` дёргать `Refresh` из пина **`On Attribute Changed`**; само значение атрибута игнорировать — `Refresh` перечитает `Cur`/`Max` сам (одна точка правды).

---
## Статус виджетов

| #   | Виджет                                      | Статус      |
| --- | ------------------------------------------- | ----------- |
| 1   | `WBP_AttributeBar`                          | ✅ Готово    |
| 2   | `WBP_Diamond`                               | ✅ Готово    |
| 3   | `WBP_ChargesPanel`                          | ✅ Готово    |
| 4   | `WBP_Balance`                               | ✅ Готово    |
| 5   | `WBP_PlayerFrame`                           | ✅ Готово    |
| 6   | `WBP_EnemyFrame` + `WBP_BossFrameContainer` | ✅ Готово    |
| 7   | `WBP_Crosshair`                             | ✅ Готово    |
| 8   | `WBP_HUD`                                   | ✅ Готово    |
| 9   | Drag + Save                                 | ✅ Готово    |
| 10  | Alt-режим                                   | ✅ Готово    |
| 11  | Тултипы                                     | 🟡 Отложено |

---
## 1. WBP_AttributeBar 
**Статус:** ✅ Готово.

Универсальный (один класс на HP / AP / MP). ASC приходит снаружи от родителя.
**Переменные (все Instance Editable где нужно):**
- `TargetASC` — `AbilitySystemComponent` (Object Ref), Instance Editable
- `TrackedAttribute` — `Gameplay Attribute`
- `MaxAttribute` — `Gameplay Attribute`
- `FillColor` — `Linear Color`

**Иерархия:** `Overlay` → `ProgressBar` "Bar" (Fill/Fill) + `TextBlock` "ValueText" (center). Оба — `Is Variable`. Дефолтный Canvas удалён.

**Percent биндинг (`GetBarPercent`):** `IsValid(TargetASC)?` → False: return `0.0`. True: `Cur = GetGameplayAttributeValue(TrackedAttribute)`, `Max = GetGameplayAttributeValue(MaxAttribute)`, `Max  0?` → False: `0.0`, True: `Cur/Max` → `Clamp 0..1` → return. (Clamp страхует от перелива AP max.)

**Text биндинг (`GetBarText`):** `Cur`/`Max` → `Round` → `Format Text "{0}/{1}"`.

**Event Construct:** `Bar → Set Fill Color and Opacity (FillColor)`.

**Апгрейд потом:** заменить property binding на `WaitAttributeChanged → SetPercent/SetText`.

---
## 2. WBP_Diamond
**Статус:** ✅ Готово.

Один ромб. Три визуальных состояния (нет / пуст / залит).

**Иерархия:**
- Корень `Size Box` — **обязательно задать `Width Override` / `Height Override`** (напр. 24×24). Без override схлопывается в 0 → не видно.
- Внутрь `Overlay`.
- Два `Image` в Overlay (порядок: нижний в дереве = виден сверху на экране):
  - `Img_Outline` — контур ◇, снизу, виден **всегда** (= «ромб есть, но пуст»).
  - `Img_Fill` — заливка ◆, сверху, `Is Variable`.
- Обоим: Alignment слота Fill/Fill. Если текстуры квадратные — `Render Transform → Rotation = 45` на каждом.

**`SetFilled(bFilled)`:** `Set Visibility (Img_Fill)` → `bFilled ? Visible : Hidden`. Контур не трогаем.

**Event Construct:** `SetFilled(false)` — дефолт пустой.

---
## 3. WBP_ChargesPanel
**Статус:** ✅ Готово.

**Что решено (финальный дизайн):**
- Ромбы = экземпляры `WBP_Diamond`, генерятся программно.
- Текстура/цвет/заливка — целиком внутри `WBP_Diamond` (панель про них не знает). Переменных `DiamondTexture`/`FilledColor`/`EmptyColor` на панели **нет**.
- Засечки `2 | 4 | 6` — **НЕ паддингом**. Статичная текстура-линейка под рядом ромбов: `Overlay` → снизу картинка-линейка, сверху `Row`. Ноль кода. (Позиции засечек фиксированы, т.к. ромбы фиксированного размера, лишние просто `Collapsed`.) Программный путь (считать засечки) — опционально позже, если размеры ромбов станут настраиваемыми в рантайме.

**Переменные:**
- `TargetASC` — `AbilitySystemComponent` (Object Ref), Instance Editable
- `Diamonds` — массив `WBP_Diamond` (Object Ref)

**Иерархия:** дефолтный Canvas удалён. `Overlay` → (низ) `Image` линейка-засечки → (верх) `Horizontal Box` "Row" (`Is Variable`).

**`BuildDiamonds`:** `Clear Children(Row)` → `Clear(Diamonds)` → **For Loop 0..11** → в теле: `Create Widget (WBP_Diamond, Owning Player = Get Owning Player)` → `Add Child to Horizontal Box (Row)` → `Add (Diamonds, ромб)`. Ни Set Brush, ни SetFilled тут нет.

**`Refresh`:** `Cur = GetGameplayAttributeValue(Charges) → Round`, `Max = GetGameplayAttributeValue(MaxCharges) → Round`. `For Each (Diamonds)` с индексом: `Index < Max?` → `Set Visibility` (Visible / Collapsed); `Index < Cur?` → `SetFilled` (true / false).

**`InitWithASC(NewASC)` — Custom Event, НЕ Function.** В Blueprint Function нельзя async/latent-ноды, а подписка `WaitAttributeChanged` именно такая — поэтому только Custom Event в Event Graph. Снаружи вызывается по ссылке как обычная функция. Цепочка:
`Set TargetASC(NewASC)` → `BuildDiamonds` → **`WaitDelay` (один кадр) → `On Finish` → `Refresh`** → подписки `WaitAttributeChanged` на `Charges` и `MaxCharges` (каждая `On Attribute Changed → Refresh`).

⚠ **`WaitDelay` после `BuildDiamonds` обязателен.** Свежесозданные `WBP_Diamond` (через `Create Widget`) не проходят построение/prepass в том же кадре — немедленный `Refresh` отрабатывает по ещё не готовым ромбам, и значения `Charges` не отображаются. Один кадр задержки (`WaitDelay` с ~0 длительностью, срабатывает `On Finish` на следующий тик) откладывает `Refresh` до готовности ромбов.

**Event Construct:** пустой. Вся инициализация (build + refresh + подписки) переехала в Custom Event `InitWithASC`, который вызывает родитель. Временный прокид ASC в Construct (использовался при отладке) — удалён, иначе панель инициализируется на невалидном ASC раньше времени.

---
### 5. WBP_Balance (−100..+100, центр 0)
**Статус:** ✅ Готово.

Обычный ProgressBar не годится — нужен центр-якорь. `Horizontal Box` → два `ProgressBar` по 50% ширины:
- левый: `Bar Fill Type = Right To Left`, `Percent = (Balance < 0) ? -Balance/100 : 0`, синий (DEX)
- правый: `Bar Fill Type = Left To Right`, `Percent = (Balance  0) ? Balance/100 : 0`, красный (STR)
Стык = ноль. Сверху вертикальная риска в центре + отметки на ±60 (граница перегруза, `combat_system.md`).

**Расположение:** шкала вынесена из `PlayerFrame` в `WBP_HUD` с якорем низ-центр экрана (глобальное состояние боя логично держать по центру, как индикатор стойки, а не в углу с ресурсами).

**Отклонённый вариант (на будущее):** «бегунок по статичной шкале» — статичный фон-градиент DEX→STR + маленький `Image`-указатель, ездящий по X от `Balance`. Рабочий, но для боя хуже: крупную заливку видно периферийным зрением, за тонкой рисочкой в замесе надо охотиться глазами. Оставлен как косметический апгрейд, механики не касается.

---
### 5. WBP_PlayerFrame
**Статус:** ✅ Готово.

ASC: `Get Owning Player Pawn → Cast ClanhallCharacter → Get Ability System Component`. `Event Construct`: `IsValid(PlayerASC)?` → True → `Set Target ASC` на HP/AP/MP барах (они на поллинге, сеттера достаточно) → `InitWithASC(PlayerASC)` на `ChargesPanel` (Custom Event). Цвета/`TrackedAttribute`/`MaxAttribute` баров задаются в Details каждого инстанса, НЕ из родителя.
- AP → `AP`/`MaxAP`, жёлто-золотой
- HP → `HP`/`MaxHP`, оранжево-красный
- MP → `MP`/`MaxMP`, сине-фиолетовый
**Компоновка:** HP / AP / MP — три отдельные полосы в `Vertical Box`. Нахлёст AP на верхние 20% HP убран — рядом с MP-баром смотрелся неравномерно; для прототипа стек чище и читаемее. Под барами — `ChargesPanel`. **Balance здесь НЕТ** — переехал в `WBP_HUD` (низ-центр экрана). Рамка всегда видима (никакой логики visibility, в отличие от `EnemyFrame`).

---
### 6. WBP_EnemyFrame + WBP_BossFrameContainer
**Статус:** ✅ Готово.

Рамку порождает/удаляет `WBP_BossFrameContainer`, по одной на каждого босса в радиусе (драйвер — `UClanhallBossSensorComponent`, двойной радиус EnterRadius/ExitRadius; НЕ рейкаст).
N боссов = N рамок.

WBP_EnemyFrame — «тупая», без сенсора и без логики видимости:
- Event Construct пустой.
- `SetupForUnit(NewASC: AbilitySystemComponent)` — `Custom Event: Set Target ASC` на HP/AP/MP → `` InitWithASC(NewASC)` на ChargesPanel →` HideNotUsedBars(NewASC)`.
- `HideNotUsedBars(NewASC)` — функция, Sequence 1–3: `GetGameplayAttributeValue(MaxAP/MaxMP/MaxCharges)<=0 ? Collapsed : Visible` на AP-бар / MP-бар / ChargesPanel. HP всегда виден.
  Так одна рамка показывает `Boss.Humanoid` (HP+AP+MP+Charges), `Boss.Monster` (HP±Charges), Часового (MP скрыт).
- `WBP_ChargesPanel.Refresh`: в начале гард `IsValid(TargetASC)` (при churn рамок ASC часто невалиден).
  Подписки `WaitAttributeChanged` гасятся при удалении рамки (иначе утечка).

WBP_BossFrameContainer — Vertical Box (FramesBox) + Map<Actor, WBP_EnemyFrame (Frames):
- Event Construct: BossSensorComponent → Bind OnFrameUnitEntered/OnFrameUnitExited.
- OnFrameUnitEntered(Unit): Get ASC (Ability System BP Library, Actor=Unit) → null-check →
  Create Widget(WBP_EnemyFrame) → SetupForUnit(ASC) → Add Child(FramesBox) → Frames.Add(Unit).
- OnFrameUnitExited(Unit): Frames.Find(Unit) → погасить подписки charges → Remove from Parent → Frames.Remove(Unit).
- Вставка контейнера в боевой WBP_HUD — в Разделе 8 (пока в тестовом HUD).
- Строка с именем босса над рамкой, плашка активной метки, иконки навыков + КД (combat_system.md) — отдельным проходом позже.

---
### 7. WBP_Crosshair
**Статус:** ✅ Готово.

- Иерархия: `Size Box` (Width/Height Override, напр. 12×12, иначе схлопывается) → `Image` "Dot" (браш-точка; временно однотонный, если текстуры нет). Дефолтный Canvas удалён. Центрирование делает родитель (`WBP_HUD`, якорь center, alignment 0.5/0.5) — внутри самого виджета не центрируем.
- Переменная: `TargetASC` (AbilitySystemComponent, Object Ref, Instance Editable).
- **ASC приходит от родителя `WBP_HUD` через Function `SetCrosshairASC(NewASC)`** — самозахват в `Event Construct` НЕ используется (убран). Функция обычная (не Custom Event): виджет на поллинге, async-подписок нет.
- Visibility-биндинг (`GetCrosshairVisibility`): `IsValid(TargetASC)?` → False: `Collapsed`; True: `Has Matching Gameplay Tag(State.InStance) ? Visible : Collapsed`. Гард `IsValid` обязателен (ASC может быть ещё не проставлен в первый кадр).
- Без анимации, исчезает мгновенно.

---
### 8. WBP_HUD + сборка 
**Статус:** ✅ Готово.

- Корень `Canvas Panel`. Дети (4):
  - `WBP_PlayerFrame` — якорь top-left, дефолт `(50, 700)`.
  - **`WBP_BossFrameContainer`** — якорь top-left, дефолт `(1870, 700)`. ВАЖНО: `EnemyPos` из `LoadHUDLayout` применяется к слоту **контейнера**, а не к отдельной `WBP_EnemyFrame` (рамки боссов лежат стеком в `Vertical Box` контейнера).
  - `WBP_Crosshair` — якорь center `(0.5,0.5)`, alignment `(0.5,0.5)`, позиция `(0,0)`.
  - `WBP_BalanceBar` — якорь низ-центр `(0.5,1.0)`, alignment `(0.5,1.0)`, X `0`, Y отрицательный (напр. `-80`).
- **Раздача ASC централизована в `WBP_HUD` (единая точка правды).** `Event Construct`: `Get Owning Player Pawn → Cast ClanhallCharacter → Get Ability System Component → IsValid?` → True → `SetBalanceASC(ASC)` + `SetCrosshairASC(ASC)`. `PlayerFrame` и `BossFrameContainer` свой ASC достают сами (игрока / врагов через сенсор) — их HUD не трогает.
- В `Event Construct` также наполняется массив `DraggableFrames` (см. раздел 9): `Add(PlayerFrame)`, `Add(BossFrameContainer)`.
- Загрузка раскладки: `LoadHUDLayout(→PlayerPos, →EnemyPos):bool` → Branch. False → дефолты из дизайнера (ничего не делаем). True → `PlayerFrame → Slot → Cast Canvas Panel Slot → Set Position(PlayerPos)`; `BossFrameContainer → Slot → Cast Canvas Panel Slot → Set Position(EnemyPos)`.
- Спавн: в `BeginPlay` персонажа — `Is Locally Controlled?` → `Create Widget(WBP_HUD, Owning Player = Get Controller → Cast PlayerController)` → (опц.) промоут в переменную `HUDWidget` на будущее → `Add to Viewport`. `Owning Player` обязателен, иначе `Get Owning Player Pawn` внутри детей вернёт null.

---
### 9. Перетаскивание + сохранение (централизовано в `WBP_HUD`)
**Статус:** ✅ Готово.

Изначальный план предполагал оверрайды мыши в каждой рамке — сделано иначе. Вся drag-логика живёт в `WBP_HUD` (там же, где ASC и сейв). Перетаскиваемые рамки — `WBP_PlayerFrame` и `WBP_BossFrameContainer` — **оверрайдов мыши НЕ имеют** («тупые»). Требование к ним одно: `Visibility = Visible` (иначе не хит-тестятся). Базовый класс `WBP_DraggableFrame` не создавался — не нужен.

Переменные в `WBP_HUD`:
- `DraggableFrames` — массив `Widget` (наполняется в Construct: PlayerFrame, BossFrameContainer). Порядок = приоритет захвата при наложении рамок.
- `ActiveFrame` — `Widget` (что тащим сейчас; класс не важен, нужен только слот).
- `DragOffset` — `Vector2D` (точка захвата внутри рамки, чтобы не прыгала углом).

`On Mouse Button Down` (оверрайд на `WBP_HUD`):
- `MouseAbs = Mouse Event → Get Screen Space Position`.
- `Set ActiveFrame = null` (сброс перед поиском).
- `For Each Loop (DraggableFrames)` → `Element → Get Cached Geometry → Is Under Location(MouseAbs)` → если True: `Set ActiveFrame = Element` → `Break`.
- `Branch(IsValid(ActiveFrame))`: True → `CanvasGeo = Canvas → Get Cached Geometry`; `DragOffset = Absolute To Local(CanvasGeo, MouseAbs) − (ActiveFrame → Slot → Cast Canvas Panel Slot → Get Position)`; вернуть `Handled → Capture Mouse(Self)`. False → `Unhandled` (клик по пустоте проваливается дальше, ничего не тащим).

`On Mouse Move`:
- Гард `Has Mouse Capture(Self)? AND IsValid(ActiveFrame)?` → иначе `Unhandled`.
- `MouseLocal = Absolute To Local(CanvasGeo, Screen Space Position)`.
- `NewPos = MouseLocal − DragOffset`.
- `ActiveFrame → Slot → Cast Canvas Panel Slot → Set Position(NewPos)` → `Handled`.
- `Absolute To Local` сам учитывает DPI — ручной коррекции скорости нет.

`On Mouse Button Up`:
- Exec-провод: `Branch(IsValid(ActiveFrame))` → True → `SaveCurrentLayout` → `Set ActiveFrame = null`.
- Event Reply (в Return Node): `Handled → Release Mouse Capture → Return Value`. (Reply-ноды цепляются друг за друга через пин `Reply`; `SaveCurrentLayout` вешается по белому exec-проводу, не в reply-цепь.)

`SaveCurrentLayout` (Function на `WBP_HUD`): читает позиции ОБЕИХ рамок поимённо и пишет обе разом (`ActiveFrame` тут не участвует — сохраняем всю раскладку целиком):
- `PlayerFrame → Slot → Cast Canvas Panel Slot → Get Position` → `PlayerPos`.
- `BossFrameContainer → Slot → Cast Canvas Panel Slot → Get Position` → `EnemyPos`.
- `SaveHUDLayout(PlayerPos, EnemyPos)`.

⚠ **Ограничение на будущее:** `SaveHUDLayout` (C++) жёстко на две позиции. Движение масштабируется через массив `DraggableFrames` свободно, но третья перетаскиваемая рамка не сохранится, пока сигнатуру `SaveHUDLayout` не расширят (массив/структура позиций). Для текущих двух рамок всё сходится.

Заметка: `Is Under Location` на `BossFrameContainer` даёт True только когда контейнер непустой (есть босс в радиусе → есть площадь `Vertical Box`). Пустой контейнер не хватается — таскать нечего, это корректно.

---
### 10. Alt-режим 
**Курсор по Alt — ✅ готово.** 

Логика на `PlayerController` (методы `Set Input Mode` / `Show Mouse Cursor` — его). `IA_HUDCursor` (Digital bool) с двумя маппингами `Left Alt` + `Right Alt` в существующем IMC (новый контекст не добавляли). Через события Enhanced Input `Started` / `Completed` (one-shot, БЕЗ опроса в Tick и без ручного bool-флага):
- `Started` (Alt зажали): `Show Mouse Cursor = true`; `Set Input Mode Game and UI` с параметрами: **In Widget to Focus = пусто** (иначе крадёт клавиатурный фокус, глохнет WASD), **Mouse Lock Mode = Do Not Lock**, **Hide Cursor During Capture = false** (иначе курсор мигает/пропадает при клике).
- `Completed` (Alt отпустили): `Set Input Mode Game Only`; `Show Mouse Cursor = false`.

---
### 11. Тултипы
🟡 отложено (заметка на будущее):** 

`On Mouse Enter` на иконках атрибутов/навыков → `Show Tooltip Widget`. Фундамент готов — у рамок уже `Visibility = Visible`, хит-тест под курсором работает.

---

## License
Copyright © 2026 Lilu Dev

This source code is provided for portfolio and review purposes only.
You may view the code on GitHub.
You may not copy, modify, redistribute, or use this code in any project without explicit written permission from the author.