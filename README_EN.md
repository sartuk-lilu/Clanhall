# Technical Document - Prototype Development Plan

**Engine:** Unreal Engine 5

**Architecture:** C++, Blueprint, Gameplay Ability System (GAS)

**Language:** [🇷🇺 Russian](README.md) | [🇬🇧 English](README_EN.md)

---

## Brief Description of the Combat System

Holding down LMB puts the character into combat stance - WASD stops moving you and starts attacking instead: overhead, left, right, low. Which attack plays next isn't decided by a fixed combo string but by a pair of directions, "was → now": the system looks at what the previous hit was and picks the matching transition from a tree. Series length belongs to the weapon - an unfamiliar blade is available at its full series length from the very first swing, no leveling required. During the wind-up, key presses are simply ignored; during the open follow-up window, the last press wins. Mistime it and the series drops into recovery and has to start over. A started attack commits, and costs nothing either way - letting go of LMB mid-swing no longer cuts it short: the hit plays out in full and can still land even after you've dropped out of stance, it just won't chain into the next one. Active skills commit harder - they take over not just your hands but the whole body, and you can't so much as move until they're done; the only thing that interrupts one is the opponent's counter-skill, not letting go of a button.

Letting go of LMB means dropping into defense, and WASD goes back to movement: strafing and backing up at a speed that depends on the weapon in your hands, not the character. Defense works like parrying, just as a different skill: parrying is about catching the timing and direction of a swing, defense is about reading the type of attack and answering with the right shift of your capsule, instead of mashing one button for everything. Q and E shift the capsule sideways - stepping out from under vertical overhead swings. Ctrl crouches as a window, not a hold - the capsule dips after a short delay and stands back up on its own, so you have to press it ahead of time, and it ducks under horizontal swings. Space pops the capsule upward, clearing low sweeps at the legs, and while sprinting, held with Shift, turns into a long dash forward that punches straight through a closing opponent - the only defensive move that costs a charge, and only in combat. Shift itself is sprinting: the body turns to face the direction of movement instead of staying locked on the enemy, and it's what gets you out from under area attacks that strafing alone can't outrun. None of these have invincibility frames - each one defends purely by physically moving the capsule out of the hit zone, not by cancelling damage on press.

There's no dedicated parry key. The parry window is an animation notify on the defender, and the moment that actually resolves it is the attacker's blade making contact - not the defender pressing a button, which would already be too late to react to. So you need to already be mid-swing in the opposing direction: W against S, A against D. If contact happens while the defender is in that state, it's a clash - no damage goes through to anyone, the attacker gains stagger and hitstop, the defender gains a charge and a brief suppression of their own attack zone. What's being tested isn't reaction speed but pattern knowledge: there's no on-screen prompt telling you which key to press, only the enemy's animation.

Active skills can't be parried - only a counter-skill interrupts them. Every skill's data lists which techniques (usually from other weapon types) can counter it. Whoever gets countered loses the skill they just used along with the charges already spent on it, and gains stagger; the counterer's own hit still lands for full damage at full cost - there's no discount for landing a successful counter. Bosses counter by the same rules, but only with techniques from their own weapon type, and mostly once the player starts repeating themselves.

There's no dodge roll in any mode - just the vocabulary of defense above, plus parrying. No hyper armor either: any contact, whether it's a clash or a hit that landed clean, shuts down the recipient's own attack zone for the rest of that step, so nobody trades hits just by virtue of having already started swinging. Of the whole defense vocabulary, only the long dash costs a resource, and only in combat; parrying doesn't pay on entry, it pays on the outcome of the clash - a charge for the defender, stagger for the attacker.

There are no cooldowns anywhere in the game - the only gate on a skill is charges, spent the instant you press the button and never refunded: a miss costs full price. Charges come from a confirmed basic attack starting with the second hit in a series, at a rate set by the weapon, and from a parried step, flat, regardless of weapon. Hitting multiple targets doesn't multiply the charge gain - it's paid once per swing. An empty bank is never a dead end: basic attacks and parrying are always free, so going broke just means playing defensively until the bank fills back up.

Enemies run on the same charge economy and the same skill code as the player. So parrying a boss's combo doesn't just soak the damage - it also denies the income from that attack and quietly builds up the boss's own stagger.

Damage hits AP before it touches HP. A landed hit returns half of whatever AP was just stripped from the target back to the attacker - half, not all of it, so that turtling stays a losing strategy. Every exchange permanently burns armor, with no regen either in or out of combat - the only way back up is by landing hits, or through a separate, limited-use refill ability. HP doesn't regenerate in combat at all; healing is a spell that only works outside of combat and spends mana earned in the previous fight.

Separate from AP, the design also calls for an armor threshold: a hit below the threshold does nothing at all rather than reduced damage - deliberately binary, so weapon choice is a real decision instead of just a multiplier. Light weapons carry no penetration and hit often but weak, heavy weapons penetrate but swing slow and cost more per hit. None of this is implemented in code yet - right now all damage flows straight into AP/HP with no threshold check; it's an open item, not a working mechanic.

Another layer that's designed but not built is aimed hit locations. The idea is that aim, not the key you press, decides where a hit lands: the torso turns to follow the player's cursor, and the same swing can land on the head or the body depending on how the fighter is posed at that moment. An enemy would then be more than one HP bar - a set of body parts, each with its own armor and health pool. None of that exists in code yet either; right now every hit lands on a single target as a whole.

There are no permanent buffs or debuffs - only marks: at most one per fighter, a new mark overwrites the old one, and marks don't last long. Basic attacks never apply marks; that's purely the job of active skills. A skill that lands on a mark it has synergy with either grants a buff to the attacker or a debuff to the target - never both - and leaves its own new mark behind. A mark never takes away control of your character.

Stagger builds up on whoever's getting outplayed, not on whoever's attacking. The first parried step in a series costs the attacker nothing, but every step after that adds stagger; a skill broken by a counter adds stagger to whoever got countered, and anti-magic adds it to the caster - one point per intercepted syllable-word of the spell. A full stagger bar doesn't stun by itself: it resets to zero and leaves behind a mark that has to be cashed in with the right skill before it expires, or it's wasted. There's exactly one path into a stun in this game, and it takes two steps to earn it - build the stagger, then land the follow-up.

Magic is fully worked out on the design side but only partially built in code. The only source of mana is a confirmed hit from a physical active skill - no regen, and basic attacks don't generate any. Spells are cast by holding RMB: your keys turn into syllables, and the eight keys across the top and bottom rows split into four schools by vertical pairs. A word is a pair of syllables, and the first syllable of a word sets its school, so longer spells are just several words chained together, pulling in several schools at once. There are no projectiles - all magic works at melee range. A new spell only gets unlocked by encountering it: watch an enemy cast it, beat them, and it shows up in your book - a combination copied from somewhere else simply won't cast. Anti-magic is parrying for spellcasting: hold both mouse buttons and repeat the exact syllables the enemy is currently casting. Beat them to the release and their cast is cancelled - you gain charges (one per intercepted word) and spend nothing, they gain stagger. Miss the window and an active effect can still be dispelled, but now it costs mana with no bonus. The enemy's syllables are never highlighted on screen - the only thing you have to go on is what you hear and remember, with a separate accessibility toggle for players who can't rely on audio cues.

There's no class-selection screen - weapon type fills that role instead: it defines the attack tree, series length, charge income rate, stance movement speed, and the whole pool of active skills. Weapon proficiency only gates skill tiers - basic attacks and parrying are available at full strength from the very first moment, with any weapon.

Information about enemies is deliberately open. A boss's plate shows their mana, their charges against the cost of their own skills, their active mark, their stagger, and which of their techniques the player has already run into. An empty bank on the boss reads as "they can't afford anything right now"; a full one reads as "here comes a spell or a heavy skill, keep a dodge ready." That predictability comes from the systems themselves, not from scripted phases, and it's exactly what makes picking a weapon loadout before a fight a real decision instead of flavor.


---

## Design Documentation

