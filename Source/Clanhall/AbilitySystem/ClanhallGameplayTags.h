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
	// Корневые теги веток навыков + листовые теги Knight Ранг 1-2.
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
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CastingAntimagic);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Parrying);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CounterWindow);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_InStance);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Stunned);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Knockdown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_ComboRecovery);
	// Активка в фазе коммита: живёт от активации до закрытия окна контакта. Пока висит —
	// нельзя начать WASD-серию и нельзя запустить вторую активку (`combat_system.md`, «Боевая стойка и переключение режимов»,
	// «начатую активку нельзя оборвать»). Выход из стойки при этом свободен всегда.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_SkillCommitted);

	// ---- Attack.Direction.* ----
	// Тег, который владелец вешает на СЕБЯ на время удара — кодирует направление СВОЕГО
	// замаха (UClanhallComboComponent::ActivateStep), стороне-нейтрален (и игрок, и AI).
	// Раньше назывался Parry.Incoming.* — имя лгало (описывало «летит откуда-то», а не
	// «я бью туда-то»); переименован (`Parrying.md`, «`UClanhallParryComponent`»). Обратная пара для клэша:
	// W↔S, A↔D (`combat_system.md`, «Механика клэша — резолв на контакте атакующего, не на реакции защищающегося»).
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Direction_W);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Direction_S);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Direction_A);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Direction_D);

	// ---- Ability.Slot.* ----
	// Слот принадлежит клавише, а не конкретному навыку (`ability_system.md`, «Слоты активных навыков») — общий для всех
	// оружий, ключует UCharacterSheetData::Skills и живёт как динамический тег спека
	// (FGameplayAbilitySpec::GetDynamicSpecSourceTags), UAbilityData его не хранит
	// (`Combatant Hierarchy.md`, «Ключ по слоту, а не по имени навыка»). Корень нужен GA_PhysicalSkill::GetAbilitySlotTag, чтобы
	// отфильтровать слот среди прочих динамических тегов спека.
	// Мигрировано из Cooldown.Slot.* (`combat_system.md`, «Боевая стойка и переключение режимов»): слот пережил смерть
	// кулдаунов, но неймспейс Cooldown.* стал бы врать. Старые теги удалены из кода; ключи
	// существующих UCharacterSheetData-ассетов, если ещё не перенесены вручную в редакторе, ссылаются
	// на несуществующий тег — гранты активок для них молчаливо сломаны, чинится только правкой
	// ассета, не кодом.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slot);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slot_Q);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slot_E);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slot_R);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slot_F);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slot_Z);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slot_X);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slot_C);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slot_V);

	// ---- Ability.Denied.* ----
	// Причина отказа TryActivateAbility, пробрасывается в OptionalRelevantTags у
	// CanActivateAbility и долетает до UAbilitySystemComponent::AbilityFailedCallbacks
	// (`DataAsset and Fragments.md`, «Denied-фидбек (только делегат)»). Charges — единственная причина, которую HUD обязан
	// показать игроку отдельно от прочих отказов (State.SkillCommitted/State.Stunned).
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Denied_Charges);

	// ---- SetByCaller.* ----
	// Служебный тег: все наши generic GameplayEffect-классы (GE_Modify*) несут
	// ровно один SetByCaller-модификатор, поэтому им достаточно одного общего тега-слота.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Magnitude);

	// ---- Event.* ----
	// GameplayEvent-сигналы от AnimNotify к активной способности.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_ApplyMark);
	// (`Combat Stance and WASD Attacks.md`): несёт BaseDamage (EventMagnitude) от UClanhallComboComponent
	// к GA_DirectionalAttackBase через TriggerAbilityFromGameplayEvent — Handle-активация сохраняется,
	// тег тут служебный (не гейтит выбор способности, тот идёт по Handle).
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_DirectionalAttack);
	// Сигналы от UClanhallHitboxComponent к живой способности.
	// Hit: Instigator = владелец зоны, Target = задетый актор, EventMagnitude = хендл зоны
	// (подписчик может отличить свою зону от чужой). Шлётся на КАЖДУЮ задетую цель.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hitbox_Hit);
	// Closed: закрылась ПОСЛЕДНЯЯ активная зона — фаза контакта удара окончена.
	// Способность, ждавшая попадания, на этом заканчивается (попала она или нет).
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hitbox_Closed);

	// ---- Damage.Type.* ----
	// Тег типа урона на FDirectionalDamage (`Combat Stance and WASD Attacks.md`). Заглушка —
	// в расчёте урона пока НЕ используется. Ровно три листа для физического урона прототипа;
	// магический (Damage.Type.Magic.*) — отдельной веткой, пока не заводить.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Type_Slash);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Type_Pierce);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Type_Blunt);

	// ---- Perk.* ----
	// Корень перк-системы. Раньше сюда указывал FComboChain.RequiredUnlock (удалён — условие
	// было на каждой цепочке, не там, где реально нужно: см. `combat_system.md`, «Цена модели
	// пар»). Планируется вернуться как fragment на уровне конкретного хода/навыка, когда
	// несколько навыков делят один MoveId.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Perk);

	// ---- Perk.Proficiency.* ----
	// Владение типом оружия (`weapon_system.md`, «Владение оружием»). Ранги НАКАПЛИВАЮТСЯ,
	// а не перезаписываются — у бойца с рангом 3 в UCharacterSheetData::Perks висят
	// одновременно Rank1, Rank2 и Rank3. Проверка «открыт ли тир» — обычный HasTag, без
	// разбора номера из имени тега. Четыре листа на Knight — единственный тип оружия в
	// проекте; новый тип оружия заводит свои четыре, как Ability.* заводит листья
	// вместе с самими навыками.
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
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unit_Role_Mob);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unit_Role_Boss);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unit_Role_Boss_Humanoid);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unit_Role_Boss_Monster);

	// ---- Mark.* ----
	// Полный канонический список из `mark_system.md`, «Типы меток» (33 метки + Compressed,
	// используемая в magic_spells.md/Juggernaut-примере, но пропущенная в исходной таблице).
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
