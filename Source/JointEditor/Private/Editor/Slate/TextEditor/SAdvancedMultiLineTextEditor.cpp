//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "TextEditor/SAdvancedMultiLineTextEditor.h"

#include "JointEditorStyle.h"
#include "SlateOptMacros.h"
#include "Framework/Text/ITextDecorator.h"
#include "Framework/Text/RichTextLayoutMarshaller.h"
#include "Framework/Text/SyntaxHighlighterTextLayoutMarshaller.h"
#include "Framework/Text/TextDecorators.h"
#include "Styling/ISlateStyle.h"

#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Input/SComboBox.h"

#define LOCTEXT_NAMESPACE "AdvancedMultiLineTextEditor"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAdvancedMultiLineTextEditor::Construct(const FArguments& InArgs)
{

	SetCanTick(false);

	TextStyles.AvailableFontFamilies.Emplace(MakeShareable(new FTextStyles::FFontFamily(
		LOCTEXT("RobotoFontFamily", "Roboto"),
		TEXT("Roboto"),
		FCoreStyle::GetDefaultFont()
	)));

	// Set some sensible defaults (these also match the default text style of "RichText.Editor.Text"
	ActiveFontFamily = TextStyles.AvailableFontFamilies[0];
	FontSize = 11;
	FontStyle = FTextStyles::EFontStyle::Regular;
	FontColor = FLinearColor::Black;
	
	RebuildMarshaller();

	// The syntax highlighter marshaller is self contained, so doesn't need any extra configuration
	SyntaxHighlighterMarshaller = FRichTextSyntaxHighlighterTextLayoutMarshaller::Create(
		FRichTextSyntaxHighlighterTextLayoutMarshaller::FSyntaxTextStyle()
	);

	OnTextChanged = InArgs._OnTextChanged;

	/*
	this->ChildSlot
	[

		SNew(SBorder)
		.Padding(0.0f)
		.BorderImage(FCoreStyle::Get().GetBrush("BoxShadow"))
		[
			SNew(SBorder)
			.BorderImage(FJointEditorStyle::Get().GetBrush("RichText.RoundedBackground"))
			.Padding(FMargin(2))
			[
				// Rich-text editor toolbar
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .Padding(FMargin(0.0f, 0.0f, 0.0f, 0.0f))
				[
					SNew(SBorder)
					.BorderImage(FJointEditorStyle::Get().GetBrush("RichText.RoundedBackground"))
					.Padding(FMargin(0))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							BuildFontSelector()
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SBox)
							.MinDesiredWidth(10)
							.MinDesiredHeight(10)
							[
								BuildFontSizeSelector()
							]
						]

						+ SHorizontalBox::Slot()
						  .AutoWidth()
						  .Padding(FMargin(0.0f, 0.0f, 0.0f, 0.0f))
						[
							BuildBoldButton()
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							BuildItallicButton()
						]

						+ SHorizontalBox::Slot()
						  .Padding(FMargin(2.0f, 0.0f, 0.0f, 0.0f))
						  .AutoWidth()
						[
							BuildColorPickerButton()
						]

						+ SHorizontalBox::Slot()
						  .Padding(FMargin(2.0f, 0.0f, 0.0f, 0.0f))
						  .AutoWidth()
						[
							BuildHyperlinkButton()
						]
					]
				]

				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .Padding(FMargin(0.0f, 0.0f, 0.0f, 0.0f))
				[
					SAssignNew(RawRichEditableTextBox, SMultiLineEditableTextBox)
								.Font(
						                                                          FJointEditorStyle::Get().
						                                                          GetWidgetStyle<
							                                                          FTextBlockStyle>(
							                                                          "RichText.Editor.Text").
						                                                          Font)
								.Text(InArgs._Text)
								.HintText(InArgs._HintText)
								.OnTextChanged(this, &SAdvancedMultiLineTextEditor::HandleRichEditableTextChanged)
								.OnCursorMoved(this, &SAdvancedMultiLineTextEditor::HandleRichEditableTextCursorMoved)
								.Marshaller(RichTextMarshaller)
								.ClearTextSelectionOnFocusLoss(false)
								.AutoWrapText(true)
								.Margin(4)
								.LineHeightPercentage(1.1f)
				]
			]
		]

	];

	*/
}

