#include "ClanhallGameplayTags.h"

namespace ClanhallGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Knight, "Ability.Knight", "Корень навыков Knight");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Knight_ShieldSlam, "Ability.Knight.ShieldSlam", "Knight Q — Shield Slam");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Knight_PowerStrike, "Ability.Knight.PowerStrike", "Knight E — Power Strike");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Knight_ShieldCharge, "Ability.Knight.ShieldCharge", "Knight R — Shield Charge");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Knight_Retribution, "Ability.Knight.Retribution", "Knight F — Retribution");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Warrior, "Ability.Warrior", "Корень навыков Warrior");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Assassin, "Ability.Assassin", "Корень навыков Assassin");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Lancer, "Ability.Lancer", "Корень навыков Lancer");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Casting, "State.Casting", "Идёт набор слогов обычного заклинания (ПКМ зажат)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CastingAntimagic, "State.CastingAntimagic", "Antimagic cast. Mechanic removed in P4; tag kept until Reference Viewer check");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Parrying, "State.Parrying", "Parry window of a WASD series. Mechanic removed in P4; tag kept until Reference Viewer check");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CounterWindow, "State.CounterWindow", "Окно контрнавыка против активного навыка врага открыто");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_InStance, "State.InStance", "Static combat stance. Mechanic removed in P4; tag kept until Reference Viewer check");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stunned, "State.Stunned", "Потеря управления на противнике, источник - эффект синергии (FMarkSynergy::EffectOnTarget); на игроке не применяется (Invariants.md, «Разрешённые асимметрии»)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Knockdown, "State.Knockdown", "Сбит с ног синергией метки");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_ComboRecovery, "State.ComboRecovery", "Lockout after a WASD series. Mechanic removed in P4; tag kept until Reference Viewer check");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "The fighter is dead (HP reached zero); closes abilities, movement and input");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_WeaponSwap, "State.WeaponSwap", "Weapon swap with a weapon in hands: one beat to put it away; skills, dodge and duck are closed");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_SkillCommitted, "State.SkillCommitted", "Active skill committed: from activation to the end of the cast montage; blocks a second skill, dodge and duck");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_InCombat, "State.InCombat", "Вешает/снимает UClanhallCombatStateComponent — живой противник в радиусе, снятие с задержкой");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_EvadeRecovery, "State.EvadeRecovery", "Лок-аут после ухода/рывка/приседа - общий тег, длительность равна Recovery-монтажу конкретного действия (было State.DodgeRecovery)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Sprinting, "State.Sprinting", "Sprinting (Shift held), body faces movement");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Direction_W, "Attack.Direction.W", "WASD series swing direction for clash. Mechanic removed in P4; tag kept until Reference Viewer check");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Direction_S, "Attack.Direction.S", "WASD series swing direction for clash. Mechanic removed in P4; tag kept until Reference Viewer check");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Direction_A, "Attack.Direction.A", "WASD series swing direction for clash. Mechanic removed in P4; tag kept until Reference Viewer check");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Direction_D, "Attack.Direction.D", "WASD series swing direction for clash. Mechanic removed in P4; tag kept until Reference Viewer check");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot, "Slot", "Корень слотов активок — фильтр динамических тегов спека в GA_PhysicalSkill::GetAbilitySlotTag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_Q, "Slot.Q", "Слот Q — общий для всех оружий (AbilitySystem.md, «Слоты активных навыков»)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_E, "Slot.E", "Слот E — общий для всех оружий");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_R, "Slot.R", "Слот R — общий для всех оружий");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_F, "Slot.F", "Слот F — общий для всех оружий");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_Z, "Slot.Z", "Слот Z — общий для всех оружий");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_X, "Slot.X", "Слот X — общий для всех оружий");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_C, "Slot.C", "Слот C — общий для всех оружий");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Slot_V, "Slot.V", "Слот V — общий для всех оружий");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Denied_Cooldown, "Denied.Cooldown", "Activation denied: the skill is on cooldown");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Denied_Charges, "Denied.Charges", "Activation denied for lack of charges. Mechanic removed in P4; tag kept until Reference Viewer check");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Magnitude, "SetByCaller.Magnitude", "Единственный SetByCaller-слот для generic GE_Modify*-эффектов (AP/HP/MP)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_ApplyMark, "Event.ApplyMark", "AnimNotify_ApplyMark отправляет этот GameplayEvent — GA_PhysicalSkill может слушать его для async-подтверждения хита");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_DirectionalAttack, "Event.DirectionalAttack", "Carried WASD series damage into the directional attack. Mechanic removed in P4; tag kept until Reference Viewer check");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Evade, "Event.Evade", "Служебный тег для TriggerAbilityFromGameplayEvent - несёт направление (EClanhallEvadeDirection, EventMagnitude) от AClanhallCharacter::TriggerEvade к UGA_Dodge, не гейтит выбор способности");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Hitbox_Hit, "Event.Hitbox.Hit", "UClanhallHitboxComponent шлёт на каждую задетую цель — Instigator = владелец зоны, Target = задетый актор, EventMagnitude = хендл зоны");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Hitbox_Closed, "Event.Hitbox.Closed", "UClanhallHitboxComponent шлёт при переходе «были зоны -> зон не осталось» — фаза контакта окончена");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Type_Slash, "Damage.Type.Slash", "Тип урона — рубящий (заглушка, в расчёте не используется)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Type_Pierce, "Damage.Type.Pierce", "Тип урона — колющий (заглушка, в расчёте не используется)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Type_Blunt, "Damage.Type.Blunt", "Тип урона — дробящий (заглушка, в расчёте не используется)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perk, "Perk", "Perk root. Mechanic removed in P4; tag kept until Reference Viewer check");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perk_Proficiency, "Perk.Proficiency", "Weapon proficiency ranks. Mechanic removed in P4; tag kept until Reference Viewer check");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perk_Proficiency_Knight_Rank1, "Perk.Proficiency.Knight.Rank1", "Knight proficiency rank 1. Mechanic removed in P4; tag kept until Reference Viewer check");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perk_Proficiency_Knight_Rank2, "Perk.Proficiency.Knight.Rank2", "Knight proficiency rank 2. Mechanic removed in P4; tag kept until Reference Viewer check");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perk_Proficiency_Knight_Rank3, "Perk.Proficiency.Knight.Rank3", "Knight proficiency rank 3. Mechanic removed in P4; tag kept until Reference Viewer check");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Perk_Proficiency_Knight_Rank4, "Perk.Proficiency.Knight.Rank4", "Knight proficiency rank 4. Mechanic removed in P4; tag kept until Reference Viewer check");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Magic_School_Materia, "Magic.School.Materia", "Школа Материи (Q/A) — класс Artisan");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Magic_School_Elemental, "Magic.School.Elemental", "Школа Стихий (W/S)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Magic_School_Aether, "Magic.School.Aether", "Школа Эфира (E/D)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Magic_School_Stars, "Magic.School.Stars", "Школа Звёзд (R/F)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unit_Role, "Unit.Role", "Умбрелла-корень: «это участник боя», без разбора подтипа — UClanhallCombatStateComponent");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unit_Role_Mob, "Unit.Role.Mob", "Пушечное мясо: без уникальных атак, рамки нет (HUD.md)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unit_Role_Boss, "Unit.Role.Boss", "Родитель Humanoid/Monster — сенсор рамки матчит этот тег, чтобы захватить оба подтипа");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unit_Role_Boss_Humanoid, "Unit.Role.Boss.Humanoid", "«Псевдоигрок»: навыки из пула игрока, есть AP/MP, игрок учится у него навыкам");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unit_Role_Boss_Monster, "Unit.Role.Boss.Monster", "Босс-монстр: уникальные атаки, без MP и без AP, ничему не учит; рамка HP-центричная");

	// Стрелки в `MarksSystem.md`, «Типы меток» — тематические семейства (группировка по стихии/типу),
	// НЕ механические цепочки прогрессии. Механика всегда парная: метка → активация (правка 1.6).
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Bleeding, "Mark.Bleeding", "Семейство 1 (кровь): Bleeding / Open Wound / Disrupted");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_OpenWound, "Mark.OpenWound", "Семейство 1 (кровь)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Disrupted, "Mark.Disrupted", "Семейство 1 (кровь)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Vulnerability, "Mark.Vulnerability", "Семейство 2 (броня): Vulnerability / Broken Guard / Armor Crack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_BrokenGuard, "Mark.BrokenGuard", "Семейство 2 (броня)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_ArmorCrack, "Mark.ArmorCrack", "Семейство 2 (броня)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Burning, "Mark.Burning", "Семейство 3 (огонь): Burning / Inflated / Conflagration");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Inflated, "Mark.Inflated", "Семейство 3 (огонь)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Conflagration, "Mark.Conflagration", "Семейство 3 (огонь)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Soaked, "Mark.Soaked", "Семейство 4 (вода): источник TBD (вода/дождь/будущий спелл Стихий)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Shocked, "Mark.Shocked", "Семейство 4 (вода→электро): Soaked → Shocked → Electrocuted");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Electrocuted, "Mark.Electrocuted", "Семейство 4 (вода→электро)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Frozen, "Mark.Frozen", "Семейство 5 (вода→лёд): Soaked → Frozen → Deep Freeze");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_DeepFreeze, "Mark.DeepFreeze", "Семейство 5 (вода→лёд)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Lifted, "Mark.Lifted", "Семейство 6 (гравитация): Lifted / Crushed / Grounded");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Crushed, "Mark.Crushed", "Семейство 6 (гравитация)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Grounded, "Mark.Grounded", "Семейство 6 (гравитация)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Slow, "Mark.Slow", "Семейство 7 (контроль): Slow / Pinned / Staggered");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Pinned, "Mark.Pinned", "Семейство 7 (контроль)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Staggered, "Mark.Staggered", "Выведен из употребления (P1d: снос шкалы Stagger)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Distress, "Mark.Distress", "Семейство 8 (паника): Distress / Shake / Stunned");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Shake, "Mark.Shake", "Семейство 8 (паника)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Stunned, "Mark.Stunned", "Не назначен ни одному треку меток");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Shackles, "Mark.Shackles", "Семейство 9 (цепи): Shackles / Impaled / Feared");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Impaled, "Mark.Impaled", "Семейство 9 (цепи)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Feared, "Mark.Feared", "Семейство 9 (цепи)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Enervated, "Mark.Enervated", "Семейство 10 (истощение): Enervated / Disarmed");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Disarmed, "Mark.Disarmed", "Семейство 10 (истощение)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Lassitude, "Mark.Lassitude", "Семейство 11 (сон): Lassitude / Sleep");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Sleep, "Mark.Sleep", "Семейство 11 (сон)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_DropBack, "Mark.DropBack", "Одиночная метка, без цепочки");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Bloodthirst, "Mark.Bloodthirst", "Одиночная метка, без цепочки");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_VoidTouched, "Mark.VoidTouched", "Одиночная метка, без цепочки (Assassin R, Singularity)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Compressed, "Mark.Compressed", "Одиночная метка (Juggernaut Q, Pressure Crush, Fire Wall-синергия)");
}
