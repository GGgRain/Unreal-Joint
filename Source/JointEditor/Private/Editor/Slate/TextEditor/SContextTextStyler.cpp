//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include <TextEditor/SContextTextStyler.h>

#include "JointEditorStyle.h"
#include "SlateOptMacros.h"
#include "Styling/SlateStyle.h"

//VOLT related
#include "JointAdvancedWidgets.h"
#include "VoltAnimationManager.h"
#include "JointEditorSettings.h"
#include "PropertyEditorModule.h"
#include "VoltDecl.h"
#include "Components/RichTextBlock.h"
#include "Module/Volt_ASM_InterpBackgroundColor.h"
#include "Modules/ModuleManager.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

#define LOCTEXT_NAMESPACE "SContextTextStyler"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

/** Constructs this widget with InArgs */
void SContextTextStyler::Construct(const FArguments& InArgs)
{
	TargetRichEditableTextBox = InArgs._TargetRichEditableTextBox;

	TextDataTableAttr = InArgs._TextStyleTable;

	ActiveTextStyleRowName = MakeShareable(new FName(TEXT("Default")));

	SetCanTick(false);

	PopulateWidget();
}

void SContextTextStyler::PopulateWidget()
{
	
	this->ChildSlot.DetachWidget();

	RebuildTextStyle();

	this->ChildSlot
	[
		SNew(SHorizontalBox)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.AutoWidth()
		[
			SAssignNew(WrappingButton, SButton)
			.ButtonStyle(FJointEditorStyle::Get(), TEXT("JointUI.Button.Empty"))
			[
				SAssignNew(TextStyleComboBox, SComboBox<TSharedPtr<FName>>)
				.ComboBoxStyle(FJointEditorStyle::Get(), "JointUI.ComboBox.Round")
				.OptionsSource(&TextStyleRows)
				.ContentPadding(FJointEditorStyle::Margin_Small)
				.OnSelectionChanged(this, &SContextTextStyler::OnActiveTextStyleRowNameChanged)
				.OnGenerateWidget(this, &SContextTextStyler::OnGenerateTextStyleRowComboEntry)
				.InitiallySelectedItem(ActiveTextStyleRowName)
				.ToolTipText(LOCTEXT("ActiveTextStyleToolTip"
							 , "It displays the row, which is referenced from the text style table, that is currently using on text editing."
							 "\nYou can style any section of the text by selecting a part of the text and pressing SHIFT + CONTROL + S(Style)."))
				[
					SAssignNew(ActiveTextStyleRowBorder, SBorder)
					.Visibility(EVisibility::HitTestInvisible)
					.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Empty"))
					[
						GetActiveTextStyleRowTextBlock()
					]
				]
			]

		]
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SAssignNew(PipetteButton, SJointOutlineButton)
			.ContentPadding(FJointEditorStyle::Margin_Small)
			.ToolTipText(LOCTEXT("StylePipetteToolTip","click the text to grab the style of the text after activating it."))
			.OnPressed(this, &SContextTextStyler::OnStylePipettePressed)
			.NormalColor(FLinearColor::Transparent)
			.OutlineNormalColor(FLinearColor::Transparent)
			[
				SNew(SBox)
				.WidthOverride(12)
				.HeightOverride(12)
				.Visibility(EVisibility::HitTestInvisible)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.EyeDropper"))
				]
			]
		]
	];
}

TSharedRef<STextBlock> SContextTextStyler::GetActiveTextStyleRowTextBlock()
{
	return SNew(STextBlock)
		.Text(this, &SContextTextStyler::GetActiveTextStyleRowName)
		.TextStyle(GetTextStyleFor((ActiveTextStyleRowName.IsValid()) ? *ActiveTextStyleRowName : NAME_None))
		.ToolTipText(LOCTEXT("ActiveTextStyleToolTip"
		                     , "It displays the row, which is referenced from the text style table, that is currently using on text editing."
		                     "\nYou can style any section of the text by selecting a part of the text and pressing SHIFT + CONTROL + S(Style)."));
}


