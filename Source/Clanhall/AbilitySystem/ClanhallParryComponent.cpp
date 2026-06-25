#include "ClanhallParryComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

void UClanhallParryComponent::ResetParry()
{
	bParrySuccessful = false;
}

void UClanhallParryComponent::TryParry(FGameplayTag ParriableTag)
{
	UAbilitySystemComponent* ASC = GetASC();
	if (ASC && ASC->HasMatchingGameplayTag(ParriableTag))
	{
		bParrySuccessful = true;
	}
}

UAbilitySystemComponent* UClanhallParryComponent::GetASC()
{
	if (!CachedASC.IsValid())
	{
		if (IAbilitySystemInterface* Owner = Cast<IAbilitySystemInterface>(GetOwner()))
		{
			CachedASC = Owner->GetAbilitySystemComponent();
		}
	}
	return CachedASC.Get();
}
