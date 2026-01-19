//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Editor/Style/JointEditorStyle.h"
#include "SAdvancedMultiLineTextEditor.h"
#include "STextPropertyEditableTextBox.h"
#include "PropertyHandle.h"
#include "SharedType/JointSharedTypes.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"

#include "Misc/EngineVersionComparison.h"

/**
 * 
 */


class SJointOutlineButton;
class SContextTextStyler;

UENUM()
enum ERawContextEditorSwapVisibility
{
	Show_MainContextTextEditor UMETA(DisplayName="Show Main Context Text Editor")
	, Show_RawContextTextEditor UMETA(DisplayName="Show Raw Context Text Editor")
	,
};


namespace
{
	/** Allows STextPropertyEditableTextBox to edit a property handle */
	class FJointEditableTextPropertyHandle : public IEditableTextProperty
	{
	public:
		FJointEditableTextPropertyHandle(const TSharedRef<IPropertyHandle>& InPropertyHandle, const TSharedPtr<IPropertyUtilities>& InPropertyUtilities)
			: PropertyHandle(InPropertyHandle)
			  , PropertyUtilities(InPropertyUtilities)
		{
			static const FName NAME_MaxLength = "MaxLength";
			MaxLength = PropertyHandle->IsValidHandle() ? PropertyHandle->GetIntMetaData(NAME_MaxLength) : 0;
		}

		virtual bool IsMultiLineText() const override
		{
			static const FName NAME_MultiLine = "MultiLine";
			return PropertyHandle->IsValidHandle() && PropertyHandle->GetBoolMetaData(NAME_MultiLine);
		}

		virtual bool IsPassword() const override
		{
			static const FName NAME_PasswordField = "PasswordField";
			return PropertyHandle->IsValidHandle() && PropertyHandle->GetBoolMetaData(NAME_PasswordField);
		}

		virtual bool IsReadOnly() const override { return !PropertyHandle->IsValidHandle() || PropertyHandle->IsEditConst(); }

		virtual bool IsDefaultValue() const override { return PropertyHandle->IsValidHandle() && !PropertyHandle->DiffersFromDefault(); }

		virtual FText GetToolTipText() const override
		{
			return (PropertyHandle->IsValidHandle())
				       ? PropertyHandle->GetToolTipText()
				       : FText::GetEmpty();
		}

		virtual int32 GetNumTexts() const override
		{
			return (PropertyHandle->IsValidHandle())
				       ? PropertyHandle->GetNumPerObjectValues()
				       : 0;
		}

		virtual FText GetText(const int32 InIndex) const override
		{
			if(PropertyHandle->IsValidHandle())
			{
				FString ObjectValue;
				if(PropertyHandle->GetPerObjectValue(InIndex, ObjectValue) == FPropertyAccess::Success)
				{
					FText TextValue;
					if(FTextStringHelper::ReadFromBuffer(*ObjectValue, TextValue)) { return TextValue; }
				}
			}

			return FText::GetEmpty();
		}

		virtual void SetText(const int32 InIndex, const FText& InText) override
		{
			if(PropertyHandle->IsValidHandle())
			{
				FString ObjectValue;
				FTextStringHelper::WriteToBuffer(ObjectValue, InText);
				PropertyHandle->SetPerObjectValue(InIndex, ObjectValue);
			}
		}

		virtual bool IsValidText(const FText& InText, FText& OutErrorMsg) const override
		{
			if(MaxLength > 0 && InText.ToString().Len() > MaxLength)
			{
				OutErrorMsg = FText::Format(NSLOCTEXT("PropertyEditor", "PropertyTextTooLongError", "This value is too long ({0}/{1} characters)"), InText.ToString().Len(), MaxLength);
				return false;
			}

			return true;
		}


#if USE_STABLE_LOCALIZATION_KEYS
		virtual void GetStableTextId(const int32 InIndex, const ETextPropertyEditAction InEditAction, const FString& InTextSource, const FString& InProposedNamespace
		                             , const FString& InProposedKey, FString& OutStableNamespace, FString& OutStableKey) const override
		{
			if(PropertyHandle->IsValidHandle())
			{
				TArray<UPackage*> PropertyPackages;
				PropertyHandle->GetOuterPackages(PropertyPackages);

				check(PropertyPackages.IsValidIndex(InIndex));

				StaticStableTextId(PropertyPackages[InIndex], InEditAction, InTextSource, InProposedNamespace, InProposedKey, OutStableNamespace, OutStableKey);
			}
		}
#endif // USE_STABLE_LOCALIZATION_KEYS
		
#if UE_VERSION_OLDER_THAN(5,0,0)
		
