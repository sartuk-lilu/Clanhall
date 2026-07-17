// Editor-пикер MoveId (combo_datatable_picker_task.md, Фаза 3). FComboStep.MoveId — имя строки в
// MovesTable назначенной на UComboData ассете; вместо текстового FName-поля рисуем выпадающий
// список имён строк. Кастомизация висит на самой структуре FComboStep (не на FName), чтобы не
// затронуть остальные FName-поля в редакторе.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Widgets/Input/SSearchableComboBox.h"

class UDataTable;

class FComboStepCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	/** Пересобирает Options из MovesTable текущего владельца (UComboData). Зовётся при открытии
	 *  комбобокса — таблицу/строки могут поменять между открытиями. */
	void RefreshOptionsSource();

	/** Ищет UComboData среди GetOuterObjects StructPropertyHandle и возвращает её MovesTable.
	 *  Несколько outer-объектов / outer не UComboData -> nullptr (список останется пустым, без крэша). */
	const UDataTable* FindMovesTable() const;

	TSharedRef<SWidget> GenerateOptionWidget(TSharedPtr<FString> InOption) const;
	void OnSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	FText GetCurrentItemLabel() const;

	TSharedPtr<IPropertyHandle> StructPropertyHandle;
	TSharedPtr<IPropertyHandle> MoveIdHandle;
	TSharedPtr<SSearchableComboBox> ComboBox;

	/** Источник для SSearchableComboBox — держит адрес стабильным между RefreshOptionsSource().
	 *  SSearchableComboBox типизирован на TSharedPtr<FString> (не FName) — конвертация в/из FName
	 *  происходит на границе (OnSelectionChanged / RefreshOptionsSource). Пункта "None" нет намеренно
	 *  (см. комментарий в .cpp): незаполненный MoveId (NAME_None) не выбирается из списка, а виден
	 *  на кнопке как плейсхолдер "<выберите ход>". */
	TArray<TSharedPtr<FString>> Options;
};
