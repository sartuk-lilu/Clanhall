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
## Статус разработки прототипа

| #   | Система                    | Статус      |
| --- | -------------------------- | ----------- |
| 1   | Фундамент GAS и атрибуты   | ✅ Готово    |
| 2   | Боевая стойка и WASD-удары | ✅ Готово    |
| 3   | Система меток              | ✅ Готово    |
| 4   | DataAsset, Fragments       | ✅ Готово    |
| 5   | Парирование                | ✅ Готово    |
| 6   | Контрнавык                 | ✅ Готово    |
| 7   | Animation Setup            | In Progress |
| 8   | Противники                 |             |
| 9   | Магическая система         |             |
| 10  | Смена оружия               |             |
| 11  | Колесо умений              |             |

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
    UAnimMontage* CastMontage;    // nullptr законен — механика работает без монтажа
    float BalanceShift;           // МОДУЛЬ сдвига шкалы; знак — из Weapon.Type.*

    // Фрагменты — только то что нужно конкретной абилке
    UPROPERTY(EditAnywhere, Instanced)
    TArray<TObjectPtr<UAbilityFragment>> Fragments;

    template<typename T>
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
### Граница «заголовок или фрагмент»

Фрагмент оправдан тогда, когда его **отсутствие** несёт смысл, невыразимый дефолтным значением поля. Нет такого смысла — поле идёт в заголовок: фрагмент дал бы только лишний клик в редакторе и молчаливый баг «забыл добавить».

Поэтому `CastMontage` (отсутствие ≡ `nullptr`) и `BalanceShift` (отсутствие ≡ `0`) живут в заголовке, а `UDamageFragment` — фрагмент: его отсутствие означает «утилитарный навык, попадание подтверждается фактом найденной цели», чего `BaseDamage = 0` не выражает.

**Знак сдвига баланса в данных не хранится.** По `combat_system.md §2` он однозначно следует из типа оружия (STR → вправо, DEX → влево) и резолвится из `Weapon.Type.*` на ASC одной функцией `UGA_ClanhallAbilityBase::GetBalanceSign` — общей для WASD-ударов и активок. В ассете лежит только модуль (`ClampMin = 0`).

### Фрагменты проекта

| Фрагмент | Поля | Когда добавлять |
|---|---|---|
| `UVFXFragment` | CastEffect, ImpactEffect | У всех с визуалом |
| `USFXFragment` | CastSound, ImpactSound | У всех со звуком |
| `UDamageFragment` | BaseDamage, DamageEffect (GE) | У наносящих урон |
| `UMarkApplyFragment` | MarkTag, MarkEffect (GE 5 сек) | У накладывающих метку |
| `UMarkTriggerFragment` | TArray\<FMarkSynergy\> | У потребляющих метки |
### FMarkSynergy — структура внутри UMarkTriggerFragment

```cpp
USTRUCT()
struct FMarkSynergy
{
    FGameplayTag RequiredMark;          // метка-условие на цели
    TSubclassOf<UGameplayEffect> EffectOnTarget;  // дебафф на врага
    TSubclassOf<UGameplayEffect> EffectOnSelf;    // бафф на себя
    // Только одно из двух заполнено — никогда оба
};
```
### Как GameplayAbility читает фрагменты

Абилка данных не содержит — только запрашивает через `FindFragment<T>()`:

```cpp
void UGA_ShieldSlam::ActivateAbility(...)
{
    // При confirmed hit:
    if (auto* Dmg = AbilityData->FindFragment<UDamageFragment>())
        ApplyGameplayEffectToTarget(Target, Dmg->DamageEffect);

    if (auto* Mark = AbilityData->FindFragment<UMarkApplyFragment>())
        ApplyGameplayEffectToTarget(Target, Mark->MarkEffect);

    if (auto* Trigger = AbilityData->FindFragment<UMarkTriggerFragment>())
        CheckAndActivateSynergy(Target, Trigger->Synergies);

    if (!FMath::IsNearlyZero(AbilityData->BalanceShift))
        ShiftBalance(GetBalanceSign(SourceASC) * AbilityData->BalanceShift);

    // Косметика последней — механика уже посчитана:
    if (AbilityData->CastMontage)
        PlayMontage(AbilityData->CastMontage);
}
```

Логика абилки не меняется при изменении данных — только DataAsset.
### Порядок добавления фрагментов (не всё сразу)

1. Заголовок + `UDamageFragment` — навык наносит урон и двигает шкалу (`BalanceShift` — тоже заголовок)
2. `UMarkApplyFragment` + `UMarkTriggerFragment` — система меток работает
3. `CastMontage`, `UVFXFragment`, `USFXFragment` — визуал и звук

**Правило:** сначала механика работает, потом она красиво выглядит. `CastMontage` в заголовке этому не мешает: `nullptr` — законное состояние до нарезки монтажей.

---
### Таксономия GameplayTags (закладывается в Разделе 1)

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

Parry.Incoming.W
Parry.Incoming.A
Parry.Incoming.S
Parry.Incoming.D

Event.ApplyMark
Event.DirectionalAttack
Event.Hitbox.Hit
Event.Hitbox.Closed

Damage.Type.Slash
Damage.Type.Pierce
Damage.Type.Blunt

SetByCaller.Magnitude
Perk


```

---

## Структура 


### Locomotion и слои боевых анимаций

#### 1. Решение по базовому ассету

Расширен существующий `ABP_Unarmed`, а не создан новый с нуля.

**Обоснование.** Slot-нода не влияет на входящую позу, пока в неё не играет монтаж —
она пропускает её насквозь. Значит добавление слотов к рабочему locomotion не может
его сломать, и дублирование ассета не требуется.

Отдельные ABP под разные типы оружия (разные боевые стойки) — возможный вариант
на будущее (Раздел 10, свап оружия). Пока один ABP на игрока: правка бага локомоции
делается в одном месте, рассинхрон между копиями невозможен.

---

#### 2. Структура AnimGraph

Поток позы сверху вниз:

```
Locomotion SM  ->  cached pose 'locomotion'
                        |
                        v
                   Main States           (locomotion + jumping)
                        |
                        v
                cached pose 'lowerbody'
                    |          |
                    |          +--------------------------+
                    v                                     |
          Slot 'upperbody'                                |
                    |                                     |
                    v                                     |
          cached pose 'upperbody'                         |
                    |                                     |
                    |  (Blend Poses[0])   (Base Pose)     |
                    +------> Layered Blend Per Bone <-----+
                                     |
                                     v
                            Slot 'fullbody'
                                     |
                                     v
                              Control Rig
                                     |
                                     v
                              Output Pose