		virtual void RequestRefresh() override
		{
			
		}
#endif
		

	private:
		TSharedRef<IPropertyHandle> PropertyHandle;
		TSharedPtr<IPropertyUtilities> PropertyUtilities;

		/** The maximum length of the value that can be edited, or <=0 for unlimited */
		int32 MaxLength = 0;
	};
}


class JOINTEDITOR_API SContextTextEditor : public SAdvancedMultiLineTextEditor
{
public:
	SLATE_BEGIN_ARGS(SContextTextEditor)
			:
			_Text(),
			_HintText(),
			_bUseStyling(false),
			_TableToEdit(nullptr),
			_TextBoxStyle(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox")),
			_BorderMargin(FMargin(2.f)),
			_InnerBorderMargin(FMargin(2.f)),
			_TextblockMargin(FJointEditorStyle::Margin_Normal),
			_TextblockPadding(FMargin(2.f)),
			_bUseCustomBorderColor(false),
			_CustomBorderColor(FLinearColor::Transparent),
			_OnTextChanged(),
			_OnTextCommitted()
		{
		}

		/** Sets the text content for this editable text box widget */
		SLATE_ATTRIBUTE(FText, Text)
		/** Hint text that appears when there is no text in the text box */
		SLATE_ATTRIBUTE(FText, HintText)
		
		/** Whether to use the styling feature.*/
		SLATE_ATTRIBUTE(bool, bUseStyling)
		
		//Override target table assets that will be applied to the text when the bShouldOverrideTable is true 
		SLATE_ATTRIBUTE(class UDataTable*, TableToEdit)


		SLATE_STYLE_ARGUMENT(FEditableTextBoxStyle, TextBoxStyle)
		/** The margin around the border of the text box */
		SLATE_ARGUMENT(FMargin, BorderMargin)
		SLATE_ARGUMENT(FMargin, InnerBorderMargin)
		/** The margin around the text block inside the text box */
		SLATE_ARGUMENT(FMargin, TextblockMargin)
		SLATE_ARGUMENT(FMargin, TextblockPadding)

		SLATE_ARGUMENT(bool, bUseCustomBorderColor)
		SLATE_ARGUMENT(FLinearColor, CustomBorderColor)

		/** Called whenever the text is changed programmatically or interactively by the user */
		SLATE_EVENT(FOnTextChanged, OnTextChanged)

		SLATE_EVENT(FOnTextCommitted, OnTextCommitted)
		
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

public:
	TAttribute<FText> TextAttr;

	TAttribute<FText> HintTextAttr;
	
	TAttribute<bool> bUseStylingAttr;
	
	TAttribute<UDataTable*> TextDataTableAttr;

public:
	
	const FEditableTextBoxStyle* TextBoxStyle;
	
	FMargin BorderMargin;
	FMargin InnerBorderMargin;
	FMargin TextblockMargin;
	FMargin TextblockPadding;
	
	bool bUseCustomBorderColor = false;
	
	FLinearColor CustomBorderColor = FLinearColor::Transparent;

public:

	virtual void RebuildMarshaller() override;

	virtual void RebuildWidget() override;

public:
	
	void AssignContextTextStyler();
	void AssignContextTextBox();
	void AssignRawContextTextBox();

public:
	
	TSharedPtr<SComboBox<TSharedPtr<FName>>> StyleComboBox;
	
	TSharedPtr<SContextTextStyler> ContextTextStyler;
	
	TSharedPtr<SBox> ContextTextStylerBox;

public:
	
	TEnumAsByte<ERawContextEditorSwapVisibility> RawContextEditorSwapVisibility = ERawContextEditorSwapVisibility::Show_MainContextTextEditor;

	TSharedPtr<SJointOutlineButton> SwapButton;

	FReply OnSwapButtonDown();

public:
	
	void RefreshEditorDetailWidget();

public:
	
	virtual void HandleRichEditableTextChanged(const FText& Text) override;

	virtual void HandleRichEditableTextCommitted(const FText& Text, ETextCommit::Type Type) override;

	virtual void HandleRichEditableTextCursorMoved(const FTextLocation& NewCursorPosition) override;
	
	FReply HandleContextTextBoxKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent);

	virtual void OnFocusLost(const FFocusEvent& InFocusEvent) override;

	virtual bool IsReadOnly() const override;

	
};