void SAdvancedMultiLineTextEditor::RebuildMarshaller()
{
	// The rich-text marshaller will be used with the rich-text editor...
	RichTextMarshaller = FRichTextLayoutMarshaller::Create(
		TArray<TSharedRef<ITextDecorator>>(),
		&FJointEditorStyle::Get()
	);

	// ... so we also need to add some decorators to handle the things we want to demo
	StaticCastSharedPtr<FRichTextLayoutMarshaller>(RichTextMarshaller)->AppendInlineDecorator(
		FHyperlinkDecorator::Create(TEXT("browser"), OnHyperlinkClicked));
	StaticCastSharedPtr<FRichTextLayoutMarshaller>(RichTextMarshaller)->AppendInlineDecorator(
		FJointTextStyleDecorator::Create(&TextStyles));
}

TSharedRef<SWidget> SAdvancedMultiLineTextEditor::BuildFontSizeSelector()
{
	return SNew(SNumericEntryBox<uint16>)
		.Value(this, &SAdvancedMultiLineTextEditor::GetFontSize)
		.OnValueCommitted(this, &SAdvancedMultiLineTextEditor::SetFontSize);
}

void RichTextHelper::OnBrowserLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata,
                                          TSharedRef<SWidget> ParentWidget)
{
	const FString* url = Metadata.Find(TEXT("href"));

	if (url)
	{
		FPlatformProcess::LaunchURL(**url, nullptr, nullptr);
	}
	else
	{
		SpawnProClickerPopUp(LOCTEXT("FailedToFindUrlPopUpMessage",
		                             "Sorry this hyperlink is not <RichText.Tagline.TextHighlight>configured incorrectly</>!"), ParentWidget);
	}
}

void RichTextHelper::SpawnProClickerPopUp(const FText& Text, TSharedRef<SWidget> ParentWidget)
{
	TSharedRef<SWidget> Widget =
		SNew(SBorder).Padding(10).BorderImage(FJointEditorStyle::Get().GetBrush("RichText.Tagline.Background"))
		[
			SNew(SRichTextBlock)
			.Text(Text)
			.TextStyle(FJointEditorStyle::Get(), "RichText.Tagline.Text")
			.DecoratorStyleSet(&FJointEditorStyle::Get())
			.Justification(ETextJustify::Center)
		];


	FSlateApplication::Get().PushMenu(
		ParentWidget,
		// Parent widget should be TestSuite, not the menu thats open or it will be closed when the menu is dismissed
		FWidgetPath(),
		Widget,
		FSlateApplication::Get().GetCursorPos(), // summon location
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
	);
}

FTextStyles::FFontFamily::FFontFamily(FText InDisplayName, FName InFamilyName,
                                      const TSharedRef<const FCompositeFont>& InCompositeFont):
	DisplayName(InDisplayName)
	, FamilyName(InFamilyName)
	, CompositeFont(InCompositeFont)
{
}

FString FTextStyles::GetFontStyleString(const EFontStyle::Flags InFontStyle)
{
	FString FontStyleString;
	if (InFontStyle == EFontStyle::Regular)
	{
		FontStyleString = TEXT("Regular");
	}
	else
	{
		if (InFontStyle & EFontStyle::Bold)
		{
			FontStyleString += TEXT("Bold");
		}
		if (InFontStyle & EFontStyle::Italic)
		{
			FontStyleString += TEXT("Italic");
		}
	}
	return FontStyleString;
}

FRunInfo FTextStyles::CreateRunInfo(const FName& InRowName)
{
	FRunInfo RunInfo(InRowName.ToString());

	return RunInfo;
}

