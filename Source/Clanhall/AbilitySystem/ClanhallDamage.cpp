#include "AbilitySystem/ClanhallDamage.h"
#include "AbilitySystem/ClanhallAttributeSet.h"
#include "AbilitySystem/ClanhallGameplayTags.h"
#include "AbilitySystem/ClanhallHitboxComponent.h"
#include "AbilitySystem/Effects/ClanhallGameplayEffects.h"
#include "ClanhallCharacterBase.h"
#include "ClanhallCombatSettings.h"
#include "AbilitySystemComponent.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING
// Принудительный бросок для проверки в PIE: смертельный и крит выпадают раз в сто и раз в пять.
static TAutoConsoleVariable<int32> CVarForceCrit(TEXT("Clanhall.Debug.ForceCrit"), 0, TEXT("1 - every hit is a crit."));
static TAutoConsoleVariable<int32> CVarForceLethal(TEXT("Clanhall.Debug.ForceLethal"), 0, TEXT("1 - every lethal-capable hit is lethal."));
#endif

namespace
{
	bool ShouldForceCrit()
	{
#if !UE_BUILD_SHIPPING
		return CVarForceCrit.GetValueOnGameThread() != 0;
#else
		return false;
#endif
	}

	bool ShouldForceLethal()
	{
#if !UE_BUILD_SHIPPING
		return CVarForceLethal.GetValueOnGameThread() != 0;
#else
		return false;
#endif
	}

	float GetAttr(const UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, float Fallback)
	{
		bool bFound = false;
		const float Value = ASC ? ASC->GetGameplayAttributeValue(Attribute, bFound) : Fallback;
		return bFound ? Value : Fallback;
	}

	void ApplyDelta(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> Effect, float Delta)
	{
		// Источник нужен контексту эффекта (убийца для кристалла); нет источника (отладка) - цель сама.
		ClanhallGameplayEffects::ApplyModifyEffect(SourceASC ? SourceASC : TargetASC, TargetASC, Effect, Delta);
	}
}

bool ClanhallDamage::IsBehind(const AActor* Target, const AActor* Source)
{
	if (!Target || !Source)
	{
		return false;
	}
	FVector ToSource = Source->GetActorLocation() - Target->GetActorLocation();
	ToSource.Z = 0.0f;
	if (!ToSource.Normalize())
	{
		return false;
	}
	FVector Forward = Target->GetActorForwardVector();
	Forward.Z = 0.0f;
	Forward.Normalize();

	// Сектор спины - симметричный конус вокруг «назад»: угол между «назад» и направлением на
	// источник не больше половины сектора.
	const float HalfSectorRad = FMath::DegreesToRadians(UClanhallCombatSettings::Get().BackSectorDegrees * 0.5f);
	return FVector::DotProduct(-Forward, ToSource) >= FMath::Cos(HalfSectorRad);
}

