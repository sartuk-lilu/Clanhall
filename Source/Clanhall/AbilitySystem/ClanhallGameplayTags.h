// Clanhall — единая точка объявления GameplayTags.
// Канон: теги закладываются один раз и полностью (см. CLAUDE.md / development_plan.md).
// Дописывать новые теги можно. Переименовывать существующие — нельзя, это ломает весь GAS-граф.

#pragma once

#include "NativeGameplayTags.h"

namespace ClanhallGameplayTags
{
	// ---- Ability.Class.* ----
	// Тег требуемого класса для активной способности (физ. ветка прототипа).
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Class_Knight);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Class_Warrior);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Class_Assassin);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Class_Lancer);

	// ---- Ability.Skill.* ----
	// Корневые теги веток навыков + листовые теги Knight Ранг 1-2 (Раздел 4).
	// Листья других классов добавляются в Разделах 7-10 вместе с самими навыками.
	// Листья нужны в Разделе 6 для контрнавыка: детектор сравнивает активный тег врага с известными.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Knight);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Knight_ShieldSlam);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Knight_PowerStrike);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Knight_ShieldCharge);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Knight_Retribution);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Warrior);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Assassin);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_Lancer);

	// ---- State.* ----
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Casting);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CastingAntimagic);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Parrying);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CounterWindow);
	// Раздел 6: навешивается на игрока на 0.1 сек при успешном контрнавыке.
	// GA_PhysicalSkill читает этот тег → пропускает Charges/КД.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CounterActive);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_InStance);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Stunned);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Knockdown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_ComboRecovery);

	// ---- Weapon.Type.* ----
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_STR);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Type_DEX);

	// ---- Balance.Overload.* ----
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Balance_Overload_STR);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Balance_Overload_DEX);

	// ---- Parry.Incoming.* ----
	// Тег, который AI-способность вешает на ASC игрока перед ударом.
	// Имя тега = направление АТАКИ AI (не ответа игрока).
	// Обратное направление: W↔S, A↔D (combat_system.md §5).
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parry_Incoming_W); // AI бьёт W → игрок жмёт S
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parry_Incoming_S); // AI бьёт S → игрок жмёт W
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parry_Incoming_A); // AI бьёт A → игрок жмёт D
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Parry_Incoming_D); // AI бьёт D → игрок жмёт A

	// ---- Cooldown.* ----
	// Тег "этот конкретный навык сейчас на КД" — отдельно от Ability.Skill.* (та таксономия
	// для идентичности навыка, нужна в Разделе 6 для контрнавыка). КД навешивается/снимается
	// вручную в GA_PhysicalSkill через GE_ApplyTimedTag, см. AbilityData.h.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Knight_ShieldSlam);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Knight_PowerStrike);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Knight_ShieldCharge);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Knight_Retribution);

	// ---- SetByCaller.* ----
	// Служебный тег: все наши generic GameplayEffect-классы (GE_Modify*) несут
	// ровно один SetByCaller-модификатор, поэтому им достаточно одного общего тега-слота.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Magnitude);

	// ---- Magic.School.* ----
	// Только корни школ. Структура рангов (Rank.*) откладывается до Раздела 9 —
	// преждевременно фиксировать форму, которая ещё не используется кодом.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magic_School_Material);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magic_School_Elemental);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magic_School_Aether);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magic_School_Stars);

	// ---- Mark.* ----
	// Полный канонический список из mark_system.md §6 (33 метки + Compressed,
	// используемая в magic_spells.md/Juggernaut-примере, но пропущенная в таблице §6).
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
