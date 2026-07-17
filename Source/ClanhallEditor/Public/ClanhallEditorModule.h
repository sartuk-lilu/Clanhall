// Editor-модуль проекта (combo_datatable_picker_task.md). StartupModule регистрирует editor-пикер
// FComboStep (выпадающий список MoveId из назначенной MovesTable — см. ComboStepCustomization).

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FClanhallEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