```

##### Назначение узлов

| Узел | Роль |
|---|---|
| `cached pose 'locomotion'` | Базовая локомоция (ходьба/бег), переиспользуется внутри Main States |
| `Main States` | Locomotion + jumping. Итоговое состояние «нижней половины» |
| `cached pose 'lowerbody'` | Кэш Main States. Используется дважды — потому и кэш, чтобы SM не считалась повторно |
| `Slot 'upperbody'` | Приёмник монтажей WASD и Q/E. Источник — `lowerbody` |
| `cached pose 'upperbody'` | Кэш результата верхнего слота |
| `Layered Blend Per Bone` | Склейка: низ из `lowerbody`, верх из `upperbody` |
| `Slot 'fullbody'` | Приёмник монтажей R/F (root motion). Перекрывает всё тело |
| `Control Rig` | Пост-обработка поверх финальной позы (foot IK и т.п.) |

##### Критичные детали подключения

- **Порядок пинов Layered Blend Per Bone:** `Base Pose = lowerbody`, `Blend Poses[0] = upperbody`.
  При перестановке фильтр по кости положит монтаж на ноги вместо верха.
- **Branch Filter:** `Bone Name = spine_01`, `Blend Depth = 1`.
  Всё от spine_01 и выше (грудь, руки, шея, голова) берётся из blend pose, остальное — из base.
- **Alpha блендера = 2.0 постоянно, гейтинг не нужен.** В покое слот `upperbody`
  пропускает `lowerbody` насквозь, обе позы идентичны, блендинг — no-op.
- **`Slot 'fullbody'` стоит ПОСЛЕ Layered Blend, не до.** Иначе верхний слой затирал бы
  full-body атаку.
- **Control Rig — последним, перед Output.** Стандартная точка для full-body пост-обработки.

---

#### 3. Слоты: создание и назначение

Слоты созданы в `Anim Slot Manager` (открывается из любого Animation Montage:
дропдаун слота на треке -> `Anim Slot Manager`). Привязаны к скелету — создаются
один раз на весь проект.

| Слот | Группа | Кому назначается |
|---|---|---|
| `upperbody` | DefaultGroup | Монтажи WASD-ударов (опенеры и переходы), **Recovery-хвосты** (`UComboData.Recovery`), Q/E |
| `fullbody` | DefaultGroup | Монтажи R/F (Shield Charge, Overhead Slam, Aerial Lunge) |

Recovery играется тем же `Montage_Play` из `UClanhallComboComponent::EndSequenceWithRecovery`, что и
удары — слот у него обязан быть тот же, иначе хвост не перебьёт терминальный удар, а заиграет поверх него.

##### Оба слота в одной группе — последствия

Монтажи внутри одной slot-группы **перебивают друг друга**. Это выбрано намеренно (альтернатива —
разные группы — дала бы параллельно тикающий комбо-монтаж со стреляющими нотифаями под видимым
`fullbody`-рывком: невидимые удары и открытое окно чтения посреди Shield Charge).

**Следствие:** любая активка Q/E/R/F, сыгранная посреди живого комбо, прерывает удар-монтаж, и
`UClanhallComboComponent::OnAttackMontageEnded` получает `bInterrupted = true`.

**Слот назначается в самом монтаже**, не в ABP. ABP только предоставляет приёмники.
При создании нового боевого монтажа обязательно выставить слот на его треке.

Типовая ошибка: монтаж остался в `DefaultSlot` -> анимация не проигрывается,
но логика (трейсы, метки, урон) работает штатно, т.к. AnimNotify живут на монтаже
независимо от того, виден он или нет. Симптом «логика есть, анимации нет» =
почти всегда несовпадение слота.

---

#### 4. Обоснование разделения upper/full

Стандартная причина слоёв (бежать и бить одновременно) **в этом проекте не работает**:
в боевой стойке при зажатой ЛКМ игрок не перемещается, WASD = удары.
Фактические причины разделения другие:

**FullBody для R/F — обязателен.** Это целотельные движения с root motion.
Положить их только на верх нельзя: ноги застрянут в idle, пока корпус летит вперёд.

**UpperBody для WASD/Q/E — про авторинг, не про механику.** Даёт:
- «прибитые» стопы при спаме лёгких ударов (нет проскальзывания на стыках
  idle -> удар -> idle);
- экономию анимаций — не нужно анимировать ноги в каждом ударе, они держат общую стойку;
- задел под реакции на удар: вне стойки игрок бегает, и флинч на верх тела поверх
  локомоции даст ему дёрнуться, продолжая движение. Сейчас этот кейс не задействован.

---
### Комбо-система WASD

#### 1. Что это и зачем

В боевой стойке (ЛКМ зажат) клавиши W/A/S/D — не движение, а четыре направленных удара (`EClanhallAttackDirection`: Overhead / RightSlash / LeftSlash / LowSweep). Из них игрок набирает серию. Система отвечает за три вещи: какие продолжения вообще существуют, какой клип и урон играет каждый шаг, и как серия завершается.

Два принципа из `combat_system.md §3`: **приоритет отзывчивости над анимацией** (ввод всегда читается, игрок не заперт в анимации) и наказание за **решения**, а не за криворукость (ошибочный ввод не карается лок-аутом сверх обычного — он просто не продолжает серию).

---

#### 2. Архитектура: данные + два исполнителя

**Данные** — один Data Asset на класс (`UComboData`), назначается в поле `ComboData` персонажа.
Класс-нейтральны (§11).

**Рантайм** — два объекта с разной ответственностью:

- `UClanhallComboComponent` — ворота ввода и единственный источник истины «что играть». Живёт на персонаже. Решает опенер / продолжение / игнор, резолвит клип и урон, ведёт состояние серии, завершает её через Recovery и сам проигрывает монтажи.
- `GA_DirectionalAttackBase` (+ 4 подкласса по направлениям) — тонкий исполнитель. Активируется ТОЛЬКО компонентом, получает урон в событии, применяет урон/MP/Balance, сообщает направление weapon-trace, выходит. Собственного монтажа не играет.

---

#### 3. Модель данных — `UComboData : UPrimaryDataAsset`

Состав комбо-данных фиксирован (всегда урон + стойка + переходы), поэтому обёртки-фрагменты не нужны — в отличие от `UAbilityData` навыков, где состав опциональный.

**Ключевая модель — пары переходов, а не пути.** Ход определяется ТОЛЬКО парой «предыдущее направление → новое». История серии до предыдущего шага в резолве не участвует: `A→W` один и тот же клип, откуда бы в `A` ни пришли. Компонент хранит лишь `LastDirection`, не список шагов.

##### 3.1 Профиль урона — 4 именованных поля

Базовый урон и тип задаются на **направление**, не на шаг серии. У игрока направления есть всегда, поэтому не массив с поиском по ключу, а четыре обязательных поля («не задал» не бывает):

```
FDirectionalDamage { float BaseDamage; FGameplayTag DamageType; }   // Direction в структуре не хранится
UComboData: FDirectionalDamage Overhead, RightSlash, LeftSlash, LowSweep;
FindDamageByDirection(Direction) -> const FDirectionalDamage&        // switch, ссылка — урон всегда есть
```

`DamageType` (`Damage.Type.Slash/Pierce/Blunt`) — тег-заглушка, в расчёте не участвует (задел под DT/резисты). Это источник истины `BaseDamage` для WASD; поля `RawDamage` на абилке нет.

##### 3.2 Поза боевой стойки — `StanceAnim`

`UAnimSequence` (не монтаж): стойка — поза в state machine, а не монтаж через слот. ABP забирает её статичным `AClanhallCharacter::GetStanceAnim(Character)` (`BlueprintPure`, принимает `ACharacter`, каст внутри) в `Event Blueprint Update Animation` и кладёт в переменную состояния `CombatStance`, которую читает Sequence Player. Per-class: позиция оружия своя у каждого класса, при свапе оружия меняется вместе с `ComboData`.

##### 3.3 Переходы — пять наборов + Recovery

Направление выводится из **имени поля**, отдельного поля `Direction` в структурах нет: дублировать его значило бы завести источник рассинхрона с профилем урона.

```
FComboTransitionSet          { ToOverhead; ToLeftSlash; ToRightSlash; ToLowSweep; }   // 4 — из стойки
FComboTransitionsFromOverhead{ ToLeftSlash; ToRightSlash; ToLowSweep; }               // 3 — без ToOverhead
FComboTransitionsFromLeftSlash / ...FromRightSlash / ...FromLowSweep                  // по 3, аналогично
FComboRecoveryAnimations     { AfterOverhead; AfterLeftSlash; AfterRightSlash; AfterLowSweep; }

