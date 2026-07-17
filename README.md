# Clanhall

# План Разработки Прототипа — Технический документ

## Стек и архитектурные решения

**Engine:** Unreal Engine 5  
**Плагин:** Gameplay Ability System (GAS)  
**Паттерн данных:** DataAsset + Fragments (композиция через данные)  
**Язык:** C++ для ядра, Blueprint для быстрого прототипирования навыков

---
## Статус виджетов

| #   | Виджет                                     | Статус      | Держит |
| --- | ------------------------------------------ | ----------- | ------ |
| 1   | Фундамент GAS и атрибуты                   | ✅ Готово    |        |
| 2   | Боевая стойка и WASD-удары                 | ✅ Готово    |        |
| 3   | Система меток                              | ✅ Готово    |        |
| 4   | DataAsset, Fragments, первые навыки Knight | ✅ Готово    |        |
| 5   | Парирование (placeholder)                  | ✅ Готово    |        |
| 6   | Контрнавык                                 | ✅ Готово    |        |
| 7   | Animation Setup                            | ✅ Готово    |        |
| 8   | Комбо-система WASD                         | In Progress |        |
| 9   | Рядовой противник                          |             |        |
| 10  | Стартовый босс                             |             |        |
| 11  | Магическая система                         |             |        |
| 12  | Смена оружия                               |             |        |
|     |                                            |             |        |

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

### Раздел 1 — Фундамент GAS и атрибуты

**Что делаем:**
- Подключение плагина GAS
- `AbilitySystemComponent`на Character и Enemy
- `UClanhallAttributeSet`: AP, HP, MP, Charges, Balance (float −100..+100)
- Таксономия GameplayTags (таблица выше)
- Минимальный HUD

---
### Раздел 2 — Боевая стойка и WASD-удары

**Что делаем:**
- Переключение стойки: LMB hold = стойка, release = обычное движение
- 4-directional WASD как атаки в стойке (W/A/S/D = 4 направления удара)
- AP-обмен: снимаешь AP врага → 50% возвращается в свой AP
- MP восстановление: STR-удар +10, DEX-удар +5
- Сдвиг Balance: STR-удар +5..+15, DEX-удар −5..−15
- Пассивный дрейф Balance к 0: 2 ед/сек без активных ударов

**Результат:** подходишь к болванчику, бьёшь в стойке, AP туда-обратно, MP растёт.

---
### Раздел 3 — Система меток

**Что делаем:**
- Наложение метки: `GameplayEffect` с Duration 5 сек, тег на `ActorComponent`. **GameplayEffect метки должен хранить SourceASC (владельца)** — для различения собственной метки игрока (можно перенести попаданием) и вражеской (атакой не снимается, см. mark_system.md §3).
- Максимум 1 метка на участника: новая перезаписывает старую (RemoveEffect старой, ApplyEffect новой)
- Проверка синергии при попадании навыка: есть ли нужный тег на цели
- Активация: метка сгорает → эффект срабатывает → навык кладёт свою новую метку
- Генерация +2 Charge при успешной синергии
- Метка на игроке как независимый трек: босс попал активкой → тег на игроке, второй удар → дебафф
- HUD: цветной индикатор типа метки + таймер 5 сек над врагом

**Результат:** вручную вешаешь метку, вторым навыком активируешь, синергия срабатывает, Charge добавляется.

---
### Раздел 4 — DataAsset, Fragments, первые навыки Knight

**Что делаем:**
- `UAbilityData` (DataAsset) и `UAbilityFragment` (базовый класс)
- Все фрагменты из таблицы выше: Animation, VFX, SFX, Damage, MarkApply, MarkTrigger, Balance
- `FindFragment<T()` хелпер
- Грант навыков с DataAsset на Character через GAS
- Knight Ранг 1–2: Shield Slam (Q), Power Strike (E), Shield Charge (R), Retribution (F).«КД по тиру: Q/E=10, R/F=20 (для прототипа Knight Q/E/R/F → 10/10/20/20)».
- КД только при confirmed hit (не при активации)
- Charges cost для R/F = 2

**Результат:** нажимаешь Q — Shield Slam, метка накладывается, синергия из Раздела 3 срабатывает. Цифры меняются в DataAsset без перекомпиляции.

---
### Раздел 5 — Парирование (placeholder)

