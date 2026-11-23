// Clanhall — единая точка объявления GameplayTags.
// Канон: теги закладываются один раз и полностью (см. CLAUDE.md).
// Дописывать новые теги можно. Переименовывать существующие — нельзя, это ломает весь GAS-граф
// (ассеты хранят тег строкой FName, ссылка порвётся молча). Удалять неиспользуемый тег можно —
// сначала проверить Reference Viewer (Project Settings → GameplayTags → поиск ссылок). Мёртвые
// теги «заделом» не держим: завести заново — одна строка, а тег с устаревшим комментарием врёт.

#pragma once

#include "NativeGameplayTags.h"

namespace ClanhallGameplayTags
{
	// ---- Ability.* ----
	// Корневые теги веток навыков + листовые теги навыков Knight.
	// Листья других классов добавляются вместе с самими навыками.
	// Листья нужны для контрнавыка: детектор сравнивает активный тег врага с известными.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Knight);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Knight_ShieldSlam);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Knight_PowerStrike);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Knight_ShieldCharge);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Knight_Retribution);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Warrior);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Assassin);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Lancer);

	// ---- State.* ----
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Casting);
	// Антимагия: механика снята в P4, тег объявлен до проверки Reference Viewer.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CastingAntimagic);
	// Окно парирования встречным замахом: механика снята в P4, тег объявлен до проверки Reference Viewer.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Parrying);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CounterWindow);
	// Статичная стойка: механика снята в P4, тег объявлен до проверки Reference Viewer.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_InStance);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Stunned);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Knockdown);
	// Лок-аут после серии WASD: механика снята в P4, тег объявлен до проверки Reference Viewer.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_ComboRecovery);
	// Активка в фазе коммита: живёт от активации до конца каст-монтажа. Пока висит — нельзя
	// запустить вторую активку, уйти или присесть: начатый приём доигрывает целиком (`CombatSystem.md`).
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_SkillCommitted);
	// Вешает и снимает UClanhallCombatStateComponent, пока в радиусе есть живой противник
	// (`CombatSystem.md`, «Боевое состояние»). Снятие идёт с задержкой — не в момент, когда
	// последний противник покинул радиус.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_InCombat);
	// Лок-аут после ухода/рывка/приседа - общий для всех трёх (`CombatSystem.md`), длительность
	// ровно длина Recovery-монтажа конкретного действия. Вешается, только если монтаж реально
	// стартовал; блокирует любую активацию защиты - тег защищает сам класс действий от спама.
	// Было State.DodgeRecovery - переименован вместе с расширением словаря защиты.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_EvadeRecovery);
	// Shift зажат - бег (`CombatSystem.md`). Вешает/снимает AClanhallCharacter::StartSprint/StopSprint.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Sprinting);

	// ---- Attack.Direction.* ----
	// Направление замаха серии WASD для клэша: механика снята в P4, тег объявлен до проверки Reference Viewer.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Direction_W);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Direction_S);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Direction_A);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Direction_D);

	// ---- Slot.* ----
	// Слот принадлежит клавише, а не конкретному навыку (`AbilitySystem.md`, «Слоты активных навыков») — общий для всех
	// оружий, ключует UWeaponTypeData::Skills и живёт как динамический тег спека
	// (FGameplayAbilitySpec::GetDynamicSpecSourceTags), UAbilityData его не хранит
	// (`CharacterHierarchy.md`, «Ключ по слоту, а не по имени навыка»). Корень нужен GA_PhysicalSkill::GetAbilitySlotTag, чтобы
	// отфильтровать слот среди прочих динамических тегов спека.
	// Корень свой, не под Ability: слот про клавишу, а не про идентичность навыка —
	// meta=(Categories="Ability") на полях идентичности навыка (CounterTag, CounteredBy,
	// SeenSkills, LearnedSkills) иначе предлагал бы слоты как валидное значение.
	// Корень принадлежит СЛОТАМ АКТИВНЫХ НАВЫКОВ. Слоты оружия 1–6 сегодня тегами не являются —
	// UCharacterSheetData::Loadout это TArray, слот там индекс массива. Если они когда-нибудь
	// станут тегами, им место в Slot.Weapon.*, а не в этом корне: иначе meta=(Categories="Slot")
	// на Skills начнёт предлагать и слоты оружия — та же дыра уровнем ниже.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Q);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_E);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_R);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_F);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Z);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_X);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_C);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_V);

	// ---- Denied.* ----
	// Причина отказа TryActivateAbility (OptionalRelevantTags у CanActivateAbility). Корень
	// остаётся под будущие причины. Denied.Charges — нехватка зарядов: механика снята в P4, тег объявлен до проверки Reference Viewer.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Denied_Charges);

	// ---- SetByCaller.* ----
	// Служебный тег: все наши generic GameplayEffect-классы (GE_Modify*) несут
	// ровно один SetByCaller-модификатор, поэтому им достаточно одного общего тега-слота.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Magnitude);

	// ---- Event.* ----
	// GameplayEvent-сигналы от AnimNotify к активной способности.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_ApplyMark);
	// Нёс урон серии WASD в направленный удар: механика снята в P4, тег объявлен до проверки Reference Viewer.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_DirectionalAttack);
	// Несёт направление (EClanhallEvadeDirection, EventMagnitude) от AClanhallCharacter::TriggerEvade
	// к UGA_Dodge::ActivateAbility через TriggerAbilityFromGameplayEvent - служебный, не гейтит
	// выбор способности (тот идёт по Handle).
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Evade);
	// Сигналы от UClanhallHitboxComponent к живой способности.
	// Hit: Instigator = владелец зоны, Target = задетый актор, EventMagnitude = хендл зоны
	// (подписчик может отличить свою зону от чужой). Шлётся на КАЖДУЮ задетую цель.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hitbox_Hit);
	// Closed: закрылась ПОСЛЕДНЯЯ активная зона — фаза контакта удара окончена.
	// Способность, ждавшая попадания, на этом заканчивается (попала она или нет).
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hitbox_Closed);

	// ---- Damage.Type.* ----
	// Тег типа урона на экземпляре оружия (UWeaponData::DamageType). Заглушка —
	// в расчёте урона пока НЕ используется. Ровно три листа для физического урона прототипа;
	// магический (Damage.Type.Magic.*) — отдельной веткой, пока не заводить.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Type_Slash);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Type_Pierce);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Type_Blunt);

	// ---- Perk.* ----
	// Перки и ранги владения оружием: механика снята в P4, тег объявлен до проверки Reference Viewer. Прогрессии в прототипе нет
	// (`Docs/FuturePlans/ProgressionSystem.md`).
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perk);

	// ---- Perk.Proficiency.* ----
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perk_Proficiency);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perk_Proficiency_Knight_Rank1);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perk_Proficiency_Knight_Rank2);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perk_Proficiency_Knight_Rank3);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perk_Proficiency_Knight_Rank4);

	// ---- Magic.School.* ----
	// Только корни школ. Структура рангов (Rank.*) откладывается —
	// преждевременно фиксировать форму, которая ещё не используется кодом.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magic_School_Materia);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magic_School_Elemental);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magic_School_Aether);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magic_School_Stars);

	// ---- Unit.Role.* ----
	// Роль юнита, навешивается loose-тегом на его ASC в BeginPlay (`HUD.md`).
	// Unit.Role.Boss — родитель для Humanoid/Monster: сенсор рамки (UClanhallBossSensorComponent)
	// запрашивает именно родителя, чтобы матчить оба подтипа боссов разом.
	// Unit_Role (корень) — умбрелла-тег для «это вообще участник боя» без разбора подтипа:
	// UClanhallCombatStateComponent матчит им любой Unit.Role.* разом, фракций в проекте нет
	// (`CombatSystem.md`, «Боевое состояние»; `CharacterHierarchy.md`).
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unit_Role);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unit_Role_Mob);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unit_Role_Boss);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unit_Role_Boss_Humanoid);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unit_Role_Boss_Monster);

	// ---- Mark.* ----
	// Полный канонический список из `MarksSystem.md`, «Типы меток» (33 метки + Compressed,
	// используемая в MagicSpells.md/Juggernaut-примере, но пропущенная в исходной таблице).
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Bleeding);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_OpenWound);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Disrupted);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Vulnerability);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_BrokenGuard);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_ArmorCrack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Burning);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Inflated);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Conflagration);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Soaked);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Shocked);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Electrocuted);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Frozen);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_DeepFreeze);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Lifted);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Crushed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Grounded);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Slow);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Pinned);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Staggered);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Distress);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Shake);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Stunned);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Shackles);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Impaled);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Feared);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Enervated);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Disarmed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Lassitude);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Sleep);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_DropBack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Bloodthirst);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_VoidTouched);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mark_Compressed);
}
