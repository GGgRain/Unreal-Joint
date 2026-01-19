//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "TextEditor/SContextTextEditor.h"

#include "JointAdvancedWidgets.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

#include "Framework/Text/ITextDecorator.h"


#include "Styling/ISlateStyle.h"
#include "SlateOptMacros.h"

#include "JointEditorSettings.h"
#include "JointEditorStyle.h"
#include "VoltDecl.h"
#include "Framework/Text/RichTextLayoutMarshaller.h"
#include "Framework/Text/RichTextMarkupProcessing.h"
#include "Framework/Text/SyntaxHighlighterTextLayoutMarshaller.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyle.h"
#include "TextEditor/SContextTextStyler.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"


#define LOCTEXT_NAMESPACE "ContextTextEditor"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SContextTextEditor::Construct(const FArguments& InArgs)
{
	SetCanTick(false);

	//default font for the editor.
	TextStyles.AvailableFontFamilies.Emplace(MakeShareable(new FTextStyles::FFontFamily(
		LOCTEXT("RobotoFontFamily", "Roboto")
		, TEXT("Roboto")
		, FCoreStyle::GetDefaultFont()
	)));

	// Set some sensible defaults (these also match the default text style of "RichText.Editor.Text"
	ActiveFontFamily = TextStyles.AvailableFontFamilies[0];
	FontSize = 11;
	FontStyle = FTextStyles::EFontStyle::Regular;
	FontColor = FLinearColor::Black;

	TextAttr = InArgs._Text;
	HintTextAttr = InArgs._HintText;
	TextDataTableAttr = InArgs._TableToEdit;
	bUseStylingAttr = InArgs._bUseStyling;
	TextBoxStyle = InArgs._TextBoxStyle;
	InnerBorderMargin = InArgs._InnerBorderMargin;
	BorderMargin = InArgs._BorderMargin;
	TextblockPadding = InArgs._TextblockPadding;
	TextblockMargin = InArgs._TextblockMargin;
	
	bUseCustomBorderColor = InArgs._bUseCustomBorderColor;
	CustomBorderColor = InArgs._CustomBorderColor;
	
	OnTextChanged = InArgs._OnTextChanged;
	OnTextCommitted = InArgs._OnTextCommitted;

	// The syntax highlighter marshaller is self contained, so doesn't need any extra configuration
	SyntaxHighlighterMarshaller = FRichTextSyntaxHighlighterTextLayoutMarshaller::Create(
		FRichTextSyntaxHighlighterTextLayoutMarshaller::FSyntaxTextStyle()
	);
	
	RebuildWidget();
}