**Канонический дизайн (combat_system.md §5):** Clash Detection — weapon trace игрока пересекается с
weapon trace врага в активном окне анимации (AnimNotify 20–80%), без UI-индикаторов, только чтение анимации.
Полная реализация — в Разделе 6.5 (AnimNotify_ParryWindowStart/End).

**Что делаем в этом разделе (placeholder до Раздела 6.5):**
- `UGA_EnemyWASDSeries` — AI-способность серии ударов с задержкой между шагами (AbilityTask_WaitDelay)
- `UClanhallParryComponent` на Character — флаг `bParrySuccessful`, `TryParry(tag)`, `ResetParry()`
- Placeholder-окно: фиксированные `WindowDuration = 0.5 сек` вместо AnimNotify-фреймов
- Placeholder-проверка: WASD-клавиша игрока в обработчике ввода вместо weapon trace пересечения
- `ApplyTimedTagToTarget(SourceASC, TargetASC, Tag, Duration)` — AI вешает `State.Parrying` + `Parry.Incoming.*` на игрока
- Успех: AI оглушён (`State.Stunned` 2 сек) + КД навыков игрока −5 сек (`ReducePlayerCooldowns` через `MakeQuery_MatchAnyOwningTags`)
- Неуспех: урон по AP игрока (`ResolveStandardDamage`)
- Правило «всё или ничего»: серия 1–4 удара; `State.ComboRecovery` после завершения серии
- `UGA_Series_Crosscut` — конкретная серия A→D Часового, гранится Training Dummy с таймером 4 сек

**Результат:** болванчик делает A→D, игрок жмёт D→A — парирует. Пропустил один — нет эффекта.

---
### Раздел 6 — Контрнавык

**Что делаем:**
- Распознавание контратаки по совпадению навыка в окне State.CounterWindow активки врага (без модификатора)
- Детектор активной `GameplayAbility` врага по тегу навыка
- Окно контратаки: открывается в начале анимации навыка врага
- Успех: `CancelAbility` врага без применения Cost и Cooldown Effect
- Успех для игрока: Charges не расходуются, КД не уходит, навык можно применить сразу
- Неуспех: навык игрока активируется обычным образом, врага не прерывает

**Результат:** враг начинает Power Strike → игрок LMB+E → навык врага прерван, у игрока он готов.

---
### Раздел 6.5 — Animation Setup

