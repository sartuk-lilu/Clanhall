// Чем этим оружием можно драться (`WeaponSystem.md`, «Тип оружия ↔ экземпляр»): его приёмы,
// скорость и пробитие. Дефолты полей совпадают с фолбэком на отсутствие данных
// (ClanhallWeaponDefaults) — так новый ассет сразу играбелен, а не мёртв.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Fragments/WeaponFragment.h"
#include "WeaponTypeData.generated.h"

class UAbilityData;
class UAnimMontage;

/** Именованные фолбэки на отсутствие данных в цепочке CharacterSheet -> Weapon -> Type — не
 *  баланс, страховка от нуля. Живут здесь, а не разъезжаются по файлам. */
namespace ClanhallWeaponDefaults
{
	constexpr float ArmorPenetration = 0.0f;
	constexpr float WeaponSpeedMultiplier = 1.0f;
	constexpr float DodgeDistance = 250.0f;
	constexpr float DodgeBeats = 1.0f;
	constexpr float SkillTurnRate = 180.0f;
}

UCLASS()
class CLANHALL_API UWeaponTypeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Школа класса оружия (`School.*`, `AbilitySystem.md`, «Приёмы оружия»): метка помнит школу,
	 *  которая её положила, третий уровень навыка сравнивает школы (`MarksSystem.md`). */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (Categories = "School"))
	FGameplayTag School;

	/** Множитель к базовым скоростям бойца (JogSpeed/WalkSpeed/SprintSpeed), не см/с -
	 *  абсолютное число непереносимо между бойцами с разной базой (`WeaponSystem.md`,
	 *  «Оружие как актор»). Применяется при экипировке ко всем трём разом
	 *  (`AClanhallHumanoidBase::PostInitializeComponents`) - тяжёлое оружие медленное
	 *  всегда, а не в момент боевого контакта. 1.0 - как обычно, тяжёлое -
	 *  0.6-0.7. ClampMin 0.1, не 0: ноль означал бы «этим оружием не двигаться вовсе», такого
	 *  решения нет. */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (ClampMin = "0.1"))
	float WeaponSpeedMultiplier = 1.0f;

	/** База плоского пробития DT. Потребитель — этап 6. */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (ClampMin = "0.0"))
	float ArmorPenetration = ClanhallWeaponDefaults::ArmorPenetration;

	UPROPERTY(EditAnywhere, Instanced, Category = "WeaponType")
	TArray<TObjectPtr<UWeaponFragment>> Fragments;

	// --- Уклонение (`CombatSystem.md`, «Уклонение»): общее действие, но анимация, дальность и
	// длина в долях - свои у оружия и зависят от его веса. ---

	/** Дальность отскока, см. Плейсхолдер. */
	UPROPERTY(EditAnywhere, Category = "WeaponType|Dodge", meta = (ClampMin = "0.0"))
	float DodgeDistance = ClanhallWeaponDefaults::DodgeDistance;

	/** Длина отскока в долях - его цена: пока идёт, новые действия закрыты. Плейсхолдер. */
	UPROPERTY(EditAnywhere, Category = "WeaponType|Dodge", meta = (ClampMin = "0.25"))
	float DodgeBeats = ClanhallWeaponDefaults::DodgeBeats;

	/** Урон отклика по касанию (Hitbox-нотифай на монтаже отскока) - просто удар, без меток.
	 *  0 - отклика нет. */
	UPROPERTY(EditAnywhere, Category = "WeaponType|Dodge", meta = (ClampMin = "0.0"))
	float DodgeTouchDamage = 0.0f;

	/** Монтажи отскока по направлениям - косметика, nullptr законен. Слот fullbody, root motion
	 *  выключен: перемещение даёт Root Motion Source способности. */
	UPROPERTY(EditAnywhere, Category = "WeaponType|Dodge")
	TObjectPtr<UAnimMontage> DodgeMontageForward;

	UPROPERTY(EditAnywhere, Category = "WeaponType|Dodge")
	TObjectPtr<UAnimMontage> DodgeMontageBack;

	UPROPERTY(EditAnywhere, Category = "WeaponType|Dodge")
	TObjectPtr<UAnimMontage> DodgeMontageLeft;

	UPROPERTY(EditAnywhere, Category = "WeaponType|Dodge")
	TObjectPtr<UAnimMontage> DodgeMontageRight;

	/** Разворот в приёме (`CombatSystem.md`, «Контроль»): пока идёт свой приём, боец доворачивается
	 *  к цели медленно, град/с; у тяжёлого ниже. Замах тяжёлого можно обежать. */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (ClampMin = "0.0"))
	float SkillTurnRate = ClanhallWeaponDefaults::SkillTurnRate;

	/** Активные навыки. Ключ — Slot.* (Q/E/R/F и далее по канону восьми
	 *  слотов, `AbilitySystem.md`, «Слоты активных навыков»; `CharacterHierarchy.md`,
	 *  «Ключ по слоту, а не по имени навыка»), а не имя скилла: слот один и тот же для всех оружий,
	 *  а именованные поля-на-скилл зашивали бы имя класса в поле, которое обязано
	 *  обслужить все классы, и их пришлось бы переписывать на восемь.
	 *  Переехало с UCharacterSheetData: набор активок принадлежит оружию, а не персонажу
	 *  (`AbilitySystem.md`, «Физические активные навыки»). */
	UPROPERTY(EditAnywhere, Category = "WeaponType", meta = (Categories = "Slot"))
	TMap<FGameplayTag, TObjectPtr<UAbilityData>> Skills;

	/** Возвращает первый фрагмент типа T, или nullptr если у этого типа оружия такого нет. */
	template <typename T>
	T* FindFragment() const
	{
		for (const TObjectPtr<UWeaponFragment>& Fragment : Fragments)
		{
			if (T* Match = Cast<T>(Fragment.Get()))
			{
				return Match;
			}
		}
		return nullptr;
	}
};
