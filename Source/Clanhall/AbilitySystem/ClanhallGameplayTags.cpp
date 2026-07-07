#include "ClanhallGameplayTags.h"

namespace ClanhallGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Class_Knight, "Ability.Class.Knight", "Требуемый класс: Knight (Меч и Щит)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Class_Warrior, "Ability.Class.Warrior", "Требуемый класс: Warrior (Двуручный меч)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Class_Assassin, "Ability.Class.Assassin", "Требуемый класс: Assassin (Кинжал)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Class_Lancer, "Ability.Class.Lancer", "Требуемый класс: Lancer (Копьё)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Skill_Knight, "Ability.Skill.Knight", "Корень навыков Knight");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Skill_Knight_ShieldSlam, "Ability.Skill.Knight.ShieldSlam", "Knight Q — Shield Slam (Ранг 1, КД 10 сек)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Skill_Knight_PowerStrike, "Ability.Skill.Knight.PowerStrike", "Knight E — Power Strike (Ранг 1, КД 10 сек)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Skill_Knight_ShieldCharge, "Ability.Skill.Knight.ShieldCharge", "Knight R — Shield Charge (Ранг 2, КД 20 сек, 2 Charges)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Skill_Knight_Retribution, "Ability.Skill.Knight.Retribution", "Knight F — Retribution (Ранг 2, КД 20 сек, 2 Charges)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Skill_Warrior, "Ability.Skill.Warrior", "Корень навыков Warrior");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Skill_Assassin, "Ability.Skill.Assassin", "Корень навыков Assassin");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Skill_Lancer, "Ability.Skill.Lancer", "Корень навыков Lancer");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Casting, "State.Casting", "Идёт набор слогов обычного заклинания (ПКМ зажат)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CastingAntimagic, "State.CastingAntimagic", "Идёт набор слогов антимагии (ПКМ зажат, Ctrl)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Parrying, "State.Parrying", "Окно парирования физической серии открыто");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CounterWindow, "State.CounterWindow", "Окно контрнавыка против активного навыка врага открыто");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CounterActive, "State.CounterActive", "Раздел 6: контрнавык успешен — следующий GA_PhysicalSkill без Charges и КД");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_InStance, "State.InStance", "ЛКМ зажат — боевая стойка (WASD = направленные удары)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stunned, "State.Stunned", "Оглушение после полного парирования серии");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Knockdown, "State.Knockdown", "Сбит с ног синергией метки");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_ComboRecovery, "State.ComboRecovery", "Лок-аут после максимальной серии парирования — комбо не продолжается");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_STR, "Weapon.Type.STR", "Текущее оружие относится к STR-ветке");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Weapon_Type_DEX, "Weapon.Type.DEX", "Текущее оружие относится к DEX-ветке");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Balance_Overload_STR, "Balance.Overload.STR", "Шкала DEX↔STR в зоне перегруза STR (+60..+100) — STR-навыки ×2 Charges");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Balance_Overload_DEX, "Balance.Overload.DEX", "Шкала DEX↔STR в зоне перегруза DEX (−60..−100) — DEX-навыки ×2 Charges");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parry_Incoming_W, "Parry.Incoming.W", "AI бьёт сверху (W) — игрок должен нажать S");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parry_Incoming_S, "Parry.Incoming.S", "AI бьёт снизу (S) — игрок должен нажать W");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parry_Incoming_A, "Parry.Incoming.A", "AI бьёт влево (A) — игрок должен нажать D");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Parry_Incoming_D, "Parry.Incoming.D", "AI бьёт вправо (D) — игрок должен нажать A");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Slot_Q, "Cooldown.Slot.Q", "Слот Q на перезарядке — общий для всех оружий (ability_system.md §3)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Slot_E, "Cooldown.Slot.E", "Слот E на перезарядке — общий для всех оружий");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Slot_R, "Cooldown.Slot.R", "Слот R на перезарядке — общий для всех оружий");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Slot_F, "Cooldown.Slot.F", "Слот F на перезарядке — общий для всех оружий");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Slot_Z, "Cooldown.Slot.Z", "Слот Z на перезарядке — общий для всех оружий");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Slot_X, "Cooldown.Slot.X", "Слот X на перезарядке — общий для всех оружий");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Slot_C, "Cooldown.Slot.C", "Слот C на перезарядке — общий для всех оружий");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Slot_V, "Cooldown.Slot.V", "Слот V на перезарядке — общий для всех оружий");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Magnitude, "SetByCaller.Magnitude", "Единственный SetByCaller-слот для generic GE_Modify*-эффектов (AP/HP/MP/Balance)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_ApplyMark, "Event.ApplyMark", "AnimNotify_ApplyMark отправляет этот GameplayEvent — GA_PhysicalSkill может слушать его для async-подтверждения хита");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Magic_School_Materia, "Magic.School.Materia", "Школа Материи (Q/A) — класс Artisan");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Magic_School_Elemental, "Magic.School.Elemental", "Школа Стихий (W/S)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Magic_School_Aether, "Magic.School.Aether", "Школа Эфира (E/D)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Magic_School_Stars, "Magic.School.Stars", "Школа Звёзд (R/F)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unit_Role_Mob, "Unit.Role.Mob", "Пушечное мясо: без уникальных атак, рамки нет (changelog_enemyframe_unitroles.md §1)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unit_Role_Boss, "Unit.Role.Boss", "Родитель Humanoid/Monster — сенсор рамки матчит этот тег, чтобы захватить оба подтипа");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unit_Role_Boss_Humanoid, "Unit.Role.Boss.Humanoid", "«Псевдоигрок»: навыки из пула игрока, есть AP/MP/Charges, игрок учится у него навыкам");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unit_Role_Boss_Monster, "Unit.Role.Boss.Monster", "Босс-монстр: уникальные атаки, без MP и без AP, ничему не учит; рамка HP-центричная");

	// Стрелки в mark_system.md §6 — тематические семейства (группировка по стихии/типу),
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
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Staggered, "Mark.Staggered", "Семейство 7 (контроль)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Distress, "Mark.Distress", "Семейство 8 (паника): Distress / Shake / Stunned");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Shake, "Mark.Shake", "Семейство 8 (паника)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Stunned, "Mark.Stunned", "Семейство 8 (паника) — не путать с State.Stunned");
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