void FTextStyles::ExplodeRunInfo(const FRunInfo& InRunInfo, TSharedPtr<FFontFamily>& OutFontFamily, uint16& OutFontSize,
                                 EFontStyle::Flags& OutFontStyle, FLinearColor& OutFontColor) const
{
	check(AvailableFontFamilies.Num());

	const FString* const FontFamilyString = InRunInfo.MetaData.Find(TEXT("FontFamily"));
	if (FontFamilyString)
	{
		OutFontFamily = FindFontFamily(FName(**FontFamilyString));
	}
	if (!OutFontFamily.IsValid())
	{
		OutFontFamily = AvailableFontFamilies[0];
	}

	OutFontSize = 11;
	const FString* const FontSizeString = InRunInfo.MetaData.Find(TEXT("FontSize"));
	if (FontSizeString)
	{
		OutFontSize = static_cast<uint16>(FPlatformString::Atoi(**FontSizeString));
	}

	OutFontStyle = EFontStyle::Regular;
	const FString* const FontStyleString = InRunInfo.MetaData.Find(TEXT("FontStyle"));
	if (FontStyleString)
	{
		if (*FontStyleString == TEXT("Bold"))
		{
			OutFontStyle = EFontStyle::Bold;
		}
		else if (*FontStyleString == TEXT("Italic"))
		{
			OutFontStyle = EFontStyle::Italic;
		}
		else if (*FontStyleString == TEXT("BoldItalic"))
		{
			OutFontStyle = EFontStyle::Bold | EFontStyle::Italic;
		}
	}

	OutFontColor = FLinearColor::Black;
	const FString* const FontColorString = InRunInfo.MetaData.Find(TEXT("FontColor"));
	if (FontColorString && !OutFontColor.InitFromString(*FontColorString))
	{
		OutFontColor = FLinearColor::Black;
	}
}

FTextBlockStyle FTextStyles::CreateTextBlockStyle(const TSharedPtr<FFontFamily>& InFontFamily, const uint16 InFontSize,
                                                  const EFontStyle::Flags InFontStyle, const FLinearColor& InFontColor)
{
	FSlateFontInfo FontInfo;
	FontInfo.CompositeFont = InFontFamily->CompositeFont;
	FontInfo.TypefaceFontName = *GetFontStyleString(InFontStyle);
	FontInfo.Size = InFontSize;

	FTextBlockStyle TextBlockStyle;
	TextBlockStyle.SetFont(FontInfo);
	TextBlockStyle.SetColorAndOpacity(InFontColor);
	return TextBlockStyle;
}

FTextBlockStyle FTextStyles::CreateTextBlockStyle(const FRunInfo& InRunInfo) const
{
	TSharedPtr<FFontFamily> FontFamily;
	uint16 FontSize;
	EFontStyle::Flags FontStyle;
	FLinearColor FontColor;
	ExplodeRunInfo(InRunInfo, FontFamily, FontSize, FontStyle, FontColor);
	return CreateTextBlockStyle(FontFamily, FontSize, FontStyle, FontColor);
}

TSharedPtr<FTextStyles::FFontFamily> FTextStyles::FindFontFamily(const FName InFamilyName) const
{
	const TSharedPtr<FFontFamily>* const FoundFontFamily = AvailableFontFamilies.FindByPredicate(
		[InFamilyName](TSharedPtr<FFontFamily>& Entry) -> bool
		{
			return Entry->FamilyName == InFamilyName;
		});
	return (FoundFontFamily) ? *FoundFontFamily : nullptr;
}

TSharedRef<FJointTextStyleDecorator> FJointTextStyleDecorator::Create(FTextStyles* const InTextStyles)
{
	return MakeShareable(new FJointTextStyleDecorator(InTextStyles));
}

FJointTextStyleDecorator::~FJointTextStyleDecorator()
{
}

