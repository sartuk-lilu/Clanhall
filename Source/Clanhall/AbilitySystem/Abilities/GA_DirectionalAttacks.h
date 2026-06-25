// Четыре направления WASD-удара (combat_system.md §4). Вся логика — в GA_DirectionalAttackBase,
// эти классы существуют только чтобы GetDirection() возвращал свою сторону (нужно для
// будущей анимации/парирования в Разделе 5/6.5).

#pragma once

#include "GA_DirectionalAttackBase.h"
#include "GA_DirectionalAttacks.generated.h"

UCLASS()
class CLANHALL_API UGA_DirectionalAttack_Overhead : public UGA_DirectionalAttackBase
{
	GENERATED_BODY()
public:
	virtual EClanhallAttackDirection GetDirection() const override { return EClanhallAttackDirection::Overhead; }
};

UCLASS()
class CLANHALL_API UGA_DirectionalAttack_RightSlash : public UGA_DirectionalAttackBase
{
	GENERATED_BODY()
public:
	virtual EClanhallAttackDirection GetDirection() const override { return EClanhallAttackDirection::RightSlash; }
};

UCLASS()
class CLANHALL_API UGA_DirectionalAttack_LeftSlash : public UGA_DirectionalAttackBase
{
	GENERATED_BODY()
public:
	virtual EClanhallAttackDirection GetDirection() const override { return EClanhallAttackDirection::LeftSlash; }
};

UCLASS()
class CLANHALL_API UGA_DirectionalAttack_LowSweep : public UGA_DirectionalAttackBase
{
	GENERATED_BODY()
public:
	virtual EClanhallAttackDirection GetDirection() const override { return EClanhallAttackDirection::LowSweep; }
};