UComboData: FromStance, FromOverhead, FromLeftSlash, FromRightSlash, FromLowSweep, Recovery
```

**Повтор направления непредставим на уровне типа.** У `From*`-наборов нет слота на своё же
направление: `W→W` требует нового полноценного замаха, то есть Recovery, а не продолжения. Слот не «оставлен пустым» — его нет вообще, поэтому заполнить его по ошибке невозможно.

Итого 20 слотов: 4 опенера + 12 переходов + 4 Recovery.

**`nullptr` — легальное состояние с тремя разными смыслами:**

| Где | Что значит |
|---|---|
| `FromStance.To*` | Этим направлением серию не начать — WASD в нейтрали не бьёт вообще |
| `From*.To*` | Продолжение запрещено → терминал, серия уходит в Recovery |
| `Recovery.After*` | Хвост возврата запечён в сам удар-монтаж, отдельно не играется |

**Recovery — монтаж, не Sequence.** `PlaySlotAnimationAsDynamicMontage` всё равно создаёт
`UAnimMontage` в рантайме: монтаж не устраняется, он становится неавторируемым, а
BlendIn/BlendOut/BlendOutTriggerTime переезжают из ассета в дефолты аргументов C++-функции — одно значение на все четыре направления, ноль настройки в редакторе. Аналогия со `StanceAnim` здесь неверна: стойка играется Sequence Player'ом в state machine (монтажа нет), Recovery играется через слот (монтаж есть).

**Резолв — простой switch,** без поиска и без логов конфликтов: слот либо заполнен, либо пуст,
конфликтовать нечему.

```
FindOpenerMontage(To)            -> UAnimMontage*    // FromStance
FindTransitionMontage(From, To)  -> UAnimMontage*    // From*, повтор направления -> nullptr
FindRecoveryMontage(LastDir)     -> UAnimMontage*    // Recovery
```

**Именование ассетов:** `Stance_W` / `Stance_A` / `Stance_D` / `Stance_S`; переходы `W_A`, `W_D`,
`W_S`, `A_W`, … ; хвосты `W_Recovery` / `A_Recovery` / `D_Recovery` / `S_Recovery`.

---

#### 4. Ввод: ворота, не буфер

Ключевое отличие от классического буфера — чтобы не терять отзывчивость.

- Пока **окно чтения** закрыто (фаза замаха), нажатия WASD **отбрасываются и не копятся**.
- Окно открывает/закрывает `AnimNotifyState_ComboWindow` на монтаже текущего удара
  (`OnComboWindowOpen` / `OnComboWindowClose`). В открытом окне действует **«последнее нажатие решает»**: хранится только последний ввод (`LatestInWindow`, overwrite), накопления нет.
- Нет буферизации → нет «подсасывания» устаревшего намерения; мэш не собирает комбо сам, тайминг окна обязателен.

Окно по умолчанию: Begin ≈ 60%, End ≈ 90% монтажа. End стоит ПОСЛЕ окна контакта (~80%), иначе следующий удар срежет собственное попадание. Разрешение (проигрыш следующего шага) — на закрытии окна. Позиция End — ручка «отзывчивость ↔ коммит», крутится в редакторе.

**Два гейта на входе в компонент, до всякого резолва:**

- висит `State.ComboRecovery` → ввод игнорируется (дублирует `ActivationBlockedTags` на
  `UGA_ClanhallAbilityBase`: тот гейтит активацию абилки, этот — вход раньше, до попытки);
- персонаж в воздухе (`IsFalling`) → удара нет. Стойка наземная; ЛКМ можно держать в падении, и тег
  `State.InStance` при этом висит, но удар-монтаж посреди падения играть нельзя.

Выход из стойки (отпуск ЛКМ) работает в ЛЮБОЙ фазе, вне ворот: `Montage_Stop` с блендаутом (`StanceExitBlendOutTime`, 0.18 с), принудительное закрытие weapon-trace, сброс серии. Ворота гейтят только продолжение, не отмену.

---

#### 5. Опенер / продолжение / игнор

- **Нейтраль** (`StepCount == 0`): нажатие = опенер. Валиден, если занят соответствующий слот
  `FromStance`. Активирует удар (урон/MP/Balance) и стартует серию.
- **Серия живёт**: нажатие в открытом окне — продолжение, валидируется слотом `From*`. Валидное играет свой клип и применяет урон; невалидное — **игнор без урона и сдвига шкал**, серия завершается через Recovery. Вне окна ввод отбрасывается.
- Добавочного штрафа за ошибку нет: базовый лок-аут (`State.ComboRecovery` на время
  Recovery-анимации, §7) вешается на ЛЮБОМ терминале одинаково. Цена ошибки в том, что решает последнее нажатие в окне: мусор в конце окна уводит серию в Recovery, успел поправиться валидным до закрытия — валидное перезаписывает.

---

#### 6. Резолв шага

Состояние серии — два поля: `LastDirection` (последнее сыгранное направление, не задано = нейтраль) и `StepCount` (длина серии, 0 = нейтраль). Оба меняются только вместе.

На **закрытии окна** (`OnComboWindowClose`) по порядку:

1. Ввода в окне не было → терминал (§7).
2. `StepCount + 1 > ClassRank` → терминал, даже если слот перехода занят (§8).
3. `LastDirection` не задан при `StepCount > 0` → рассинхрон состояния, защитный терминал.
4. `FindTransitionMontage(LastDirection, Dir)` вернул `nullptr` → невалидное продолжение, терминал.
5. Активация шага не прошла (например, стойку сняли между вводом и разрешением окна) → терминал,
   состояние не фиксируется.
6. Иначе: `LastDirection = Dir`, `++StepCount`. Если `StepCount >= ClassRank` — взводится
   `bCeilingReached` (§7, §8).

Клип и урон берутся по одному и тому же направлению шага: клип — из слота перехода, урон — из профиля (`FindDamageByDirection`).

---

#### 7. Завершение серии и Recovery

**Recovery-анимация ≠ тег `State.ComboRecovery`** — две разные сущности, путать нельзя.

- **Recovery-анимация** — визуальный возврат к стойке, играет ВСЕГДА после терминального удара (и одиночного опенера, и последнего в серии). Берётся по `LastDirection` из `Recovery`. `nullptr` =  хвост запечён в удар-монтаж, отдельный клип не играется.
- **Тег `State.ComboRecovery`** — геймплейный лок-аут НОВЫХ атак (WASD и физактивки Q/E/R/F, `ActivationBlockedTags` на `UGA_ClanhallAbilityBase`). Вешается на КАЖДОМ терминале. Длительность: `Recovery->GetPlayLength() * RecoveryLockFraction` (0, если Recovery-монтажа нет), плюс  `ComboRecoveryDuration` сверху, если серия упёрлась в потолок `ClassRank`.

**Единая точка завершения — `EndSequenceWithRecovery()`.** Вызывается из fall-through
`OnComboWindowClose` и из делегата конца удар-монтажа. Порядок внутри критичен:

1. Guard: `StepCount == 0` → выход. Не даёт сыграть Recovery дважды на одну серию.
2. Резолв Recovery-анимации и чтение `bCeilingReached` — **до** `ResetCombo()`: сброс обнуляет `LastDirection` и гасит флаг.
3. `ResetCombo()`.
4. Расчёт и наложение лок-аута.
5. Проигрыш Recovery-монтажа **без делегата конца** — иначе его собственный конец снова вызвал бы
   завершение серии (петля). По этой же причине здесь не используется хелпер `PlayMontage()`: он
   вешает делегат.

**Почему тег вешается здесь, а не при достижении потолка.** `ApplyComboRecovery()` только взводит `bCeilingReached`. Повесь тег там — таймер стартовал бы, пока терминальный удар-монтаж ещё доигрывает, и к возврату в стойку лок-аут был бы почти израсходован.

**Выход из стойки тег не снимает.** `ResetCombo()` намеренно его не трогает: иначе «выйти из стойки и сразу войти обратно» бесплатно отменяло бы лок-аут. Сам выход из стойки при этом остаётся бесплатным всегда — блокируются новые атаки, не отмена. `UGA_CombatStance` этой блокировкой не затронута.

**Страховка состояния (важно при расстановке нотифаев).** Серия завершается и по закрытию окна, и по естественному концу удар-монтажа (делегат `OnMontageEnded`, `bInterrupted == false`). Это спасает от залипания, если на монтаже нет или не сработал `AnimNotifyState_ComboWindow`. Weapon-trace принудительно закрывается в делегате ДО раннего return по `bInterrupted` — прерванный монтаж и есть основной кейс залипшего трейса.

**Прерывание чужим монтажом** — `CancelSequenceForExternalMontage()`, зовётся активкой Q/E/R/F перед
своим `Montage_Play` (оба слота в одной группе, `locomotion_structure.md §3`). Гасит trace и
сбрасывает серию, но БЕЗ Recovery-анимации и БЕЗ тега: чужой монтаж уже занимает слот, Recovery дрался бы с ним за него, а наказания за прерывание нет — тот же принцип, что у невалидного продолжения. Сам монтаж не останавливается: `Montage_Play` вызывающего перебьёт его, прилетит `OnAttackMontageEnded(bInterrupted = true)` → ранний return → состояние уже чистое. `LastPlayedMontage` намеренно не обнуляется: если игрок отпустит ЛКМ во время каста, `OnStanceExit` сделает `Montage_Stop` по мёртвому удар-монтажу (безвредный no-op) вместо живого каста.

---

#### 8. Ранг и длина комбо

Потолок длины серии = `AClanhallCharacter::ClassRank` (0–4, `BlueprintReadWrite`, плейсхолдер до системы прокачки; рядом `ClassTag` из `Ability.Class.*`). Потолок живёт на персонаже, а не в ассете: длина зависит от бойца, а не от оружия. Фолбэк компонента при отсутствии персонажа — 1.

Заполненные переходы существуют независимо от ранга, ранг лишь ограничивает глубину: кандидат длиннее ранга → продолжение запрещено → терминал + Recovery. Отдельных наборов данных под ранги не нужно.

`ClassRank = 0` (будущее): отдельная деградированная ветка — более «корявый» удар с долгим Recovery, в духе безоружного замаха как в Gothic. Не реализовано. Сейчас опенер ранг не проверяет, поэтому при 0 одиночный удар всё равно проходит — при реализации нулевого ранга опенер нужно будет направить на эту ветку.

---

#### 9. Цена модели пар

Модель пар обменивает выразительность на простоту, и обмен нужно помнить.

**Чего в ней нет.** Уникального клипа на конкретной глубине или в конкретной ветке. Переход `A→W` один на всё дерево: нельзя дать особую анимацию для «`D` → `A` → `W`», отличную от «`S` → `A` → `W`» — в резолв приходит только `(A, W)`. Раньше это выражалось ссылкой на другую строку таблицы ходов (`Special_D_W`); вместе с путями ушло и это.

**Что получено взамен.** Невозможные состояния непредставимы (повтор направления), коллизий данных не существует в принципе, резолв — switch без поиска, а редактор показывает ровно 20 слотов вместо дерева цепочек с ручным вводом идентификаторов.

**Задел.** Корень тега `Perk` (`ClanhallGameplayTags`) оставлен под будущую условную разблокировку. Если она понадобится, вешать её планируется фрагментом на уровне конкретного навыка/хода, которому условие реально нужно (паттерн DataAsset + Fragments), а не полем на каждой записи данных.

---

#### 10. Урон — канал до абилки

Компонент резолвит `BaseDamage` через `FindDamageByDirection` ДО активации и кладёт в
`FGameplayEventData::EventMagnitude`; `DamageType`-тег — в `InstigatorTags` события (пока не читается). Активация — `TriggerAbilityFromGameplayEvent` по хэндлу из
`AClanhallCharacter::GetAttackHandle(Direction)`, тег события `Event.DirectionalAttack` служебный. `GA_DirectionalAttackBase::ActivateAbility` берёт урон из `TriggerEventData->EventMagnitude`, применяет `ResolveStandardDamage` + MP/Balance по `combat_system.md §4`. Монтаж проигрывается компонентом только при успешной активации — вызывающий код фиксирует состояние серии лишь по `true`.

Компонент — единственный владелец решения «что играть и сколько урона»; абилка в данные не лезет.

---

#### 11. Класс-нейтральность и переиспользование врагом

Данные не содержат ничего player-специфичного: ключ — `EClanhallAttackDirection` (доменное «тип удара», нейтрально к источнику: клавиша игрока или выбор ИИ). Никаких `UInputAction`, ссылок на игрока.

Это задел: враги (Часовой, Страж) на этапе ИИ смогут переиспользовать те же клипы и тот же ассет через свой исполнитель `GA_EnemyWASDSeries` — БЕЗ этого реактивного компонента (врагу не нужны ворота, окна и резолв ввода, он исполняет выбранную ИИ серию по скрипту). Общий предок игрока и врага, ранг врага и общий исполнитель — отдельная задача этапа ИИ; сейчас `ClassTag`/`ClassRank` живут только на `AClanhallCharacter` (с TODO).

---

#### 12. Нарезка анимаций (узловые позы)

Модель клипов — **граф узловых поз**, и она ложится на модель пар один в один.

- **Узлы:** `Stance` (якорь) и по одному «после удара»: `after-W`, `after-A`, `after-D`, `after-S`.
- **Клип = ребро:** переход из узла в узел с анимацией ОДНОГО удара. Ребро адресуется парой «входной узел, удар» — ровно то, что хранит `UComboData`.
- **Резать из самой длинной связки:** удары внутри ветки брать из одного mocap-дубля (`W` из `DAW`  несёт реальную инерцию серии), точки реза ставить по узловым позам — тогда конец `D→A` совпадает со стартом `A→W`, стык внутри одного дубля, без рывка. Собирать удары из разных дублей хуже — рвётся инерция тела.
- **Согласованные узлы:** все клипы, входящие в узел, и все выходящие из него должны сходиться в ОДНОЙ каноничной позе этого узла. Расхождение между дублями — подчистить концы или микро-бленд 0.05–0.08 с на узле (там скорость минимальна, бленд невидим).
- **Опенер** — ребро `Stance→X`, стартует из стойки. Брать `W` из `DAW` как опенер нельзя: он
  начинается не из стойки и валиден только как `A→W`.
- **Recovery** — ребро `узел→Stance`, своё на каждый узел. Разные, потому что меч остаётся слева или справа: общий бленд между несовместимыми позами даёт кашу. Четыре узла — четыре хвоста.

Именование сырых mocap-дублей — `AS_Mocap_..._Raw`, отдельно от нарезки; имена нарезанных монтажей — §3.3.

---

#### 13. Карта кода

- `ComboData.h/.cpp` — `FDirectionalDamage`, `FComboTransitionSet`, `FComboTransitionsFrom*` (×4),
  `FComboRecoveryAnimations`, `UComboData` (поля профиля, `StanceAnim`, `FromStance`, `From*`,
  `Recovery`; `FindDamageByDirection` / `FindOpenerMontage` / `FindTransitionMontage` /
  `FindRecoveryMontage`).
- `ClanhallComboComponent.h/.cpp` — ворота ввода, `HandleAttackInput`, `TryStartSequence`,
  `OnComboWindowOpen/Close`, `ActivateStep` (канал урона), `EndSequenceWithRecovery`,
  `CancelSequenceForExternalMontage`, `OnStanceExit`, делегат-страховка, потолок `ClassRank`.
  Настройки: `ComboRecoveryDuration`, `RecoveryLockFraction`, `StanceExitBlendOutTime`.
- `GA_DirectionalAttackBase.h/.cpp` — исполнитель: урон из `EventMagnitude`, MP/Balance, направление в weapon-trace.
- `AClanhallCharacter` — `ComboData`, `GetComboData()`, `GetStanceAnim()`, `GetAttackHandle()`, `ClassTag`/`ClassRank`, `OnAttackX` → `HandleAttackInput`, `OnStanceReleased` → `OnStanceExit`.
- `AnimNotifyState_ComboWindow`, `AnimNotifyState_WeaponTrace` — окна на монтажах.
- Теги: `State.ComboRecovery`, `Damage.Type.*`, `Perk.*`, `Event.DirectionalAttack`
  (`ClanhallGameplayTags`).

---
### Симуляция боевой системы

![prototype_fight](https://github.com/user-attachments/assets/efa74915-a4a9-48dc-af5b-1831a61b71c3)

---

## Реализация 

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
  - **Вход закрыт на время Recovery.** `ActivationBlockedTags` дополнительно содержит `State.ComboRecovery`: пока доигрывает Recovery-анимация, в стойку не войти (Раздел 7). ВЫХОД свободен всегда — он идёт через `CancelAbilityHandle` и `ActivationBlockedTags` не касается.
  - **Ретрай ввода.** `StanceAction` привязан не только к `Started`, но и к `Triggered`. Без этого нажатие во время лока проглатывалось бы: `Started` израсходован, и после спадания тега персонаж так и не вошёл бы в стойку при зажатой ЛКМ. `Triggered` повторяет попытку каждый кадр удержания; при уже активной стойке это дешёвый отказ по `State.InStance`.
- **Стойка гасит движение мгновенно.** `UGA_CombatStance::ActivateAbility` зовёт `StopMovementImmediately()` на аватаре. `DoMove` и так не принимает WASD в стойке, но остаточная инерция от бега гаслась бы за пару десятых; снап в 0 роняет Speed в тот же кадр, чтобы переход Locomotion→CombatStance блендился из idle-позы, а не из бега. Фолбэк, если снап читается резко: высокий `BrakingDecelerationWalking` на время стойки (скид-ин) вместо мгновенного стопа.
- **В стойке нельзя прыгать.** `AClanhallCharacter::CanJumpInternal_Implementation()` возвращает `false` при `State.InStance`. `JumpAction` привязан к `ACharacter::Jump` напрямую, но фактический прыжок проходит через `CanJump()`/`CanJumpInternal()` в movement-тике — это единственная точка отсечения, входные биндинги трогать не нужно. Когда Раздел 9 введёт umbrella-тег `State.Stance`, проверку перевести на него вместе с `DoMove` — прыжок блокируется во всех трёх стойках.
- **Гейт на стойку для всех физдействий.** Общий предок `UGA_ClanhallAbilityBase` ставит `ActivationRequiredTags = State.InStance`. Поэтому вне стойки WASD-абилки просто не активируются (`TryActivateAbility` отказывает), и клавиши работают как движение — без отдельной ветки кода.
- **WASD-удары** — база `UGA_DirectionalAttackBase` (`InstancedPerExecution`) + четыре тонких наследника (`GA_DirectionalAttacks.h`), каждый переопределяет только `GetDirection()` → `Overhead(W)/RightSlash(D)/LeftSlash(A)/LowSweep(S)`. Enum `EClanhallAttackDirection` вынесён в `ClanhallCombatTypes.h`, чтобы не плодить взаимозависимости заголовков (его же читают Parry- и Hitbox-компоненты).
- **Поиск цели (геометрия мгновенного фолбэка и запрос цели для контрнавыка, Раздел 7)** — `FindMeleeTarget` в базовом классе: `SphereOverlapActors` сферой перед персонажем (`TraceRange 200`, `TraceRadius 75`), берётся первый актор с `IAbilitySystemInterface`.
- **AP-обмен** — `ResolveStandardDamage` (в базовом классе): у цели снимается `min(урон, AP цели)`, 50 % снятого возвращается атакующему в свой AP, переполнение (урон больше остатка AP) идёт прямо в HP. Возврат `true` = «confirmed hit» — на нём завязаны КД и синергии. Всё через generic-эффекты `GE_Modify*` с SetByCaller-магнитудой.
- **MP и Balance на удар** (`GA_DirectionalAttackBase::ActivateAbility`): STR-оружие (`Weapon.Type.STR` на ASC) → MP +10, Balance +5..+15; DEX → MP +5, Balance −5..−15 (сдвиг рандомный в диапазоне). Само число урона теперь приходит из комбо-компонента через `TriggerEventData->EventMagnitude` (Раздел 7) — но STR/DEX-логика MP/Balance осталась здесь. Применяются они вместе с уроном — в момент контакта зоны либо на активации в фолбэк-режиме.
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
- **Метка летит только от атакующего к цели и только при попадании** (`mark_system.md §2 Правило 1`). Концепция «горячей картошки» (одна метка на двоих, кочующая между игроком и врагом) отменена целиком: метка на себя за промах не вешается и своя метка на цель не переносится. Промах побочных эффектов не имеет вовсе: заряды и КД уже списаны на активации (Раздел 4), метка не ложится ни на кого.
- **Источник метки** — `CurrentMarkSourceASC` (`TWeakObjectPtr`), `IsOwnMark(QueryASC)`. Потребителя в геймплее сейчас нет — синергия смотрит только на тег цели и в источнике не нуждается. Метод сохранён под HUD («кто повесил метку») и синергии врага в Разделе 8. Метку, наложенную врагом, игрок снять не может никак — только переждать 5 сек.
- **Кэш ≠ истина.** `CachedMarkTag` — только подсказка «какой тег проверять»; истина всегда в теге на ASC. `GetCurrentMark()` каждый раз перепроверяет `HasMatchingGameplayTag`, потому что по истечении 5 сек GE снимает тег сам, а кэш мог не узнать об этом мгновенно.
- **Проверка синергии, активация и генерация заряда** живут в `GA_PhysicalSkill` (Раздел 4), метод `ResolveMarkLogic`: метка на цели сгорает → эффект срабатывает (бафф на себя ИЛИ дебафф на цель, никогда оба) → навык кладёт свою новую метку.
- **Заряд синергии — число в данных**, `FMarkSynergy::ChargeGain` (дефолт 2). Зашитых чисел меток и синергий в коде не осталось.
- **Мультицель — разделение «ресурс / состояние»**, не «на цель / на себя» (`mark_system.md §2 Правило 5`). Урон, наложение метки, `EffectOnTarget` и `ChargeGain` — на каждую задетую цель; `EffectOnSelf` и сдвиг Balance — один раз за применение навыка. Заряды масштабируются числом целей намеренно: бой рассчитан на одного противника, изредка на 2–3, то есть мультицель — редкая ситуация повышенного риска, и она должна награждать. От перекоса защищает кламп `MaxCharges`, а не отдельное правило убывания.
- HUD: цветной индикатор типа метки + таймер 5 сек над врагом.

**Проверка:** вручную повесить метку, вторым навыком активировать синергию — эффект срабатывает, заряд добавляется, новая метка ложится.

---
### Раздел 4 — DataAsset, Fragments, первые навыки Knight
**Статус:** ✅ Готово.

**Итог:** один класс-навык `UGA_PhysicalSkill` обслуживает все физические активки; всё их содержание (урон/метка/синергия/баланс/КД/стоимость) — данные в `UAbilityData`, меняются без перекомпиляции.

**Реализация (по факту):**
- **`UAbilityData`** (`AbilitySystem/AbilityData.h`) : `UPrimaryDataAsset`. Заголовок: `DisplayName`, `Icon`, `Cooldown` (по умолч. 10), `CooldownTag` (`Cooldown.Slot.*`), `RequiredClass` (`Ability.Class.*`), `CounterTag` (`Ability.Skill.*` — идентичность для контрнавыка), `ChargeCost` (0), `CastMontage`, `BalanceShift` (модуль, `ClampMin = 0`), массив `Fragments` (Instanced). Шаблон `FindFragment<T>()` возвращает первый фрагмент типа T или nullptr.
- **Фрагменты.** База `UAbilityFragment` (`Abstract, DefaultToInstanced, EditInlineNew` — массив полиморфных подобъектов, редактируемых прямо внутри ассета). Механические (`GameplayFragments.h`): `UDamageFragment`(BaseDamage), `UMarkApplyFragment`(MarkTag), `UMarkTriggerFragment`(TArray\<FMarkSynergy\>). Презентационные (VFX/SFX) вынесены в `PresentationFragments.h`.
- **Один класс на все активки.** `UGA_PhysicalSkill` (`InstancedPerExecution`) гранится 4 раза (Shield Slam / Power Strike / Shield Charge / Retribution), каждый раз со своим `SourceObject` (=`UAbilityData`) через `FGameplayAbilitySpec::SourceObject`. `GetAbilityData` достаёт `SourceObject` через `Handle`, а не `GetCurrentSourceObject()` — у `InstancedPerExecution` на момент `CanActivateAbility` персистентного инстанса ещё нет (метод может вызваться на CDO).
- **Активка коммитится на активации.** `CanActivateAbility` проверяет тег КД и наличие Charges. Charges списываются на активацию всегда, включая контр и промах; тег КД вешается на активации (`ApplyTimedTag` на `Data->Cooldown`) — кроме успешного контрнавыка, там КД не уходит вовсе. Заблокированный канон «КД только при confirmed hit» отменён осознанно.
- **Порядок в `ActivateAbility`:** поиск цели → резолвер контрнавыка → списание Charges (безусловно) → ранний выход без КД, если контр → тег КД → погасить чужую WASD-серию и сыграть `CastMontage` (если задан), ДО подписки на события Hitbox → выбор режима резолва.
- **Два режима резолва урона (Порция E2, тот же паттерн, что у WASD в Порции D).** `UAnimNotifyState_Hitbox::MontageHasHitbox(Data->CastMontage)` решает: **контактный** (монтаж есть и на нём расставлена зона) — способность ждёт `Event.Hitbox.Hit`/`Event.Hitbox.Closed` через `UAbilityTask_WaitGameplayEvent` и резолвит через `ResolveHitOn` каждую цель, задетую зоной; **мгновенный фолбэк** (`CastMontage == nullptr` или зоны на нём нет) — тот же `ResolveHitOn` вызывается один раз, сразу на активации, по цели из `FindMeleeTarget`. `ResolveHitOn`: урон через `UDamageFragment`/`ResolveStandardDamage` (утил-навык без урона считается попавшим по факту найденной цели) → `ResolveMarkLogic` (синергия + новая метка) → сдвиг Balance → debug. Мультицель на контактном пути — ресурс (урон/метка/`ChargeGain`) на каждую цель, состояние (`EffectOnSelf`/Balance) один раз за применение (`mark_system.md §2 Правило 5`). Ветка промаха/непопадания: заряды списаны, КД повешено на активации, метка не ложится ни на кого (Раздел 3). Пока `CastMontage` у всех четырёх ассетов Knight `nullptr` — фолбэк-путь единственный наблюдаемый.
- **`State.SkillCommitted`.** Висит на `GA_PhysicalSkill` (`ActivationOwnedTags`) от активации до `Event.Hitbox.Closed` на контактном пути (на фолбэке способность заканчивается сразу же, тега фактически нет). Блокирует вторую активку и WASD-серию (`ActivationBlockedTags` на `UGA_ClanhallAbilityBase` + ранний гейт в `HandleAttackInput`) — «начатую активку нельзя оборвать» перестало быть верным только для зарядов/КД и стало верным для самого удара.
- **Knight Ранг 1–2:** Q Shield Slam, E Power Strike, R Shield Charge, F Retribution. КД по тиру Q/E=10, R/F=20 (прототип). Charges: Q/E=0, R/F=2. Все числа — в DataAsset'ах.

**Пройденные этапы:**
- **`CastMontage` и `BalanceShift` переехали из фрагментов в заголовок.** Изначально были отдельными `UAnimationFragment` и `UBalanceFragment`. Причина переезда — критерий из «Граница «заголовок или фрагмент»»: отсутствие обоих не несло смысла сверх `nullptr`/`0`, то есть фрагмент давал только лишний клик в редакторе и молчаливый баг «забыл добавить — анимации нет, логика работает». Вместе с `UAnimationFragment` удалено поле `ImpactMontage`: реакция на удар принадлежит получателю, а не навыку атакующего, и станет отдельной системой.
- **Знак сдвига баланса перестал храниться в данных.** Раньше в ассете лежало со знаком число; теперь только модуль, а знак резолвится из `Weapon.Type.*` общим `GetBalanceSign` — одним на WASD-удары и активки. Правило из `combat_system.md §2` (STR → вправо, DEX → влево) стало жить в одной точке, а `ClampMin = 0` сделал невозможной опечатку знаком на масштабе 8 слотов × 8 оружий.

**Проверка:** нажатие Q → Shield Slam, метка ложится, синергия из Раздела 3 срабатывает; правка цифр в DataAsset меняет поведение без перекомпиляции.

---
### Раздел 5 — Парирование (placeholder)
**Статус:** ✅ Готово.

**Канонический дизайн** (`combat_system.md §5`): Clash Detection — weapon-trace игрока пересекается с weapon-trace врага в активном окне анимации, без UI-индикаторов. **Полная trace-версия — в Разделе 7** (диспетчер зон `UClanhallHitboxComponent`). Ниже — то, что реализовано в этом разделе как placeholder до готовых монтажей.

**Реализация (по факту):**
- **`UClanhallParryComponent`** (`AbilitySystem/ClanhallParryComponent.h/.cpp`) на Character: флаг `bParrySuccessful`, `ResetParry()`, `TryParry(HitEnemy, PlayerDirection, HitLocation)`. Маппинг направления удара игрока → парируемый тег `Parry.Incoming.*` по правилу «обратное направление»: W парирует входящий S, S→W, D→A, A→D. Проверяет, что нужный `Parry.Incoming.*` висит на ASC игрока, ставит флаг, играет clash-звук.
  - *Точка вызова переехала.* В Разделе 5 `TryParry` дёргался из обработчиков ввода WASD; теперь его зовёт диспетчер зон поражения — `UClanhallHitboxComponent::CheckAndHandleParry` (Раздел 7).
- **Активки в клэше не участвуют** — WASD парируют WASD, активки контрят активки (`ability_system.md §2`). Выражено данными, а не спецкейсом: у каждой зоны (`FClanhallHitboxDesc`) есть флаг `bParryable`, и `CheckAndHandleParry` вызывается только для зон с `true`. WASD-удары парируемы по дефолту; активки Q/E/R/F ставят `false`, и протухшее `CurrentDirection` последнего WASD-удара до проверки парирования не доходит вовсе.
  - **Отдельных щедрых сфер под парирование НЕ делаем.** Геометрия оружия защищающегося в проверке не участвует и никогда не участвовала: условие — «зона атаки нападающего коснулась актора, на котором `State.Parrying`, и направление совпало». Это модель Sekiro (тайминг + состояние, а не столкновение оружия об оружие), и она уже самая снисходительная из возможных — парируется то, что в тебя и так попало бы. Щедрые сферы дали бы обратный эффект — парировались бы удары, проходившие мимо, и это читается как случайное срабатывание. Если парировать окажется тяжело — крутить длину окна `State.Parrying`, не размеры зон.
- **`UGA_EnemyWASDSeries`** (`Abilities/GA_EnemyWASDSeries.h/.cpp`) — AI-серия ударов, `InstancedPerExecution`, снимает требование стойки, заблокирована `State.Stunned` и `State.ComboRecovery`. Шагает по `AttackDirections` через `AbilityTask_WaitDelay`. На каждый удар (`PrepareHit`): сброс парирования, `State.Parrying` на СЕБЯ (враг — паррируемый актор) на `WindowDuration`, `Parry.Incoming.*` на игрока на то же окно, ожидание. По истечении окна (`OnWindowExpired`): парировано → счётчик; иначе → `ResolveStandardDamage` по игроку (AI получает свои 50 % AP). Между ударами — пауза `DelayBetweenHits`.
  - *Интерим-нюанс:* `State.Parrying` тут вешается кодом — это временная замена `AnimNotifyState_ParryWindow`. Когда монтажи врага будут готовы, эту строку убирают (п.17 Раздела 7), иначе окно откроется дважды.
- **Правило «всё или ничего»** — `FinalizeSeries`: полное парирование (`ParriedCount == число ударов`) → AI оглушён (`State.Stunned` на `StunDuration`) + КД игрока сокращаются на `CDReduction`. Пропустил хоть один — эффекта нет. После серии — `State.ComboRecovery` против мгновенного перезапуска (длительность: при полном парировании `Stun+0.5`, иначе 1.0 сек).
- **Сокращение КД игрока** — `ReducePlayerCooldowns`: запрос активных эффектов по родительскому тегу `Cooldown` (`MakeQuery_MatchAnyOwningTags` — в UE 5.3+ смотрит и asset-, и granted-теги; наш `GE_ApplyTimedTag` кладёт тег в `DynamicGrantedTags`), считает `остаток − CDReduction`, снимает все КД-эффекты и перевешивает ещё не истёкшие с укороченной длительностью.
- **Конкретная серия** — `UGA_Series_Crosscut`:  «Перекрёстный» Часового A→D (ответ игрока D→A). Гранится Training Dummy по таймеру 4 сек.

**Проверка:** болванчик делает A→D, игрок жмёт D→A — парирует и оглушает; пропустил один — эффекта нет.

---
### Раздел 6 — Контрнавык
**Статус:** ✅ Готово.

**Итог:** активка врага, начатая в открытом окне, прерывается любым навыком игрока из её набора `CounteredBy` — активка врага уходит на полный КД, а навык игрока при этом не тратит ни стоимости, ни КД.

**Реализация (по факту):**
- **`UClanhallCounterComponent`** (`AbilitySystem/ClanhallCounterComponent.h/.cpp`) — на обоих бойцах, симметрично. `OpenWindow(CounteredBy, CounteredHandle, CooldownTag, CooldownDuration)` запоминает **набор тегов, которыми контрится** текущая активка, её хендл и КД, вешает `State.CounterWindow` (loose-тег) на владельца. `IsCounterableBy(Tag)` = окно открыто И `CounteredByTags.HasTag(Tag)` — именно `HasTag`, а не `HasTagExact`: запись `Ability.Skill.Lancer` матчит любой навык Ланцера, так что крупность настраивается в данных без кода. Пустой контейнер = не контрится ничем. `ConsumeCounter` → `CancelAbilityHandle` активки врага + полный КД на неё + закрытие окна. Статический `TryResolveCounter(Target, Tag)` — общий резолвер, который дёргают навыки игрока.
- **Асимметрия данных.** Атакующий предъявляет ОДИН тег — свою идентичность (`UAbilityData::CounterTag` у игрока, `AbilityTags` у врага). Защищающийся держит НАБОР (`CounteredBy`). Набор именно на защищающемся, потому что окно открывает он сам и передаёт контейнер напрямую; держи список на атакующем — каждое открытие окна требовало бы скана всех ассетов. Симметричного `CounteredBy` у навыков игрока пока нет: окно под них никто не открывает (босс контрит игрока — бэклог).
- **Сторона игрока** (`GA_PhysicalSkill::ActivateAbility`): ДО списания Charges/КД зовётся `TryResolveCounter(Target, Data->CounterTag)`. Успех → активка врага сбита + полный КД, а навык игрока не коммитится вовсе (ранний `EndAbility` — без стоимости, без КД, без урона). Это и есть «Charges не расходуются, КД не уходит, навык готов сразу».
- **Сторона врага** (`GA_EnemyActiveSkill.h/.cpp`): `InstancedPerExecution`, без требования стойки, заблокирован `Stunned`/`ComboRecovery`. `ActivateAbility` открывает окно контрнавыка на своём CounterComponent и запускает `WaitDelay(CounterWindowDuration)` до удара. Если снаружи `ConsumeCounter` вызвал `CancelAbilityHandle` — задача `WaitDelay` снимается, `OnHitDelayExpired` не вызовется, урона нет. Если окно не прервали — по истечении: враг сам закрывает своё окно, `ResolveStandardDamage` по игроку, наложение `HitMarkTag` (если задан).
  - *Интерим-нюанс:* окно врага открывается кодом — временная замена `AnimNotifyState_CounterWindow` (реальных монтажей пока нет). Убирается в п.18 Раздела 7, иначе окно задвоится.
- **Конкретная активка** — `UGA_Enemy_PowerStrike`: `AbilityTags` = `Ability.Skill.Knight.PowerStrike` (идентичность), `CounteredBy` = тот же тег — самоконтр теперь выражается записью в наборе, а не зашит в механику. `Cooldown.Slot.E`, 10 сек, `CounterWindowDuration 1.2`, `HitDamage 40`. (`HitMarkTag = Mark.BrokenGuard` отложен до Раздела 7 вместе с полным Часовым.)

**Пройденные этапы:**
- **От «контрит тот же навык» к набору контрящих.** Изначально активку прерывал тот же самый навык — совпадение одного `CounterTag`. Правило заменено на «разные классы контрят разные навыки», чтобы мотивировать носить второе оружие и изучать чужие классы.
- **Следствие для данных.** Одно поле `CounterTag` тащило два смысла сразу — «кто я» и «кого я контрю»; работало только потому, что это было одно и то же. Связь стала многие-ко-многим → смыслы разведены: идентичность осталась у атакующего, набор `CounteredBy` лёг на защищающегося.
- **Побочные выигрыши.** Проверка через `HasTag` вместо сравнения тегов дала крупность записи бесплатно («любой навык Ланцера» или конкретный удар), а пустой контейнер стал естественным способом сказать «не контрится ничем» (War Shout). Самоконтр перестал быть автоматическим и стал явной записью в наборе.
- **Цена правила.** Самообучающее свойство старого правила (увидел навык босса → выучи его → контри) утеряно: теперь без плашки босса и боевого журнала правило нечитаемо, и они из украшения стали обязательной частью механики (`ability_system.md §2`, §4).

---
### Раздел 7 — Animation Setup / Комбо-система WASD
**Статус:** In Progress — C++ готов, идут редакторные ассеты (Этапы ниже).

**Итог:** WASD-удары стали направленным деревом комбо: `UClanhallComboComponent` сам валидирует ввод по данным и активирует шаг, `GA_DirectionalAttackBase` больше не решает ничего, кроме урона/MP/Balance.
Анимационный слой навешивается поверх готовой механики.

**Канон:** механика комбо и нарезка узловыми позами — `combo_system.md`; направления, парирование, clash detection — `combat_system.md §4/§5`; AnimGraph, слои и слоты — `locomotion_structure.md`.

**Ключевой инвариант.** На нажатии решаются **выбор шага, валидация по дереву, потолок ранга и стоимость** (Charges, КД активки) — это не может зависеть от анимации. **Применение урона, MP и Balance у WASD-ударов отдано окну контакта** (`AnimNotifyState_Hitbox`): удар засчитывается тогда, когда зона реально касается цели.
Механика по-прежнему работает без единой анимации: если у шага нет монтажа или на монтаже не расставлены зоны, `GA_DirectionalAttackBase` резолвит урон мгновенно сферой `TraceRange`/`TraceRadius`, как до Порции D.
`Montage` хода может быть `nullptr` — дерево комбо проверяется до нарезки анимаций.

**Реализация (по факту):**
- **`UComboData`** (`Fragments/ComboData.h`) — один Data Asset на класс. Модель пар переходов, не путей (канон — `combo_system.md §3`): профиль урона на НАПРАВЛЕНИЕ (`Overhead`/`RightSlash`/`LeftSlash`/`LowSweep`), `StanceAnim`, и пять наборов переходов — `FromStance`, `FromOverhead`, `FromLeftSlash`, `FromRightSlash`, `FromLowSweep`, `Recovery` — 20 слотов итого (4 опенера + 12 переходов + 4 Recovery-хвоста).
- **Инвариант резолва:** ход определяется ТОЛЬКО парой «предыдущее направление → новое» (`FindOpenerMontage`/`FindTransitionMontage`); история серии до предыдущего шага не участвует — компонент хранит только `LastDirection`. Повтор направления (`W→W`) непредставим на уровне типа: у `From*`-наборов нет слота на своё же направление.
- **Ворота ввода, не буфер.** До окна чтения нажатия отбрасываются и не копятся; в окне «последнее решает». Нет накопления → нет залипания отзывчивости.
- **Потолок длины серии = `AClanhallCharacter::ClassRank + 1`** (`ClassRank` 0–4, плейсхолдер до прокачки)  — на персонаже, не в ассете: длина зависит от бойца, а не от оружия. Дерево может содержать записи длиннее ранга — они просто недоступны.
- **Урон — из профиля, не с абилки.** Компонент резолвит `BaseDamage` по направлению шага ДО активации и передаёт через `TriggerAbilityFromGameplayEvent` (`EventMagnitude` = урон, `InstigatorTags` несут `DamageType` — задел, в расчёте не читается). Формулы MP/Balance в GA не тронуты.
- **Recovery-анимация ≠ тег `State.ComboRecovery`.** Анимация берётся из завершившейся цепочки (`RecoveryMontage`, nullptr = хвост запечён в удар) и играет всегда после терминального удара, кроме выхода из стойки посреди удара. Тег — геймплейный лок-аут НОВЫХ атак И входа в стойку (не выхода), вешается на терминале ТОЛЬКО если Recovery-монтаж реально стартовал: длительность ровно
  `RecoveryMontage->GetPlayLength()`, без долей и добавок. Тюнинговых полей (`RecoveryLockFraction`, `ComboRecoveryDuration`) больше нет. Единая точка завершения — `EndSequenceWithRecovery`.
- **Диспетчер зон поражения — `UClanhallHitboxComponent`.** Заменил жёстко зашитый трейс оружия. Собственной
  геометрии не имеет: кость, смещение, поворот, форма (сфера/капсула/бокс), размеры и роль каждой зоны приходят из `FClanhallHitboxDesc`, заданного на `AnimNotifyState_Hitbox` конкретного монтажа. Одновременно
  может быть открыто несколько зон, у каждой свой набор задетых целей (`AlreadyHit`) — поэтому «каждый враг на пути рывка — ровно один раз» получается из данных, без отдельного сбора целей по траектории.
  Пара Begin/End связывается по указателю на нотифай, а не по хендлу: `UAnimNotifyState` — разделяемый const-объект, состояние конкретного проигрывания на нём жить не может. Отладочная отрисовка — `bDrawDebugHitboxes` (форма + заметённый путь + точки попадания), без неё размечать зоны вслепую.
- **Инвариант «один резолвер на актора».** `Event.Hitbox.Hit`/`Event.Hitbox.Closed` летят на весь ASC без адресации, и система корректна ровно потому, что в любой момент урон по контакту резолвит не более
  одной способности. Держится на порядке вызовов: `ActivateStep` закрывает зоны предыдущего шага ДО активации следующей способности, а активка гасит серию ДО того, как подпишется на события.
  `Event.Hitbox.Closed` шлётся СТРОГО на переходе «были зоны → зон не осталось»: вызов `End*` по пустому списку оборвал бы способность, которая активировалась, но зону ещё не открыла.
- **Поза стойки — в данных класса.** `UComboData::StanceAnim` (`UAnimSequence`, не монтаж — стойка это поза в State Machine, а не монтаж через слот). ABP тянет её через `AClanhallCharacter::GetStanceAnim` в Sequence
  Player состояния `CombatStance`. Магическая и антимагическая стойки сюда НЕ попадают — они глобальные, одни на все классы (Раздел 9).
- **Отмена серии чужим монтажом.** `upperbody` и `fullbody` делят `DefaultGroup`, поэтому активка Q/E/R/F прерывает удар-монтаж и без мер заставила бы серию залипнуть. `CancelSequenceForExternalMontage()` гасит
  состояние без Recovery-анимации и без `State.ComboRecovery` (чужой монтаж уже занимает слот, и наказания за прерывание нет — тот же принцип, что у невалидного продолжения). Монтаж не останавливает:
  `Montage_Play` вызывающего сам его перебьёт.
- **Страховки состояния.** Серия сбрасывается и по концу окна, и по концу удар-монтажа (делегат) — даже если нотифая на монтаже нет. `ResetCombo` гасит `bReadWindowOpen`. Зоны поражения закрываются (`ForceEndHitboxes`) при конце/прерывании своего монтажа и при выходе из стойки — но только свои:
  делегат конца монтажа сверяет идентичность с `LastPlayedMontage` и отсекает устаревшие вызовы (он приходит асинхронно, после блендаута, когда владелец зон уже сменился).
- **Гейт на данные:** нет цепочки-опенера на направление → соответствующий WASD не бьёт вообще.
- **Класс-нейтральность данных** (задел под ИИ босса): ключ хода `EClanhallAttackDirection` нейтрален к источнику ввода, player-специфики в `UComboData` нет. Враг сейчас идёт через `GA_EnemyWASDSeries`.

**Служебные теги раздела:** `State.ComboRecovery` (лок-аут новых атак и входа в стойку ровно на время Recovery-анимации, только если она реально стартовала), `Event.DirectionalAttack`(несёт `BaseDamage` от компонента к абилке — не notify, выбор способности не гейтит).

**Разметка нотифай-стейтов (общее для всех окон):** Montage Tick Type = **Branching Point** — обычные нотифаи обрабатываются на границе тика и на низком FPS съедают кадры, что заметно на окнах в 0.2–0.5 сек.
Конец стейта не ставить впритык к концу монтажа — 1–2 кадра запаса, иначе `NotifyEnd` попадёт в блендаут.
У зоны с `bParryable = false` (активки Q/E/R/F, Порция C) направление удара не читается вовсе — ставить `SetCurrentDirection` для таких монтажей не требуется.
У шага без Hitbox-нотифая урон резолвится мгновенно на активации (фолбэк, Порция D) — разметка `AnimNotifyState_Hitbox` обязательна только когда нужен контактный резолв.

**Рабочее правило:** числа в доках — плейсхолдеры, менять свободно. Структура и архитектура — нет. Если для монтажа нужно то, чего нет в коде (новый Notify, поле, сигнал) — не собирать в BP втихую, а вернуться с конкретным вопросом. Анимация и код переключаются с интерима на финал одновременно (Блок E2).

**Правило на каждый новый монтаж.** Любой новый источник `Montage_Play` в `DefaultGroup` (реакции на удар, стан, касты) обязан сначала гасить серию через `CancelSequenceForExternalMontage()` — и делать это ДО подписки на `Event.Hitbox.*`, иначе собственный `Closed` от закрываемых зон оборвёт способность до контакта.

**Проверка:** в стойке W→A играет цепочку с разными клипами; невалидное продолжение гасит серию без урона и сдвига шкал; удар на потолке `ClassRank` вешает лок-аут; правка `UComboData` меняет дерево без перекомпиляции.
#### Этапы

##### Блок A — Фундамент
**Статус:** ✅ Готово.

1. `[Редактор]` Сокет `WeaponSocket` на скелете оружия игрока (и врага, если у него отдельная модель оружия) — его ждёт `WeaponTraceComponent`.
2. `[Редактор]` ABP: `Main States` (локомоция + прыжок) и два слота — `upperbody` и `fullbody`, склейка `Layered Blend Per Bone` по `spine_01`. Слота `Cast` НЕТ — решение отложено (п.22), Q/E идут через `upperbody`. Канон структуры — `locomotion_structure.md`.
3. `[Редактор]` Центровка pelvis по X/Y относительно root на mocap-клипах — свой `UAnimationModifier` (`AM_CenterPelvisXY`), прогоняется на клипе. По Z общего модификатора нет — только визуальная проверка поклипно. 

##### Блок B — Монтажи игрока и нотифаи
**Статус:** 🔜 — по таблице ниже остались два перехода.

4. `[Редактор]` Четыре монтажа-опенера из стойки (W=Overhead, D=RightSlash, A=LeftSlash, S=LowSweep) — строка `Stance` в таблице ниже. Опенер режется именно из стойки: удар из середины дубля на эту роль не годится.  

**Mocap**

https://github.com/user-attachments/assets/1a1860be-d227-4b33-a482-516d13a3bf6f

**Sequencer**

https://github.com/user-attachments/assets/30800fd3-17b4-4992-8a20-a2b77ff6b3f6

**Статус Комбо Анимаций Knight**
Строка = откуда, колонка = куда. Строка `Stance` — опенеры (`FromStance`), колонка `Stance` — возврат в стойку (`Recovery`), остальное — переходы между ударами (`From*`). ✅ готово · 🔜 в работе · ❌ невозможно по дизайну (повтор направления не представлен в типах, заполнять нечего). Таблица — прямая карта слотов `UComboData` для п.9.

| col/row | Stance |  W  |  A  |  S  |  D  |
| :------ | :----: | :-: | :-: | :-: | :-: |
| Stance  |   ❌    |  ✅  |  ✅  |  ✅  |  ✅  |
| W       |   ✅    |  ❌  |  ✅  |  ✅  | 🔜  |
| A       |   ✅    |  ✅  |  ❌  |  ✅  |  ✅  |
| S       |   ✅    |  ✅  |  ✅  |  ❌  | 🔜  |
| D       |   ✅    |  ✅  |  ✅  |  ✅  |  ❌  |

5. `[Редактор]` `AnimNotifyState_WeaponTrace` вокруг фазы контакта — на КАЖДОМ удар-монтаже (опенеры и переходы), не на Recovery. Montage Tick Type = **Branching Point**.  
6. `[Редактор]` `AnimNotifyState_ComboWindow` на каждом удар-монтаже: Begin ≈ 60%, End ≈ 90%, причём End ПОСЛЕ окна контакта — иначе следующий шаг срежет собственное попадание. На Recovery не ставится: он ввод не читает.
7. `[Редактор]` Остальные монтажи матрицы: 12 переходов между ударами (по 3 из каждого направления) с теми же двумя нотифай-стейтами из п.5–6, плюс 4 Recovery-хвоста «узел → стойка» без нотифаев. Итого по блоку — 20 монтажей: 4 опенера + 12 переходов + 4 Recovery. Клип адресуется ПАРОЙ «предыдущее направление → новое», а не полным путём: `A→W` один на все ветки, откуда бы в `A` ни пришли — второй вариант того же перехода резать некуда. Нарезка узловыми позами и правило «резать из самого длинного дубля» — `combo_system.md`.

##### Блок C — Data Asset комбо-оружия
**Статус:** 🔜 — структура заполняется по мере готовности монтажей (п.7).

8. `[Редактор]` Создать `UComboData` — один ассет на класс. Заполнить профиль урона — четыре обязательных поля `Overhead`/`RightSlash`/`LeftSlash`/`LowSweep` (`BaseDamage` + `DamageType`-заглушка, в расчёте пока не читается) — и `StanceAnim` (loop-поза боевой стойки, `UAnimSequence`). Назначить ассет в поле `ComboData` на BP-персонаже, там же выставить `ClassRank` — потолок длины серии живёт на персонаже, не в ассете.
9. `[Редактор]` Разложить монтажи по слотам переходов по таблице из п.4: `FromStance` (4 опенера), `FromOverhead`/`FromLeftSlash`/`FromRightSlash`/`FromLowSweep` (по 3) и `Recovery` (4 по последнему направлению серии) — 20 слотов, один к одному с монтажами Блока B:
    - Пустой слот — легальное состояние, а не недоделка: `nullptr` в `From*` = продолжение запрещено → серия уходит в Recovery; `nullptr` в `FromStance` = этим направлением серию не начать; `nullptr` в `Recovery` = хвост запечён в сам удар-монтаж.
    - Повтора направления (W→W и т.д.) в типах нет — слота просто не существует, заполнять нечего и ошибиться негде.
    - Ни Data Table ходов, ни списка цепочек нет: клип выбирается парой направлений, история серии до предыдущего шага в резолве не участвует.

##### Блок D — Активки Q/E/R/F (Knight)
**Статус:** 🔜 — код готов полностью (Раздел 4), всё оставшееся — редакторное. До заполнения навыки работают через мгновенный фолбэк.

10. `[Редактор]` `CastMontage` в заголовке `UAbilityData` каждого из Q/E/R/F. Пока поле `nullptr`, навык резолвится мгновенным фолбэком (Раздел 4) — механика работает, но зона не участвует.
    - **Разметка зон.** На каждом каст-монтаже поставить `AnimNotifyState_Hitbox` вокруг фазы контакта с `bParryable = false` (активки в клэше не участвуют, Раздел 5). Форма — своя на навык: Power Strike — капсула вдоль клинка; Shield Slam — сфера на кости щита, малый радиус (не дотянулся — не попал, и это видно в редакторе); Shield Charge — капсула на пелвисе на весь рывок (см. п.12); War Shout и подобные — большая сфера на `root`, без привязки к оружию. С этого момента навык сам переключается с мгновенного резолва на контактный — кода менять не нужно.  

    (п.11 зарезервирован — пропущен при нумерации, свободный слот под будущий пункт блока.)
11. `[Редактор + дизайн]` Root Motion: **Shield Charge (R)** — рывок 3–4 шага. Зона — капсула на пелвисе, живущая весь рывок: multi-hit получается из данных (`AlreadyHit` ведётся на зону, каждый враг на пути получает ровно один раз), отдельного кода под это нет.
12. `[Редактор]` Проставить `CounterTag` в 4 Knight `UAbilityData` (`Ability.Skill.Knight.ShieldSlam / PowerStrike / ShieldCharge / Retribution`) — сейчас пусто во всех четырёх. Это **идентичность** навыка («чем я контрю»), не список целей: набор контрящих живёт на защищающемся навыке (Раздел 6).
##### Блок E1 — Плейсхолдер-анимации и нотифаи врага (редактор)

14. `[Редактор]` Placeholder-анимации Часового (Training Dummy) и Стража.  
15. `[Редактор]` На ударах серий врага: `AnimNotifyState_Hitbox` (для клэша парирования через зону игрока, `bParryable = true`).  

16. `[Редактор]` На ударах серий врага: `AnimNotifyState_ParryWindow` (Begin ~20%, End ~80%). Montage Tick Type = **Branching Point** — окно короткое, терять кадры нельзя.  

##### Блок E2 — Переключение интеримов код↔нотифай

17. `[C++]` Когда монтажи врага с Parry-нотифаями из п.16 готовы → убрать интерим-строку `ApplyTimedTag(SelfASC, State_Parrying, WindowDuration)` в `GA_EnemyWASDSeries::PrepareHit`. Иначе окно `State.Parrying` откроется дважды.  
18. `[C++]` Когда появится реальный монтаж Power Strike с `AnimNotifyState_CounterWindow` → убрать интерим `OpenWindow/CloseWindow` из `GA_EnemyActiveSkill.cpp`. Иначе окно контрнавыка задвоится.

##### Блок F — Слоты и состояния анимации

19. `[Редактор]` Выставить слоты в монтажах: `DefaultSlot` → **`upperbody`** на всех WASD-ударах, продолжениях и Recovery-хвостах; **`fullbody`** на Q/E/R/F. Канон — `locomotion_structure.md §3`. Типовая ошибка: монтаж остался в `DefaultSlot` → логика работает, анимации не видно.  
20. `[Редактор + дизайн]` Решить по слоту `Cast`. Собрано два слота (`upperbody`, `fullbody`), Q/E сидят на `upperbody`. Если отдельный приёмник под заклинания нужен — создать в Anim Slot Manager и добавить ноду `Slot 'Cast'` в AnimGraph.
21. `[Редактор]` Боевая стойка отдельным состоянием `CombatStance` в Main States. Sequence Player, Loop=true, пин `Sequence` промоутнут в переменную `CurrentStanceAnim`, заполняется из `Character->ComboData->StanceAnim` в `NativeInitializeAnimation` (и при свапе оружия — Раздел 10, там же меняется `ComboData`). Переход Idle/Locomotion ↔ CombatStance по булю `bInStance` = `HasMatchingGameplayTag(State.InStance)`, читать в `Event Blueprint Update Animation` (НЕ Thread Safe — обращение к ASC из воркер-треда небезопасно). Duration перехода 0.18 в обе стороны = `StanceExitBlendOutTime` в комбо-компоненте, чтобы на выходе слот `upperbody` и состояние доехали одновременно. Ноги берутся из этого состояния (Base Pose = `lowerbody`), руки — из слота `upperbody` поверх; монтаж кончился/оборван → слот пуст → руки сами вернулись в стойку (без кода возврата). Поле `StanceAnim` в `UComboData` уже есть (см. «Поза стойки — в данных класса» выше), дожидаться кода не нужно. Структура AnimGraph не меняется, меняется наполнение Main States.

---
### Раздел 8 — Противники

#### Часовой (Dummy)

Блок  — Монтажи врага и его нотифаи

14. `[Редактор]` Placeholder-анимации Часового (Training Dummy) и Стража.  
15. `[Редактор]` На ударах серий врага: `AnimNotifyState_WeaponTrace` (для клэша парирования через trace игрока).  
16. `[Редактор]` На ударах серий врага: `AnimNotifyState_ParryWindow` (Begin ~20%, End ~80%). Montage Tick Type = **Branching Point** — окно короткое, терять кадры нельзя.  

**Характеристики:** AP 150 / HP 300 / MP 0 / Charges 2 / DT 8

**Что делаем:**
- Behavior Tree или простой State Machine
- Две WASD-серии: A→D («Перекрёстный»), W→W («Сверху дважды»)
- «Сверху дважды» появляется при HP < 60%
- Активный навык: **Power Strike** (идентичный тег Ability.Skill.Warrior/Knight → контрнавык работает)
- Накладывает на игрока BROKEN GUARD при попадании
- При HP < 50%: Power Strike после каждой второй серии, агрессия если BROKEN GUARD на игроке
- Пауза 1.5 сек после промаха Power Strike

**Результат:** первый полноценный боевой цикл с противником. Парирование, контрнавык, метка на себе — всё проверяется.

#### Стартовый босс (Старый Страж)

**Характеристики:** AP 300 / HP 700 / MP 80 / Charges 4 / DT 12

**Что делаем:**

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

**Результат:** полный тестовый босс. Все системы до магии проверены. Первое знакомство с кастом.

---
### Раздел 9 — Магическая система

#### Прототип магической системы

![spells_prototype](https://github.com/user-attachments/assets/82a408f8-28a7-425d-aa9c-2fc725c807c9)

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

##### Три стойки — расширение боевой стойки Раздела 2

До этого раздела стойка одна (`State.InStance`, ЛКМ). Магия делает её трёхзначной. Все три стойки взаимоисключающие и все три блокируют движение — WASD/8 клавиш заняты действиями.

| Стойка | Вход | Тег | Что делают клавиши |
|---|---|---|---|
| Боевая | ЛКМ | `State.InStance` | направленные удары, комбо-дерево Раздела 7 |
| Магия | ПКМ | `State.Casting` | слоги |
| Антимагия | ЛКМ+ПКМ | `State.CastingAntimagic` | слоги, Режим А/Б |

**Стойка — чистая функция от зажатых кнопок мыши,** а не машина состояний с переходами по событиям. При каждом изменении набора зажатых кнопок стойка пересчитывается заново. Переключение между всеми тремя свободное, возврат в Idle не требуется: игрок начал каст, увидел замах босса, дожал ЛКМ — уже в боевой и парирует.

**Гейт — занятость, а не ввод.** Пересчёт заморожен, пока персонаж отыгрывает действие: висит `State.Busy`, его ставит абилка удара/каста и снимает по завершении. Гейтить по `IsAnyMontagePlaying` нельзя — между концом удар-монтажа и стартом Recovery есть кадры без монтажа, и в ту же slot-группу попадают посторонние монтажи (реакции на удар, эмоции), которые блокировали бы стойку без причины. Каст обязан ставить `State.Busy` независимо от своей механики (мгновенный, канал, прицеливание) — иначе стойка переключится посреди каста.

**Частичное отпускание — не событие, а новое значение аргумента.** Отпустил ЛКМ из антимагии, ПКМ держит → как только `State.Busy` спадёт, окажется в магии. Отдельной обработки не требует.

**Отпущены все кнопки мыши — единственное сквозное событие,** работает поверх `State.Busy`: стойка снимается, низ уходит в локомоцию. Recovery доигрывает в своём слоте (upperbody) поверх локомоции — не обрывается выходом из стойки; стык между Recovery и бегом закрывается блендаутом слота, отдельного клипа под каждую цепочку не нужно. Живой удар-монтаж (в отличие от Recovery) обрывается `Montage_Stop` с блендаутом. Канон — `UClanhallComboComponent::OnStanceExit` (Раздел 7), то же поведение распространяется на все три стойки.

**Анимация:** три состояния (боевая/магия/антимагия) в Main States — там же, где боевая стойка Раздела 7 (п.23), полный меш переходов между ними; нейтраль = обычная локомоция, отдельного `Relaxed`-состояния нет. Источник ассета асимметричен: боевая тянется из `ComboData.StanceAnim` через переменную (per-class — позиция оружия своя у каждого класса), магия и антимагия захардкожены в своих Sequence Player (глобальные, одни на все классы). Кратковременный проход через боевую при «ЛКМ → ПКМ через 30 мс» гасится инерциализацией 0.15–0.2 с — окна разрешения ввода не нужны. Длительность переходов держать близкой к блендауту слота `upperbody`, иначе на стыке ступенька.

**Новые теги раздела:** `State.Stance` (umbrella — вешают все три стойки-абилки дополнительно к своему; `AClanhallCharacter::DoMove` гейтится по нему вместо перечисления трёх, теги не переименовываем — добавляем), `State.Busy` (лок пересчёта стойки на время отыгрыша действия).

**Результат:** игрок открывает гримуар, кастует Fire Spark, может сделать антимагию на Стража.

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