void SContextTextStyler::ApplyCurrentStyleToSelectedText()
{
	if (!ActiveTextStyleRowName.IsValid() || !GetTargetRichEditableTextBox().IsValid()) return;

	const FRunInfo RunInfo = (ActiveTextStyleRowName->ToString().ToLower() == "default")
		                         ? FRunInfo()
		                         : FTextStyles::CreateRunInfo(*ActiveTextStyleRowName);
	const FTextBlockStyle* TextBlockStyle = GetTextStyleFor(*ActiveTextStyleRowName);

	GetTargetRichEditableTextBox().Pin()->ApplyToSelection(RunInfo, *TextBlockStyle);

	GetTargetRichEditableTextBox().Pin()->ClearSelection();
}

void SContextTextStyler::OnActiveTextStyleRowNameChanged(TSharedPtr<FName> Name, ESelectInfo::Type Type)
{
	if (!Name.IsValid()) return;

	ActiveTextStyleRowName = Name.Get()->IsNone() ? MakeShareable(new FName(TEXT("Default"))) : Name;

	RefreshActiveTextStyleRowTextBlock();
}

TSharedRef<SWidget> SContextTextStyler::OnGenerateTextStyleRowComboEntry(TSharedPtr<FName> SourceEntry)
{
	return SNew(STextBlock)
		.Margin(FJointEditorStyle::Margin_Small)
		.Text(FText::FromName(*SourceEntry))
		.TextStyle(GetTextStyleFor(*SourceEntry));
}

FText SContextTextStyler::GetActiveTextStyleRowName() const
{
	return ActiveTextStyleRowName ? FText::FromName(*ActiveTextStyleRowName) : FText();
}

void SContextTextStyler::SetTargetRichEditableTextBox(
	const TWeakPtr<SMultiLineEditableTextBox>& NewTargetRichEditableTextBox)
{
	TargetRichEditableTextBox = NewTargetRichEditableTextBox;
}

TWeakPtr<SMultiLineEditableTextBox> SContextTextStyler::GetTargetRichEditableTextBox()
{
	return TargetRichEditableTextBox;
}

void SContextTextStyler::RefreshEditorDetailWidget()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(
		"PropertyEditor");
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void SContextTextStyler::RefreshActiveTextStyleRowTextBlock()
{
	if (!ActiveTextStyleRowBorder.IsValid()) return;

	ActiveTextStyleRowBorder->ClearContent();

	ActiveTextStyleRowBorder->SetContent(GetActiveTextStyleRowTextBlock());
}

void SContextTextStyler::SetActiveRowForRun(TSharedPtr<const IRun> Run) const
{
	for (TSharedPtr<FName> AvailableTextStyleRow : TextStyleRows)
	{
		if (!AvailableTextStyleRow.Get()->IsEqual(*Run->GetRunInfo().Name)) continue;

		TextStyleComboBox->SetSelectedItem(AvailableTextStyleRow);

		return;
	}

	TextStyleComboBox->SetSelectedItem(MakeShareable(new FName(TEXT("Default"))));
}


void SContextTextStyler::ReallocateTextStyleSetInstance()
{
	const FTextBlockStyle* SavedDefaultTextStyle = GetSystemDefaultTextStyle(); //Init

	TextStyleSetInstance = MakeShareable(new FSlateStyleSet(TEXT("RichTextStyle")));
}

