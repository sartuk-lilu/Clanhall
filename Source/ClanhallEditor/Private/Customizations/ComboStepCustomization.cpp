#include "Customizations/ComboStepCustomization.h"
#include "AbilitySystem/Fragments/ComboData.h"
#include "DetailWidgetRow.h"
#include "Engine/DataTable.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ComboStepCustomization"

TSharedRef<IPropertyTypeCustomization> FComboStepCustomization::MakeInstance()
{
	return MakeShared<FComboStepCustomization>();
}

void FComboStepCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle;
	MoveIdHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FComboStep, MoveId));

	RefreshOptionsSource();

	HeaderRow
	.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(250.0f)
	[
		SAssignNew(ComboBox, SSearchableComboBox)
		.OptionsSource(&Options)
		.OnGenerateWidget(this, &FComboStepCustomization::GenerateOptionWidget)
		.OnSelectionChanged(this, &FComboStepCustomization::OnSelectionChanged)
		.OnComboBoxOpening(this, &FComboStepCustomization::RefreshOptionsSource)
		[
			SNew(STextBlock)
			.Text(this, &FComboStepCustomization::GetCurrentItemLabel)
		]
	];
}

void FComboStepCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

void FComboStepCustomization::RefreshOptionsSource()
{
	Options.Reset();

	const UDataTable* MovesTable = FindMovesTable();
	TArray<FName> RowNames;
	if (MovesTable)
	{
		RowNames = MovesTable->GetRowNames();
	}

	// Пункта "None" нет намеренно: шаг без валидного MoveId всё равно ломает цепочку в резолвере
	// (FindMoveById -> nullptr -> цепочка отбрасывается) — незачем предлагать заведомо нерабочее
	// значение. Не нужен шаг — удали запись из Steps, а не выбирай None.
	for (const FName& RowName : RowNames)
	{
		Options.Add(MakeShared<FString>(RowName.ToString()));
	}

	// Текущее значение переименовали/удалили из таблицы — всё равно показать его в списке, а не
	// тихо потерять данные при следующем выборе.
	FName CurrentValue;
	if (MoveIdHandle.IsValid() && MoveIdHandle->GetValue(CurrentValue) == FPropertyAccess::Success
		&& !CurrentValue.IsNone() && !RowNames.Contains(CurrentValue))
	{
		Options.Add(MakeShared<FString>(CurrentValue.ToString()));
	}

	if (ComboBox.IsValid())
	{
		ComboBox->RefreshOptions();
	}
}

const UDataTable* FComboStepCustomization::FindMovesTable() const
{
	if (!StructPropertyHandle.IsValid())
	{
		return nullptr;
	}

	TArray<UObject*> OuterObjects;
	StructPropertyHandle->GetOuterObjects(OuterObjects);

	for (const UObject* Outer : OuterObjects)
	{
		if (const UComboData* ComboData = Cast<UComboData>(Outer))
		{
			return ComboData->MovesTable;
		}
	}

	// Несколько выделенных объектов без общего UComboData (или outer не UComboData вовсе) —
	// деградируем к пустому списку (останется только осиротевшее текущее значение, если есть), не крашим.
	return nullptr;
}

TSharedRef<SWidget> FComboStepCustomization::GenerateOptionWidget(TSharedPtr<FString> InOption) const
{
	const FString Value = InOption.IsValid() ? *InOption : FString();
	return SNew(STextBlock)
		.Text(FText::FromString(Value));
}

void FComboStepCustomization::OnSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (!NewSelection.IsValid() || NewSelection->IsEmpty() || !MoveIdHandle.IsValid())
	{
		return;
	}

	MoveIdHandle->SetValue(FName(*NewSelection));
}

FText FComboStepCustomization::GetCurrentItemLabel() const
{
	FName CurrentValue;
	const FPropertyAccess::Result Result = MoveIdHandle.IsValid() ? MoveIdHandle->GetValue(CurrentValue) : FPropertyAccess::Fail;

	if (Result == FPropertyAccess::MultipleValues)
	{
		return LOCTEXT("MultipleValues", "Multiple Values");
	}

	if (Result != FPropertyAccess::Success || CurrentValue.IsNone())
	{
		return FindMovesTable() ? LOCTEXT("Unset", "<выберите ход>") : LOCTEXT("NoMovesTable", "<назначьте MovesTable>");
	}

	return FText::FromName(CurrentValue);
}

#undef LOCTEXT_NAMESPACE