FClanhallHitResult ClanhallDamage::ResolveHit(const FClanhallHitContext& Context)
{
	FClanhallHitResult Result;
	Result.Source = Context.SourceActor;

	UAbilitySystemComponent* TargetASC = Context.TargetASC;
	if (!TargetASC)
	{
		return Result;
	}

	const UClanhallCombatSettings& Settings = UClanhallCombatSettings::Get();
	Result.bFromBack = IsBehind(Context.TargetActor, Context.SourceActor);

	AClanhallCharacterBase* TargetCharacter = Cast<AClanhallCharacterBase>(Context.TargetActor);

	if (Context.bUtility || Context.RawDamage <= 0.0f)
	{
		// Утилита без урона подтверждает попадание фактом цели; порог её не касается.
		Result.bConfirmed = Context.bUtility;
	}
	else
	{
		const float TargetAP = GetAttr(TargetASC, UClanhallAttributeSet::GetAPAttribute(), 0.0f);
		const float TargetHP = GetAttr(TargetASC, UClanhallAttributeSet::GetHPAttribute(), 0.0f);
		const float TargetMaxHP = GetAttr(TargetASC, UClanhallAttributeSet::GetMaxHPAttribute(), 0.0f);

		// 1. Смертельный удар - свойство приёма (`CombatSystem.md`, «Случайность»). Сработал - по HP
		//    мимо AP и порога: рядовой погибает, босс и игрок теряют большую долю HP. Крит не бросается.
		const bool bLethalEligible = Context.LethalMode == EClanhallLethalMode::Always
			|| (Context.LethalMode == EClanhallLethalMode::FromBackOnly && Result.bFromBack);
		if (bLethalEligible && (ShouldForceLethal() || FMath::FRand() < Settings.LethalChance))
		{
			const bool bMob = TargetASC->HasMatchingGameplayTag(ClanhallGameplayTags::Unit_Role_Mob.GetTag());
			const float Loss = bMob ? TargetHP : FMath::Min(TargetHP, TargetMaxHP * Settings.LethalHPFraction);
			if (Loss > 0.0f)
			{
				ApplyDelta(Context.SourceASC, TargetASC, UGE_ModifyHP::StaticClass(), -Loss);
			}
			Result.bLethal = true;
			Result.DamageToHP = Loss;
			Result.bConfirmed = true;
		}
		else
		{
			// 2. Множители: версия приёма, метки атакующего и цели (складываются через
			//    MultiplyAdditive, `MarksSystem.md`, «Слоты меток»).
			float Damage = Context.RawDamage * Context.DamageMultiplier
				* GetAttr(Context.SourceASC, UClanhallAttributeSet::GetDamageDealtMultiplierAttribute(), 1.0f)
				* GetAttr(TargetASC, UClanhallAttributeSet::GetDamageTakenMultiplierAttribute(), 1.0f);

			// 3. Крит - шанс обрезан потолком, одинаково у игрока и противника.
			const float CritChance = FMath::Min(GetAttr(Context.SourceASC, UClanhallAttributeSet::GetCritChanceAttribute(), 0.0f), Settings.CritChanceCap);
			if (ShouldForceCrit() || FMath::FRand() < CritChance)
			{
				Damage *= Settings.CritMultiplier;
				Result.bCrit = true;
			}

			if (Context.bBackstab && Result.bFromBack)
			{
				// 4. Удар в спину - мимо AP и порога, прямо в HP (`CombatSystem.md`, «Удар в спину»).
				Result.bBackstab = true;
				Result.DamageToHP = Damage;
			}
			else if (TargetAP > 0.0f)
			{
				// 5. Порог - свойство брони, действует только пока у цели есть AP. Урон меньше
				//    порога - ноль, а не ослабленный удар (`CombatSystem.md`, «Damage Threshold (DT)»).
				float Threshold = 0.0f;
				if (!Context.bIgnoreThreshold && TargetCharacter)
				{
					Threshold = FMath::Max(0.0f, TargetCharacter->GetDamageThreshold(Context.DamageType)
						- Context.ArmorPenetration
						- GetAttr(TargetASC, UClanhallAttributeSet::GetThresholdReductionAttribute(), 0.0f));
				}

				if (Damage < Threshold)
				{
					Result.bAbsorbed = true;
					Damage = 0.0f;
				}
				else
				{
					Result.DamageToAP = FMath::Min(Damage, TargetAP);
					Result.DamageToHP = Damage - Result.DamageToAP;
					Result.bArmorBroken = Result.DamageToAP >= TargetAP;
				}
			}
			else
			{
				// AP сбит - порога нет, по HP бьёт любое оружие.
				Result.DamageToHP = Damage;
			}

			if (Result.DamageToAP > 0.0f)
			{
				ApplyDelta(Context.SourceASC, TargetASC, UGE_ModifyAP::StaticClass(), -Result.DamageToAP);
			}
			if (Result.DamageToHP > 0.0f)
			{
				ApplyDelta(Context.SourceASC, TargetASC, UGE_ModifyHP::StaticClass(), -Result.DamageToHP);
			}
			Result.bConfirmed = (Result.DamageToAP + Result.DamageToHP) > 0.0f;
		}
	}

	// Подтверждённое попадание срывает насмешку без награды (`CombatSystem.md`, «Насмешка»); удар,
	// целиком поглощённый порогом, - нет.
	if (Result.bConfirmed && TargetASC->HasMatchingGameplayTag(ClanhallGameplayTags::State_Taunting.GetTag()))
	{
		FGameplayTagContainer TauntTags;
		TauntTags.AddTag(ClanhallGameplayTags::Ability_Common_Taunt.GetTag());
		TargetASC->CancelAbilities(&TauntTags);
	}

	if (Result.bConfirmed && Context.TargetActor)
	{
		// Пробивший контакт сбивает зону получателя до конца его текущего действия - только
		// подтверждённое попадание (`CombatSystem.md`, «Пробивший контакт сбивает зону получателя»).
		// Удар, целиком поглощённый порогом, зону не глушит.
		if (UClanhallHitboxComponent* TargetHitbox = Context.TargetActor->FindComponentByClass<UClanhallHitboxComponent>())
		{
			TargetHitbox->SuppressHitboxes();
		}
	}

	if (TargetCharacter)
	{
		TargetCharacter->NotifyHitReceived(Result);
	}

	return Result;
}
