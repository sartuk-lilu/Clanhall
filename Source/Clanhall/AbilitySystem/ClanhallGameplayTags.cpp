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

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Knight_ShieldSlam, "Cooldown.Knight.ShieldSlam", "Shield Slam (Q) на перезарядке");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Knight_PowerStrike, "Cooldown.Knight.PowerStrike", "Power Strike (E) на перезарядке");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Knight_ShieldCharge, "Cooldown.Knight.ShieldCharge", "Shield Charge (R) на перезарядке");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Knight_Retribution, "Cooldown.Knight.Retribution", "Retribution (F) на перезарядке");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Magnitude, "SetByCaller.Magnitude", "Единственный SetByCaller-слот для generic GE_Modify*-эффектов (AP/HP/MP/Balance)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_ApplyMark, "Event.ApplyMark", "AnimNotify_ApplyMark отправляет этот GameplayEvent — GA_PhysicalSkill может слушать его для async-подтверждения хита");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Magic_School_Material, "Magic.School.Material", "Школа Материи (Q/A)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Magic_School_Elemental, "Magic.School.Elemental", "Школа Стихий (W/S)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Magic_School_Aether, "Magic.School.Aether", "Школа Эфира (E/D)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Magic_School_Stars, "Magic.School.Stars", "Школа Звёзд (R/F)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Bleeding, "Mark.Bleeding", "Цепочка 1, метка 1: Bleeding → Open Wound → Disrupted");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_OpenWound, "Mark.OpenWound", "Цепочка 1, метка 2");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Disrupted, "Mark.Disrupted", "Цепочка 1, метка 3");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Vulnerability, "Mark.Vulnerability", "Цепочка 2, метка 1: Vulnerability → Broken Guard → Armor Crack");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_BrokenGuard, "Mark.BrokenGuard", "Цепочка 2, метка 2");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_ArmorCrack, "Mark.ArmorCrack", "Цепочка 2, метка 3");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Burning, "Mark.Burning", "Цепочка 3, метка 1: Burning → Inflated → Conflagration");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Inflated, "Mark.Inflated", "Цепочка 3, метка 2");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Conflagration, "Mark.Conflagration", "Цепочка 3, метка 3");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Soaked, "Mark.Soaked", "Общий старт цепочек 4 и 5: Soaked → Shocked/Frozen");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Shocked, "Mark.Shocked", "Цепочка 4, метка 2: Soaked → Shocked → Electrocuted");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Electrocuted, "Mark.Electrocuted", "Цепочка 4, метка 3");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Frozen, "Mark.Frozen", "Цепочка 5, метка 2: Soaked → Frozen → Deep Freeze");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_DeepFreeze, "Mark.DeepFreeze", "Цепочка 5, метка 3");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Lifted, "Mark.Lifted", "Цепочка 6, метка 1: Lifted → Crushed → Grounded");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Crushed, "Mark.Crushed", "Цепочка 6, метка 2");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Grounded, "Mark.Grounded", "Цепочка 6, метка 3");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Slow, "Mark.Slow", "Цепочка 7, метка 1: Slow → Pinned → Staggered");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Pinned, "Mark.Pinned", "Цепочка 7, метка 2");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Staggered, "Mark.Staggered", "Цепочка 7, метка 3");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Distress, "Mark.Distress", "Цепочка 8, метка 1: Distress → Shake → Stunned");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Shake, "Mark.Shake", "Цепочка 8, метка 2");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Stunned, "Mark.Stunned", "Цепочка 8, метка 3 (не путать с State.Stunned)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Shackles, "Mark.Shackles", "Цепочка 9, метка 1: Shackles → Impaled → Feared");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Impaled, "Mark.Impaled", "Цепочка 9, метка 2");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Feared, "Mark.Feared", "Цепочка 9, метка 3");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Enervated, "Mark.Enervated", "Цепочка 10, метка 1: Enervated → Disarmed");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Disarmed, "Mark.Disarmed", "Цепочка 10, метка 2");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Lassitude, "Mark.Lassitude", "Цепочка 11, метка 1: Lassitude → Sleep");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Sleep, "Mark.Sleep", "Цепочка 11, метка 2");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_DropBack, "Mark.DropBack", "Одиночная метка, без цепочки");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Bloodthirst, "Mark.Bloodthirst", "Одиночная метка, без цепочки");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_VoidTouched, "Mark.VoidTouched", "Одиночная метка, без цепочки (Assassin R, Singularity)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mark_Compressed, "Mark.Compressed", "Одиночная метка (Juggernaut Q, Pressure Crush, Fire Wall-синергия)");
}
