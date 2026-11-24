// Отладочное сообщение боя: на экран и в лог одновременно. Экранное сообщение в лог не попадает,
// а проверка из редактора (скриптом, по логу) должна видеть то же, что видит человек: версию
// приёма, удар из ножен, разбор попадания, контр. Категория LogClanhallCombat.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"

CLANHALL_API DECLARE_LOG_CATEGORY_EXTERN(LogClanhallCombat, Log, All);

namespace ClanhallDebug
{
	inline void Screen(const FColor& Color, const FString& Message)
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogClanhallCombat, Log, TEXT("%s"), *Message);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, Color, Message);
		}
#endif
	}
}