#### Комбо-система WASD (монтажи, окна, ворота ввода)

 **Комбо-система WASD — реализована в C++. Полный канон механики — `combo_system.md`.
 
 - **Модель данных — один Data Asset `UComboData` с тремя прямыми полями (не фрагменты, не единая таблица путей):**
   - `DamageProfile` (`FDirectionalDamage`) — `BaseDamage` + `DamageType`-
     заглушка на НАПРАВЛЕНИЕ (не на комбо-запись — не дублируется по цепочкам).
   - `Moves` (`FComboMove`) — база ходов класса: `MoveId` (стабильный идентификатор,
     НЕ индекс массива) → `Direction` + `Montage`. Несколько ходов могут делить одно `Direction` (общий
     и уникальный удар одного направления — различаются `MoveId`).
   - `Chains` (`FComboChain`) — цепочки: `Steps` — массив `MoveId` (не направлений).
     Последовательность направлений выводится из `Move.Direction` каждого шага. `RecoveryMontage` — свой
     на КАЖДУЮ цепочку (не общий на весь ассет). `RequiredUnlock` — задел на перки.
 - **Резолв шага** (`UClanhallComboComponent::ResolveChain`) — направление ВАЛИДИРУЕТ путь, `MoveId`
   ВЫБИРАЕТ клип (инвариант, не сводить обратно к «клип по направлению»). По кандидат-пути направлений
   ищутся все цепочки дерева с совпадающим префиксом, результат делится на `ExactMatches` (цепочка длиной
   ровно N — терминируется на этом шаге) и `PrefixMatches` (цепочка длиннее — кандидат её промежуточный
   узел, дальше идёт ветвление).
 - **Коллизия vs ветвление — не путать.** Несколько `PrefixMatches` с общим префиксом, расходящихся
   дальше по дереву, — нормальное ветвление, не ошибка данных (Warning на каждом шаге ветвления был бы
   багом). Коллизия (несколько цепочек с ПОЛНОСТЬЮ одинаковой последовательностью направлений) разрешается
   ТОЛЬКО среди `ExactMatches`: приоритет у записи с непустым И выполненным `RequiredUnlock` (перк-системы
   нет — заглушка: непустой тег всегда «не выполнен», на практике выигрывает первая запись с пустым
   условием) + `UE_LOG` Warning при реальной неоднозначности (вне shipping, явно не падает).
 - **Потолок длины серии = `AClanhallCharacter::ClassRank`** (0–4, плейсхолдер до системы прокачки, живёт
   на персонаже, не общий предок с врагом — враг использует отдельный `GA_EnemyWASDSeries`), не поле
   ассета, как раньше. Дерево может содержать записи длиннее ранга — они просто недоступны.
 - **Урон — из профиля, не с абилки.** `RawDamage` на `GA_DirectionalAttackBase` убран. Компонент
   резолвит `BaseDamage` по направлению шага ДО активации и передаёт через
   `ASC-TriggerAbilityFromGameplayEvent(Handle, ActorInfo, Event.DirectionalAttack, &EventData, *ASC)` —
   `EventData.EventMagnitude` = урон, `InstigatorTags` несёт `DamageType` (задел, в расчёте пока не
   читается). Handle-активация (Часть B1, инверсия потока) сохранена — тег события служебный, не гейтит
   выбор абилки.
 - **Ворота ввода, не буфер:** до окна чтения нажатия отбрасываются и не копятся; в окне запоминается
   только последнее («последнее решает»). Нет накопления → нет залипания отзывчивости.
 - **Опенер / продолжение / игнор:** из нейтрали любое нажатие — опенер, если есть цепочка из одного шага
   с этим направлением (по соглашению `Steps[0]` каждой цепочки — «из стойки», отдельного значения enum
   для стойки нет). В живом комбо нажатие валидируется по дереву: валидное — монтаж + урон/MP/Balance;
   невалидное — игнор без урона и сдвига шкал, серия завершается. Лок-аута за ошибку нет.
 - **Recovery-анимация ≠ тег `State.ComboRecovery`.** Recovery-анимация теперь берётся из КОНКРЕТНОЙ
   завершившейся цепочки (`FComboChain.RecoveryMontage`, опционально; nullptr = хвост запечён в
   удар-монтаж), не из общего поля фрагмента, как раньше — играет ВСЕГДА после терминального удара серии.
   Чтобы остановка на промежуточном узле имела свой recovery, этот узел должен быть задан отдельной
   `ExactMatch`-записью в данных — иначе recovery возьмётся из чужой длинной ветки (заметка по данным, не
   по коду). Тег `State.ComboRecovery` — геймплейный лок-аут ввода, вешается ТОЛЬКО после удара на
   потолке ранга. Единая точка завершения — `EndSequenceWithRecovery`.
 - **Страховка состояния:** серия сбрасывается и по концу окна, и по естественному концу удар-монтажа
   (делегат) — даже если на монтаже нет/не сработал `AnimNotifyState_ComboWindow`. `ResetCombo` также
   гасит `bReadWindowOpen` (упрочнение против незакрытого окна при форс-сбросе, напр. `OnStanceExit`).
 - **Гейт на данные:** пока в `ComboData` нет цепочек-опенеров на все 4 направления, соответствующий
   WASD НЕ БЬЁТ вообще. `Montage` хода может быть nullptr — удар/урон сработают без анимации.
 - **Класс-нейтральность данных (задел под ИИ босса):** все три фрагмента не содержат ничего
   player-специфичного (`UInputAction`, ссылок на `AClanhallCharacter`) — ключ хода `EClanhallAttackDirection`,
   нейтральный к источнику ввода (клавиша игрока или решение ИИ). Общий исполнитель для врага — отдельная
   будущая задача (ИИ босса), сейчас враг в этом резолве не участвует.
 - **ОСТАЁТСЯ (B2, deferred):** перенести фиксацию урона с активации (frame 0) на окно контакта weapon
   trace (~20–80%) — закрывает «ударил мгновенно → тут же увернулся». Пока урон на активации валидного удара.

Мувсет — направленное дерево (W/A/S/D = разные удары), длина комбо = `ClassRank` персонажа (0–4, плейсхолдер).
Не строить единый монтаж с внутренними прыжками — ветвление по направлению делает его нечитаемым.

Схема (расстановка в редакторе поверх готового C++):
- Каждый удар = отдельный AnimMontage, ссылается на себя из `FComboMove.Montage` по `MoveId`. На нём
  `AnimNotifyState_ComboWindow` — окно чтения следующего ввода: Begin ≈ 60%, End ≈ 90% (End после конца
  контакта ~80%, чтобы следующий удар не срезал своё попадание).