bool FJointTextStyleDecorator::Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const
{
	return (RunParseResult.Name == TEXT("TextStyle"));
}

TSharedRef<ISlateRun> FJointTextStyleDecorator::Create(const TSharedRef<FTextLayout>& TextLayout,
                                                          const FTextRunParseResults& RunParseResult,
                                                          const FString& OriginalText,
                                                          const TSharedRef<FString>& InOutModelText,
                                                          const ISlateStyle* Style)
{
	FRunInfo RunInfo(RunParseResult.Name);
	for (const TPair<FString, FTextRange>& Pair : RunParseResult.MetaData)
	{
		RunInfo.MetaData.Add(
			Pair.Key, OriginalText.Mid(Pair.Value.BeginIndex, Pair.Value.EndIndex - Pair.Value.BeginIndex));
	}

	FTextRange ModelRange;
	ModelRange.BeginIndex = InOutModelText->Len();
	*InOutModelText += OriginalText.Mid(RunParseResult.ContentRange.BeginIndex,
	                                    RunParseResult.ContentRange.EndIndex - RunParseResult.ContentRange.BeginIndex);
	ModelRange.EndIndex = InOutModelText->Len();

	return FSlateTextRun::Create(RunInfo, InOutModelText, TextStyles->CreateTextBlockStyle(RunInfo), ModelRange);
}

FJointTextStyleDecorator::FJointTextStyleDecorator(FTextStyles* const InTextStyles): TextStyles(InTextStyles)
{
}

SAdvancedMultiLineTextEditor::FArguments::FArguments(): _Text()
                                                        , _HintText()
{
}

TSharedRef<SWidget> SAdvancedMultiLineTextEditor::BuildFontSelector()
{
	return SNew(SComboBox<TSharedPtr<FTextStyles::FFontFamily>>)
		.ComboBoxStyle(FJointEditorStyle::Get(), "RichText.Toolbar.ComboBox")
		.OptionsSource(&TextStyles.AvailableFontFamilies)
		.OnSelectionChanged(this, &SAdvancedMultiLineTextEditor::OnActiveFontFamilyChanged)
		.OnGenerateWidget(this, &SAdvancedMultiLineTextEditor::GenerateFontFamilyComboEntry)
		.InitiallySelectedItem(ActiveFontFamily)
		[
			SNew(SBox)
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 0.0f))
			[
				SNew(STextBlock)
				.Text(this, &SAdvancedMultiLineTextEditor::GetActiveFontFamilyName)
			]
		];
}

TSharedRef<SWidget> SAdvancedMultiLineTextEditor::BuildColorPickerButton()
{
	return SNew(SButton)
		.ButtonStyle(FJointEditorStyle::Get(), "RichText.Toolbar.Button")
		.ContentPadding(FJointEditorStyle::Margin_Normal)
		.OnClicked(this, &SAdvancedMultiLineTextEditor::OpenFontColorPicker)
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 2.0f))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Bottom)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "RichText.Toolbar.BoldText")
				.Text(LOCTEXT("ColorLabel", "A"))
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Bottom)
			[
				SNew(SColorBlock)
				.Color(this, &SAdvancedMultiLineTextEditor::GetFontColor)
				.Size(FVector2D(10.0f, 10.0f))
			]
		];
}

TSharedRef<SWidget> SAdvancedMultiLineTextEditor::BuildBoldButton()
{
	return SNew(SCheckBox)
		.Style(FJointEditorStyle::Get(), "RichText.Toolbar.ToggleButtonCheckbox")
		.IsChecked(this, &SAdvancedMultiLineTextEditor::IsFontStyleBold)
		.OnCheckStateChanged(
			this, &SAdvancedMultiLineTextEditor::OnFontStyleBoldChanged)
		[
			SNew(SBox)
			.WidthOverride(10)
			.HeightOverride(10)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "RichText.Toolbar.BoldText")
				.Text(LOCTEXT("BoldLabel", "B"))
			]

		];
}