void SContextTextStyler::StoreTextStyleData()
{
	TextStyleRows.Empty();

	UDataTable* DataTableToUse = nullptr;

	DataTableToUse = TextDataTableAttr.Get();

	if (DataTableToUse && DataTableToUse->GetRowStruct()->IsChildOf(FRichTextStyleRow::StaticStruct()))
	{
		for (const auto& Entry : Cast<UDataTable>(DataTableToUse)->GetRowMap())
		{
			FName SubStyleName = Entry.Key;

			const FRichTextStyleRow* RichTextStyle = reinterpret_cast<FRichTextStyleRow*>(Entry.Value);

			FTextBlockStyle TargetTextStyle = RichTextStyle->TextStyle;

			TargetTextStyle.Font.Size *= UJointEditorSettings::Get()->ContextTextEditorFontSizeMultiplier;

			TextStyleSetInstance->Set(SubStyleName, TargetTextStyle);

			TSharedPtr<FName> Row = MakeShareable(new FName(SubStyleName));

			if (TextStyleRows.Find(Row) == INDEX_NONE)
			{
				if (!ActiveTextStyleRowName.IsValid()) { ActiveTextStyleRowName = Row; }

				TextStyleRows.Add(Row);
			}
		}
	}

	if (!TextStyleRows.Num())
	{
		const TSharedPtr<FName> DefaultRow = MakeShareable(new FName(TEXT("Default")));

		TextStyleRows.Add(DefaultRow);
	}
}

void SContextTextStyler::FeedDefaultStyleToTargetRichEditableTextBox()
{
	if (TargetRichEditableTextBox.IsValid())
	{
		if ( UJointEditorSettings::Get() && UJointEditorSettings::Get()->bOverrideDefaultStyleFromDataTable)
		{
			TargetRichEditableTextBox.Pin()->SetTextStyle(GetDefaultTextStyle());
		}
	}
}

void SContextTextStyler::RebuildTextStyle()
{
	ReallocateTextStyleSetInstance();

	StoreTextStyleData();
}


void SContextTextStyler::OnStylePipettePressed()
{
	SetIsStylePipetteActivating(!GetIsStylePipetteActivating());

	if(!PipetteButton.IsValid()) return;
	
	if(GetIsStylePipetteActivating())
	{
		PipetteButton->OutlineNormalColor = FLinearColor(0.5,0.5,0.6);

		PipetteButton->PlayUnhoveredAnimation();
		
	}else
	{
		PipetteButton->OutlineNormalColor = FLinearColor::Transparent;
		
		PipetteButton->PlayUnhoveredAnimation();
	}
}

void SContextTextStyler::OnTextBoxCursorMoved(const FTextLocation& NewCursorPosition)
{
	if (!TargetRichEditableTextBox.IsValid()) return;
	if (!GetIsStylePipetteActivating()) return;

	SetActiveRowForRun(TargetRichEditableTextBox.Pin()->GetRunUnderCursor());

	SetIsStylePipetteActivating(false);

	//Play Animation
	
	PipetteButton->OutlineNormalColor = FLinearColor::Transparent;
		
	PipetteButton->PlayUnhoveredAnimation();
	
}


const bool SContextTextStyler::GetIsStylePipetteActivating()
{
	return bIsStylePipetteActivating;
}

void SContextTextStyler::SetIsStylePipetteActivating(const bool NewValue)
{
	bIsStylePipetteActivating = NewValue;
}

const FTextBlockStyle* SContextTextStyler::GetTextStyleFor(const FName& RowName) const
{
	return (!RowName.IsNone())
		       ? &TextStyleSetInstance.Get()->GetWidgetStyle<FTextBlockStyle>(RowName)
		       : GetSystemDefaultTextStyle();
}

const FTextBlockStyle* SContextTextStyler::GetDefaultTextStyle() const
{
	if (TextStyleSetInstance.Get()->HasWidgetStyle<FTextBlockStyle>("Default")) return &TextStyleSetInstance.Get()->
		GetWidgetStyle<FTextBlockStyle>("Default");

	return GetSystemDefaultTextStyle();
}

const FTextBlockStyle* SContextTextStyler::GetSystemDefaultTextStyle()
{
	return &FJointEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("JointUI.TextBlock.Regular.h3");
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
