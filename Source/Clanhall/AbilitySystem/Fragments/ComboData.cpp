#include "ComboData.h"

const FDirectionalDamage& UComboData::FindDamageByDirection(EClanhallAttackDirection Direction) const
{
	switch (Direction)
	{
	case EClanhallAttackDirection::Overhead:
		return Overhead;
	case EClanhallAttackDirection::RightSlash:
		return RightSlash;
	case EClanhallAttackDirection::LeftSlash:
		return LeftSlash;
	case EClanhallAttackDirection::LowSweep:
		return LowSweep;
	default:
		checkNoEntry();
		return Overhead;
	}
}

UAnimMontage* UComboData::FindOpenerMontage(EClanhallAttackDirection To) const
{
	switch (To)
	{
	case EClanhallAttackDirection::Overhead:
		return FromStance.ToOverhead;
	case EClanhallAttackDirection::RightSlash:
		return FromStance.ToRightSlash;
	case EClanhallAttackDirection::LeftSlash:
		return FromStance.ToLeftSlash;
	case EClanhallAttackDirection::LowSweep:
		return FromStance.ToLowSweep;
	default:
		checkNoEntry();
		return nullptr;
	}
}

UAnimMontage* UComboData::FindTransitionMontage(EClanhallAttackDirection From, EClanhallAttackDirection To) const
{
	switch (From)
	{
	case EClanhallAttackDirection::Overhead:
		switch (To)
		{
		case EClanhallAttackDirection::LeftSlash:
			return FromOverhead.ToLeftSlash;
		case EClanhallAttackDirection::RightSlash:
			return FromOverhead.ToRightSlash;
		case EClanhallAttackDirection::LowSweep:
			return FromOverhead.ToLowSweep;
		default:
			// Overhead -> Overhead: повтор направления запрещён по дизайну, слота нет.
			return nullptr;
		}
	case EClanhallAttackDirection::LeftSlash:
		switch (To)
		{
		case EClanhallAttackDirection::Overhead:
			return FromLeftSlash.ToOverhead;
		case EClanhallAttackDirection::RightSlash:
			return FromLeftSlash.ToRightSlash;
		case EClanhallAttackDirection::LowSweep:
			return FromLeftSlash.ToLowSweep;
		default:
			// LeftSlash -> LeftSlash: повтор направления запрещён по дизайну, слота нет.
			return nullptr;
		}
	case EClanhallAttackDirection::RightSlash:
		switch (To)
		{
		case EClanhallAttackDirection::Overhead:
			return FromRightSlash.ToOverhead;
		case EClanhallAttackDirection::LeftSlash:
			return FromRightSlash.ToLeftSlash;
		case EClanhallAttackDirection::LowSweep:
			return FromRightSlash.ToLowSweep;
		default:
			// RightSlash -> RightSlash: повтор направления запрещён по дизайну, слота нет.
			return nullptr;
		}
	case EClanhallAttackDirection::LowSweep:
		switch (To)
		{
		case EClanhallAttackDirection::Overhead:
			return FromLowSweep.ToOverhead;
		case EClanhallAttackDirection::LeftSlash:
			return FromLowSweep.ToLeftSlash;
		case EClanhallAttackDirection::RightSlash:
			return FromLowSweep.ToRightSlash;
		default:
			// LowSweep -> LowSweep: повтор направления запрещён по дизайну, слота нет.
			return nullptr;
		}
	default:
		checkNoEntry();
		return nullptr;
	}
}

UAnimSequence* UComboData::FindRecoveryAnimation(EClanhallAttackDirection LastDirection) const
{
	switch (LastDirection)
	{
	case EClanhallAttackDirection::Overhead:
		return Recovery.AfterOverhead;
	case EClanhallAttackDirection::RightSlash:
		return Recovery.AfterRightSlash;
	case EClanhallAttackDirection::LeftSlash:
		return Recovery.AfterLeftSlash;
	case EClanhallAttackDirection::LowSweep:
		return Recovery.AfterLowSweep;
	default:
		checkNoEntry();
		return nullptr;
	}
}