TSharedRef<SWidget> SAdvancedMultiLineTextEditor::BuildItallicButton()
{
	return SNew(SCheckBox)
		.Style(FJointEditorStyle::Get(), "RichText.Toolbar.ToggleButtonCheckbox")
		.IsChecked(this, &SAdvancedMultiLineTextEditor::IsFontStyleItalic)
		.OnCheckStateChanged(
			this, &SAdvancedMultiLineTextEditor::OnFontStyleItalicChanged)
		[
			SNew(SBox)
			.WidthOverride(10)
			.HeightOverride(10)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(),
				           "RichText.Toolbar.ItalicText")
				.Text(LOCTEXT("ItalicLabel", "I"))
			]
		];
}

TSharedRef<SWidget> SAdvancedMultiLineTextEditor::BuildHyperlinkButton()
{
	return SAssignNew(HyperlinkComboButton, SComboButton)
		.ComboButtonStyle(FJointEditorStyle::Get(),
		                  "RichText.Toolbar.ComboButton")
		.HasDownArrow(false)
		.OnComboBoxOpened(
			this,
			&SAdvancedMultiLineTextEditor::HandleHyperlinkComboOpened)
		.ButtonContent()
		[
			SNew(SBox)
			.WidthOverride(10)
			.HeightOverride(10)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				//.Image(FJointEditorStyle::Get().GetBrush("RichText.Toolbar.HyperlinkImage"))
			]
		]
		.MenuContent()
		[
			SNew(SGridPanel)
			.FillColumn(1, 1.0f)

			+ SGridPanel::Slot(0, 0)
			.HAlign(HAlign_Right)
			.Padding(FMargin(2.0f))
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "RichText.Toolbar.Text")
				.Text(LOCTEXT("HyperlinkNameLabel", "Name:"))
			]
			+ SGridPanel::Slot(1, 0)
			.Padding(FMargin(2.0f))
			[
				SNew(SBox)
				.WidthOverride(300)
				[
					SAssignNew(HyperlinkNameTextBox, SEditableTextBox)
				]
			]

			+ SGridPanel::Slot(0, 1)
			.HAlign(HAlign_Right)
			.Padding(FMargin(2.0f))
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "RichText.Toolbar.Text")
				.Text(LOCTEXT("HyperlinkURLLabel", "URL:"))
			]
			+ SGridPanel::Slot(1, 1)
			.Padding(FMargin(2.0f))
			[
				SNew(SBox)
				.WidthOverride(300)
				[
					SAssignNew(HyperlinkURLTextBox, SEditableTextBox)
				]
			]

			+ SGridPanel::Slot(0, 2)
			.HAlign(HAlign_Right)
			.Padding(FMargin(2.0f))
			.ColumnSpan(2)
			[
				SNew(SButton)
				.ButtonStyle(FJointEditorStyle::Get(), "RichText.Toolbar.Button")
				.ContentPadding(FJointEditorStyle::Margin_Normal)
				.OnClicked(
					this, &SAdvancedMultiLineTextEditor::HandleInsertHyperlinkClicked)
				[
					SNew(STextBlock)
					.TextStyle(FJointEditorStyle::Get(), "RichText.Toolbar.Text")
					.Text(LOCTEXT("HyperlinkInsertLabel", "Insert Hyperlink"))
				]
			]
		];
}

