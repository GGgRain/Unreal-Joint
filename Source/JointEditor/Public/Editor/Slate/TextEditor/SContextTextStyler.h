//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SAdvancedMultiLineTextEditor.h"
#include "Widgets/Input/SComboBox.h"

/**
 * Styler widget for the context text editor.
 * It notifies the text style update request holds all the references related to the style instance and text style. 
 */

class STextBlock;
class SJointOutlineButton;
class SContextTextEditor;
class SDataTablePicker;
class UVoltAnimationManager;

class JOINTEDITOR_API SContextTextStyler : public SCompoundWidget
{
	
public:
	
	SLATE_BEGIN_ARGS( SContextTextStyler ) {}
		//Target RichEditableTextBox to apply styling with the styler.
		SLATE_ARGUMENT( TWeakPtr<SMultiLineEditableTextBox>, TargetRichEditableTextBox)
		//Default text style table to use on styling when text style table is not specified.
		SLATE_ATTRIBUTE( class UDataTable*, TextStyleTable)
		
	SLATE_END_ARGS()
	
public:
	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

public:
	
	void PopulateWidget();

public:
	
	//Callbacks
	void OnActiveTextStyleRowNameChanged(TSharedPtr<FName> Name, ESelectInfo::Type Arg);
	
	TSharedRef<SWidget> OnGenerateTextStyleRowComboEntry(TSharedPtr<FName> SourceEntry);

public:
	
	FText GetActiveTextStyleRowName() const;

	TSharedRef<STextBlock> GetActiveTextStyleRowTextBlock();

public:

	void SetTargetRichEditableTextBox(const TWeakPtr<SMultiLineEditableTextBox>& NewTargetRichEditableTextBox);

	TWeakPtr<SMultiLineEditableTextBox> GetTargetRichEditableTextBox();

public:

	
	/**
	 * Apply the current style to the selected text.
	 * If no text is selected, then a new (empty) run will be inserted with the appropriate style.
	 */
	void ApplyCurrentStyleToSelectedText();


	/**
	 * Get the text style for the provided row name from the saved TextStyleSetInstance.
	 * if there is no style for the provided row name, return GetSystemDefaultTextStyle().
	 */
	const FTextBlockStyle* GetTextStyleFor(const FName& RowName) const;

	/**
	 * Get the default text style.
	 */
	const FTextBlockStyle* GetDefaultTextStyle() const;
	
	/**
	 * Reallocate text style.
	 * Rebuild the instance and store text style elements on it and TextStyleRows.
	 */
	void RebuildTextStyle();
	
	void ReallocateTextStyleSetInstance();
	
	void StoreTextStyleData();
	void FeedDefaultStyleToTargetRichEditableTextBox();

	/**
	 * Request refresh action to the FPropertyEditorModule.
	 * This will force refresh action of the detail tab of the editor.
	 */
	void RefreshEditorDetailWidget();

	/**
	 * Refresh the text block on the styler combobox.
	 */
	void RefreshActiveTextStyleRowTextBlock();

	
	/**
	 * Set active row name from the provided run instance.
	 * if there is no row available for the provided run, It will select the default row.
	 */
	void SetActiveRowForRun(TSharedPtr<const IRun> Run) const;


public:
	
	/**
	 * Target RichEditableTextBox that we will apply the changes and styling.
	 */
	TWeakPtr<SMultiLineEditableTextBox> TargetRichEditableTextBox;

public:

	/**
	 * ComboBox that pops up the rows that refers to the style element that are available. 
	 * Selecting an option of this combobox will apply following text style to the provided styler.
	 */
	TSharedPtr<SComboBox<TSharedPtr<FName>>> TextStyleComboBox;
	
	/**
	 * DataTablePicker that is used to pick up the data table to use on the text styling.
	 */
	TSharedPtr<SBorder> ActiveTextStyleRowBorder;

	TSharedPtr<SJointOutlineButton> PipetteButton;

public:

	TAttribute<UDataTable*> TextDataTableAttr;

public:
	
	

	/**
	 * Array of the rows that refer to the saved style element in the TextStyleSetInstance.
	 * Used to provide the element list for the TextStyleComboBox.
	 */
	TArray<TSharedPtr<FName>> TextStyleRows;

	/**
	 * Name of the active row from TextStyleRows.
	 * It refer to the active text style.
	 */
	TSharedPtr<FName> ActiveTextStyleRowName;

public:

	/**
	 * The set instance of saved styles for the text.
	 */
	TSharedPtr<class FSlateStyleSet> TextStyleSetInstance;

private:

	/**
	 * Whether the pipette is activating or not.
	 */
	bool bIsStylePipetteActivating = false;

public:
	
	const bool GetIsStylePipetteActivating();

	void SetIsStylePipetteActivating(const bool NewValue);

public:

	//Animation Related
	
	//Wrapper slate for the hover & unhover animation event.
	TSharedPtr<SButton> WrappingButton;
	void OnStylePipettePressed();
	void OnTextBoxCursorMoved(const FTextLocation& NewCursorPosition);

public:

	/**
	 * Get the default text style for the Joint editor system.
	 */
	static const FTextBlockStyle* GetSystemDefaultTextStyle();
	
};