void SContextTextEditor::RebuildWidget()
{
	TAttribute<FSlateColor> EditorBackgroundColor_Attr = TAttribute<FSlateColor>::CreateLambda([this]
	{
		const FLinearColor& Color = bUseCustomBorderColor ? CustomBorderColor : (UJointEditorSettings::Get())? UJointEditorSettings::Get()->ContextTextEditorBackgroundColor : FLinearColor::Black;

		return (IsReadOnly() ? Color + FLinearColor(0.03,0.03,0.03,0.03) : Color); 
	});
	
	const TAttribute<EVisibility> VisibilityAttr = TAttribute<EVisibility>::CreateLambda([this]
	{
		return bUseStylingAttr.Get() ? EVisibility::Visible : EVisibility::Collapsed;
	});

	TAttribute<EVisibility> MainBoxVisibility_Attr = TAttribute<EVisibility>::CreateLambda([this]
	{
		return RawContextEditorSwapVisibility == ERawContextEditorSwapVisibility::Show_MainContextTextEditor ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
	});
	
	TAttribute<EVisibility> RawBoxVisibility_Attr = TAttribute<EVisibility>::CreateLambda([this]
	{
		return RawContextEditorSwapVisibility == ERawContextEditorSwapVisibility::Show_RawContextTextEditor ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
	});
	

	ContextTextStyler.Reset();
	ContextTextBox.Reset();
	RawContextTextBox.Reset();

	AssignContextTextStyler();
	// make it rebuild its text style
	ContextTextStyler->RebuildTextStyle();
	// Rebuild the marshaller to use the new text style set
	RebuildMarshaller();
	
	
	AssignContextTextBox();
	AssignRawContextTextBox();

	// Link the text styler to the text box
	ContextTextStyler->SetTargetRichEditableTextBox(ContextTextBox);
	ContextTextStyler->FeedDefaultStyleToTargetRichEditableTextBox();
	

	OnCursorMoved.BindSP(ContextTextStyler.ToSharedRef(), &SContextTextStyler::OnTextBoxCursorMoved);

	this->ChildSlot
	[
		SNew(SBorder)
		.Visibility(EVisibility::SelfHitTestInvisible)
		.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.BorderBackgroundColor(FLinearColor::Transparent)
		.Padding(BorderMargin)
		[
			// Rich-text editor toolbar
			SNew(SVerticalBox)
			.Visibility(EVisibility::SelfHitTestInvisible)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::SelfHitTestInvisible)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SAssignNew(ContextTextStylerBox, SBox)
					.Visibility(VisibilityAttr)
					[
						ContextTextStyler.ToSharedRef()
					]
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SAssignNew(SwapButton, SJointOutlineButton)
					.ContentPadding(FJointEditorStyle::Margin_Small)
					.HAlign(HAlign_Center)
					.Visibility(VisibilityAttr)
					.NormalColor(FLinearColor::Transparent)
					.OutlineNormalColor(FLinearColor::Transparent)
					.OnClicked(this, &SContextTextEditor::OnSwapButtonDown)
					.ToolTipText(LOCTEXT("SwapButtonTooltip", "Whether to show the raw text."))
					[
						SNew(SBox)
						.Visibility(EVisibility::HitTestInvisible)
						.WidthOverride(12)
						.HeightOverride(12)
						[
							SNew(SImage)
							.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Find"))
						]
					]
				]

			]
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			[
				SNew(SBorder)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(EditorBackgroundColor_Attr)
				.Visibility(MainBoxVisibility_Attr)
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				.Padding(InnerBorderMargin)
				[
					ContextTextBox.ToSharedRef()
				]
			]
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			[
				SNew(SBorder)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(EditorBackgroundColor_Attr)
				.Visibility(RawBoxVisibility_Attr)
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				.Padding(InnerBorderMargin)
				[
					RawContextTextBox.ToSharedRef()
				]
			]
		]

	];

}

void SContextTextEditor::AssignContextTextStyler()
{
	const TAttribute<EVisibility> VisibilityAttr = TAttribute<EVisibility>::CreateLambda([this]
	{
		return bUseStylingAttr.Get() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
	});

	SAssignNew(ContextTextStyler, SContextTextStyler)
	.Visibility(VisibilityAttr)
	.TextStyleTable(TextDataTableAttr)
	.TargetRichEditableTextBox(ContextTextBox);
}


void SContextTextEditor::AssignContextTextBox()
{
	const TAttribute<float> ContextTextAutoTextWrapAt_Attr = TAttribute<float>::CreateLambda([this]
	{
		return (UJointEditorSettings::Get())
			       ? UJointEditorSettings::Get()->ContextTextAutoTextWrapAt
			       : 100;
	});

	SAssignNew(ContextTextBox, SMultiLineEditableTextBox)
	.Visibility(EVisibility::SelfHitTestInvisible)
	.Text(TextAttr)
	.HintText(HintTextAttr)
	.IsReadOnly(this, &SContextTextEditor::IsReadOnly)
	.OnTextChanged(this, &SContextTextEditor::HandleRichEditableTextChanged)
	.OnTextCommitted(this, &SContextTextEditor::HandleRichEditableTextCommitted)
	.OnCursorMoved(this, &SContextTextEditor::HandleRichEditableTextCursorMoved)
	.OnKeyDownHandler(this, &SContextTextEditor::HandleContextTextBoxKeyDown)
	.Marshaller(RichTextMarshaller)
	.WrapTextAt(ContextTextAutoTextWrapAt_Attr)
	.AutoWrapText(false)
	.Margin(TextblockMargin)
	.LineHeightPercentage(1.1f)
	.BackgroundColor(FLinearColor(0, 0, 0, 0))
	.ForegroundColor(FLinearColor(0.5, 0.5, 0.5, 1))
	.Padding(TextblockPadding)
	.Style(TextBoxStyle)
	.ToolTipText(LOCTEXT("TextEditor_ToolTip", "It displays the final visual result of this node's context text."));
}