void SAdvancedMultiLineTextEditor::RebuildWidget()
{
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION


FText SAdvancedMultiLineTextEditor::GetRichEditableText() const
{
	return ContextTextBox->GetText();
}

void SAdvancedMultiLineTextEditor::HandleRichEditableTextChanged(const FText& Text)
{
	OnTextChanged.ExecuteIfBound(Text);
}

void SAdvancedMultiLineTextEditor::HandleRichEditableTextCommitted(const FText& Text, ETextCommit::Type Type)
{
	OnTextCommitted.ExecuteIfBound(Text, Type);
}

void SAdvancedMultiLineTextEditor::HandleRichEditableTextCursorMoved(const FTextLocation& NewCursorPosition)
{
	OnCursorMoved.ExecuteIfBound(NewCursorPosition);
}

bool SAdvancedMultiLineTextEditor::IsReadOnly() const
{
	return false;
}

FText SAdvancedMultiLineTextEditor::GetActiveFontFamilyName() const
{
	if (ActiveFontFamily)
	{
		return ActiveFontFamily->DisplayName;
	}
	return FText();
}

void SAdvancedMultiLineTextEditor::OnActiveFontFamilyChanged(TSharedPtr<FTextStyles::FFontFamily> NewValue,
                                                             ESelectInfo::Type)
{
	ActiveFontFamily = NewValue;
	StyleSelectedText();
}

TSharedRef<SWidget> SAdvancedMultiLineTextEditor::GenerateFontFamilyComboEntry(
	TSharedPtr<FTextStyles::FFontFamily> SourceEntry)
{
	return SNew(STextBlock).Text(SourceEntry->DisplayName);
}


TOptional<uint16> SAdvancedMultiLineTextEditor::GetFontSize() const
{
	return FontSize;
}

void SAdvancedMultiLineTextEditor::SetFontSize(uint16 NewValue, ETextCommit::Type)
{
	FontSize = NewValue;
	StyleSelectedText();
}

ECheckBoxState SAdvancedMultiLineTextEditor::IsFontStyleBold() const
{
	return (FontStyle & FTextStyles::EFontStyle::Bold) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SAdvancedMultiLineTextEditor::OnFontStyleBoldChanged(ECheckBoxState InState)
{
	if (InState == ECheckBoxState::Checked)
	{
		FontStyle |= FTextStyles::EFontStyle::Bold;
	}
	else
	{
		FontStyle &= ~FTextStyles::EFontStyle::Bold;
	}
	StyleSelectedText();
}

ECheckBoxState SAdvancedMultiLineTextEditor::IsFontStyleItalic() const
{
	return (FontStyle & FTextStyles::EFontStyle::Italic) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SAdvancedMultiLineTextEditor::OnFontStyleItalicChanged(ECheckBoxState InState)
{
	if (InState == ECheckBoxState::Checked)
	{
		FontStyle |= FTextStyles::EFontStyle::Italic;
	}
	else
	{
		FontStyle &= ~FTextStyles::EFontStyle::Italic;
	}
	StyleSelectedText();
}

FLinearColor SAdvancedMultiLineTextEditor::GetFontColor() const
{
	return FontColor;
}

void SAdvancedMultiLineTextEditor::SetFontColor(FLinearColor NewValue)
{
	FontColor = NewValue;
	StyleSelectedText();
}

FReply SAdvancedMultiLineTextEditor::OpenFontColorPicker()
{
	FColorPickerArgs PickerArgs;
	PickerArgs.bOnlyRefreshOnMouseUp = true;
	PickerArgs.ParentWidget = AsShared();
	PickerArgs.bUseAlpha = false;
	PickerArgs.bOnlyRefreshOnOk = false;
	PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateSP(
		this, &SAdvancedMultiLineTextEditor::SetFontColor);
	PickerArgs.OnColorPickerCancelled = FOnColorPickerCancelled::CreateSP(
		this, &SAdvancedMultiLineTextEditor::SetFontColor);
	
#if UE_VERSION_OLDER_THAN(5,2,0)
	PickerArgs.InitialColorOverride = FontColor;
#else
	PickerArgs.InitialColor = FontColor;
#endif

	OpenColorPicker(PickerArgs);

	return FReply::Handled();
}

void SAdvancedMultiLineTextEditor::StyleSelectedText()
{
	// Apply the current style to the selected text
	// If no text is selected, then a new (empty) run will be inserted with the appropriate style
	const FRunInfo RunInfo = TextStyles.CreateRunInfo(FName("TextStyle"));

	const FTextBlockStyle TextBlockStyle = TextStyles.CreateTextBlockStyle(
		ActiveFontFamily, FontSize, FontStyle, FontColor);

	ContextTextBox->ApplyToSelection(RunInfo, TextBlockStyle);

	FSlateApplication::Get().SetKeyboardFocus(ContextTextBox, EFocusCause::SetDirectly);
}

void SAdvancedMultiLineTextEditor::HandleHyperlinkComboOpened()
{
	// Read any currently selected text, and use this as the default name of the hyperlink
	FString SelectedText = ContextTextBox->GetSelectedText().ToString();
	for (int32 SelectedTextIndex = 0; SelectedTextIndex < SelectedText.Len(); ++SelectedTextIndex)
	{
		if (FChar::IsLinebreak(SelectedText[SelectedTextIndex]))
		{
#if UE_VERSION_OLDER_THAN(5, 5, 0)
			SelectedText.LeftInline(SelectedTextIndex, false);
#else
			SelectedText.LeftInline(SelectedTextIndex, EAllowShrinking::No);
#endif
			break;
		}
	}
	HyperlinkNameTextBox->SetText(FText::FromString(SelectedText));

	// We can use GetRunUnderCursor to query whether the cursor is currently over a hyperlink
	// If it is, we can use that as the default URL for the hyperlink
	TSharedPtr<const IRun> Run = ContextTextBox->GetRunUnderCursor();
	if (Run.IsValid() && Run->GetRunInfo().Name == TEXT("a"))
	{
		const FString* const URLUnderCursor = Run->GetRunInfo().MetaData.Find(TEXT("href"));
		HyperlinkURLTextBox->SetText((URLUnderCursor) ? FText::FromString(*URLUnderCursor) : FText());
	}
	else
	{
		HyperlinkURLTextBox->SetText(FText());
	}
}

FReply SAdvancedMultiLineTextEditor::HandleInsertHyperlinkClicked()
{
	HyperlinkComboButton->SetIsOpen(false);

	const FText& Name = HyperlinkNameTextBox->GetText();
	const FText& URL = HyperlinkURLTextBox->GetText();

	// Create the correct meta-information for this run, so that valid source rich-text formatting can be generated for it
	FRunInfo RunInfo(TEXT("a"));
	RunInfo.MetaData.Add(TEXT("id"), TEXT("browser"));
	RunInfo.MetaData.Add(TEXT("href"), URL.ToString());
	RunInfo.MetaData.Add(TEXT("style"), TEXT("RichText.Editor.Hyperlink"));

	// Create the new run, and then insert it at the cursor position
	TSharedRef<FSlateHyperlinkRun> HyperlinkRun = FSlateHyperlinkRun::Create(
		RunInfo,
		MakeShareable(new FString(Name.ToString())),
		FJointEditorStyle::Get().GetWidgetStyle<FHyperlinkStyle>(FName(TEXT("RichText.Editor.Hyperlink"))),
		OnHyperlinkClicked,
		FSlateHyperlinkRun::FOnGenerateTooltip(),
		FSlateHyperlinkRun::FOnGetTooltipText()
	);
	ContextTextBox->InsertRunAtCursor(HyperlinkRun);

	return FReply::Handled();
}

ECheckBoxState SAdvancedMultiLineTextEditor::IsEnableSyntaxHighlightingChecked() const
{
	return (SyntaxHighlighterMarshaller->IsSyntaxHighlightingEnabled())
		       ? ECheckBoxState::Checked
		       : ECheckBoxState::Unchecked;
}

void SAdvancedMultiLineTextEditor::OnEnableSyntaxHighlightingChanged(ECheckBoxState InState)
{
	SyntaxHighlighterMarshaller->EnableSyntaxHighlighting(InState == ECheckBoxState::Checked);
}

#undef LOCTEXT_NAMESPACE
