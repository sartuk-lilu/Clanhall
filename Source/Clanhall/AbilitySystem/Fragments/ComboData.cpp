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

const FComboMove* UComboData::FindMoveById(FName MoveId) const
{
	return MovesTable ? MovesTable->FindRow<FComboMove>(MoveId, TEXT("ComboResolve"), false) : nullptr;
}