void SContextTextEditor::AssignRawContextTextBox()
{
	const TAttribute<float> ContextTextAutoTextWrapAt_Attr = TAttribute<float>::CreateLambda([this]
	{
		return UJointEditorSettings::Get() ? UJointEditorSettings::Get()->ContextTextAutoTextWrapAt : 100;
	});

	SAssignNew(RawContextTextBox, SMultiLineEditableTextBox)
	.Visibility(EVisibility::SelfHitTestInvisible)
	.Text(TextAttr)
	.IsReadOnly(this, &SContextTextEditor::IsReadOnly)
	.OnTextChanged(this, &SContextTextEditor::HandleRichEditableTextChanged)
	.OnTextCommitted(this, &SContextTextEditor::HandleRichEditableTextCommitted)
	.WrapTextAt(ContextTextAutoTextWrapAt_Attr)
	.AutoWrapText(false)
	.Margin(TextblockMargin)
	.LineHeightPercentage(1.1f)
	.BackgroundColor(FLinearColor(0, 0, 0, 0))
	.ForegroundColor(FLinearColor(0.5, 0.5, 0.5, 1))
	.Style(TextBoxStyle)
	.Padding(TextblockPadding)
	.ToolTipText(LOCTEXT("RawEditor_ToolTip", "It displays the raw text without the styles and decoration. You can hide this editor on the visibility section on the toolbar."));
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION


void SContextTextEditor::RebuildMarshaller()
{
	// The rich-text marshaller will be used with the rich-text editor...
	RichTextMarkupParser = FDefaultRichTextMarkupParser::Create();
	RichTextMarkupWriter = FDefaultRichTextMarkupWriter::Create();

	RichTextMarshaller = FRichTextLayoutMarshaller::Create(
		RichTextMarkupParser
		, RichTextMarkupWriter
		, TArray<TSharedRef<ITextDecorator>>()
		, &FJointEditorStyle::Get()
	);

	StaticCastSharedPtr<FRichTextLayoutMarshaller>(RichTextMarshaller)->SetDecoratorStyleSet(ContextTextStyler->TextStyleSetInstance.Get());
}

FReply SContextTextEditor::OnSwapButtonDown()
{
	
	switch (RawContextEditorSwapVisibility)
	{
	case ERawContextEditorSwapVisibility::Show_RawContextTextEditor:
		{
			RawContextEditorSwapVisibility = ERawContextEditorSwapVisibility::Show_MainContextTextEditor;

			if(!SwapButton.IsValid()) break;

			VOLT_STOP_ANIM(SwapButton->OutlineColorTrack);
		
			SwapButton->OutlineNormalColor = FLinearColor::Transparent;

			SwapButton->PlayUnhoveredAnimation();
			
			break;
		}

	case ERawContextEditorSwapVisibility::Show_MainContextTextEditor:
		{
			RawContextEditorSwapVisibility = ERawContextEditorSwapVisibility::Show_RawContextTextEditor;

			if(!SwapButton.IsValid()) break;

			VOLT_STOP_ANIM(SwapButton->OutlineColorTrack);

			SwapButton->OutlineNormalColor = FLinearColor(0.5,0.5,0.6);
		
			SwapButton->PlayUnhoveredAnimation();
			
			break;
		}
	default:
		break;
	}

	return FReply::Handled();
}


void SContextTextEditor::RefreshEditorDetailWidget()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(
		"PropertyEditor");
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void SContextTextEditor::HandleRichEditableTextChanged(const FText& Text)
{
	if (IsReadOnly()) return;

	SAdvancedMultiLineTextEditor::HandleRichEditableTextChanged(Text);
}

void SContextTextEditor::HandleRichEditableTextCommitted(const FText& Text, ETextCommit::Type Type)
{
	if (IsReadOnly()) return;

	SAdvancedMultiLineTextEditor::HandleRichEditableTextCommitted(Text, Type);
}

void SContextTextEditor::HandleRichEditableTextCursorMoved(const FTextLocation& NewCursorPosition)
{
	// We can use GetRunUnderCursor to query the style of the text under the cursor
	// We can then use this to update the toolbar
	
	if (IsReadOnly()) return;

	SAdvancedMultiLineTextEditor::HandleRichEditableTextCursorMoved(NewCursorPosition);
	
}

FReply SContextTextEditor::HandleContextTextBoxKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	if (!ContextTextStyler) return FReply::Unhandled();

	if (KeyEvent.IsControlDown() && KeyEvent.IsShiftDown() && KeyEvent.GetKey() == EKeys::S)
	{
		ContextTextStyler->ApplyCurrentStyleToSelectedText();

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SContextTextEditor::OnFocusLost(const FFocusEvent& InFocusEvent)
{
	SAdvancedMultiLineTextEditor::OnFocusLost(InFocusEvent);
}

bool SContextTextEditor::IsReadOnly() const
{
	const FText TextValue = TextAttr.Get();
	if (TextValue.IsFromStringTable())
	{
		return true;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