- Опенеры (цепочки из одного шага с `Direction` W/A/S/D) и продолжения (цепочки из нескольких `Steps`) —
  записи в `Chains`, ссылки на ходы `Moves` по `MoveId`. Опенеры
  обязательны на все 4 направления, иначе соответствующий WASD не бьёт.
- `WeaponTraceStart` (~20%) / `WeaponTraceEnd` (~80%) на ударе — те же нотифаи переиспользуются для
  клэша парирования (этот же раздел).
- Терминальный удар серии играет Recovery-хвост завершившейся цепочки (`FComboChain.RecoveryMontage` или
  запечённый в монтаж) — см. канон выше.

Notify/теги:
- AnimNotifyState_ComboWindow (Begin/End) — окно приёма следующего ввода.
- State.ComboRecovery — после удара на потолке `ClassRank`.
- WeaponTraceStart / WeaponTraceEnd — общие с парированием.
- Event.DirectionalAttack — служебный GameplayEvent-тег, несёт `BaseDamage` от компонента к абилке
  (не notify, не гейтит выбор способности).

Статус C++: ядро готово — `UComboData` (профиль урона + база ходов + дерево — прямыми полями)
(Fragments/ComboData.h, заменил фрагментную модель), `UClanhallComboComponent` (ворота ввода,
резолв ExactMatches/PrefixMatches, коллизия RequiredUnlock, опенер/продолжение/игнор,
EndSequenceWithRecovery, делегат-страховка), инверсия активации + передача урона через
`TriggerAbilityFromGameplayEvent` в `GA_DirectionalAttackBase`, `ClassTag`/`ClassRank` на
`AClanhallCharacter`, выход из стойки (Montage_Stop + сброс). Осталось: редакторные ассеты (блоки ниже,
переразметка старых DataAsset'ов на `UComboData` — не C++, работа в редакторе) + B2.

- Настройка ABP: Locomotion SM + UpperBody/FullBody/Cast слоты
- Сокеты на скелете (weapon socket для трейса)
- `AnimNotifyState_ComboWindow (окно приёма следующего удара комбо)`
- Базовый набор Notify на анимациях игрока: `WeaponTraceStart/End`, `ApplyMark`
- Базовый набор Notify на анимациях врага: `WeaponTraceStart/End`, `AnimNotify_ParryWindowStart`, `AnimNotify_ParryWindowEnd`
- Root Motion включён для R/F навыков с перемещением
- **Интеграция парирования (замена placeholder из Раздела 5):**
  - `AnimNotify_ParryWindowStart` на анимации удара врага (≈ 20%) → открывает `State.Parrying` на ASC врага
  - `AnimNotify_ParryWindowEnd` (≈ 80%) → закрывает `State.Parrying`
  - Weapon trace игрока (во время его WASD-удара) проверяется против активного weapon trace врага
  - Пересечение + противоположные направления + `State.Parrying` активен → `bParrySuccessful = true` + clash VFX (искры, звук)
  - `UClanhallParryComponent::TryParry` вызывается из weapon trace коллбэка, а не из обработчика ввода
- Placeholder анимации готовы для Часового и Стража

#### Этапы

**Блок A — Фундамент**
**Статус:** ✅ Готово.
1. `[Редактор]` Сокет `WeaponSocket` на скелете оружия игрока (и врага, если у него отдельная модель оружия) — его ждёт `WeaponTraceComponent`.
2. `[Редактор]` ABP: Locomotion state machine + слои UpperBody / FullBody / Cast.

**Блок B — Монтажи игрока и базовые нотифаи**  

3. `[Редактор]` 4 базовых монтажа WASD-ударов (W=Overhead, D=RightSlash, A=LeftSlash, S=LowSweep) — это опенеры: по ходу (`FComboMove`) в `UComboMovesFragment.Moves` на каждое направление + по цепочке из одного шага (`FComboChain.Steps = [MoveId]`) в `UComboTreeFragment.Chains`.  
4. `[Редактор]` На каждом из 4: `AnimNotify_WeaponTraceStart` (~20%) / `WeaponTraceEnd` (~80%) вокруг фазы контакта.  
5. `[Редактор]` На каждом из 4: `AnimNotifyState_ComboWindow` (Begin ~60%, End ~90%, после конца контакта).  
6. `[Редактор]` Монтажи продолжений комбо (2-й удар и далее по направлениям), с теми же двумя наборами нотифаев из п.4–5.

**Блок C — DataAsset комбо-оружия**  

7. `[Редактор]` Создать Data Asset типа `UComboData`, заполнить три поля: `DamageProfile` (`BaseDamage` на каждое из 4 направлений), `Moves` (`MoveId`+`Direction`+`Montage` на каждый ход), `Chains` (включая опенеры на все 4 направления, без них WASD не бьёт). Назначить ассет в поле `ComboData` на BP-персонаже, там же выставить `ClassRank` (потолок длины серии — больше не поле ассета).

**Блок D — Активки Q/E/R/F (Knight)**  

8. `[Редактор]` `CastMontage` в `UAnimationFragment` каждого из Q/E/R/F.  
9. `[Редактор]` `AnimNotify_ApplyMark` на кадре удара (задел на будущее — эффекта пока не даёт, ставить безопасно).  
10. `[Редактор + дизайн]` Root Motion: **Shield Charge (R)** — рывок 3–4 шага. Для **Retribution (F)** сперва уточнить у геймдизайнера, нужно ли ему перемещение, прежде чем включать.  
11. `[Редактор]` Проставить `CounterTag` в 4 Knight `UAbilityData` (`Ability.Skill.Knight.ShieldSlam / PowerStrike / ShieldCharge / Retribution`) — сейчас пусто во всех четырёх.

**Блок E — Монтажи врага и его нотифаи**  

12. `[Редактор]` Placeholder-анимации Часового (Training Dummy) и Стража.  
13. `[Редактор]` На ударах серий врага: `WeaponTraceStart/End` (для клэша парирования через trace игрока).  
14. `[Редактор]` На ударах серий врага: `AnimNotify_ParryWindowStart` (~20%) / `ParryWindowEnd` (~80%).

**Блок F — Переключение интеримов код↔нотифай (тут работаю я)**  
15. `[C++ — я]` Когда монтажи врага с Parry-нотифаями из п.14 готовы → убрать интерим-строку `ApplyTimedTag(SelfASC, State_Parrying, WindowDuration)` в `GA_EnemyWASDSeries::PrepareHit`. Иначе окно `State.Parrying` откроется дважды.  
16. `[C++ — я]` Когда появится реальный монтаж Power Strike с `AnimNotifyState_CounterWindow` → убрать интерим `OpenWindow/CloseWindow` из `GA_EnemyActiveSkill.cpp`. Иначе окно контрнавыка задвоится.

**Блок G — Чистка**  
**Статус:** ✅ Готово.
17. `[Редактор]` Удалить орфанный `IA_CounterMode` Input Action (Ctrl-путь контрнавыка убран).

---
### Раздел 7 — Рядовой противник (Часовой)

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

---
### Раздел 8 — Стартовый босс (Старый Страж)

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

**Что делаем:**
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

**Результат:** игрок открывает гримуар, кастует Fire Spark, может сделать антимагию на Стража.

---
### Раздел 10 — Смена оружия и финальная интеграция

**Что делаем:**
- Weapon slot система: клавиши 1–6
- Swap AbilitySet при смене: RemoveAbilities старого набора → GrantAbilities с нового DataAsset
- Эффекты перегруза Balance: при выходе за ±60 → GameplayEffect: навыки перегруженной стороны ×2 Charges
- Добавить второе оружие (Warrior / Assassin) для теста свопа
- Сквозной тест всего цикла: два оружия, два противника, метки в обе стороны, магия

**Результат:** рабочий прототип. Смена оружия меняет набор навыков, Balance-штраф работает, весь боевой цикл проверен.

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
## Правила разработки

- **Не начинать следующий раздел** пока текущий не запускается без ошибок
- **Не строить все фрагменты сразу** — добавлять по мере необходимости
- **Теги закладываются один раз в Разделе 1** — переименование тегов ломает весь GAS-граф
- **Механика раньше визуала** — Animation/VFX/SFX фрагменты добавляются после того как логика работает
- **Хардкод допустим в Разделах 1–3** — DataAsset появляется в Разделе 4, до этого можно проверять логику напрямую


## License
lilu_dev
MIT

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

| #   | Виджет                                      | Статус   | Держит                                                                            |
| --- | ------------------------------------------- | -------- | --------------------------------------------------------------------------------- |
| 1   | `WBP_AttributeBar`                          | ✅ Готово | HP / AP / MP полоска                                                              |
| 2   | `WBP_Diamond`                               | ✅ Готово | один ромб заряда (контур + заливка)                                               |
| 3   | `WBP_ChargesPanel`                          | ✅ Готово | ряд ромбов + засечки 2/4/6                                                        |
| 4   | Шкала Balance                               | ✅ Готово | −100..+100, два встречных бара (переехала в HUD)                                  |
| 5   | `WBP_PlayerFrame`                           | ✅ Готово | сборка 3 бара + charges (balance вынесен в HUD)                                   |
| 6   | `WBP_EnemyFrame` + `WBP_BossFrameContainer` | ✅ Готово | рамка по `Unit.Role.Boss.*` в радиусе (0..N через контейнер), адаптивные бары     |
| 7   | `WBP_Crosshair`                             | ✅ Готово | точка по `State.InStance`                                                         |
| 8   | `WBP_HUD`                                   | ✅ Готово | корень, добавляется в вьюпорт                                                     |
| 9   | Drag + Save                                 |          | перетаскивание рамок + `SaveHUDLayout` (логика в `WBP_HUD`, рамки без оверрайдов) |
| 10  | Alt-режим                                   | ✅ Готово | курсор готов, тултипы отложены                                                    |
| 11  | Тултипы                                     | 🟡       | описание поверх HUD при наведении курсора                                         |

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

### 4. Шкала Balance (−100..+100, центр 0)
**Статус:** ✅ Готово.
Обычный ProgressBar не годится — нужен центр-якорь. `Horizontal Box` → два `ProgressBar` по 50% ширины:
- левый: `Bar Fill Type = Right To Left`, `Percent = (Balance < 0) ? -Balance/100 : 0`, синий (DEX)
- правый: `Bar Fill Type = Left To Right`, `Percent = (Balance  0) ? Balance/100 : 0`, красный (STR)
Стык = ноль. Сверху вертикальная риска в центре + отметки на ±60 (граница перегруза, `combat_system.md`).

**Расположение:** шкала вынесена из `PlayerFrame` в `WBP_HUD` с якорем низ-центр экрана (глобальное состояние боя логично держать по центру, как индикатор стойки, а не в углу с ресурсами).

**Отклонённый вариант (на будущее):** «бегунок по статичной шкале» — статичный фон-градиент DEX→STR + маленький `Image`-указатель, ездящий по X от `Balance`. Рабочий, но для боя хуже: крупную заливку видно периферийным зрением, за тонкой рисочкой в замесе надо охотиться глазами. Оставлен как косметический апгрейд, механики не касается.

### 5. WBP_PlayerFrame
**Статус:** ✅ Готово.
ASC: `Get Owning Player Pawn → Cast ClanhallCharacter → Get Ability System Component`. `Event Construct`: `IsValid(PlayerASC)?` → True → `Set Target ASC` на HP/AP/MP барах (они на поллинге, сеттера достаточно) → `InitWithASC(PlayerASC)` на `ChargesPanel` (Custom Event). Цвета/`TrackedAttribute`/`MaxAttribute` баров задаются в Details каждого инстанса, НЕ из родителя.
- AP → `AP`/`MaxAP`, жёлто-золотой
- HP → `HP`/`MaxHP`, оранжево-красный
- MP → `MP`/`MaxMP`, сине-фиолетовый
**Компоновка:** HP / AP / MP — три отдельные полосы в `Vertical Box`. Нахлёст AP на верхние 20% HP убран — рядом с MP-баром смотрелся неравномерно; для прототипа стек чище и читаемее. Под барами — `ChargesPanel`. **Balance здесь НЕТ** — переехал в `WBP_HUD` (низ-центр экрана). Рамка всегда видима (никакой логики visibility, в отличие от `EnemyFrame`).

### 6. WBP_EnemyFrame + WBP_BossFrameContainer
**Статус:** ✅ Готово.

Рамку порождает/удаляет WBP_BossFrameContainer, по одной на каждого босса в радиусе (драйвер — UClanhallBossSensorComponent, двойной радиус EnterRadius/ExitRadius; НЕ рейкаст).
N боссов = N рамок.

WBP_EnemyFrame — «тупая», без сенсора и без логики видимости:
- Event Construct пустой.
- SetupForUnit(NewASC: AbilitySystemComponent) — Custom Event: Set Target ASC на HP/AP/MP →
  InitWithASC(NewASC) на ChargesPanel → HideNotUsedBars(NewASC).
- HideNotUsedBars(NewASC) — функция, Sequence 1–3: GetGameplayAttributeValue(MaxAP/MaxMP/MaxCharges)<=0
  ? Collapsed : Visible на AP-бар / MP-бар / ChargesPanel. HP всегда виден.
  Так одна рамка показывает Boss.Humanoid (HP+AP+MP+Charges), Boss.Monster (HP±Charges), Часового (MP скрыт).
- WBP_ChargesPanel.Refresh: в начале гард IsValid(TargetASC) (при churn рамок ASC часто невалиден).
  Подписки WaitAttributeChanged гасятся при удалении рамки (иначе утечка).

WBP_BossFrameContainer — Vertical Box (FramesBox) + Map<Actor, WBP_EnemyFrame (Frames):
- Event Construct: BossSensorComponent → Bind OnFrameUnitEntered/OnFrameUnitExited.
- OnFrameUnitEntered(Unit): Get ASC (Ability System BP Library, Actor=Unit) → null-check →
  Create Widget(WBP_EnemyFrame) → SetupForUnit(ASC) → Add Child(FramesBox) → Frames.Add(Unit).
- OnFrameUnitExited(Unit): Frames.Find(Unit) → погасить подписки charges → Remove from Parent → Frames.Remove(Unit).
- Вставка контейнера в боевой WBP_HUD — в Разделе 8 (пока в тестовом HUD).
- Строка с именем босса над рамкой, плашка активной метки, иконки навыков + КД (combat_system.md) — отдельным проходом позже.

### 7. WBP_Crosshair
**Статус:** ✅ Готово.
- Иерархия: `Size Box` (Width/Height Override, напр. 12×12, иначе схлопывается) → `Image` "Dot" (браш-точка; временно однотонный, если текстуры нет). Дефолтный Canvas удалён. Центрирование делает родитель (`WBP_HUD`, якорь center, alignment 0.5/0.5) — внутри самого виджета не центрируем.
- Переменная: `TargetASC` (AbilitySystemComponent, Object Ref, Instance Editable).
- **ASC приходит от родителя `WBP_HUD` через Function `SetCrosshairASC(NewASC)`** — самозахват в `Event Construct` НЕ используется (убран). Функция обычная (не Custom Event): виджет на поллинге, async-подписок нет.
- Visibility-биндинг (`GetCrosshairVisibility`): `IsValid(TargetASC)?` → False: `Collapsed`; True: `Has Matching Gameplay Tag(State.InStance) ? Visible : Collapsed`. Гард `IsValid` обязателен (ASC может быть ещё не проставлен в первый кадр).
- Без анимации, исчезает мгновенно.

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

### 10. Alt-режим — 🟡 курсор готов, тултипы отложены
**Курсор по Alt — ✅ готово.** 

Логика на PlayerController (методы `Set Input Mode` / `Show Mouse Cursor` — его). `IA_HUDCursor` (Digital bool) с двумя маппингами `Left Alt` + `Right Alt` в существующем IMC (новый контекст не добавляли). Через события Enhanced Input `Started` / `Completed` (one-shot, БЕЗ опроса в Tick и без ручного bool-флага):
- `Started` (Alt зажали): `Show Mouse Cursor = true`; `Set Input Mode Game and UI` с параметрами: **In Widget to Focus = пусто** (иначе крадёт клавиатурный фокус, глохнет WASD), **Mouse Lock Mode = Do Not Lock**, **Hide Cursor During Capture = false** (иначе курсор мигает/пропадает при клике).
- `Completed` (Alt отпустили): `Set Input Mode Game Only`; `Show Mouse Cursor = false`.

### **Тултипы
⬜ отложено (заметка на будущее):** 

`On Mouse Enter` на иконках атрибутов/навыков → `Show Tooltip Widget`. Фундамент готов — у рамок уже `Visibility = Visible`, хит-тест под курсором работает.

---

## Правила
- Не переходить к следующему виджету, пока текущий не работает.
- Механика раньше визуала.
- ASC/атрибуты в переиспользуемые суб-виджеты (`AttributeBar`, `ChargesPanel`) задаёт **родитель**, сами они ASC не достают.

## License
lilu_dev
MIT