| ![design-doc](https://github.com/user-attachments/assets/fae4721f-fc54-47a4-b6cb-4d64d4d72d8e) |
| ---------------------------------------------------------------------------------------------- |

---

## Prototype Development Status

| Status         | System                                                                |
| -------------- | --------------------------------------------------------------------- |
| ✅Ready         | [`Architecture`](#Architecture)                                       |
| ✅Ready         | [`GAS Fundamentals and Attributes`](#GAS-Fundamentals-and-Attributes) |
| ✅Ready         | [`DataAsset, Fragments`](#DataAsset-and-Fragments)                    |
| ✅Ready         | [`HUD`](#HUD)                                                         |
| ✅Ready         | [`Combat Stance and WASD Attacks`](#Combat-Stance-and-WASD-Attacks)   |
| ✅Ready         | [`Weapons`](#Weapons)                                                 |
| ✅Ready         | [`Animation Setup`](#Animation-Setup)                                 |
| 🟡 In Progress | [`Parrying`](Parrying)                                                |
| 🟡 In Progress | [`Counter Ability`](#Counter-Ability)                                 |
| 🟡 In Progress | [`Marking System`](#Marking-System)                                   |
| 🔜             | [`Enemy`](#Enemy)                                                     |
|                | [`Magic System`](#Magic-System)                                       |
|                | [`Weapon Swap`](#Weapon-Swap)                                         |
|                | [`AI`](#AI)                                                           |

---
## Architecture

The project's central pattern. Every skill, spell, or weapon is a `UPrimaryDataAsset` with a header and an array of fragments. A weapon is three such assets - details in `Weapons.md`.
### DataAsset Structure (header)

```cpp
UCLASS()
class UAbilityData : public UPrimaryDataAsset
{
    // Header of a physical skill (spells have a different resource - MP only,
    // they'll need their own asset). There's no cooldown here or anywhere else
    // in the project - the only gate on using an active skill is Charges.
    FText DisplayName;
    UTexture2D* Icon;
    int32 ChargeCost;                 // by tier: Q/E=2, R/F=4, Z/X=6, C/V=8
    float ManaGain;                   // on confirmed hit, once per use; 0 is legal
    UAnimMontage* CastMontage;        // nullptr is legal - the mechanic works without a montage

    // Fragments - only what a specific skill actually needs
    UPROPERTY(EditAnywhere, Instanced)
    TArray<TObjectPtr<UAbilityFragment>> Fragments;

    template<typename T>
    T* FindFragment() const;  // fragment lookup by type → nullptr if absent
};
```

### Key specifiers of the base fragment class

```cpp
UCLASS(Abstract, DefaultToInstanced, EditInlineNew)
class UAbilityFragment : public UObject {};
// DefaultToInstanced - every array entry is a unique instance
// EditInlineNew    - the editor expands the contents inline, right inside the DataAsset
```

There are two base fragment classes in the project: `UAbilityFragment` (skills) and `UWeaponFragment`
(weapons, `Weapons.md`, "Weapon Actor") - both carry the same specifiers. They're separate classes rather than one shared base: a skill fragment and a weapon fragment go into different arrays and shouldn't be interchangeable in the editor's picker.

### The "header or fragment" boundary

A fragment earns its place when its **absence** carries meaning that a field's default value can't express. When there's no such meaning, the field goes in the header: a fragment there would only add an extra editor click and a silent "forgot to add it" bug.

That's why `CastMontage` (absence ≡ `nullptr`) lives in the header, while `UDamageFragment` is a fragment: its absence means "a utility skill, the hit is confirmed by finding a target," which `BaseDamage = 0` doesn't express.

### Project fragments

| Fragment | Fields | When to add it |
|---|---|---|
| `UVFXFragment` | CastEffect, ImpactEffect | Anything with visuals |
| `USFXFragment` | CastSound, ImpactSound | Anything with sound |
| `UDamageFragment` | BaseDamage | Anything that deals damage |
| `UMarkApplyFragment` | MarkTag | Anything that applies a mark |
| `UMarkTriggerFragment` | TArray\<FMarkSynergy\> | Anything that consumes marks |
| `UWeaponOffhandFragment` | OffhandClass | Weapons with an occupied off-hand (shield, second blade) |

`UWeaponOffhandFragment` is currently the only `UWeaponFragment` subclass, and it lives
on the instance (`UWeaponData::Fragments`), not on the type: the actor class is content belonging to a specific item, while the type defines shape, not magnitude. Putting the off-hand on the type would be a mistake - every shield of that type would become the same shield. The fragment's criterion is literal here: absence means "left hand is free," not "forgot to set a shield" (`weapon_system.md`, "Weapon as Actor").
### FMarkSynergy - structure inside UMarkTriggerFragment

```cpp
USTRUCT()
struct FMarkSynergy
{
    FGameplayTag RequiredMark;          // required mark on the target
    TSubclassOf<UGameplayEffect> EffectOnTarget;  // debuff on the enemy
    TSubclassOf<UGameplayEffect> EffectOnSelf;    // buff on self
    // Only one of the two is ever filled in - never both
};
```

**Synergy never pays out charges.** The `ChargeGain` field no longer exists on the struct - removed
along with the "active skill charge generation" rule (`mark_system.md`): as long as synergy paid out charges, mark-skill-charges was a loop feeding itself. Charges are now earned exclusively by landing a hit or parrying (`combat_system.md`, "Character Resources").

**One subtlety that remains.** `RequiredMark` is matched via `MatchesTag`, not equality: a specific tag (`Mark.BrokenGuard`) only matches itself, while the root `Mark` matches any mark at all (this is how Retribution's "triggers regardless of mark type" is expressed). Consequence: the FIRST match in the array wins, so specific entries must be placed ABOVE broad ones.
### How a GameplayAbility reads fragments

An ability holds no data of its own - it only queries fragments through `FindFragment<T>()`:

```cpp
// Resolves one hit target - called from the contact window or from the instant fallback
void UGA_PhysicalSkill::ResolveHitOn(AActor* Target)
{
    // Damage goes through a fragment; its absence means "utility skill"
    if (auto* Dmg = AbilityData->FindFragment<UDamageFragment>())
        bConfirmedHit = ResolveStandardDamage(SourceASC, TargetASC, Dmg->BaseDamage);

    if (!bConfirmedHit) return;

    // Synergy off the target's mark, and applying our own - both through fragments
    ResolveMarkLogic(AbilityData, SourceASC, TargetASC, TargetMarkComponent);
}
```

The ability's logic doesn't change when the data changes - only the DataAsset does.

`CastMontage` in this list is no longer "cosmetic, last line": it fires before the resolve and itself decides whether the resolve will be contact-based or instant. But the "mechanics before visuals" rule survives intact: `nullptr` is a legal state, and the skill works fully through the instant fallback.

### Order for adding fragments (not all at once)

1. Header + `UDamageFragment` - the skill deals damage
2. `UMarkApplyFragment` + `UMarkTriggerFragment` - the mark system works
3. `CastMontage`, `UVFXFragment`, `USFXFragment` - visuals and sound


**Rule:** the mechanic works first, then it looks good. `CastMontage` living in the header doesn't get in the way of that: `nullptr` is a legal state until montages get cut.

---

## GAS-Fundamentals-and-Attributes

**Bottom line:** GAS is wired in at the level of the fighters' common ancestor - the same `AbilitySystemComponent` and the same `UClanhallAttributeSet` for both the player and AI, no separate paths. The five resources live as ten attributes, get clamped at two points, and change exclusively through five generic effects with `SetByCaller` magnitude. Tag taxonomy is laid down natively, in one file, and locked against renaming.

### Where it all lives

`AClanhallCombatantBase` (`ClanhallCombatantBase.h/.cpp`) is the common ancestor of every fighter. ASC and AttributeSet are created by its constructor, not the player's:

```cpp
AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
AbilitySystemComponent->SetIsReplicated(true);
AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

AttributeSet = CreateDefaultSubobject<UClanhallAttributeSet>(TEXT("AttributeSet"));
```

In `BeginPlay`: `InitAbilityActorInfo(this, this)` - **`OwnerActor == AvatarActor == this`**,
the ASC doesn't live on `PlayerState` for either the player or the enemy. The prototype is single-player, and splitting owner from avatar right now would only add steps to every access.

Right next to it in the same constructor are all the combat components, also shared by both sides:
`UClanhallMarkComponent`, `UClanhallHitboxComponent`, `UClanhallCounterComponent`,
`UClanhallCombatStateComponent` (`Combatant Hierarchy.md`, "Three Layers").

A trap for later: the hitbox subobject's name is `"WeaponTraceComponent"`, while the class has
long since been `UClanhallHitboxComponent`. The name is kept letter-for-letter on purpose - changing the string breaks overrides in the player's BP subclass. **The class was renamed, the subobject name wasn't.**

---

### Attributes

`AbilitySystem/ClanhallAttributeSet.h/.cpp`. Ten `FGameplayAttributeData` fields - five resources and their five caps.

| Pair | Meaning | Rule owner |
| --- | --- | --- |
| `AP` / `MaxAP` | absorbs damage first; while `AP > 0`, HP isn't touched | `combat_system.md`, "AP - Armor Points" |
| `HP` / `MaxHP` | never regenerates in combat | `combat_system.md`, "HP - Hit Points" |
| `MP` / `MaxMP` | the only source is a confirmed hit from a physical active skill | `ability_system.md`, "Physical Active Skills" |
| `Charges` / `MaxCharges` | active-skill bank; hard-capped at 16 | `combat_system.md`, "Charges - Active Skill Points" |
| `Stagger` / `MaxStagger` | fatigue; three feeders, builds up on whoever's getting outplayed | `combat_system.md`, "Stagger" |

#### `ATTRIBUTE_ACCESSORS` is declared in the project

The engine only gives you four separate "bricks" - every project assembles the combined macro itself (the Lyra / GASDocumentation pattern):

```cpp
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
```

#### Starting values

Placeholder `UPROPERTY(EditDefaultsOnly)` fields on `AClanhallCombatantBase`, overridden per-class in the defaults of the BP subclass:

| Field | Value | Note |
| --- | --- | --- |
| `DefaultMaxAP` | `300.0` | |
| `DefaultMaxHP` | `500.0` | |
| `DefaultMaxMP` | `200.0` | |
| `DefaultMaxCharges` | `6.0` | bank base; at 4, rank 3 would unlock Z/X costing 6 with a bank of 4 - the skill would be physically unusable |
| `DefaultMaxStagger` | `4.0` | stagger cap, tuned by playtesting |

`BeginPlay` initializes them in pairs (`InitMaxAP` → `InitAP`, and so on), `Stagger` starts
at zero. Full resources at spawn is a prototype convention, not a design rule.

**Initialization lives on the base class, not in `AClanhallCharacter::BeginPlay`** - and that already
cost a bug once. An instance without the player's path (`AClanhallHumanoidBoss` with an empty constructor) was left
with zeroed attributes: `MaxStagger = 0` clamped `Stagger` to `[0, 0]`, and `GetStagger() >= GetMaxStagger()` was already true on the very first clash - the boss got stunned off a single parry instead of the intended four (`Combatant Hierarchy.md`, "Attribute Starting Values on the Base Class").

A DataAsset or `GameplayEffect` for attribute initialization will come later, alongside the rest of the data system.

---

### Clamping happens at two points, not one

```cpp
void UClanhallAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);      // instant code-side edits: SetHP, etc.
}

void UClanhallAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);
    // GameplayEffect modifiers go through the Aggregator and can get one step ahead
    // of the limit recalculation - clamping again guarantees the final value stays in bounds
    ...
}
```

The two points aren't redundancy - they're different sources of change. `PreAttributeChange` catches
direct setters, `PostGameplayEffectExecute` catches the result of an effect that
`PreAttributeChange` may have seen before its limit had a chance to update.

#### What gets clamped and what doesn't

- `AP`, `HP`, `MP`, `Charges` → `[0, Max*]`;
- `Stagger` → `[0, MaxStagger]`;
- `MaxCharges` → `[0, 16]` - a **technical** boundary, not a balance one: 16 = the number of cells in the 4×4 grid
  in `WBP_ChargesPanel`. The design value for the bank lives in `DefaultMaxCharges` and in
  `economy_system.md`, "Bank";

---

### How attributes actually change

There are no direct `SetXxx` calls in gameplay code - everything goes through five generic effects, each with
exactly one Additive modifier using `SetByCaller` magnitude:

`UGE_ModifyAP`, `UGE_ModifyHP`, `UGE_ModifyMP`, `UGE_ModifyCharges`, `UGE_ModifyStagger`.

**The sign of the magnitude decides whether it's damage or a heal**: `-30` to AP is damage, `+15` to AP is
healing. There's no separate "damage effect" and "heal effect," and there doesn't need to be.

Four helpers in `namespace ClanhallGameplayEffects`:

| Function | When |
| --- | --- |
| `ApplyModifyEffect(Source, Target, EffectClass, Magnitude)` | anything that changes an attribute's number |
| `ApplyEffect(Source, Target, EffectClass)` | mark synergy effects - magnitude is baked into the class itself |
| `ApplyTimedTag(ASC, Tag, Duration)` | a temporary state on self: `State.ComboRecovery`, `State.DodgeRecovery` |
| `ApplyTimedTagToTarget(Source, Target, Tag, Duration)` | same, but source and target are different actors |

`ApplyTimedTag` is **not** for cooldowns: none remain anywhere in the project
(`economy_system.md`, "Why There Are No Cooldowns").

#### The one and only damage formula

`UGA_ClanhallAbilityBase::ResolveStandardDamage` - shared by both WASD attacks and active skills:

```cpp
const float TargetCurrentAP = TargetAttributes->GetAP();
const float APRemoved = FMath::Min(RawDamage, TargetCurrentAP);
const float Overflow   = RawDamage - APRemoved;

if (APRemoved > 0.0f)
{
    ApplyModifyEffect(SourceASC, TargetASC, UGE_ModifyAP, -APRemoved);
    ApplyModifyEffect(SourceASC, SourceASC, UGE_ModifyAP, APRemoved * 0.5f);  // 50% return
}
if (Overflow > 0.0f)
{
    ApplyModifyEffect(SourceASC, TargetASC, UGE_ModifyHP, -Overflow);
}
return true;   // "confirmed hit"
```

Returning `true` means **confirmed hit**, and marks, synergies, and `ManaGain` on active skills all key off of it.
`Charges`, meanwhile, are deducted unconditionally, on activation, regardless of whether the hit lands.

The AP return is **50%**, a project invariant. Not to be changed to 100%: passive defense has to stay
a losing strategy.

This function does **not** suppress the target's zone or apply hitstop to the attacker - that moved to the
contact itself (`UClanhallHitboxComponent::TickHitbox`). Reason: a hit fully absorbed by the threshold, and utility skills that deal no damage, never reach this function, yet contact has already happened either way. Duplicating the call here would mean doing it twice on a normal hit and never on an absorbed one.

---

### Base ability class

`UGA_ClanhallAbilityBase` (`Abstract`) - the common ancestor of `GA_DirectionalAttackBase` (WASD)
and `UGA_PhysicalSkill` (active skills). Its entire constructor is four tags:

| Tag | Role |
| --- | --- |
| `State.InStance` | `ActivationRequiredTags` - outside of stance the same keys just move the character, no separate code branch needed |
| `State.ComboRecovery` | `ActivationBlockedTags` - the lockout after Recovery covers both WASD and active skills |
| `State.SkillCommitted` | `ActivationBlockedTags` - a started active skill can't be interrupted by a second active skill or by WASD |
| `State.Stunned` | `ActivationBlockedTags` - a stunned fighter doesn't attack; the tag is symmetric, it's applied to the player too |

`UGA_CombatStance` does **not** inherit from this class (it inherits from `UGameplayAbility`
directly) and takes `State.ComboRecovery` on itself, explicitly, in its own constructor. The difference
matters: entering stance is blocked for the duration of Recovery, but **leaving is always free** - it goes
through `CancelAbilityHandle`, which `ActivationBlockedTags` doesn't touch.

Plus two geometry fields for the instant fallback - `TraceRange = 200`, `TraceRadius = 75`, read by
`FindMeleeTarget` (`SphereOverlapActors` in front of the character, first actor
implementing `IAbilitySystemInterface`). This is **not** the main damage path: both WASD and active skills resolve
through zone contact (`DataAsset and Fragments.md`, "Two Damage Resolution Modes").

#### Denied feedback

The only rejection-reason tag is `Denied.Charges`, placed into `OptionalRelevantTags`
in `UGA_PhysicalSkill::CanActivateAbility`. Rejections from `State.SkillCommitted` / `State.Stunned` are filtered out by `Super::CanActivateAbility` **before** this point and never reach the tag - the two can't be confused.

It travels upward via relay, because engine multicasts aren't visible in Blueprint:

```cpp
AbilitySystemComponent->AbilityFailedCallbacks.AddUObject(this, &AClanhallCombatantBase::HandleAbilityFailed);
// → HasTagExact(Denied.Charges) → OnChargesDenied.Broadcast()   (BlueprintAssignable)
```

Details in `Combatant Hierarchy.md`, "Denied Feedback".

---

### Replication

Laid down from the start, groundwork for multiplayer even though the prototype is single-player. All ten
attributes use `DOREPLIFETIME_CONDITION_NOTIFY(..., COND_None, REPNOTIFY_Always)` plus their own `OnRep_*` (`GAMEPLAYATTRIBUTE_REPNOTIFY`). The ASC's replication mode is `Mixed`.

`REPNOTIFY_Always`, not `OnChanged`: without it, reassigning the same value wouldn't
wake `OnRep`, and client-side prediction would silently drift out of sync.

---

### GameplayTags taxonomy

`AbilitySystem/ClanhallGameplayTags.h` - native tags via `UE_DECLARE_GAMEPLAY_TAG_EXTERN`, not a DataTable. Laid down fully and up front; the full list and branching rules live in `gameplayTags.md`, this is just a map of the roots.

| Root | What it addresses |
| --- | --- |
| `Ability.*` | skill **identity**, leaves by weapon type: `Ability.Knight.*`, `Ability.Warrior.*`, `Ability.Assassin.*`, `Ability.Lancer.*` |
| `Slot.*` | `Q/E/R/F/Z/X/C/V` - keys the kit and input binding, **not** cooldown |
| `Denied.*` | activation rejection reason; currently the only leaf is `Denied.Charges` |
| `State.*` | fighter states: `InStance`, `Parrying`, `CounterWindow`, `SkillCommitted`, `ComboRecovery`, `DodgeRecovery`, `Stunned`, `Knockdown`, `Casting`, `CastingAntimagic`, `InCombat` |
| `Attack.Direction.*` | `W/A/S/D` - symmetric telegraph of the fighter's own swing direction |
| `Event.*` | `ApplyMark`, `DirectionalAttack`, `Hitbox.Hit`, `Hitbox.Closed` |
| `Mark.*` | the project's full set of marks, including `Staggered` and `Compressed` |
| `Perk.*` | including `Perk.Proficiency.<Type>.Rank1..4` - weapon-type proficiency ranks |
| `Magic.School.*` | roots only: `Elemental`, `Aether`, `Materia`, `Stars` - ranks are deferred |
| `Unit.Role.*` | `Mob`, `Boss.Humanoid`, `Boss.Monster` |
| `Damage.Type.*` | `Slash`, `Pierce`, `Blunt` - groundwork, not read in damage calculation |
| `SetByCaller.Magnitude` | the magnitude key for all five `UGE_Modify*` effects |

**`Slot.*` and `Denied.*` were deliberately split off from under `Ability`.** While they lived under it,
`meta = (Categories = "Ability")` on `UAbilityData::CounterTag` / `CounteredBy` and on
`UCharacterSheetData::SeenSkills` / `LearnedSkills` offered them up in the tag picker as valid
values for a skill's identity field. After the split, only identity remains under `Ability`.

---

### Debugging

- `Clanhall.Debug.ListStats` - prints the names of every attribute the commands below accept.
- `Clanhall.Player.ShowStats` / `Clanhall.Enemy.ShowStats` - a single line with all five
  resources: `AP · HP · MP · Charges · Stagger`, each shown as `current/cap`.
- `Clanhall.Player.SetStat <Name> <Value>` / `Clanhall.Enemy.SetStat ...` - live-edits an attribute; `UClanhallAttributeSet`'s clamps still apply, so the command can't push a value past its bound.

---

## DataAsset-and-Fragments

**Bottom line:** one skill class, `UGA_PhysicalSkill`, handles every physical active skill. All of their
content - damage, mark, synergy, dash, mana, cost - lives as data in `UAbilityData` and is edited in the editor with no recompile. The only gate on using an active skill is Charges.

The canonical pattern lives in `Architecture.md`; weapon assets, built on the same principle, are in `Weapons.md`.

### The idea

One skill, one `UPrimaryDataAsset`. The asset's header holds what **every** active skill has
(name, icon, cost, montage, counter tags), while the `Fragments` array holds what only a specific
one has (damage, mark, dash, VFX). The alternative we moved away from was one class with fifty fields, forty of which are null depending on the skill's type.

The boundary between header and fragment follows one criterion: **a fragment earns its place when its absence carries meaning that a field's default value can't express.** `UDamageFragment` is a fragment because its absence means "a utility skill, the hit is confirmed by finding a target," not "zero damage." `CastMontage` is a header field because "no montage" and `nullptr` are the same thing, and a fragment there would only add an extra editor click and a silent bug - "forgot to add it, there's no animation, but the logic works."

The asset is symmetric: the same description serves both the player and the enemy. An enemy activates the same `UGA_PhysicalSkill` with the same `SourceObject`, there's no separate enemy path left in the project (`Counter Ability.md`, "Enemy Side - Shared Code").

The slot (`Slot.*`) isn't stored on the asset - that's a property of the grant, not the skill: the slot arrives as a key
in `UWeaponTypeData::Skills` and lives on afterward as the spec's dynamic tag. There's no required class either - class as an in-game entity no longer exists, access to a skill is decided by weapon-type proficiency rank and the sheet's `LearnedSkills` (`Weapons.md`, "Weapon Proficiency: Grant Gates").

### Key variables

#### `UAbilityData` header

`AbilitySystem/AbilityData.h`, a `UPrimaryDataAsset` subclass.

- **`DisplayName`** (`FText`), **`Icon`** (`UTexture2D*`) - the label and icon for the HUD slot.
- **`CounterTag`** (`FGameplayTag`, `meta = (Categories = "Ability")`) - the skill's identity, "what I
  counter with." Goes into `TryResolveCounter` as the incoming tag.
- **`CounteredBy`** (`FGameplayTagContainer`, same category) - vulnerability, "what can interrupt THIS
  skill." Empty means the skill can't be countered by anything, and that's a legal state. The
  `CounterTag` / `CounteredBy` pair is covered in `Counter Ability.md`, "Data Asymmetry".
- **`ChargeCost`** (`int32`, `ClampMin = 0`, default **2**) - cost in Charges. Canon by tier:
  Q/E=2, R/F=4, Z/X=6, C/V=8 (`combat_system.md`, "Character Resources"). Default is 2, not 0,
  so a new asset doesn't look free by default.
- **`ManaGain`** (`float`, `ClampMin = 0.0`, default **0**) - mana on confirmed hit,
  once per use regardless of how many targets got hit. The rate is nonlinear by tier: Q/E=4, R/F=10, Z/X=18, C/V=28. Zero is legal - it's fine for a utility skill to grant no mana.
- **`CastMontage`** (`UAnimMontage*`) - the skill's montage, **fullbody** slot for every active skill without
  exception. `nullptr` is legal: the mechanic works without a montage, resolve falls through to the instant fallback. A common markup mistake is leaving the montage in `DefaultSlot` - the logic still works, the animation just doesn't show.
- **`Fragments`** (`TArray<TObjectPtr<UAbilityFragment>>`, `Instanced`) - the fragments themselves.
- **`FindFragment<T>()`** - a template that returns the first fragment of type T, or `nullptr`.

Deliberately absent from the header: `Cooldown` (cooldown as a mechanism no longer exists anywhere in the project), `CounterStunDuration` (landing a counter doesn't grant a stun), a slot, a required class, `ImpactMontage` (the reaction to being hit belongs to the recipient, not the attacker's skill).

#### Skill fragments

Base class - `UAbilityFragment` (`Fragments/AbilityFragment.h`), `UCLASS(Abstract, DefaultToInstanced,EditInlineNew)`. This pair of specifiers is the whole mechanism: `DefaultToInstanced` makes every array entry a unique subobject rather than a shared CDO, `EditInlineNew` expands its contents right inside the asset rather than as a separate asset with its own reference.

Mechanical ones, `Fragments/GameplayFragments.h` - the ones `UGA_PhysicalSkill` actually reads:

- **`UDamageFragment`** - `float BaseDamage`. Just stores the number; the AP/HP exchange is computed by
  `ResolveStandardDamage` in the base ability class.
- **`UMarkApplyFragment`** - `FGameplayTag MarkTag`, the mark the skill puts on the target.
- **`UMarkTriggerFragment`** - `TArray<FMarkSynergy> Synergies`, the marks the skill knows how to
  cash in.
- **`UDashFragment`** (`DisplayName = "Dash"`) - `float Distance` (default 300 cm),
  `float Duration` (default 0.35 s). A skill's displacement is data, not a property of the clip: distance baked into an animation would require a re-export on every tweak and would break the "the mechanic works without a single animation" invariant.

`FMarkSynergy` (`ClanhallMarkTypes.h`) - `RequiredMark`, `EffectOnTarget`, `EffectOnSelf`.
Filled in as EITHER a debuff on the target, OR a buff on self, never both. Synergy never pays out charges, the `ChargeGain` field no longer exists on the struct.

Presentational, `Fragments/PresentationFragments.h` - `UVFXFragment` (`CastEffect`,
`ImpactEffect`, both `TSoftObjectPtr<UObject>`, no concrete effect type yet) and
`USFXFragment` (`CastSound`, `ImpactSound`).

Weapons don't reuse skill fragments: they have their own base, `UWeaponFragment`
(`Fragments/WeaponFragment.h`), with the same specifiers. A shared base class would have mixed things in the editor's picker for free, and incorrectly - a skill fragment and a weapon fragment go into different arrays. The only subclass right now is `UWeaponOffhandFragment` (`Weapons.md`, "Weapon Actor").

#### `UGA_PhysicalSkill` runtime state

`InstancingPolicy == InstancedPerExecution`, so every activation spawns a fresh instance, and no field needs manual resetting. It's the instancing policy itself that makes them correct - don't turn them into statics.

- **`bSelfSynergySpent`** - the self buff from synergy has already been applied for this activation.
- **`bManaApplied`** - mana has already been granted, no matter how many more targets get hit afterward.
- **`bPrimaryTerminatorFired`** - the cast montage ended, or the instant resolve completed.
- **`bDashPending`** - a dash has started and hasn't finished yet.
- **`PlayedCastMontage`** (`TWeakObjectPtr<UAnimMontage>`) - the montage this skill actually started. The identity key for cleaning up stuck zones in `EndAbility`: only its own get torn down.
- **`MarkResolvedTargets`** (`TArray<TWeakObjectPtr<AActor>>`) - targets the mark logic has already run for during this activation.

### Points of interest

#### One class, different `SourceObject`s

`UGA_PhysicalSkill` is granted once per entry in the current weapon's `UWeaponTypeData::Skills` (for sword+shield that's Shield Slam, Power Strike, Shield Charge, Retribution), each time with its own `UAbilityData` in `FGameplayAbilitySpec::SourceObject`. It's read through the `Handle`, not through `GetCurrentSourceObject()`:

```cpp
const UAbilityData* UGA_PhysicalSkill::GetAbilityData(const FGameplayAbilitySpecHandle Handle,
                                                      const FGameplayAbilityActorInfo* ActorInfo) const
```

The reason is the instancing policy: with `InstancedPerExecution` there's no persistent instance yet at the point of `CanActivateAbility`, and the method can end up called on the CDO - `GetCurrentSourceObject()` would return garbage there.

#### The cost is read by the exact same expression in the check and in the deduction

`CanActivateAbility` only checks Charges. There are no cost modifiers at all, and that's
deliberate: the check and the deduction have to see the same number.

```cpp
const float EffectiveCost = static_cast<float>(Data->ChargeCost);
if (EffectiveCost > 0.0f)
{
    const UClanhallAttributeSet* Attributes = ASC->GetSet<UClanhallAttributeSet>();
    if (!Attributes || Attributes->GetCharges() < EffectiveCost)
    {
        if (OptionalRelevantTags)
        {
            OptionalRelevantTags->AddTag(ClanhallGameplayTags::Denied_Charges.GetTag());
        }
        return false;
    }
}
```

Charges are deducted on activation **unconditionally**, including on a miss and on being countered: the
cost of the decision "hold this skill for a counterattack" is exactly these charges. An earlier canon, "Charges only on confirmed hit," was deliberately abandoned.

`Denied.Charges` landing in `OptionalRelevantTags` is the standard GAS path: `AbilityFailedCallbacks` receives the container on a failed `TryActivateAbility`. `AClanhallCombatantBase` subscribes in `BeginPlay` (`AddUObject`, the callback isn't dynamic) and relays it into `BlueprintAssignable FOnClanhallChargesDenied OnChargesDenied` only when it matches this tag - rejections from `State.SkillCommitted` and `State.Stunned` aren't marked by this signal, they're filtered out earlier by `Super::CanActivateAbility`. The actual reaction (sound, a flash on `WBP_ChargesPanel`) isn't implemented yet, the delegate is currently just a hookup point.

#### The order inside `ActivateAbility` doesn't get reshuffled

1. `ResetSuppression()` on its own hitbox component - a new active skill clears the suppression left by the previous step's zones.
2. `FindMeleeTarget` - the target is only needed for the fallback path. A counter doesn't take its target from here.
3. Charges are deducted.
4. The opponent's WASD series is cancelled, and `Montage_Play` fires.
5. `StartDashIfNeeded` - after the deduction and `Montage_Play`, but before branching into the contact vs. fallback path: the dash has to work on both.
6. Branching, and on the contact path, subscribing to `Event.Hitbox.Hit`.

Step 4 relative to step 6 is the critical part:

```cpp
if (UClanhallComboComponent* Combo = Char->FindComponentByClass<UClanhallComboComponent>())
{
    Combo->CancelSequenceForExternalMontage();
}
MontagePlayLength = AnimInst->Montage_Play(Data->CastMontage);
```

`CancelSequenceForExternalMontage()` calls `ForceEndHitboxes()`, which, if the zone list isn't empty, sends `Event.Hitbox.Closed`. Had the skill subscribed to events any earlier, it would have caught this event itself and ended before its own zone even opened.

#### Two damage resolution modes

Markup decides, not code: `UAnimNotifyState_Hitbox::MontageHasHitbox(Data->CastMontage)`.

**Contact mode** (a montage exists, a zone is placed on it, the montage has started): the ability waits on
`Event.Hitbox.Hit` via `UAbilityTask_WaitGameplayEvent` and resolves every target the zone touched.
**Instant fallback** (`CastMontage == nullptr`, no zone on the montage, or `Montage_Play`
didn't start): the same `ResolveHitOn` is called once, right on activation, against the target from
`FindMeleeTarget`. The fallback isn't a stub - it's a working path: you can run the mechanic on a dummy before any animations are cut, and counters remain testable on it.

#### `ResolveHitOn` and three levels of application

There are three levels, not two, and that difference matters:

```cpp
bool bConfirmedHit = false;
if (const UDamageFragment* Damage = Data->FindFragment<UDamageFragment>())
    bConfirmedHit = ResolveStandardDamage(SourceASC, TargetASC, Damage->BaseDamage);
else
    bConfirmedHit = (TargetASC != nullptr);   // utility: a hit means a target was found

if (!bConfirmedHit) return;

UClanhallCounterComponent::TryResolveCounter(Target, Data->CounterTag);   // per contact

if (!bMarkAlreadyResolved) { /* mark and synergy */ }                     // per target per activation

if (!bManaApplied && Data->ManaGain > 0.0f) { /* mana */ }                // per activation
```

**Per contact**: damage and counter. **Per target per activation**: mark and synergy. **Per activation**:
mana and `EffectOnSelf`. Collapse the second level into the first, and the second phase of a multi-zone hit would see the mark applied by the first phase: synergy against a root `RequiredMark` (Retribution, "any mark") would fire off its own mark and trigger twice in a single activation. The counter doesn't need a separate gate - `ConsumeCounter` closes the window, and a repeat contact on the same target returns `false`.

`RequiredMark` matches via `MatchesTag`, not equality, so specific entries are placed **above** broad ones in the array: the first match wins, and an entry with root `Mark` would eclipse everything after it.

#### Multi-zone hits

Several `AnimNotifyState_Hitbox` notifies on the same montage always mean several separate damage instances. There's no such thing as "two shapes of the same contact" - `HitGroup` and an attack ID aren't needed. Damage is gated only by the zone's own `AlreadyHit`; mark, synergy, and counter are gated by the skill's `MarkResolvedTargets`. There's one strict markup rule: every Hitbox notify on a given montage sits on the **same track**. The engine allows overlap on different tracks - this is our own restriction.

#### The ability waits for whichever of two terminators fires last

```cpp
void UGA_PhysicalSkill::TryFinishAbility()
{
    if (bPrimaryTerminatorFired && !bDashPending)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}
```

The primary terminator is always the end of the cast montage, whenever a montage started, through
`Montage_SetEndDelegate` (an `AnimInstance` delegate, not a GameplayEvent - fires on both normal completion and interruption). The second one is the end of the dash.

`Event.Hitbox.Closed` does **not** serve as a terminator. `Closed` means "no zones are currently open,"
not "the attack is over": between two sequential zones (shield closed, sword hasn't opened yet) the list goes empty, and the old terminator would have cut the skill off in that gap, losing the second hit. The active skill isn't subscribed to this event at all.

#### `State.SkillCommitted`

Applied as a loose tag in `ActivateAbility`, whenever the cast montage starts, or when there's no montage but a dash has been launched. Removed at a single point - the overridden `EndAbility`, no matter how the skill ended, including cancellation by a counter.

It's deliberately absent from `ActivationOwnedTags`: on the fallback path, with no montage and no dash, it would end up applied for nothing. Blocks a second active skill and the WASD series through the base class's `ActivationBlockedTags`, as well as movement and jumping (`Combat Stance and WASD Attacks.md`, "An Active Skill Can't Be Interrupted Manually").

That same `EndAbility` also does targeted zone cleanup keyed on `PlayedCastMontage`, rather than `EndAllHitboxes()`: `EndAbility` arrives asynchronously (with a dash, much later than the montage ends), and by that point the zones may already belong to the next combo step.


---
## HUD

### HUD elements

#### Crosshair
- Appears **only** while combat stance is active (`State.InStance`)
- A minimalist dot at the center of the screen
- No animation; disappears instantly on leaving stance

#### Player Frame
Always visible in combat (independent of stance).

| Attribute           | Display type      | Source                                                                |
| ----------------- | --------------------- | ----------------------------------------------------------------------- |
| HP                | Bar + number      | `ClanhallAttributeSet::HP / MaxHP`                                      |
| AP                | Bar + number      | `ClanhallAttributeSet::AP / MaxAP`                                      |
| MP                | Bar + number      | `ClanhallAttributeSet::MP / MaxMP`                                      |
| Charges           | Charge cells, 4×4 grid (up to 16) | `Charges / MaxCharges`                                                  |
| AP-refill charges | Icons (up to N, TBD)   | Special ability, not a GAS attribute; N=3 is a placeholder (see `combat_system.md`, "Character Resources") |

**Future extension:** below the attributes - a row of active skill icons Q/E/R/F/Z/X/C/V with
the charge cost on each icon. No cooldown timer - cooldown as a mechanism doesn't exist in the project (`combat_system.md`, "Character Resources").

#### Enemy Frame
This is **boss-fight UI**, not "where I'm aiming" (raycast targeting under the crosshair turned out to be unusable - a tiny ray, a camera at ~45° that often looks at the character's back, and the frame flickering during dodges).

Appears on units with the `Unit.Role.Boss.*` role within the player's radius (later - based on the boss's combat state); N bosses in radius = N frames, one per boss.

| Attribute                 | Display                                                |
| ----------------------- | ---------------------------------------------------------- |
| HP                      | Bar + number (always visible)                             |
| AP                      | Bar + number (hidden if `MaxAP <= 0`)                |
| MP                      | Bar + number (hidden if `MaxMP <= 0`)                |
| Charges                 | Charge cells, 4×4 grid (hidden if `MaxCharges <= 0`)  |
| boss's AP-refill charges | open info, like MP/Charges; hidden if the unit doesn't have any |

Bars adapt to the target's own ASC, with no hardcoding by role: monster bosses with no AP/MP show only HP (+ Charges if present), pseudo-player bosses show all four.

This is the entire threat telemetry: lots of Charges and mana → expect active skills and magic.
A single placeholder icon for "hidden skills/magic" - a visual hint that the boss has unknown techniques, WITHOUT revealing which ones. Not a list, not a cooldown - just a "there's something hidden here" indicator. Note: the player learns exactly which skill/spell was used from the **combat log** / **spellbook**, not from a real-time plate.

---
### Targeting system

Two independent components on `AClanhallCharacter` with different jobs - previously a single raycast drove both the soft target and the Enemy Frame at once; now the frame is driven only by the sensor.

#### Soft target (for hits/marks)

- **Component:** `UClanhallTargetingComponent`
- **Method:** `LineTraceSingleByChannel(ECC_Visibility)` forward from the camera position
- **Range:** 2000 cm (20 m), editable via `MaxRange`
- **Frequency:** component tick ~20 Hz (`TickInterval = 0.05f`)
- **Valid target:** only an actor implementing `IAbilitySystemInterface`
- **Delegate:** `OnTargetChanged(AActor* NewTarget)` - the Enemy Frame **no longer** listens to it

#### Enemy Frame sensor (bosses in radius)
- **Component:** `UClanhallBossSensorComponent`
- **Method:** interval-based `SphereOverlapActors` around the owner (not a per-frame tick)
- **Frequency:** `TickInterval = 0.1f` (10 Hz - a frame isn't a crosshair)
- **Valid candidate:** implements `IAbilitySystemInterface` **and** `ASC->HasMatchingGameplayTag(Unit.Role.Boss)`
  (the parent tag matches both `.Humanoid` and `.Monster`; `Unit.Role.Mob` doesn't match - mobs get no frame)
- **Hysteresis:** `EnterRadius` (1200 cm) adds a unit, `ExitRadius` (1800 cm) removes it - kills the flicker at the boundary
- **Delegates:** `OnFrameUnitEntered(AActor*)` / `OnFrameUnitExited(AActor*)` - the container widget creates/destroys `WBP_EnemyFrame` off of them

---

### Draggable frames

- Both frames support free mouse dragging (UMG DragDrop)
- Positions are saved to `UClanhallHUDSaveGame` (slot `"HUDLayout"`) when each drag finishes
- Loaded on widget creation (`BeginPlay`)
- Default positions (first run):
  - Player Frame: bottom-left corner (~50, ~700 from top-left)
  - Enemy Frame: bottom-right corner (~1870, ~700) - depends on resolution

#### Saving position

```
SaveHUDLayout(PlayerFramePosition, EnemyFramePosition)   // ClanhallUILibrary (BP callable)
LoadHUDLayout(→ PlayerFramePosition, → EnemyFramePosition)
```


---

## Combat-Stance-and-WASD-Attacks

**Bottom line:** holding LMB puts the character into combat stance - an ability that, for its lifetime, swaps out movement parameters and repurposes WASD from movement into four directional attacks. Outside of stance the same keys just move the character, and no separate code branch is needed for that: physical actions require the `State.InStance` tag, they don't check for it in a branch.

The combo tree, input windows, and montage lifecycle are a neighboring topic (`Animation Setup.md`). Here: stance, input, and the attack abilities themselves.

### Three layers, bottom to top

```
AClanhallCharacter          - input: bindings, Shift, Space, torso realignment in Tick
    │
UGA_CombatStance            - state: State.InStance + swapped movement parameters
    │
UClanhallComboComponent     - validator: what to hit and whether to hit at all  (Animation Setup.md)
    │
UGA_DirectionalAttack_*     - resolve: damage, AP exchange, charge income
```

No layer reaches over its neighbor's head. The character doesn't know whether the input is valid;
the component doesn't know whether it's the player's side or the AI's; the ability doesn't know where the
damage number came from.

---

### Input

| Action | Triggers | Handler |
| --- | --- | --- |
| `StanceAction` (LMB) | `Started`, **`Triggered`**, `Completed`, `Canceled` | `OnStancePressed` / `OnStanceReleased` |
| `StanceMoveModifierAction` (Shift) | `Started`, `Completed`, `Canceled` | `OnStanceMoveModifier*` |
| `AttackOverhead/RightSlash/LeftSlash/LowSweep` (W/D/A/S) | `Started` | `OnAttack*` → `ComboComponent->HandleAttackInput` |
| `ActiveSkillQ/E/R/F` | `Started` | `OnActiveSkill*` |
| `SpaceAction` (Space) | `Started`, `Completed`, `Canceled` | `OnSpacePressed` / `OnSpaceReleased` |
| `MoveAction` (the same WASD) | `Triggered` | `Move` → `DoMove` |

**The stance-entry retry is on `Triggered`, and that's not a duplicate.** `Started` can arrive during
`State.ComboRecovery` and get rejected; having spent it, the character wouldn't enter stance even after the lockout drops, with LMB still held down. `Triggered` retries every frame it's held. With stance already active, this is just a cheap rejection via `ActivationBlockedTags(State.InStance)`.

**WASD attacks are separate discrete actions**, not a read of the `MoveAction` axis: they fire
once per press, not every frame. The same physical keys, different `InputAction`s.

Releasing LMB does two things, both unconditional:

```cpp
void AClanhallCharacter::OnStanceReleased()
{
    AbilitySystemComponent->CancelAbilityHandle(StanceAbilityHandle);   // instant exit
    ComboComponent->OnStanceExit();                                     // regardless of phase
}
```

#### Shift gates input, not the series

`bStanceMoveHeld` is a plain flag on the character. Checked at the top of every `OnAttack*`:

```cpp
void AClanhallCharacter::OnAttackOverhead()
{
    if (bStanceMoveHeld) return;          // this input moves, it doesn't attack
    ComboComponent->HandleAttackInput(EClanhallAttackDirection::Overhead);
}
```

The gate sits here, **not** in `UClanhallComboComponent::HandleAttackInput`: Shift is
player input, and the component is side-neutral, serving AI (for whom Shift
doesn't exist) exactly the same way.

Shift **doesn't** reset a series in progress. An open follow-up window is left untouched and ticks down
on its own timer: holding Shift mid-series means "these next hits won't happen," not "the series was
cancelled."

---

### `UGA_CombatStance`

`Abilities/GA_CombatStance.h/.cpp`, `InstancedPerActor`. Inherits from `UGameplayAbility`
**directly**, not from `UGA_ClanhallAbilityBase` - that class has `ActivationRequiredTags = State.InStance`, which is meaningless for stance itself.

Three tags in the constructor:

| Tag | Role |
| --- | --- |
| `State.InStance` | `ActivationOwnedTags` - held while the ability is active |
| `State.InStance` | `ActivationBlockedTags` - re-entry is impossible |
| `State.ComboRecovery` | `ActivationBlockedTags` - can't enter stance while Recovery is still playing out |

**Entry is closed, exit is always free.** Exit goes through `CancelAbilityHandle`, which
`ActivationBlockedTags` doesn't touch at all. The lockout kills the ability to keep attacking and to re-enter,
but not the ability to run away.

#### What stance swaps out

`ActivateAbility` saves five values and sets its own:

| What | Value in stance | Why |
| --- | --- | --- |
| `bOrientRotationToMovement` | `false` | the torso doesn't turn to face the run direction |
| `bUseControllerRotationYaw` | `false` | **not** an instant snap to the camera's yaw |
| `bUseControllerDesiredRotation` | `false` on entry | smooth realignment is enabled via `Tick` past a threshold |
| `RotationRate.Yaw` | `GetStanceTurnRate()` | torso realignment speed |
| `MaxWalkSpeed` | `StanceBaseSpeed × StanceSpeedMultiplier` | stance speed is its own, not derived from running |

`EndAbility` restores all five and calls `ResetStanceTurning()`.

`bUseControllerRotationYaw = true` would mean the capsule snapping to the camera instantly - there would be no angle at all between "where the camera looks" and "which way the torso is turned," and both the realignment and the AimOffset rely on that angle existing. Instead there's the engine's smooth
`bUseControllerDesiredRotation`, but it isn't always on - `Tick` controls it.

```cpp
Character->CancelSpaceHoldAndSprint();      // BEFORE reading MaxWalkSpeed below
// ... saving the five values ...
const UWeaponTypeData* WeaponType = Character->GetWeaponType();
const float SpeedMultiplier = WeaponType ? WeaponType->StanceSpeedMultiplier
                                         : ClanhallWeaponDefaults::StanceSpeedMultiplier;
Movement->MaxWalkSpeed = Character->GetStanceBaseSpeed() * SpeedMultiplier;
Movement->StopMovementImmediately();
```

The order - "kill the sprint first, then read `MaxWalkSpeed`" - has to be preserved, otherwise the saved value would end up carrying the running speed, and leaving stance would send the character back into a sprint. `CancelSpaceHoldAndSprint` is called from right here, not from `OnStancePressed`: that one runs **every frame** LMB is held (the `Triggered` retry) and would kill the hold timer during an active Recovery. One frame ahead of Space, before entering stance even happens.

`StopMovementImmediately()` drops Speed the same frame: `DoMove` already refuses WASD
in stance, but leftover momentum from running would bleed off over a couple tenths of a second, and the
Locomotion → CombatStance transition would blend from a run rather than from the idle pose. Fallback if the snap reads as too abrupt: a high `BrakingDecelerationWalking` for the duration of stance instead of an instant stop.

The weapon multiplier applies to a **separate base stance speed** (`StanceBaseSpeed`
on the fighter), not to running speed - `weapon_system.md`, "Weapon Proficiency".

#### Torso realignment

`AClanhallCharacter::TickStanceTurn()`, only runs while `State.InStance` is held. Two
modes:

**While moving** (Shift + WASD, `GetCurrentAcceleration()` nonzero) - realignment runs continuously, no threshold. Otherwise the fighter would run sideways relative to their facing for several seconds. The subtle turn-in-place doesn't play.

**Standing still** - hysteresis-driven:

| Field | Value | Role |
| --- | --- | --- |
| `StanceTurnThreshold` | `60.0` | entry angle: realignment starts here |
| `StanceTurnSettleAngle` | `10.0` | exit angle: realignment is considered done here |
| `StanceTurnRate` | `300.0` | speed, °/sec |
| `bStanceTurning` | - | set → the ABP plays the turn-in-place |

Two different numbers, not one: at a single shared threshold, realignment would flicker "started - immediately stopped" every frame right on the boundary.

`StanceTurnDirection` updates **every frame** the turn-in-place is playing, not only when it's first set. If
the camera swings to the other side mid-turn, `|YawDelta|` stays large, `bStanceTurning` doesn't drop - without the update the ABP would keep playing a turn toward a direction that's no longer current, even though the engine already turned the capsule correctly.

---

### What stance forbids

**Jumping.** `CanJumpInternal_Implementation()` returns `false` under `State.InStance`
and separately under `State.SkillCommitted` (a fullbody active skill occupies the legs too - otherwise the character would jump mid-dash). `SpaceAction` is bound to its own logic, but the actual jump still goes through `CanJump()` in the movement tick - that's the single cutoff point, no need to touch the input bindings.

**WASD movement.** `DoMove` bails early on `State.InStance && !bStanceMoveHeld
&& !IsFalling()`. Three conditions, and the third isn't cosmetic: the tag can be held
in midair too (holding LMB while jumping or falling), so the code checks the same `IsFalling()` predicate that gates the stance pose in the ABP. Air control isn't cut by this - the tag will "turn on" stance itself on the landing frame.

`DoMove`'s second early exit is on `State.SkillCommitted`, and it's **not** about stance:
an active skill occupies `fullbody` entirely, there are no free legs. It's deliberately not gated on `State.ComboRecovery` here - that one's about the `upperbody` tail, which it's legal to run away from.

**Space** in stance is unambiguous: no jump, no double-tap, no run - just a short
dodge, firing straight off `Started`. The split between tap, double-tap, and hold lives
in C++ on the character and only works outside of stance (`combat_system.md`, "Dodge").

Once the umbrella tag `State.Stance` exists (not implemented), move the checks in `CanJumpInternal`
and `DoMove` onto it - jumping and movement then get blocked across all three stances at once.

#### The stance gate applies to every physical action

`UGA_ClanhallAbilityBase` sets `ActivationRequiredTags = State.InStance`. So outside of
stance, WASD abilities and active skills simply don't activate - `TryActivateAbility` rejects them,
the keys act as movement, and there's no separate "if not in stance" branch anywhere in the input code.

---

### WASD attacks

`UGA_DirectionalAttackBase` (`Abstract`, `InstancedPerExecution`) plus four thin
subclasses in `GA_DirectionalAttacks.h`, each overriding exactly one method:

```cpp
virtual EClanhallAttackDirection GetDirection() const override { return EClanhallAttackDirection::Overhead; }
```

The `EClanhallAttackDirection` enum (`Overhead` = W, `RightSlash` = D, `LeftSlash` = A,
`LowSweep` = S) lives in `ClanhallCombatTypes.h`, to avoid piling up header
interdependencies: the Parry and Hitbox components read it too.

The ability **doesn't activate itself and doesn't play its own montage**. Both of those decisions are made
by `UClanhallComboComponent` upstream; the ability just receives a pre-computed damage number in
`TriggerEventData->EventMagnitude` and a ready-made montage in `OptionalObject`
(`Animation Setup.md`).

Weapon-type proficiency has **no effect at all** on the WASD series: a new weapon lets the fighter throw the
full-length series from the very first swing, with no rank whatsoever. Rank only gates access to active skills (`Weapons.md`, "Weapon Proficiency: Grant Gates").

#### Series length cap

```cpp
int32 UClanhallComboComponent::GetMaxSeriesLength() const
{
    const AClanhallHumanoidCombatant* Character = Cast<AClanhallHumanoidCombatant>(GetOwner());
    const UWeaponTypeData* WeaponType = Character ? Character->GetWeaponType() : nullptr;
    return WeaponType ? WeaponType->SeriesLength : ClanhallWeaponDefaults::SeriesLength;
}
```

The stop condition is **`StepCount >= GetMaxSeriesLength()`, with "greater than or equal to"**.
The arithmetic isn't obvious and is worth spelling out: the old condition was
`StepCount > GetClassRank()`, and the old `MaxSeriesLength` was defined as `ClassRank + 1`.
Substituting one into the other: `StepCount > ClassRank` is equivalent to `StepCount > MaxSeriesLength - 1`, which is exactly `StepCount >= MaxSeriesLength`. Carrying the `>` sign over literally, without that substitution, would give one extra hit per series compared to what was intended - neither the compiler nor a diff review would catch that mistake, the symptom only shows up in playtesting as one extra hit.

`SeriesLength` lives on the weapon type and isn't modified by proficiency. The fallback for a broken `CurrentWeapon → Type` chain is `ClanhallWeaponDefaults::SeriesLength`, not 0: zero would mean "a series of zero hits," i.e. the fighter doesn't attack at all.

#### Two resolve modes

Chosen on activation based on whether the step's montage has an `AnimNotifyState_Hitbox`
placed on it (editor markup, not code):

- **contact mode** - the ability waits for `Event.Hitbox.Hit` and resolves damage the moment the
  zone actually touches something, `EndAbility` arrives via `Event.Hitbox.Closed`;
- **instant fallback** - resolved by a sphere check right on activation, `EndAbility` the same frame.

The fallback is what preserves the invariant "the combo tree can be tested before any animations are cut."
The mechanics and markup are covered in `Animation Setup.md`.

#### Target lookup

`FindMeleeTarget` in the base class is geometry for the **instant fallback only**:
`SphereOverlapActors` with a sphere in front of the character (`TraceRange = 200`, `TraceRadius = 75`),
taking the first actor with `IAbilitySystemInterface`. A counter doesn't take its target from here - it resolves off contact (`Counter Ability.md`).

#### AP exchange

`ResolveStandardDamage` in the base class: the target loses `min(damage, target's AP)`, **50%**
of what was removed returns to the attacker's own AP, overflow goes straight into HP. All of it goes through
the generic `GE_Modify*` effects with `SetByCaller` magnitude (`GAS Fundamentals and Attributes.md`).

Returning `true` means **confirmed hit**, and marks, synergies, and mana on active skills all key off of it.
`Charges`, meanwhile, are deducted unconditionally, on activation.

#### Charge income

The income is the weapon type's `ChargeIncome`, not a flat `+1`, and it's granted starting from the
**second** hit of the series (`economy_system.md`, "Charges: Income"). Both values are captured in `ActivateAbility`, not read at the moment of contact, and each has its own reason:

```cpp
bChargeEligible     = Combo->GetStepCount() >= 1;                  // "this is at least the second hit"
PendingChargeIncome = WeaponType ? WeaponType->ChargeIncome
                                 : ClanhallWeaponDefaults::ChargeIncome;
```

**`PendingChargeIncome`** - because a weapon swap is instant, and a swing that started with a sword has to resolve with a sword's income, not a greatsword's that got drawn mid-swing.

**`bChargeEligible`** - because the snapshot is taken **before** the component increments
`StepCount`. Reading `GetStepCount()` directly from `ResolveHitOn` would give different thresholds
depending on the resolve mode of the very same hit: on the contact path (async, via
`Event.Hitbox.Hit`) `StepCount` is already incremented for this step by the time of resolve, on the instant
fallback (synchronous, inside `ActivateStep`) it isn't yet.

Granted once per swing (`bChargeApplied`), no matter how many targets got hit. No manual
reset needed: `InstancedPerExecution` gives a fresh instance on every activation.

#### Multi-target hits

The same "resource / state" split as with active skills (`mark_system.md`, "Multi-Target: Resource
vs. State"). A swing hits everyone the zone passed through (`AlreadyHit` prevents hitting one target
twice); damage and AP return apply per hit target; charges are granted once per swing.

The alternative ("a swing only hits the nearest target") was rejected: it would have required extra code that runs **against** the game's own geometric model - damage is resolved by the zone, not by target selection. Which attacks hit an area falls out of the zone's shape for free: the wide A/D/S swings pass through two targets, the narrow vertical W almost never does.

The instant fallback stays single-target by construction, and that doesn't create a discrepancy: all 20 WASD montages have zones marked up, the contact path resolves on the real swing, and the fallback for WASD effectively never fires in practice.

---

### Ability grants

| Ability | Where it's granted | Handle |
| --- | --- | --- |
| `UGA_CombatStance` | `AClanhallCharacter::BeginPlay` | `StanceAbilityHandle` |
| `UGA_Dodge` | same place, via `DodgeAbilityClass` | `DodgeAbilityHandle` |
| WASD attacks ×4 | `AClanhallHumanoidCombatant::BeginPlay` | `AttackOverheadHandle`, etc. |
| active skills Q/E/R/F | same place, from `UWeaponTypeData::Skills` | `ActiveSkillHandles`, keyed by slot tag |

Stance and dodge are player-only for now: enemies don't have stance as an ability yet. Attacks and active skills sit a level up, shared by both sides (`Combatant Hierarchy.md`, "Grant in `BeginPlay`").

Dodge is granted through the `UPROPERTY` field `DodgeAbilityClass`, rather than
`UGA_Dodge::StaticClass()` directly - otherwise the class's `EditDefaultsOnly` fields (distances, montages) would have nowhere to be exposed in the editor.

---

### Debugging

- `Clanhall.Player.ShowStats` - AP/HP/MP/Charges/Stagger on one line; check the AP exchange and charge income.
- `Clanhall.Player.ShowWeaponEconomy <directions>` (e.g. `WDAS`) - breaks down a series
  and the weapon economy invariant. Takes directions because series duration depends on which steps composed it (`Weapons.md`, "Cheat").
- `Clanhall.Player.ShowCombatState` - combat state.
- The on-screen message `WASD hit | self AP ... Charges ... ChargeIncome N` (not in shipping) is printed from `ResolveHitOn` on every confirmed hit.

| Symptom | Where to look |
| --- | --- |
| WASD moves the character instead of attacking | no `State.InStance` - stance didn't activate (or was rejected by `State.ComboRecovery`) |
| nothing happens on W while in stance | empty `FromStance.ToOverhead` slot in `UComboData` - the series can't start from this direction |
| a series runs one hit longer than intended | the cap condition's sign: should be `>=`, not `>` |
| charges trickle in from the very first hit | `bChargeEligible` is reading `StepCount` at the wrong moment |
| the character slides under an active skill's animation | the `DoMove` gate on `State.SkillCommitted` isn't firing |

---
## Weapons

**Bottom line:** a weapon is three `UPrimaryDataAsset`s (type, instance, character sheet) plus an actor
(`AClanhallWeaponActor`) that carries the visuals. A fighter's active weapon (`CurrentWeapon`) is
runtime state, set in `PostInitializeComponents`, not in `BeginPlay` - the one trap in this file worth understanding in full, not just from its symptom.

### Three assets

### `UWeaponTypeData` - what you can fight with

`AbilitySystem/WeaponTypeData.h`. The former `UClassKitData`: `ComboData` moved over unchanged, income and series cap became fields of the weapon type.

| Field | Type | Meaning |
| --- | --- | --- |
| `ComboData` | `UComboData*` | the move tree and damage profile of the WASD series |
| `SeriesLength` | `int32`, `ClampMin=1` | series length cap, TOTAL hits |
| `ChargeIncome` | `int32`, `ClampMin=1` | Charges granted per hit, starting from the second |
| `StanceSpeedMultiplier` | `float`, `ClampMin=0.1` | multiplier on the fighter's base stance speed |
| `ArmorPenetration` | `float`, `ClampMin=0.0` | flat armor-threshold penetration base |
| `Fragments` | `TArray<UWeaponFragment*>`, Instanced | empty for sword+shield - no fragments yet |
| `Skills` | `TMap<FGameplayTag, UAbilityData*>` | active skills by slot, keyed by `Slot.*` |
| `ProficiencyTagsByRank` | `TArray<FGameplayTag>` | proficiency tags, index 0 = rank 1 |

`ChargeIncome` and `SeriesLength` live **only** on the type and aren't overridden on the instance -
a legendary sword that gives more charges for the same hit would be exactly the kind of arbitrage the weapon economy invariant was written to kill (`economy_system.md`, "Weapon Economy Invariant"). Legendary status is expressed through damage, penetration, and marks, not technique.

`StanceSpeedMultiplier` is a multiplier, not cm/s: an absolute number wouldn't carry over between fighters with different base speeds. `ClampMin = 0.1`, not `0`: zero would mean "doesn't move at all in stance," and that's not a state the design calls for. There's no consumer yet, it'll arrive in stage 4.

Fallbacks for missing data live in one place, not scattered across three files:

```cpp
namespace ClanhallWeaponDefaults
{
    constexpr int32 ChargeIncome = 1;
    constexpr int32 SeriesLength = 2;
    constexpr float ArmorPenetration = 0.0f;
}
```

The `UPROPERTY` defaults match these constants - an empty `UWeaponTypeData` asset is immediately
playable, not dead.

#### `UWeaponData` - what's actually in your hands

`AbilitySystem/WeaponData.h`.

| Field | Type | Meaning |
| --- | --- | --- |
| `Type` | `UWeaponTypeData*` | what this instance can fight with - an object, not a tag |
| `WeaponClass` | `TSubclassOf<AClanhallWeaponActor>` | the main-hand actor class, not a mesh |
| `DamageBonus` | `float` | a flat bonus on top of the type's damage profile, no consumer in C++ yet |
| `ArmorPenetrationBonus` | `float` | a bonus on top of the type's penetration base, no consumer in C++ yet |
| `Fragments` | `TArray<UWeaponFragment*>`, Instanced | this is where `UWeaponOffhandFragment` lives |

#### `UCharacterSheetData` - what I know and what I'm proficient with

`AbilitySystem/CharacterSheetData.h`.

| Field | Type | Meaning |
| --- | --- | --- |
| `Loadout` | `TArray<UWeaponData*>` | the starting weapon set, not what's currently in hand |
| `STR`, `DEX` | `int32` | starting stat values, no scaling from them exists in code |
| `Perks` | `FGameplayTagContainer` | including proficiency ranks, `Perk.Proficiency.*` |
| `SeenSkills` | `FGameplayTagContainer` | skills seen in combat; nothing writes to it yet |
| `LearnedSkills` | `FGameplayTagContainer` | skills the fighter has learned |

`Loadout` is an array, not a single field: a single field couldn't express an enemy that swaps weapons mid-fight, and that's exactly the kind of enemy the design calls for. For the player, the loadout is slots 1-6; for an enemy, it's a set that the AI cycles through. The loadout doesn't hold the active weapon - that's the fighter's runtime state, see "`CurrentWeapon` and `PostInitializeComponents`" below.

### Weapon actor

`ClanhallWeaponActor.h/.cpp`. A weapon instance references an actor class, not a mesh: bare geometry has nowhere to hang a burning blade, blood on the edge, sound, or a trail - all of those are components, and components live on an actor.

```cpp
UCLASS(Abstract, Blueprintable)
class CLANHALL_API AClanhallWeaponActor : public AActor
{
    GENERATED_BODY()
public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    TObjectPtr<UStaticMeshComponent> WeaponMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    FName AttachSocketName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
    FTransform AttachRelativeTransform;
};
```

`Abstract` + `Blueprintable` - the `TSubclassOf<AClanhallWeaponActor>` picker will only offer Blueprint subclasses, not an arbitrary actor: exactly "so nobody accidentally puts a tree in someone's hand." `WeaponMesh` collision is disabled in the constructor (`ECollisionEnabled::NoCollision`) - hit zones live on montage notifies (`ClanhallHitboxComponent`), and physical collision on a sword in hand would only get in their way.

`AttachSocketName` and `AttachRelativeTransform` are properties of the actor, not the type or the instance:
grips differ even within the same weapon type, and the socket lives on the wielder's skeleton - which socket actually fits is something only the specific weapon's geometry knows. The transform sits right there too - it's easier to eyeball it in the weapon's Blueprint than to guess numbers blind in a data asset.

The off-hand works the same way: `UWeaponOffhandFragment::OffhandClass` is also an actor class, not a mesh. The fragment's criterion hasn't changed - absence of an off-hand means "left hand is free."

### Spawning and attachment

`AClanhallHumanoidCombatant::SpawnAndAttachWeapon`, called twice from `BeginPlay` - once for `CurrentWeapon->WeaponClass`, and if a `UWeaponOffhandFragment` is present, once for its `OffhandClass`.

```cpp
FActorSpawnParameters SpawnParams;
SpawnParams.Owner = this;
AClanhallWeaponActor* SpawnedActor = GetWorld()->SpawnActor<AClanhallWeaponActor>(WeaponClass, SpawnParams);
// ...
const FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, false);
SpawnedActor->AttachToComponent(OwnerMesh, AttachRules, SocketToUse);
SpawnedActor->SetActorRelativeTransform(SpawnedActor->AttachRelativeTransform);
```

The order matters and isn't reshuffled: `SnapToTarget` zeroes the actor's relative transform against the socket, and only after that does `SetActorRelativeTransform` lay the
designer's offset (`AttachRelativeTransform`) on top of that zero. In the reverse order the snap would wipe the offset with that same zero.

Four fallbacks, all logged - there's no silent path:

| Case | Level | What happens |
| --- | --- | --- |
| `WeaponClass` not set | `Verbose` | a legal state (weaponless-visual testing), `nullptr` with no spawn |
| `SpawnActor` returned `nullptr` | `Warning` | a real case - `WeaponClass` points at `AClanhallWeaponActor` itself (`Abstract`, but the `TSubclassOf` picker still shows abstract classes) |
| `GetMesh()` returned `nullptr` | `Warning` | nowhere to attach to, the actor stays in the world at the root transform |
| socket empty or not found on the skeleton | `Warning` | attachment falls back to the wielder mesh's root |

The last fallback is a deliberate choice, not an oversight: silently dropping the weapon at the character's center isn't acceptable - "sword sticking out of the belly" is a symptom that takes half an hour to spot by eye.

`EndPlay` destroys both spawned actors (`SpawnedWeapon`, `SpawnedOffhand`) - without this,
running PIE twice leaves swords lying around on the level.

### `CurrentWeapon` and `PostInitializeComponents`

This is the file's main gotcha.

`CurrentWeapon` - the weapon currently in hand, a runtime field on `AClanhallHumanoidCombatant`,
not serialized. Set in `PostInitializeComponents`:

```cpp
void AClanhallHumanoidCombatant::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    CurrentWeapon = (CharacterSheet && CharacterSheet->Loadout.IsValidIndex(0))
        ? CharacterSheet->Loadout[0] : nullptr;
}
```

**Not in `BeginPlay` - that's a trap, not a stylistic choice.** `AActor::BeginPlay` dispatches `BeginPlay`
to components before the actor's own overridden `BeginPlay` body runs.
`UClanhallParryComponent::BeginPlay` reads `GetWeaponType()` via `HasOpponentWithMarkSynergy`, to decide whether the owner's `Stagger` builds up at all (`combat_system.md`, "Stagger"). Put `CurrentWeapon` initialization in `BeginPlay` after `Super::`, and at the moment of that read `CurrentWeapon` is still `null`: the `Stagger` bar would silently decide the opponent has nothing to cash a mark in with, and would never build up at all. **No error, no warning** - the mechanic simply never turns on.

`PostInitializeComponents` for actors placed in the level runs for **every** actor before even one of them starts `BeginPlay` - which means the other side's sheet is already ready by this point too. An empty `Loadout` is a legal state, `CurrentWeapon` stays `null`, and the fallbacks in `BeginPlay` (a warning about the broken chain, fighting on `ClanhallWeaponDefaults`) run just as before.

`GetWeaponType()` is the single point that reads weapon type: `CurrentWeapon ? CurrentWeapon->Type : nullptr`. Nobody reads the type from `CharacterSheet` directly.

### Weapon proficiency: grant gates

`UWeaponTypeData::GetRequiredProficiencyRank(SlotTag)` - the rule is the same for every weapon, so it lives in code rather than being duplicated on every asset:

| Slot | Required rank |
| --- | --- |
| Q, E | 1 |
| R, F | 2 |
| Z, X | 3 |
| C, V | 4 |

`IsSlotUnlocked(SlotTag, Perks)` checks `ProficiencyTagsByRank[RequiredRank - 1]` in the sheet's perk container; a rank outside the array's bounds (an under-filled asset) is treated as locked, not a crash. `ProficiencyTagsByRank` is an array, not a runtime name-built tag: string assembly breaks silently on the very first rename, and the project's tags are locked precisely against that.

`AClanhallHumanoidCombatant::BeginPlay` grants `UGA_PhysicalSkill` once per entry
in `GetWeaponType()->Skills`, running each entry through seven checks in sequence:

1. the key is valid and isn't the root `Slot` (but a leaf like Q/E/R/F/…) - otherwise `Warning`, the slot
   technically gets granted but `GetActiveSkillHandle` will never find it;
2. the value (`UAbilityData*`) isn't `nullptr` - otherwise `Warning`;
3. `GetRequiredProficiencyRank(Key)` recognized the slot (not 0) - otherwise `Warning`;
4. `IsSlotUnlocked` - the tier is unlocked by proficiency rank. Locked → `Verbose`, a normal
   proficiency state, not a data error;
5. `Skill.Value->CounterTag` is valid - otherwise `Warning`, there's nothing to learn the skill as;
6. the skill is listed in `CharacterSheet->LearnedSkills` - not learned → `Verbose`, also normal;
7. passes all six - it's granted, the slot is carried via the spec's dynamic tag
   (`Spec.GetDynamicSpecSourceTags().AddTag(Skill.Key)`), the handle goes into `ActiveSkillHandles`.

A final `Log` line: "N skills granted, N filtered by rank, N filtered by unlearned" - without it the symptom "I press Q, nothing happens" can't be diagnosed: the cause (rank or knowledge) isn't visible.

### Cheat

`Clanhall.Player.ShowWeaponEconomy <directions>` (e.g. `WDAS`) - prints a breakdown of a series and the weapon economy invariant (`economy_system.md`, "Weapon Economy Invariant"). Takes a directions argument because series duration depends on which steps composed it - transitions between different direction pairs play different montages of different lengths. Prints two income-per-second figures - with the recovery animation in the denominator and without: recovery deals no damage and pays no charges, but it does take up series time, and both numbers are needed to avoid confusing "income while the fighter is actively swinging" with "income while the fighter is occupied by the whole series."

Resolves the `Combatant->GetCurrentWeapon() -> Type -> ComboData` chain manually, rather than through `GetWeaponType()`: that one doesn't suit a cheat - `GetWeaponType()` returns `nullptr` on any break in the chain, erasing exactly WHERE it broke. `GetCharacterSheet()` and `GetCurrentWeapon()` exist on the fighter for precisely this distinction - reading the chain step by step with a precise break message.

---
## Animation-Setup

**Bottom line:** the animation layer sits on top of a finished mechanic and owns none of the
decisions. What to hit is decided by `UClanhallComboComponent` from `UComboData`; when to hit is
decided by notifies on the montage; what to hit with is decided by `FClanhallHitboxDesc`, set on that
same notify. Strip out every animation and the combo tree, damage, and economy keep working through the
instant fallback. That's not degradation, it's an invariant: the tree gets tested before any clips are cut
(`CLAUDE.md`, "The Mechanic Works Without Animation Assets").

---

*Knight animation status:*

| col/row | Stance |  W  |  A  |  S  |  D  |
| :------ | :----: | :-: | :-: | :-: | :-: |
| Stance  |   ❌    |  ✅  |  ✅  |  ✅  |  ✅  |
| W       |   ✅    |  ❌  |  ✅  |  ✅  |  ✅  |
| A       |   ✅    |  ✅  |  ❌  |  ✅  |  ✅  |
| S       |   ✅    |  ✅  |  ✅  |  ❌  |  ✅  |
| D       |   ✅    |  ✅  |  ✅  |  ✅  |  ❌  |

---

**Mocap**

https://github.com/user-attachments/assets/1a1860be-d227-4b33-a482-516d13a3bf6f

**Sequencer**

https://github.com/user-attachments/assets/30800fd3-17b4-4992-8a20-a2b77ff6b3f6

**Animation Montage**

https://github.com/user-attachments/assets/65780ea9-4e29-4790-a1eb-3d9732e2fdaa

---
### One attack's flow

```
WASD input ──► UClanhallComboComponent::HandleAttackInput
                 │  gates, validates against the tree, picks the montage
                 ▼
              ActivateStep ──► TriggerAbilityFromGameplayEvent ──► GA_DirectionalAttack_*
                 │                                                │ waits on Event.Hitbox.*
                 ▼
              Montage_Play(upperbody)
                 │
                 ├─ AnimNotifyState_Hitbox   ──► UClanhallHitboxComponent::BeginHitbox
                 │                                  │ sweep frame by frame
                 │                                  └─► Event.Hitbox.Hit ──► ResolveHitOn
                 │                              EndHitbox ──► Event.Hitbox.Closed ──► EndAbility
                 │
                 └─ AnimNotifyState_ComboWindow ──► OnComboWindowOpen / OnComboWindowClose
                                                        │
                                                        ├─ there's input and a valid transition → next step
                                                        └─ no → EndSequenceWithRecovery
```

The key inversion: the ability no longer activates itself and doesn't play its own montage.
It receives a ready-made damage number and a ready-made montage from the component, and its only job is resolving damage. Invalid input never reaches it at all.

---

### Data: `UComboData`

`AbilitySystem/Fragments/ComboData.h`. One asset per **weapon type**, assigned to the
`ComboData` field on `UWeaponTypeData` (`Weapons.md`, "Three Assets").

The model is **transition pairs, not paths**. A move is decided purely by the pair "previous
direction → new"; series history before the previous step plays no role in the resolve. There's neither a Data
Table of moves nor a list of chains anywhere in the project. Consequence: `A→W` is a single clip for every branch, no matter which direction you arrived at `A` from.

#### Asset fields

| Field | Type | Meaning |
| --- | --- | --- |
| `Overhead`, `RightSlash`, `LeftSlash`, `LowSweep` | `FDirectionalDamage` | damage profile PER DIRECTION: `BaseDamage` + `DamageType` (placeholder tag, not read in the calculation) |
| `StanceAnim` | `UAnimSequence*` | the combat-stance idle loop pose - a Sequence, not a montage |
| `StanceLocomotion` | `UBlendSpace*` | stance locomotion (Shift + WASD); `nullptr` is legal - the ABP plays `StanceAnim` |
| `FromStance` | `FComboTransitionSet` | 4 openers out of stance |
| `FromOverhead` / `FromLeftSlash` / `FromRightSlash` / `FromLowSweep` | `FComboTransitionsFrom*` | 3 continuations each |
| `Recovery` | `FComboRecoveryAnimations` | 4 tails back into stance, keyed by the series' last direction |

That's **20 montage slots** total: 4 openers + 12 transitions + 4 Recovery.

#### Direction is a field's name, not a value

There's no `Direction` field inside `FDirectionalDamage`: direction is set by the name of the
owning field. A separate field would have been a second source of truth alongside the damage profile - a mismatch between the two wouldn't be caught by either the compiler or a diff review.

#### Repeating a direction isn't representable in the type

The continuation sets **lack** a slot for their own direction - it's not empty, it doesn't
exist:

```cpp
/** Continuations after a W hit. There's no ToOverhead slot: repeating a direction is disallowed by design. */
USTRUCT(BlueprintType)
struct FComboTransitionsFromOverhead
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category = "Combo") TObjectPtr<UAnimMontage> ToLeftSlash;   // A
    UPROPERTY(EditAnywhere, Category = "Combo") TObjectPtr<UAnimMontage> ToRightSlash;  // D
    UPROPERTY(EditAnywhere, Category = "Combo") TObjectPtr<UAnimMontage> ToLowSweep;    // S
};
```

This isn't field-count economy, it's design expressed in the type system: a second identical hit isn't a continuation, it's a new attack, with Recovery in between them. The movement is the same either way, there's nothing to cut a W→W "transition" out of. Nothing to fill in means nowhere to get it wrong.

#### An empty slot is a legal state

`nullptr` carries three different meanings depending on where it sits:
- in `FromStance` - the series can't start with this direction, the matching WASD key doesn't attack at all;
- in `From*` - the continuation is disallowed, the series drops into Recovery;
- in `Recovery` - the tail is baked into the hit montage itself, there's nothing separate to play.

Resolving is three simple switches (`FindOpenerMontage` / `FindTransitionMontage` /
`FindRecoveryMontage`), no searching and no conflict logging: a slot is either filled or empty,
there's nothing to conflict.

#### Why `UComboData` has no fragments

Its composition is fixed - damage + stance + transitions always exist. A fragment wrapper only earns its place when **absence** carries meaning a field's default can't express (`Architecture.md`, "The Header-or-Fragment Boundary"). `UCharacterSheetData` follows the same reasoning.

#### Recovery is a montage, not a Sequence

`PlaySlotAnimationAsDynamicMontage` still creates a `UAnimMontage` at runtime: the montage isn't eliminated, it just becomes non-authorable, and `BlendIn`/`BlendOut`/`BlendOutTriggerTime`
move from the asset into the C++ function's argument defaults - one value for all four
directions, zero tuning in the editor. The analogy to `StanceAnim` doesn't hold: stance plays through the
Sequence Player in a state machine (no montage), Recovery plays through a slot (there is a montage).

---

### The series component: `UClanhallComboComponent`

`AbilitySystem/ClanhallComboComponent.h/.cpp`. Lives on `AClanhallHumanoidCombatant` -
for both the player and an AI fighter. One entry point, side-neutral.

#### State

| Field | Type | Meaning |
| --- | --- | --- |
| `LastDirection` | `TOptional<EClanhallAttackDirection>` | the last direction played; unset = neutral. Drives both the next transition and Recovery |
| `StepCount` | `int32` | length of the current series; 0 = neutral. Checked against `GetMaxSeriesLength()` |
| `bReadWindowOpen` | `bool` | the input-reading window is open |
| `LatestInWindow` | `TOptional<...>` | the last press within the window |
| `LastPlayedMontage` | `TWeakObjectPtr<UAnimMontage>` | identity key for the montage-end delegate |
| `StanceExitBlendOutTime` | `float = 0.18` | blend-out on leaving stance; **must match** the Duration of the Locomotion ↔ CombatStance transition in the ABP |

`GetMaxSeriesLength()` reads the current weapon's `UWeaponTypeData::SeriesLength`, falling back to
`ClanhallWeaponDefaults::SeriesLength` on a broken `CurrentWeapon → Type` chain.
The cap belongs to the weapon, not the fighter, and isn't modified by proficiency rank
(`Combat Stance and WASD Attacks.md`, "Series Length Cap"). The tree can contain entries longer than the cap - they're simply unreachable.

`OnComboWindowOpened` / `OnComboWindowClosed` (`BlueprintAssignable`) - the only
extension of the public API meant for AI: the moment the driver has to supply a direction. Which one - the BT decides, the component gives no hint.

#### An input gate, not a buffer

Before the window opens, presses are discarded outright and don't accumulate; inside the open
window, "last one wins." No accumulation - no sticky responsiveness.

#### Entry gates

Five early returns, strictly in this order:

```cpp
void UClanhallComboComponent::HandleAttackInput(EClanhallAttackDirection Direction)
{
    UAbilitySystemComponent* ASC = GetASC();
    if (!ASC || ASC->HasMatchingGameplayTag(State_ComboRecovery))  return;  // series tail
    if (ASC->HasMatchingGameplayTag(State_DodgeRecovery))          return;  // dodge tail
    if (ASC->HasMatchingGameplayTag(State_SkillCommitted))         return;  // active skill committed
    if (ASC->HasMatchingGameplayTag(State_Stunned))                return;  // stunned
    if (Char->GetCharacterMovement()->IsFalling())                 return;  // stance is ground-only

    if (StepCount == 0)   { TryStartSequence(Direction); return; }
    if (bReadWindowOpen)  { LatestInWindow = Direction;  return; }
    // window closed (wind-up phase) - the input is discarded outright
}
```

The first four **deliberately duplicate** `ActivationBlockedTags` on `UGA_ClanhallAbilityBase`:
those gate the ability's activation, these gate entry into the component earlier, before `ActivateStep` and its
`ForceEndHitboxes()`. Without the `State.SkillCommitted` gate, a WASD press right after an active skill would cut off its contact window even though the charges were already spent. The `IsFalling` gate: holding LMB while falling is fine and `State.InStance` can stay set, but a hit montage can't play mid-fall.

#### Resolve order on window close

`OnComboWindowClose`, top to bottom:

1. no input arrived → terminal;
2. `StepCount >= GetMaxSeriesLength()` → terminal, **even if the transition slot is filled**;
3. `LastDirection` unset while `StepCount > 0` → defensive terminal (a desync:
   `LastDirection` and `StepCount` are required to change together);
4. `FindTransitionMontage` returned `nullptr` → terminal (an invalid continuation - no damage
   and no scale movement, there's no penalty for it);
5. `ActivateStep` returned `false` → terminal, state is **not committed**;
6. otherwise `LastDirection = Dir`, `++StepCount`.

#### Order inside `ActivateStep`

Critical as a whole - every step relies on the one before it:

```cpp
ForceEndHitboxes();                    // (1) previous step's zones - BEFORE activating the next
HitboxComp->ResetSuppression();        // (2) "until the end of the current step" expires right here
// ...
if (!ASC->TriggerAbilityFromGameplayEvent(Handle, ..., Event_DirectionalAttack, &EventData, *ASC))
    return false;                      // (3) failed - state isn't committed
ClearSwingDirectionTag();              // (4) only on success
ApplySwingDirectionTag(Direction);
OwnParry->ResetParry();                // (5) dedup "I just parried"
PlayMontage(Montage);                  // (6)
```

(1) - otherwise `Event.Hitbox.Closed` from the interrupted montage would land on the **new**
ability and cut it off before its own zone even opens.

(4) sits **after** the early return on purpose. It used to sit before the activation attempt:
on failure the tag would get cleared, `LastDirection` wouldn't get reassigned (the caller only does that
on success), and `ResetCombo()` would clear the same tag a second time - the refcount drifted.

`EventData` carries two fields: `EventMagnitude` = the direction's `BaseDamage` from the profile
(the ability no longer stores damage itself) and `OptionalObject` = the `Montage` itself - the
ability uses it to pick a resolve mode.

The parried-step counter reset (`ResetStaggerSeries`) doesn't live here, it lives in
`TryStartSequence` - the single point where a series actually **starts**, unconditionally and before the
activation attempt. No need to duplicate it in `ResetCombo()`: the counter has to be zero between series,
while activation itself can fail.

#### Finishing: `EndSequenceWithRecovery`

The single point where a series ends - both the fall-through from `OnComboWindowClose`
and the montage-end delegate land here. The order is just as critical, as a whole:

```cpp
void UClanhallComboComponent::EndSequenceWithRecovery()
{
    if (StepCount == 0) return;                        // (1) guard against a double Recovery

    const UComboData* Data = GetComboData();
    UAnimMontage* RecoveryMontage = (Data && LastDirection.IsSet())
        ? Data->FindRecoveryMontage(LastDirection.GetValue()) : nullptr;   // (2) BEFORE resetting

    ResetCombo();                                      // (3) clears LastDirection

    if (!RecoveryMontage) return;                      // (4) tail is baked in - no lockout at all

    // (5) NO Montage_SetEndDelegate: PlayMontage() here would be a trap loop
    //     (end of Recovery → OnAttackMontageEnded → EndSequenceWithRecovery → Recovery again)
    const float PlayedDuration = AnimInst->Montage_Play(RecoveryMontage);
    if (PlayedDuration == 0.f) { UE_LOG(..., Warning, ...); return; }
    LastPlayedMontage = RecoveryMontage;

    // (6) lockout exactly as long as the animation, no fractions, no padding
    ClanhallGameplayEffects::ApplyTimedTag(ASC, State_ComboRecovery, RecoveryMontage->GetPlayLength());
}
```

**Recovery animation ≠ the `State.ComboRecovery` tag.** The animation always plays after a terminal hit, except when leaving stance mid-hit. The tag is the gameplay lockout on new attacks **and on entering stance** (not on leaving it), and is only applied if the montage actually started. There are no tuning fields (`RecoveryLockFraction`, `ComboRecoveryDuration`) in the project: there's no such thing as an invisible lock.

`ResetCombo()` **doesn't** clear this tag - deliberately. Otherwise the combination "leave stance and
immediately re-enter" would cancel the lockout for free. The tag lives on its own timer, independent of the
series' state.

#### Interruptions: three different paths

**A foreign montage** - `CancelSequenceForExternalMontage()`. Mandatory to call before any
`Montage_Play` in `DefaultGroup` (an active skill, a hit reaction, a stun, a cast) - and **before**
subscribing to `Event.Hitbox.*`, otherwise its own `Closed` from the zones being closed would cut off the ability before contact. Clears state without a Recovery animation and without a tag: a foreign montage is already occupying the slot, and there's no penalty for interruption. It doesn't stop the montage itself - the caller's own `Montage_Play` will override it. Clears `LastPlayedMontage` - otherwise the guard in `OnAttackMontageEnded` would let through the delegate of the interrupted hit montage and close the zones of an already-live active skill.

**Leaving stance** - `OnStanceExit()`. `Montage_Stop` with `StanceExitBlendOutTime`
and `ForceEndHitboxes()` - **only when `StepCount > 0`**. At `StepCount == 0` it's either
neutral, or Recovery is already playing: it has to finish playing out, it's exactly what shows the player why new attacks and entering stance don't work yet. Zones can't be closed at `StepCount == 0` - anything open there belongs to an active skill in its commit phase, and `Event.Hitbox.Closed` would cut it off before contact.

**End of montage** - `OnAttackMontageEnded`, a safety net for when there's no notify on the montage
or it didn't fire. First thing, a guard:

```cpp
if (Montage != LastPlayedMontage.Get())
{
    // delegate from a montage that's already been superseded: mustn't touch the current owner's zones
    return;
}
ForceEndHitboxes();          // before the early return on bInterrupted - covers the main stuck-zone case
if (bInterrupted) return;    // a chain or OnStanceExit already ran its own path
EndSequenceWithRecovery();
```

The guard is mandatory because the delegate arrives **asynchronously**, after the blend-out, by which
point the zones already belong to the next step. Synchronous ordering inside `ActivateStep` isn't enough on its own for this.

---

### Hit zones

#### `FClanhallHitboxDesc` - what we're hitting with

`AbilitySystem/ClanhallHitboxTypes.h`. The dispatcher owns no geometry of its own: the whole shape comes from data set on the specific notify of the specific montage.

| Field | Type | Meaning |
| --- | --- | --- |
| `Bone` | `FName = "WeaponSocket"` | the attach bone or socket; the default reproduces the old hardcoded attachment |
| `LocationOffset` | `FVector` | offset in the bone's space |
| `RotationOffset` | `FRotator` | rotation; for a capsule this sets the axis (the capsule's axis is local Z) |
| `Shape` | `Sphere` / `Capsule` / `Box` | `Sphere` is the default and the fallback |
| `Radius`, `HalfHeight`, `BoxExtent` | `float` / `FVector` | dimensions, field visibility gated by `EditCondition` on `Shape` |
| `bParryable` | `bool = true` | whether the swing participates in a parry clash |

`bParryable = false` on every Q/E/R/F active skill: an active skill never participates in a clash at all. The side effect matters more than the flag itself - a stale `CurrentDirection` from the last WASD hit **never even reaches** the parry check, because for these zones the check isn't called in the first place.

Three geometry functions live on the struct itself, not on the component:

```cpp
bool  GetWorldTransform(const USkeletalMeshComponent* Mesh, FVector& OutLoc, FQuat& OutRot) const;
float GetEffectiveHalfHeight() const { return FMath::Max(HalfHeight, Radius); }
FCollisionShape MakeCollisionShape() const;
```

This isn't for convenience, it's an invariant: the same function is called by both the runtime sweep and the
editor preview, so what's drawn and what actually hits can never drift apart. `GetEffectiveHalfHeight` clamps
a degenerate capsule (`HalfHeight < Radius`) - a cross-field `ClampMin` can't be expressed in
metadata.

#### `UClanhallHitboxComponent` - the dispatcher

A zone's lifecycle:

```
AnimNotifyState_Hitbox::NotifyBegin → BeginHitbox(Desc, this) → a handle (>0) or 0 on rejection
AnimNotifyState_Hitbox::NotifyEnd   → EndHitbox(this)
```

The Begin/End pair is linked **by the notify's pointer**, not by handle: `UAnimNotifyState` is a
shared const object, per-playback state can't live on it.

Several zones can be open at once, and each has **its own** set of hit targets
(`AlreadyHit`). This is exactly what gives Shield Charge "every enemy in the dash's path, exactly once"
for free, with no separate target collection along the trajectory.

**Several zones = several damage instances, always** (`DataAsset and Fragments.md`, "Multi-Zone
Hits"). There's no such thing as "two shapes of one contact": a blade capsule plus a shield sphere on the same montage will deal two damage instances on one swing. To cover the geometry of a single swing, grow the **shape** of an existing zone, not add a second one. Multiple zones are only justified for sequential contact phases (shield closed - sword opened).

The sweep goes from last frame's pose to the current one (`SweepMultiByObjectType`), the zone's
first tick is skipped - a sweep from a nonexistent previous position would give a false hit.

#### Events

| Event | When | Payload |
| --- | --- | --- |
| `Event.Hitbox.Hit` | on every target hit for the first time | `Target` = the target, `EventMagnitude` = the zone's handle |
| `Event.Hitbox.Closed` | **strictly** on the transition "there were zones → none remain" | - |

`Closed` can't be sent on every `End*` call: sending it against an empty list would cut off an
ability that activated but hasn't opened its zone yet.

Beyond that, the component also fires `OnHitboxHit` (`BlueprintAssignable`) - for VFX/SFX.

#### The "one resolver per actor" invariant

`Event.Hitbox.Hit` and `Event.Hitbox.Closed` broadcast across the whole ASC **with no addressing**. The
system is correct precisely because, at any given moment, no more than one ability is resolving
contact damage. It's held up by call order, not by filters:

- `ActivateStep` closes the previous step's zones **before** activating the next ability;
- an active skill cancels the WASD series **before** it subscribes to events.

The zone handle in `EventMagnitude` is a ready-made extension point, should the invariant ever
need loosening. For now, no filter is added against it: that would create the appearance of protection where
what's actually protecting the system is call order.

#### Zone suppression

`SuppressHitboxes()` - any contact against the owner (a clash or a landed hit) suppresses their zones until the end of the current step: open ones close, `BeginHitbox()` rejects further attempts from then on. `ResetSuppression()` is called on the owner's next step/activation - "until the end of the current step" expires exactly there (`combat_system.md`, "The Cross-Cutting Principle: Contact Shuts Down the Recipient's Zone").

One subtlety: if there were no open zones (the typical clash case - the fighter's own zone hasn't managed to
open yet), `Event.Hitbox.Closed` is sent **explicitly**. Without this, `EndAllHitboxes()` wouldn't
emit it (there's no "were → aren't" transition), and `UGA_DirectionalAttackBase` on the contact path
waits on exactly that as its sole terminator - the ability would never end.

Suppression and hitstop live **on the contact itself**, in `TickHitbox`, not in the damage calculation: a
hit fully absorbed by the armor threshold, and utility skills without a `UDamageFragment`, never reach
`ResolveStandardDamage`. Contact is a fact that happened regardless of whatever the formula decides afterward.

#### Hitstop

`ApplyHitstop(Duration)` - slows the current montage **belonging to whoever's zone landed the hit**
(not both, and not the recipient). Play rate recovers via a timer.

| Field | Value | Meaning |
| --- | --- | --- |
| `HitstopPlayRate` | `0.15` | play rate for the duration of the hitstop |
| `HitstopDurationOnDamage` | `0.1` | confirmed damage, ~3-5 frames @30fps |
| `HitstopDurationOnClash` | `0.166` | clash, **keep it ≤ 5 frames** |

The clash upper bound isn't cosmetic: on the player, hitstop pushes their own release rightward, while a boss has no symmetrical delay. At 8+ frames, a chain of parries stops converging by the math (`Parrying.md`).

#### Cleaning up after a `NotifyEnd` that got eaten

Zones are opened by a notify, not by the ability, and a `NotifyEnd` swallowed by the blend-out would leave a zone sweeping forever. `UGA_PhysicalSkill::EndAbility` calls `EndHitboxesFromMontage(PlayedCastMontage)` - targeted, only its own.

The identity key is the montage itself: `UAnimMontage::Notifies[i].NotifyStateClass`, despite the field's name, is actually a pointer to the notify **instance**, and that exact pointer arrives in `BeginHitbox` as
`Source`. A pointer comparison, no new state needed.

The blunt `EndAllHitboxes()` is off-limits here for two reasons: `EndAbility` arrives asynchronously (with `UDashFragment` - from `OnDashFinished`, long after the montage ends, once the zones already belong to the next combo step), and `GA_DirectionalAttackBase` listens for `Event.Hitbox.Closed` - someone else's cleanup would cut off a live WASD hit. A nonzero count of closed zones logs a warning with a direct hint ("move the notify end 1-2 frames earlier"): a stuck zone is a markup mistake, not the norm.

WASD hits don't have this hole: the combo component owns the hit montage's lifecycle
and is backstopped by `ForceEndHitboxes()`.

#### Two rendering points

At runtime - `bDrawDebugHitboxes` on the component: shape, swept path, and hit points.
In the montage window - `UAnimNotifyState_Hitbox::DrawInEditor` (`WITH_EDITOR`), same color coding:
**cyan** = `bParryable`, **orange** = not. Without the second one, zones on cast montages would be marked up
blind: `DrawDebug*` needs a `UWorld`, and Persona has neither a world nor an actor with the component.

A preview limitation worth remembering: the zone drawn is **the pose at that frame**, not the volume
actually swept. On a fast swing the real hit area is wider than what's drawn.

---

### Two damage resolution modes

Chosen on activation, with a single question to the montage:

```cpp
const UAnimMontage* StepMontage = TriggerEventData
    ? Cast<UAnimMontage>(TriggerEventData->OptionalObject.Get()) : nullptr;
const bool bResolveOnContact = UAnimNotifyState_Hitbox::MontageHasHitbox(StepMontage);

if (!bResolveOnContact)
{
    // no montage, or no zones placed on it - resolve instantly with a sphere
    ResolveHitOn(FindMeleeTarget(Avatar));
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    return;
}

// contact path: EndAbility is NOT called here - waiting on Event.Hitbox.Hit/Closed
```

The instant fallback is a deliberate choice, not a degradation: it's exactly what lets you
test the combo tree before any animations are cut. The geometry is `SphereOverlapActors` in front of the
character, single-target by construction (`Combat Stance and WASD Attacks.md`, "Target Lookup").

A side effect of the contact path: if the zone never actually got opened (LMB released mid-wind-up →
`OnStanceExit` → `ForceEndHitboxes` → `Closed` with not a single `Hit`), the ability ends
without a single `ResolveHitOn`. "An interrupted hit doesn't count" becomes true in the code, not just in the doc.

#### An active skill waits for whichever of two terminators fires last

`UGA_PhysicalSkill::TryFinishAbility()` - `EndAbility` only when
`bPrimaryTerminatorFired && !bDashPending`. Without this, the fallback path would kill the dash task
in the very same frame, and a short montage would cut a dash off halfway through.

The primary terminator is always the end of the cast montage, whenever the montage started, regardless of
resolve mode: whether zones are marked up is a question of how damage is counted, not of how
long the playout lasts (`DataAsset and Fragments.md`, "Ability Terminator").

The flip side is preserved deliberately: an external `CancelAbility` from a counter kills the task,
and the character stops mid-windup - that's exactly the visible result of a broken active skill.

---

### Notifies: what to mark up on a montage

| Notify | Where it's placed | What it does |
| --- | --- | --- |
| `AnimNotifyState_Hitbox` | on every hit montage and cast montage, around the contact phase | opens/closes the hit zone |
| `AnimNotifyState_ComboWindow` | on every hit montage; **not** on Recovery | opens/closes the input-reading window |
| `AnimNotifyState_CounterWindow` | on cast montages | the window in which a skill can be countered |
| `AnimNotifyState_ParryWindow` | on the defender's hit montages | applies `State.Parrying` (owned by `Parrying.md`) |

#### General markup rules

**Montage Tick Type = Branching Point** - for all four, no exceptions. Regular notifies
are processed at the tick boundary and, at low FPS, eat frames - noticeable on windows
of 0.2-0.5 s.

**Don't place the end of a state flush against the end of the montage** - leave 1-2 frames of margin, otherwise `NotifyEnd`
lands in the blend-out and the zone leaks (the warning from `EndHitboxesFromMontage` is precisely about this case).

**`ComboWindow`: starts around 70% into the montage, ends exactly on the second-to-last frame.** The end has to be **after** the contact window - otherwise the next step would cut off the current step's own hit.

**A zone with `bParryable = false` never reads the hit direction** - `SetCurrentDirection` isn't required for such montages.

**A step with no Hitbox notify still works** - damage resolves instantly on activation. Markup
is only required where a contact resolve is needed.

#### `CounterWindow` - markup with no data

The notify has no fields of its own. The lookup key is the montage itself: `NotifyBegin` searches for the active spec whose `UAbilityData::CastMontage` matches what's playing, and pulls `CounteredBy` from there. An empty `CounteredBy` means the window never opens at all.

`NotifyBegin` and `NotifyEnd` search using the **exact same** criterion - the shared
`FindOwningAbilityData`. No handle is stored between Begin and End because there's nowhere to store it: the notify is a
shared const object, the criterion is recomputed from scratch. This symmetry is load-bearing, not
cosmetic: a different criterion would give a `NotifyEnd` that closes someone else's window.

One asset serves both the player and whichever enemy the skill was granted to - there are no tags to
duplicate on the track. The constraint when placing it: the countering skill's contact frame has to
land inside this window (`Counter Ability.md`).

---

### ABP and slots

#### The graph

```
Main States (locomotion + jump)
   └─► cached pose 'lowerbody'
          ├──────────────────────────────► Base Pose ─┐
          └─► Slot 'upperbody' ─► cached ─► Blend[0] ─┴─► Layered Blend Per Bone
                                                                  │  (spine_01, depth 1)
                                                                  ▼
                                                          Slot 'fullbody'
                                                                  ▼
                                                           Control Rig ─► Output
```

The pin order and `fullbody`'s position after the blend node aren't stylistic, they're a correctness condition;
details and reasoning live in `locomotion_structure.md`, "Slots: Creation and Assignment".

#### Slot assignment

- **`upperbody`** - WASD hits (openers and transitions) and Recovery tails.
- **`fullbody`** - **every** Q/E/R/F active skill, no exceptions.

The old "light ones on top, heavy ones full-body" split has been dropped: an active skill always plays out full-body. A common mistake is a montage left in `DefaultSlot`: the logic works, the animation just doesn't show.

Both slots are in **`DefaultGroup`**, so montages override each other. This is deliberate (the alternative would give a combo montage ticking in parallel with notifies firing under a visible `fullbody` dash). A direct consequence is the rule to call `CancelSequenceForExternalMontage()` for every new source of `Montage_Play`.

#### The freedom to leave is tied to the slot, not the action type

Exactly what the montage occupied is what's locked. `upperbody` - legs are free, releasing LMB and
running away mid-playout is legal. `fullbody` - the montage occupies the whole body, there's nowhere to leave from; without a gate this produced ground-sliding under a full-body animation.

A separate `State.Busy` isn't needed for this: `State.SkillCommitted` already means exactly that
and lasts exactly as long. The cost: a stuck tag costs character control, not just attack blocking.

A started active skill can't be interrupted manually - `State.SkillCommitted` lives until `EndAbility`,
not until the first `Event.Hitbox.Closed`. The only way to break it is the opponent's counterattack (`CancelAbilityHandle` from `ConsumeCounter`). Leaving combat stance isn't affected by this: `GA_CombatStance` inherits from `UGameplayAbility` directly and doesn't have the base class's `ActivationBlockedTags`.

#### The `CombatStance` state

Inside `Main States`: a Sequence Player, Loop. The `Sequence` pin is promoted to `CurrentStanceAnim`
and filled from data, not hardcoded - otherwise swapping weapons wouldn't change the pose:

```cpp
// AClanhallCharacter, both static and take ACharacter: inside Event Blueprint Update Animation
// there's usually already a cached Character variable typed as ACharacter, no second Cast needed.
static UAnimSequence* GetStanceAnim(const ACharacter* Character);        // UComboData::StanceAnim
static UBlendSpace*   GetStanceBlendSpace(const ACharacter* Character);  // UComboData::StanceLocomotion
```

`nullptr` for `StanceLocomotion` is a legal state: the ABP plays `StanceAnim` like before,
stance locomotion works without a single new asset.

The Idle/Locomotion ↔ CombatStance transition runs on `bInStance` = `HasMatchingGameplayTag(State.InStance)`, read in `Event Blueprint Update Animation` (**not** Thread Safe - the ASC isn't safe from a worker thread). **Duration 0.18 both ways** = the combo component's `StanceExitBlendOutTime`, otherwise a visible step shows on exit.

Legs come from this state, arms come from the slot layered on top. The montage ends → the slot is empty → the
arms return to stance with not a single line of return code.

Magic and anti-magic stances don't fit into this - they're global, one set for every weapon
type, and aren't implemented yet.

---

### A skill's displacement is data, not a property of the clip

`UDashFragment` (`Distance` / `Duration`) + `UAbilityTask_ApplyRootMotionMoveToForce`
in `UGA_PhysicalSkill`. The clip is cut **in-place**, `Enable Root Motion` is **disabled** on it -
otherwise animation root motion would stack with the Root Motion Source and the dash would fly twice as far.

A fragment, not a header field: its absence means "the skill doesn't move the character," which
`Distance = 0` doesn't express.

A consequence that's easy to miss: the dash's multi-hit only works because the capsule actually
travels. With an in-place animation and no fragment, the zone stays put and can only ever hit one target.

An example of tuning - **Shield Charge (R)**, `Duration = 0.433`. The number isn't arbitrary: the clip
is 1.1 s / 33 frames at 30 fps, the movement phase is frames 1-13, i.e. 13/30 s. Any longer and the character
keeps flying over the recovery frames and slides along the ground. `Distance` is the
only free number (starting at 300 cm → 6.9 m/s); as distance grows, foot-slide grows with it,
and that's exactly what caps the value.

---

### Layer utility tags

| Tag | Who applies it | What it means |
| --- | --- | --- |
| `State.ComboRecovery` | `EndSequenceWithRecovery`, for Recovery's `GetPlayLength()` | lockout on new attacks and **entering** stance; leaving is free |
| `State.SkillCommitted` | `UGA_PhysicalSkill`, until `EndAbility` | the active skill commits fully; gates both `DoMove` and `CanJumpInternal` |
| `State.DodgeRecovery` | `UGA_Dodge`, for the length of `DodgeRecoveryMontage` | the tail of a short dodge in stance |
| `Attack.Direction.*` | `ApplySwingDirectionTag` in `ActivateStep` | symmetric telegraph of one's own swing direction; read by `TryParry` |
| `Event.DirectionalAttack` | `ActivateStep` | carries `BaseDamage` from the component to the ability; does **not** gate which ability is chosen |
| `Event.Hitbox.Hit` / `.Closed` | `UClanhallHitboxComponent` | zone contact / end of the contact phase |

The general rule for all three `*Recovery` locks: the tag is applied **only if the montage actually
started**, and for exactly its length. There's no such thing as an invisible lock in this system, under any circumstances.

---

### Cutting and markup: the animator's job

#### Prep work

**The `WeaponSocket` socket lives on the character's skeleton**, for both the player and the enemy. Not on the
weapon's skeleton: `UClanhallHitboxComponent` only looks for the bone/socket on `ACharacter::GetMesh()`
and never queries a separate weapon mesh at all. The default `FClanhallHitboxDesc::Bone` expects exactly this name.

**Centering the pelvis on X/Y** relative to root on mocap clips - a custom `UAnimationModifier`
(`AM_CenterPelvisXY`), run on the clip. There's no shared modifier for Z, only a visual check per clip.

#### The cutting model: a graph of anchor poses

Nodes: `Stance` (the anchor) and one "after a hit" per direction - after-W / after-A /
after-D / after-S. A clip is an **edge** of the graph: a transition from node to node carrying one hit's
animation, addressed by the pair "entry node, hit" - exactly what a transition slot stores. The
cutting model maps onto the data model one-to-one.

**Cut from the longest chain.** Take hits within a branch from the same mocap take
(e.g., the `W` inside the `D→A→W` chain carries the series' real momentum), place cut points at the
anchor poses - then the end of `D→A` matches the start of `A→W`, a seam inside a single take, with no
snap. Assembling hits from different takes is worse: it tears the body's momentum apart.

**Consistent nodes.** Every clip entering a node and every clip leaving it must converge on one
canonical pose for that node. A mismatch between takes - clean up the ends or add a micro-blend of 0.05-0.08 s at
the node: velocity is minimal there, the blend isn't visible.

**Openers are cut specifically from stance.** A hit taken from the middle of a take doesn't work for this role.

**Recovery gets its own tail per node**, not a shared one. The sword stays left or right
depending on the last hit's direction; a shared blend between incompatible poses produces a mess.

#### Naming convention

- openers: `Stance_W` / `Stance_A` / `Stance_D` / `Stance_S`;
- transitions: `W_A`, `W_D`, `W_S`, `A_W`, …;
- tails: `W_Recovery` / `A_Recovery` / `D_Recovery` / `S_Recovery`;
- raw mocap takes, kept separate: `AS_Mocap_..._Raw`.

**Status:** all 20 montages are cut and marked up; repeating a direction (`W→W` etc.) doesn't
exist in the model, there's nothing to cut for it.

#### Zone shape on a cast montage

The shape answers "what am I hitting with," not "where's the enemy":

- Power Strike - a capsule along the blade;
- Shield Slam - a sphere on the shield bone, small radius (didn't reach - didn't hit, and it's visible
  right in the editor);
- Shield Charge - a capsule on `pelvis`;
- War Shout and similar - a large sphere on `root`, not tied to a weapon.

As soon as a zone is placed, the skill switches itself from instant resolve to contact resolve automatically - no code changes needed. Every cast montage has `bParryable = false`.

On WASD hits the zone stays at its default (a sphere on `WeaponSocket`, `bParryable = true`) -
existing montages don't need remarking.

---
## Parrying

**Bottom line:** the clash resolves on the **attacker**, not the defender. The window is marked up on the
defender's montage (`AnimNotifyState_ParryWindow` → `State.Parrying`), the opposing direction is telegraphed by code (`Attack.Direction.*`), and the resolve itself is triggered by the attacker's blade making contact. The defender's weapon geometry plays no part in the check whatsoever. A second subsystem grows out of the clash - the `Stagger` fatigue bar, which has three feeders and a single entry point.

Canon lives in `combat_system.md`, "Parrying". The interim path through `GA_EnemyWASDSeries` /
`UGA_Series_Crosscut` (code applying tags manually on a timer, with no animation) didn't survive: both classes and `AClanhallTrainingDummy` were removed once the enemy moved to the shared `UClanhallComboComponent` and montage markup.

### One clash's flow

```
defender                              attacker
─────────────                         ──────────
ActivateStep(D)                       ActivateStep(A)
  └─ Attack.Direction.D on self         └─ Attack.Direction.A on self
Montage_Play                          Montage_Play
  └─ ParryWindow.NotifyBegin            └─ Hitbox.NotifyBegin ─► BeginHitbox
       └─ State.Parrying                     └─ sweep ─► contact against the defender
                                                  │
                                    UClanhallHitboxComponent::CheckAndHandleParry
                                      │ does the target hold State.Parrying?  (otherwise - normal damage)
                                      ▼
                                    ParryComponent(attacker)::TryParry(target, A, point)
                                      │ A is parried by the tag Attack.Direction.D - matched
                                      ▼
                        ┌─────────────┴─────────────┐
             defender                          attacker
             +1 Charge                        AddStagger(from the second step onward)
             SuppressHitboxes()               ApplyHitstop(OnClash)
                        damage goes through to neither
```

### Resolve direction is an inversion, not a detail

The clash used to resolve on blades meeting: "my hitbox reached the tag on the target." That's
unworkable on timing - the defender reacts to an animation and is therefore objectively behind on their own swing. The model was flipped, and everything else in this file follows from that:

**`TryParry` lives on the ATTACKER's component** - the owner of the zone that touched the target. It's called from `UClanhallHitboxComponent::CheckAndHandleParry`, not from input handlers. Success is checked against tags on the **target**, not on self. Credit goes to the attacker because their blade landed first - so they're the one reacting to the clash, not the one who reacted in time.

Three clash conditions, all checked at the moment of contact:

1. the attacker's zone has `bParryable == true` (checked by the caller);
2. the target holds `State.Parrying` (checked by the caller);
3. the target holds `Attack.Direction.<opposite>` (checked by `TryParry`).

The opposing pairs are W↔S, A↔D. The mapping inside `TryParry` goes from **one's own**
direction to the tag that parries it:

```cpp
switch (MyDirection)
{
case EClanhallAttackDirection::Overhead:    ParriableTag = Attack_Direction_S; break;  // W ← S
case EClanhallAttackDirection::LowSweep:    ParriableTag = Attack_Direction_W; break;  // S ← W
case EClanhallAttackDirection::RightSlash:  ParriableTag = Attack_Direction_A; break;  // D ← A
case EClanhallAttackDirection::LeftSlash:   ParriableTag = Attack_Direction_D; break;  // A ← D
default: return false;
}
```

Not to be confused with the similarly-named switch `DirectionToOwnSwingTag` in `UClanhallComboComponent`: that one maps **forward** (one's own direction → the tag you apply to yourself),
this one maps **backward** (one's own direction → the tag that parries it). These are different
questions, and both switches have to exist separately.

The direction tag is applied by **code**, not by animation (`ActivateStep`) - unlike
`State.Parrying`, which is applied by montage markup. The reason is simple: hit direction
isn't readable from the animation. Both tags are applied to **self**, symmetrically for the player and AI.

#### Clash credit

| Who | Gets |
| --- | --- |
| The attacker - whose zone landed the contact | `Stagger` (from the second parried step of the series onward) + hitstop `HitstopDurationOnClash` |
| The defender - whose parry was recognized as a clash | `+1 Charge` + suppression of their own zone until the end of the current step |

Damage never goes through to anyone on a clash: `CheckAndHandleParry` returns `true` → `TickHitbox`
does a `continue` and `Event.Hitbox.Hit` never fires.

The defender's charge is a **flat `+1`, not scaled by weapon** (`economy_system.md`,
"Charges: Income"). Scaling the defender's income by their weapon would mean "parrying with a dagger doesn't pay off," i.e. a penalty on defense, and defense is half the income. Only the attacking channel gets normalized. Don't swap it for `ChargeIncome`.

---

### `UClanhallParryComponent`

`AbilitySystem/ClanhallParryComponent.h/.cpp`. Lives on the fighter, symmetric between player and AI - role in the resolve plays no part anywhere.

#### State and settings

| Field | Type | Meaning |
| --- | --- | --- |
| `bStepParried` | `bool`, public | **the owner's current step has already been counted as parried** - a dedup within the step, not "I parried" |
| `ParriedStepsThisSeries` | `int32`, private | how many steps of the owner's current series have been parried; grows the bar from the second onward |
| `bStaggerGateOpen` | `bool`, private | whether the owner's `Stagger` subsystem is enabled; computed once in `BeginPlay` |
| `StaggerDecayDelay` | `float = 10.0` | the pause with no events before decay starts |
| `StaggerDrainDuration` | `float = 5.0` | how many seconds it takes to drain the **full** bar, not one tick |
| `StaggerDecayTimer` | `FTimerHandle` | one handle for both decay phases |
| `ClashSound` | `USoundBase*` | the sound of weapons colliding, played inside `TryParry` |

The name `bStepParried` survived a reversal of meaning: it used to be `bParrySuccessful` ("I parried"), it's now "my current step has already been parried." It's a guard against a single swing granting two `Stagger` values if its multi-phase or multi-target zone hit several parrying defenders.

#### Two resets that must not be confused

```cpp
void ResetParry();                                     // per-STEP dedup guard
void ResetStaggerSeries() { ParriedStepsThisSeries = 0; }  // per-SERIES income counter
```

`ResetParry()` is called by `UClanhallComboComponent::ActivateStep` - before **every** new step the owner takes. `ResetStaggerSeries()` is called by `TryStartSequence` - the moment a new series actually starts, unconditionally and before the activation attempt.

The series counter reset sits on the **opening** of a new series, not the closing of the previous one:
there are several completion paths (terminal, Recovery, interruption by stance, leaving stance) and there
will be more, and each would need its own copy of the reset. On opening, there's exactly one path.

`ParriedStepsThisSeries` is private with no getter on purpose. It's an **income** counter, not
a marker of "consecutive" parries, which the BT will need for a retreat decision. The semantics differ (example: `D→A→D` - two parries of the same direction, not consecutive), one shared counter doesn't fit both purposes.

#### Order inside `TryParry`

```cpp
if (bStepParried || !HitTarget)              return false;   // (1) step dedup
if (!TargetASC)                              return false;   // (2)
// (3) the opposing-direction switch - see above
if (!TargetASC->HasMatchingGameplayTag(ParriableTag)) return false;

bStepParried = true;                                          // (4) lock in the step
PlaySoundAtLocation(ClashSound, HitLocation);

OwnHitbox->ApplyHitstop(OwnHitbox->HitstopDurationOnClash);    // (5) hitstop - to SELF
TargetHitbox->SuppressHitboxes();                              // (6) suppression - to the TARGET

++ParriedStepsThisSeries;                                      // (7) the first step is free
AddStagger(ParriedStepsThisSeries > 1 ? 1.0f : 0.0f);

ApplyModifyEffect(OwnASC, TargetASC, UGE_ModifyCharges, 1.0f); // (8) charge - to the TARGET
return true;
```

(5) and (6) go in opposite directions, and that isn't a typo: hitstop goes to the zone's owner, because
it's their blade that landed the contact; suppression goes to the target, because contact hit them (`combat_system.md`, "The Cross-Cutting Principle: Contact Shuts Down the Recipient's Zone"). The defender's own zone at this point usually hasn't even opened yet - the parry window always closes before their Hitbox notify. It's exactly for this case that `SuppressHitboxes()` sends `Event.Hitbox.Closed` explicitly, otherwise the defender's ability would never end.

(8) doesn't depend on `AddStagger` or on the subsystem gate: **the charge is paid for the action**,
while fatigue is paid for the pattern.

---

### Window markup

`AnimNotifyState_ParryWindow` (`Animation/AnimNotifyState_ParryWindow.h/.cpp`). Carries
no fields at all - `NotifyBegin`/`NotifyEnd` add and remove the loose tag `State.Parrying`
on the owner's ASC, i.e. whoever is playing this hit montage.

It's specifically an `AnimNotifyState`, not a pair of single `AnimNotify_ParryWindowStart/End`: a single `End` wouldn't fire on montage interruption and would leave `State.Parrying` stuck on indefinitely.

Boundaries on the montage (`combat_system.md`, "The Clash Mechanic: Resolves on the Attacker's Contact, Not on the Defender's Reaction"):

- **start at frame 2**, not 0: the zero frame gets cut off by the chain during a transition between steps;
- **end 1 frame before the start of the montage's own `AnimNotifyState_Hitbox`**: the wind-up is over,
  the fighter's own active frames have begun;
- Montage Tick Type = **Branching Point**, like every window in the project (`Animation Setup.md`).

The timing penalty falls out of these boundaries by itself, with not a single line of code: press too early and by the time the enemy's blade lands you've already left the wind-up for your own active frames; press too late and you never made it into the wind-up.

---

### What doesn't participate in a clash

**Q/E/R/F active skills.** WASD parries WASD, active skills counter active skills (`ability_system.md`,
"Counter Skill"). Expressed through data, not a special case: `CheckAndHandleParry` is only called for zones with `FClanhallHitboxDesc::bParryable == true`, and cast montages set `false`. The side effect matters more than the flag itself - a stale `CurrentDirection` from the last WASD hit **never even reaches** the parry check, because for these zones the check isn't called in the first place.

**The defender's weapon geometry.** It plays no part, and never has. The whole condition
is "the attacker's attack zone touched an actor holding `State.Parrying`, and the direction
matched." This is the Sekiro model (timing + state, rather than weapon-on-weapon collision),
and it's already the most forgiving version possible: what gets parried is exactly what would have
hit you anyway.

**No separate generous spheres for parrying.** They'd produce the opposite effect -
parrying hits that would have missed, which reads as an accidental trigger. If parrying turns out to be hard, tune the **window's length** for `State.Parrying`, not the zones' size.

What can't be parried at all - `combat_system.md`, "What Can't Be Parried".

---

### The `Stagger` bar

A separate resource on `UClanhallAttributeSet`, builds up on whoever's getting outplayed. The full model and numbers live in `combat_system.md`, "Stagger"; here's just the implementation.

### Attributes

| Attribute | Where it lives | Value |
| --- | --- | --- |
| `Stagger` | `UClanhallAttributeSet` | clamped to `[0, MaxStagger]` in both `PreAttributeChange` **and** `PostGameplayEffectExecute` |
| `MaxStagger` | same | initialized from `AClanhallCombatantBase::DefaultMaxStagger` (placeholder `4.0`) |

`DefaultMaxStagger` lives on the base fighter, not in `AClanhallCharacter::BeginPlay` - that
already cost a bug: an instance without this path (`AClanhallHumanoidBoss` with an empty constructor)
was left with zeroed attributes, `MaxStagger = 0` clamped `Stagger` to `[0, 0]`,
and `GetStagger() >= GetMaxStagger()` was already true on the very first clash - the boss got stunned off a single parry instead of the intended four.

#### `AddStagger` - the single entry point

Three feeders of the bar: parrying (`TryParry`), a counter skill (`ConsumeCounter`), and future
anti-magic (`Amount` = the number of words in an intercepted spell). There are no direct attribute writes anywhere in their code.

The order inside is strict:

```cpp
void UClanhallParryComponent::AddStagger(float Amount)
{
    if (!bStaggerGateOpen) return;                        // (1) a FULL no-op, timers included

    World->GetTimerManager().ClearTimer(StaggerDecayTimer);  // (2) interrupt any active drain

    if (Amount > 0.0f)                                    // (3) AddStagger(0) is legal
        ApplyModifyEffect(ASC, ASC, UGE_ModifyStagger, Amount);

    if (Attributes->GetMaxStagger() > 0.0f &&             // (4) the cap
        Attributes->GetStagger() >= Attributes->GetMaxStagger())
    {
        ApplyModifyEffect(ASC, ASC, UGE_ModifyStagger, -Attributes->GetStagger());  // reset to 0
        MarkComp->ApplyMark(Mark_Staggered, nullptr);     // a mark, NOT a stun
    }

    ScheduleStaggerDecay();                               // (5) restart the pause
}
```

**`AddStagger(0)` is legal and required** for the first parried step of a series. It isn't a
no-op for no reason - it's a deliberate signal, distinguishable from no event having happened at all: a counter-action occurred, there's no payout, but the drain still gets interrupted and the pause restarts.

**The `MaxStagger > 0` check in (4) is mandatory.** An unset cap gives `0 >= 0` -
true on **every** call, including `AddStagger(0)`, and the `Staggered` mark would get applied
instantly. A copy of this same check sits in `OnStaggerDecayDelayElapsed` - there it
guards against division by zero, here against a false cap; there's no reason to merge them.

**The cap doesn't stun.** It applies `Mark.Staggered` on the owner, which the opponent has to
cash in in time with a skill that has synergy for it (`mark_system.md`, "Staggered - a Mark With No Source Skill"). `State.Stunned` is never granted from here at all: its only source is `UMarkTriggerFragment`. A stun is earned twice. The mark's source is unknown at this level (`AddStagger` only carries `Amount`), so `nullptr` in place of an instigator is legal: `IsOwnMark` for `Mark.Staggered` has no consumer yet.

#### Subsystem gate

```cpp
void UClanhallParryComponent::BeginPlay()
{
    Super::BeginPlay();
    if (const AClanhallHumanoidCombatant* Character = Cast<AClanhallHumanoidCombatant>(GetOwner()))
        bStaggerGateOpen = Character->HasOpponentWithMarkSynergy(Mark_Staggered);
}
```

As long as the owner's **opponent** has not a single skill that cashes in `Staggered`, the bar
doesn't build up and the mark never gets applied. An indicator that fills up with nothing to spend it on teaches the player to ignore it.

`HasOpponentWithMarkSynergy` goes through `FindPrototypeOpponent` - the first other `AClanhallHumanoidCombatant` found in the level (`Combatant Hierarchy.md`, "Prototype Opponent Lookup"). The skill set isn't deliberately filtered by proficiency gates: the question "is there anything to cash it in with" decides whether the **opponent's** bar builds up, and filtering it by rank would mean tying someone else's bar to progression, which is unresolved anywhere.

This is also the main limitation: the gate is computed **once, in `BeginPlay`**, there's no
recompute on a weapon swap or a change of opponent.

#### Decay: one handle for two phases

```cpp
ScheduleStaggerDecay()          // SetTimer(StaggerDecayTimer, OnStaggerDecayDelayElapsed,
                                //          StaggerDecayDelay, /*loop*/ false)
   └─ the StaggerDecayDelay pause expired with not a single event
OnStaggerDecayDelayElapsed()    // SetTimer(the same handle, DecayStaggerStep,
                                //          StaggerDrainDuration / MaxStagger, /*loop*/ true)
   └─ every tick
DecayStaggerStep()              // −1 Stagger; Stagger <= 0 → ClearTimer, stop
```

`SetTimer` with the same handle fully replaces the previous schedule - so one field is enough for both the one-shot pause and the repeating tick, and any call to `AddStagger` (including a zero one) cuts the drain off mid-way, preserving the remainder.

**The interval is computed from the cap, not a fixed constant:** `StaggerDrainDuration / MaxStagger`. This fixes the **speed** of draining the full bar, and it's independent of any particular fighter's cap. With a constant interval, a fighter with a higher cap would drain more slowly - meaning the penalty for inaction would grow right along with their toughness.

`DecayStaggerStep` writes to the attribute directly, bypassing `AddStagger` - a legal exception
to the "single entry point" rule. It's not a feeder, it's an internal mechanism of the same
subsystem, with nothing to add besides `−1` per tick; routing `−1` through `AddStagger`
would mean every decay tick interrupting and re-scheduling itself.

---

### The counter skill - the second feeder

`UClanhallCounterComponent` (`AbilitySystem/ClanhallCounterComponent.h/.cpp`). The topic's
owner is `Counter Ability.md`; here, just the joint with the bar.

While the window is open, the owner holds `State.CounterWindow`, and the component remembers the set of skills
that can interrupt this active skill (`CounteredByTags`) and its handle. A match via
`HasTag` (tag hierarchy included) = a counter:

```cpp
void UClanhallCounterComponent::ConsumeCounter()
{
    if (!bWindowOpen) return;
    ASC->CancelAbilityHandle(CounteredHandle);            // the active skill is broken
    OwnParry->AddStagger(1.0f);                           // the same entry point parrying uses
    OwnHitbox->ApplyHitstop(OwnHitbox->HitstopDurationOnClash);
    OnCounterConsumed.Broadcast();
    CloseWindow();
}
```

Hitstop happens **synchronously**, not through `OnCounterConsumed`: that delegate is a hookup
point for the recipient's future reaction (flinch/VFX/sound), while hitstop has to land the same frame
as the counter itself.

The penalty for whoever got countered: the cast is cancelled, the charges already spent are lost, `+1` fatigue, and hitstop. There's no stun and no cooldown to reset: no cooldowns remain anywhere in the project (`economy_system.md`, "Why There Are No Cooldowns"), and the fields `CounterStunDuration`, `FullParryStunDuration`,
`FullParryCooldownReduction`, `ReduceCooldowns` were removed both as fields and as mechanics.

**Control-stripping marks were cut.** `Staggered`/`Stunned` as marks applied by a **skill** (rather than the bar's cap) are gone from the project: a mark changes parameters and vulnerabilities, a mark never takes away input. Loss of control only ever comes from the fatigue bar, cashed in via synergy - never directly from a skill.

---
### Debugging

- `Clanhall.Player.ShowStats` / `Clanhall.Enemy.ShowStats` - print the resource line,
  including `Stagger N/M`. The first thing to check for "parry isn't counting."
- `Clanhall.Player.SetStat MaxStagger <N>` / `Clanhall.Enemy.SetStat ...` - tune the cap with no recompile; `UClanhallAttributeSet`'s clamps still apply.
- `Clanhall.Enemy.AddMark Mark.Staggered` - test the cash-in without filling the bar.
- `bDrawDebugHitboxes` on `UClanhallHitboxComponent` - see whether a zone actually reached anything: cyan = `bParryable`, orange = not.
- The on-screen message `✓ CLASH (parry)!` (not in shipping) is printed from `TryParry` exactly on success - its absence with visible contact means the direction condition or the step dedup failed.


---

## Counter-Ability

**Bottom line:** an active skill started inside an open window is interrupted by any skill from its
`CounteredBy` set - the moment the countering zone touches the one who gets countered. Whoever got countered loses the charges already spent and the cast, gains `+1 Stagger` and visible hitstop; the countering skill still deals full damage and commits as an ordinary application. There's no second layer of punishment - no stun, no cooldown reset: a counter is just an ordinary hit, with a broken cast thrown in.

Canon lives in `ability_system.md`, "Counter Skill"; the punishment economy is in `economy_system.md`,
"Punishing Whoever Gets Countered".

### One counter's flow

```
victim (getting countered)                counterer
──────────────────                        ─────────
UGA_PhysicalSkill::ActivateAbility        UGA_PhysicalSkill::ActivateAbility
  └─ Charges deducted unconditionally       └─ Charges deducted unconditionally
Montage_Play(fullbody)                    Montage_Play(fullbody)
  └─ CounterWindow.NotifyBegin              └─ Hitbox.NotifyBegin
       └─ FindOwningAbilityData                  └─ sweep ─► contact against the victim
            └─ OpenWindow(CounteredBy, Handle)        │
                 └─ State.CounterWindow              ▼
                                        UGA_PhysicalSkill::ResolveHitOn(victim)
                                          │ (1) ResolveStandardDamage - FULL damage
                                          │ (2) TryResolveCounter(victim, own CounterTag)
                                          ▼
                        UClanhallCounterComponent::IsCounterableBy(tag)
                          │ window open AND CounteredByTags.HasTag(tag)
                          ▼
                        ConsumeCounter()
                          ├─ CancelAbilityHandle  → the victim's cast is cancelled,
                          │                          their EndAbility clears State.SkillCommitted
                          ├─ AddStagger(1.0)      → +1 fatigue for the victim
                          ├─ ApplyHitstop(OnClash)
                          ├─ OnCounterConsumed.Broadcast()   (flinch/VFX - not implemented)
                          └─ CloseWindow()
                                          │ (3) mark/synergy  (4) mana
```

### Data asymmetry

Two paired fields in `UAbilityData`'s header, and they must not be confused:

```cpp
/** The skill's identity - WHAT I COUNTER WITH, not who I counter. */
UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Ability"))
FGameplayTag CounterTag;

/** What can interrupt THIS skill - vulnerability. Empty = this skill can't be countered by anything. */
UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Ability"))
FGameplayTagContainer CounteredBy;
```

The attacker presents **one** tag - its own identity. The defender holds a **set**. Both
fields are read by the exact same code for the player and for the enemy; there's no separate enemy path anymore.

**The set lives on the defender, not the attacker**, because it's the defender who opens the window
and hands the container over directly. Keep the list on the attacker instead, and every window opening would need to scan every skill asset in the game looking for whichever ones counter this one.

The historical reason there are two fields at all: originally an active skill was interrupted by that exact same skill - matching one `CounterTag` against itself. The rule changed to "different classes counter different skills," the relationship became many-to-many, and a single field carrying both "who I am" and "who I counter" had to be split apart. It worked exactly as long as those two things were the same thing.

#### Granularity is tuned in the data

`IsCounterableBy` matches via **`HasTag`**, not `HasTagExact`. Consequence: an entry
`Ability.Lancer` in `CounteredBy` matches any Lancer skill, while `Ability.Knight.PowerStrike` matches only that one specific hit. Granularity is chosen by how the asset is filled in, with not a single line of code.

An empty container is a legal and meaningful state: "not countered by anything" (War Shout
and similar utility skills). Self-countering stopped being automatic and became an explicit entry in the set.

---

### `UClanhallCounterComponent`

`AbilitySystem/ClanhallCounterComponent.h/.cpp`. On both fighters, symmetrically.

#### State

| Field | Type | Meaning |
| --- | --- | --- |
| `bWindowOpen` | `bool` | the window is open |
| `CounteredByTags` | `FGameplayTagContainer` | what can break the active skill currently in progress |
| `CounteredHandle` | `FGameplayAbilitySpecHandle` | that active skill's handle - used to cancel it |
| `CachedASC` | `TWeakObjectPtr<...>` | a lazy cache, resolved on first access |

The component holds neither a stun parameter nor cooldown parameters: `CounterStunDuration` was removed
along with the field on `UAbilityData`, and `CounteredCooldownTag` / `CounteredCooldownDuration` -
along with the entire slot-based cooldown scheme.

#### API

```cpp
void OpenWindow(const FGameplayTagContainer& InCounteredBy, FGameplayAbilitySpecHandle InCounteredHandle);
void CloseWindow();                                     // the window closed with no counter
bool IsCounterableBy(FGameplayTag IncomingTag) const;   // bWindowOpen && CounteredByTags.HasTag(Tag)
void ConsumeCounter();                                  // the counter happened
static bool TryResolveCounter(AActor* Target, FGameplayTag IncomingCounterTag);  // shared resolver
```

`OpenWindow` / `CloseWindow` additionally apply and remove `State.CounterWindow` - a loose tag on the owner's ASC. That's the externally visible state, "my active skill can be countered right now."

`TryResolveCounter` is static and takes the target as its first argument: it looks for the component on the **target**, not on self. The calling skill doesn't need to know anything beyond its own `CounterTag`.

#### `ConsumeCounter` - order

```cpp
void UClanhallCounterComponent::ConsumeCounter()
{
    if (!bWindowOpen) return;                    // (1) a double trigger is impossible

    ASC->CancelAbilityHandle(CounteredHandle);   // (2) the cast is cancelled

    OwnParry->AddStagger(1.0f);                  // (3) the bar's single entry point
    OwnHitbox->ApplyHitstop(OwnHitbox->HitstopDurationOnClash);   // (4) reuses the clash implementation

    OnCounterConsumed.Broadcast();               // (5) flinch/VFX/sound - a hookup point
    CloseWindow();                               // (6)
}
```

(1) isn't a formality: `ConsumeCounter` closes the window, so a repeat call within the same
application returns immediately. Double-countering a single active skill is impossible by construction, no separate dedup is needed.

(3) goes through `AddStagger`, not a direct attribute write: a counter is the **second of three
feeders** of one shared bar (the first is parrying, the third is future anti-magic). None of the feeders write directly to `Stagger` anywhere in the code (`Parrying.md`).

(4) reuses `UClanhallHitboxComponent::ApplyHitstop` - the same implementation the parry clash resolve uses, with the same `HitstopDurationOnClash` duration. Called
**synchronously**, not from `OnCounterConsumed`: that delegate is a hookup point for the recipient's future reaction, while hitstop has to land the same frame as the counter itself.

(2) cancels the active skill via `CancelAbilityHandle`, and from there the overridden
`UGA_PhysicalSkill::EndAbility` takes over - the single point that clears `State.SkillCommitted`. Whoever got countered doesn't get stuck committed, the tag leaves along with the ability. If the countered active skill had a dash running (`UDashFragment`), the Root Motion Source task is cleared along with the ability, and the character stops mid-windup - that's exactly the visible result of a broken active skill.

---

### The window is opened by markup, not code

`AnimNotifyState_CounterWindow` (`Animation/AnimNotifyState_CounterWindow.h/.cpp`) - carries no fields of its own. The lookup key is the montage itself:

```cpp
const UAbilityData* FindOwningAbilityData(UAbilitySystemComponent* ASC, const UAnimMontage* Montage,
                                          FGameplayAbilitySpecHandle& OutHandle)
{
    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        if (!Spec.IsActive()) continue;
        const UAbilityData* Data = Cast<UAbilityData>(Spec.SourceObject.Get());
        if (!Data || Data->CastMontage != Montage || Data->CounteredBy.IsEmpty()) continue;
        OutHandle = Spec.Handle;
        return Data;
    }
    return nullptr;
}
```

`NotifyBegin` takes `CounteredBy` from the asset it found and calls `OpenWindow`. An empty
`CounteredBy` means the window never opens at all - exactly the same condition as "the skill can't be countered."

**`NotifyBegin` and `NotifyEnd` search using the exact same criterion** - the shared
`FindOwningAbilityData`. No handle is stored between Begin and End because there's nowhere to:
`UAnimNotifyState` is a shared const object, per-playback state can't live on it, so the criterion is recomputed from scratch. This symmetry is load-bearing, not
cosmetic: a different criterion would give a `NotifyEnd` that closes someone else's window.

One asset serves both sides. The same `UAbilityData`, granted to an enemy, gives the enemy the same window - there are no tags to duplicate on the track.

Markup for the notify lives in `Animation Setup.md`; there's one hard constraint when placing it:
**the countering skill's contact frame has to land inside this window.**

---

### A counter resolves on contact, not on activation

`TryResolveCounter` is called from `UGA_PhysicalSkill::ResolveHitOn`, at a strictly defined point:

```cpp
if (!bConfirmedHit) return;                                   // damage was already applied above

UClanhallCounterComponent::TryResolveCounter(Target, Data->CounterTag);   // ← counter

// ... then mark/synergy (once per target) and mana (once per application)
```

That is, **after** the confirmed hit and **before** the mark block. It's the same pattern
parrying uses (`UClanhallHitboxComponent::CheckAndHandleParry`): the attacker's zone touches the defender's body, the defender's state is checked at the moment of contact. The sweep runs over the same zones (`ECC_Pawn`) that resolve damage - not hitbox-against-hitbox.

Three things follow from this, worth keeping in mind:

- **There's no such thing as a free counter, by construction.** The `Charges` cost is paid
  on the button press, all the counter's effects happen on contact. A miss or insufficient
  range means no counter happened, and the charges are gone regardless.
- **A counter doesn't cancel damage.** On success the countering skill still lands its
  **full** damage on top: its cost is indistinguishable from an ordinary use, and the gain
  is the opponent's broken cast.
- **On the fallback path a counter stays testable.** With `CastMontage == nullptr` (or if
  `Montage_Play` never started), `ResolveHitOn` is called right on activation.

#### The penalty for whoever gets countered

| What's lost | Mechanism |
| --- | --- |
| the cast | `CancelAbilityHandle`, along with the dash, if there was one |
| charges | already spent on activation, not refunded |
| `+1 Stagger` | `AddStagger(1.0f)` on their own `UClanhallParryComponent` |
| tempo | hitstop `HitstopDurationOnClash` |

What's **not** lost: `State.Stunned` isn't granted from here. The only remaining source
of that tag is cashing in the `Mark.Staggered` synergy (`mark_system.md`, "Staggered - a Mark
With No Source Skill"). A stun is earned twice: first fill the bar to the cap, then cash the mark in. There's no cooldown reset either - cooldown as a mechanism doesn't exist anywhere in the project (`economy_system.md`, "Why There Are No Cooldowns").

---

### Enemy side - shared code

The interim `GA_EnemyActiveSkill` (its own `WaitDelay(CounterWindowDuration)`, `OnHitDelayExpired`, a duplicate `CounterStunDuration` field) has been removed entirely, along with `UGA_Enemy_PowerStrike`.

An enemy activates the same `UGA_PhysicalSkill` with the same `SourceObject` (`UAbilityData`) as the player:
the counter window is opened by `AnimNotifyState_CounterWindow` against the shared asset, not by ability code; the hit resolves through hitbox contact, not a timer. The former enemy Power Strike's numbers (`Slot.E`, damage) and the tag `Ability.Knight.PowerStrike` now live in the shared asset (`DataAsset and Fragments.md`, "One Class for Every Active Skill").

---
### Debugging

- The on-screen message `✓ COUNTER! Skill interrupted, charges burned, +1 Stagger` (not in shipping) is printed from `ConsumeCounter` exactly on success.
- `Clanhall.Enemy.ShowStats` - check that the victim's `Stagger` went up.
- `bDrawDebugHitboxes` on the counterer - see whether the zone actually reached anything.

| Symptom | Where to look |
| --- | --- |
| the counter doesn't trigger, damage goes through | the victim's `CounteredBy` doesn't contain the counterer's `CounterTag` (check the entry's granularity) |
| the counter doesn't trigger, no window at all | the victim's `CastMontage` isn't the one playing, or `CounteredBy` is empty - `FindOwningAbilityData` returned `nullptr` |
| the window's there, contact happens, nothing | the counterer's contact frame falls outside the victim's window |
| `Stagger` isn't growing | the `bStaggerGateOpen` gate on the victim's `UClanhallParryComponent` is closed (`Parrying.md`) |


---

## Marking-System

**Bottom line:** a mark isn't an attribute or a struct, it's a **timed tag on the ASC**, owned by a
separate component. Every fighter has their own independent track, at most one mark, five seconds, a new one overwrites the old. The component stores and hands back the mark; all the synergy logic lives not in it but in `UGA_PhysicalSkill::ResolveMarkLogic`.

Canon lives in `mark_system.md`.

### Why a tag, not a field

A mark has to be visible from outside to everyone who asks about it: abilities, the ABP, the HUD,
foreign code. A tag on the ASC is the one form GAS already knows how to match hierarchically (`Mark` matches `Mark.BrokenGuard`), replicate, and remove on its own timer. A field on the component would have needed its own timer, its own replication, and its own hierarchical-comparison mechanism - three copies of something that already exists.

This is where the file's main subtlety comes from: **the component isn't the source of truth**, it only knows
which tag to ask about.

---

### `UClanhallMarkComponent`

`AbilitySystem/ClanhallMarkComponent.h/.cpp`. Created in `AClanhallCombatantBase`'s
constructor - one instance per player and per enemy, two independent tracks (`mark_system.md`, "Receiving a Mark From an Enemy"). Ticking is disabled: the component is reactive, it has nothing to compute per-frame.

#### State

| Field                     | Type                                       |
| ------------------------ | ----------------------------------------- |
| `CachedMarkTag`          | `FGameplayTag`                            |
| `ActiveMarkEffectHandle` | `FActiveGameplayEffectHandle`             |
| `CurrentMarkSourceASC`   | `TWeakObjectPtr<UAbilitySystemComponent>` |

The duration is the constant `MarkDurationSeconds = 5.0f` in an anonymous namespace in the `.cpp`. Not a
`UPROPERTY`: the rule "a mark lives 5 seconds" belongs entirely to design, and it shouldn't be different on different fighters.

#### API

```cpp
void ApplyMark(FGameplayTag NewMark, UAbilitySystemComponent* InSourceASC = nullptr);
void ClearMark();                                    // remove with no replacement
FGameplayTag GetCurrentMark() const;                 // an invalid tag if there's no mark
bool HasMark(FGameplayTag MarkTag) const;
bool IsOwnMark(const UAbilitySystemComponent* QueryASC) const;
```

#### The cache ≠ the truth

```cpp
FGameplayTag UClanhallMarkComponent::GetCurrentMark() const
{
    const UAbilitySystemComponent* ASC = GetOwnerASC();
    if (ASC && CachedMarkTag.IsValid() && ASC->HasMatchingGameplayTag(CachedMarkTag))
    {
        return CachedMarkTag;
    }
    return FGameplayTag();
}
```

Every call re-checks the tag against the ASC rather than returning the cache as-is. The reason: once
the five seconds are up, the GE removes the tag **on its own**, the component never gets notified, and the
cache stays valid for a while longer. Returning it without checking would mean the component lying about an expired mark - and synergy would fire off a mark that's already gone.

The cache is still needed: without it, every single zone hit would require scanning the entire tag container
looking for anything under the `Mark` root.

#### At most one mark

```cpp
void UClanhallMarkComponent::ApplyMark(FGameplayTag NewMark, UAbilitySystemComponent* InSourceASC)
{
    if (!ASC || !NewMark.IsValid()) return;

    ClearMark();                     // the old one is removed BEFORE the new one, there's no stacking
    ActiveMarkEffectHandle = ClanhallGameplayEffects::ApplyTimedTag(ASC, NewMark, MarkDurationSeconds);
    if (ActiveMarkEffectHandle.IsValid())
    {
        CachedMarkTag = NewMark;
        CurrentMarkSourceASC = InSourceASC;
    }
}
```

`ClearMark()` as the first line is the entire overwrite rule (`mark_system.md`,
"Overwriting"). There's no stacking, and none is planned: two marks on one fighter would make synergy
ambiguous, and unreadable for the player.

The cache and source are only written **if the handle is valid**. If `ApplyTimedTag` fails, the
component stays clean after `ClearMark`, rather than left with a cache pointing at an effect that doesn't exist.

#### The mark's source

`CurrentMarkSourceASC` + `IsOwnMark(QueryASC)` - "who applied this mark." A weak pointer,
not a strong one: the source can die before the mark expires.

**There's no gameplay consumer right now.** Synergy only looks at the target's tag and needs
no source. The method is kept for the HUD and for future enemy synergies. Passing your own mark
along to the target was removed together with the "hot potato" concept: a mark only ever flies from the
attacker to the target, and only on a hit (`mark_system.md`, "Applying a Mark").

A consequence worth remembering for balance: **a mark applied by an enemy can't be removed by the
player at all** - only outlasted for five seconds. Attacking doesn't clear it.

---

### Synergy data

Three classes, all in `Fragments/`:

| Type | File | Role |
| --- | --- | --- |
| `UMarkApplyFragment` | `GameplayFragments.h` | one field, `MarkTag` - which mark the skill applies |
| `UMarkTriggerFragment` | `GameplayFragments.h` | the `Synergies` array - which marks the skill consumes |
| `FMarkSynergy` | `ClanhallMarkTypes.h` | one synergy entry |

```cpp
USTRUCT()
struct FMarkSynergy
{
    GENERATED_BODY()

    FGameplayTag RequiredMark;                     // the required mark on the target
    TSubclassOf<UGameplayEffect> EffectOnTarget;   // debuff on the enemy
    TSubclassOf<UGameplayEffect> EffectOnSelf;     // buff on self
    // Filled in as EITHER one, OR the other - never both
};
```

**There's no `ChargeGain` field on the struct**, and there won't be. As long as synergy paid out
charges, "mark → skill → charges" was a loop feeding itself. Charges are earned
exclusively through a hit or a parry (`combat_system.md`, "Character Resources").

Both fragments are independent: a skill can only apply a mark, only consume one, do both (the standard case - a synergy burns and a new mark lands), or neither.

---

### Resolve: `UGA_PhysicalSkill::ResolveMarkLogic`

Synergy logic lives on the ability, not on the component: the component is storage, it has
no need to know about fragments or effects.

The order inside is strict - **consume first, then apply**:

```cpp
if (const UMarkTriggerFragment* Trigger = Data->FindFragment<UMarkTriggerFragment>())
{
    const FGameplayTag CurrentMark = TargetMarkComponent->GetCurrentMark();
    if (CurrentMark.IsValid())
    {
        for (const FMarkSynergy& Synergy : Trigger->Synergies)
        {
            if (!Synergy.RequiredMark.IsValid() || !CurrentMark.MatchesTag(Synergy.RequiredMark))
                continue;

            TargetMarkComponent->ClearMark();          // the mark BURNS

            if (Synergy.EffectOnTarget)                     // debuff - on every target hit
                ApplyEffect(SourceASC, TargetASC, Synergy.EffectOnTarget);
            else if (Synergy.EffectOnSelf && !bSelfSynergySpent)   // buff - once per application
            {
                ApplyEffect(SourceASC, SourceASC, Synergy.EffectOnSelf);
                bSelfSynergySpent = true;
            }

            break;                                      // the first match wins
        }
    }
}

if (const UMarkApplyFragment* MarkApply = Data->FindFragment<UMarkApplyFragment>())
{
    TargetMarkComponent->ApplyMark(MarkApply->MarkTag, SourceASC);   // its own new mark
}
```

The reverse order would break everything: a skill that both applies and consumes would apply its own mark first, then find that same mark as the synergy's condition.

#### Entry granularity - `MatchesTag`, not `==`

`RequiredMark` matches hierarchically. A specific tag (`Mark.BrokenGuard`) matches only itself; **the
root `Mark` matches any mark at all** - this is how "triggers regardless of mark type" (Knight
Retribution) is expressed. The same trick as `CounteredBy` on the counter skill
(`Counter Ability.md`).

From this comes a fill-in rule that whoever edits the asset has to keep in mind:
**the FIRST match in the array wins** (`break` at the end of the loop iteration), so specific
entries must be placed **above** broad ones. An entry with the root `Mark` will eclipse everything placed after it.

#### Multi-target: what's shared and what isn't

The split follows the test "derived from contact, or granted for the fact of the action"
(`mark_system.md`, "Multi-Target: Resource vs. State"), not a fixed list:

| Value | Frequency | Mechanism |
| --- | --- | --- |
| damage | per target hit | `ResolveStandardDamage` on every `Event.Hitbox.Hit` |
| `EffectOnTarget` (debuff) | per target hit | by construction - a different target each time |
| applying its own mark | per target hit | `UMarkApplyFragment` |
| `EffectOnSelf` (buff) | **once per application** | the `bSelfSynergySpent` flag |
| mana | once per application | the `bManaApplied` flag |
| charges | never granted by synergy at all | sources are a WASD hit and a parry |

The self-buff is deliberately capped: a buff applied three times over is either meaningless or stacks unpredictably.

#### One mark resolve per target per application

`ResolveMarkLogic` is called from `ResolveHitOn` under a separate guard - a list of targets already
handled (`MarkResolvedTargets`):

```cpp
// damage happens on EVERY contact (two hitboxes on a montage = two damage instances),
// while mark/synergy happens once per target per application
if (!bMarkAlreadyResolved)
{
    MarkResolvedTargets.Add(Target);
    ResolveMarkLogic(...);
}
```

Without it, the second contact phase would see the mark the first phase applied, and synergy
against a root `RequiredMark` ("any mark") would fire off its own mark, running twice within a single application.

All three flags (`bSelfSynergySpent`, `bManaApplied`, `MarkResolvedTargets`) are plain fields
with no manual reset: `InstancingPolicy == InstancedPerExecution` gives the ability a fresh instance
on every application. Don't turn them into statics and don't reset them by hand - it's the instancing policy itself that makes them correct.

---

### A second mark source: the `Stagger` cap

Besides skills, the fatigue subsystem also applies a mark: at the bar's cap,
`UClanhallParryComponent::AddStagger` resets it to 0 and applies
`Mark.Staggered` to the owner (`Parrying.md`).

```cpp
MarkComp->ApplyMark(ClanhallGameplayTags::Mark_Staggered.GetTag(), nullptr);
```

The source is **`nullptr`, and that's legal**: `AddStagger` only carries `Amount` and at its
level has no idea who specifically drove the bar to the cap. `IsOwnMark` has no consumer for `Mark.Staggered` anyway.

This is the project's one mark with no source skill (`mark_system.md`, "Staggered - a Mark
With No Source Skill"), and it's also the sole entry point into loss of control: `State.Stunned`
is only ever granted by the synergy that cashes it in through `UMarkTriggerFragment`.

---

### Debugging

- `Clanhall.Player.AddMark <TagName>` / `Clanhall.Enemy.AddMark <TagName>` - applies a mark
  directly through `ApplyMark`, bypassing any skill. The main way to test synergy without
  fighting your way to the condition.
- `Clanhall.Player.ShowStats` / `Clanhall.Enemy.ShowStats` - don't show the mark; checking
  whether synergy fired has to go through whatever effect it applies.

| Symptom | Where to look |
| --- | --- |
| synergy doesn't fire | the mark expired (5 sec) - `GetCurrentMark()` returned an invalid tag |
| the wrong entry fires | a specific entry sits **below** a broad one with the root `Mark` |
| a self-buff landed once across three targets | working as intended, `bSelfSynergySpent` |
| synergy fired twice in one application | the `MarkResolvedTargets` guard got bypassed - check whether `ResolveMarkLogic` is being called somewhere outside `ResolveHitOn` |

---
## Magic-System

| ![spells_prototype](https://github.com/user-attachments/assets/dcc6b793-cff8-4e09-945a-52b17984e5e3) |
| ---------------------------------------------------------------------------------------------------- |

# License
Copyright © 2026 Lilu Dev

This source code is provided for portfolio and review purposes only.
You may view the code on GitHub.
