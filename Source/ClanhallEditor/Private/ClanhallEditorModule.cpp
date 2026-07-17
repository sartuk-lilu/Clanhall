#include "ClanhallEditorModule.h"
#include "Customizations/ComboStepCustomization.h"
#include "PropertyEditorModule.h"

namespace
{
	const FName ComboStepPropertyName("ComboStep");
}

void FClanhallEditorModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("ClanhallEditor module loaded"));

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(ComboStepPropertyName,
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FComboStepCustomization::MakeInstance));
	PropertyModule.NotifyCustomizationModuleChanged();
}

void FClanhallEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout(ComboStepPropertyName);
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

IMPLEMENT_MODULE(FClanhallEditorModule, ClanhallEditor)
