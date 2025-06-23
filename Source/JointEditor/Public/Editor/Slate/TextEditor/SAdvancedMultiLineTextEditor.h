//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Text/SlateHyperlinkRun.h"
#include "Framework/Text/SlateTextRun.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/SRichTextBlock.h"

/**
 * 
 */

#define LOCTEXT_NAMESPACE "AdvancedMultiLineTextEditor"

class ITextLayoutMarshaller;
class FDefaultRichTextMarkupWriter;
class FDefaultRichTextMarkupParser;

class SComboButton;
class SEditableTextBox;
class FSyntaxHighlighterTextLayoutMarshaller;
class SMultiLineEditableTextBox;

struct RichTextHelper
{
	static void OnBrowserLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata, TSharedRef<SWidget> ParentWidget);

	static void SpawnProClickerPopUp(const FText& Text, TSharedRef<SWidget> ParentWidget);
};

#undef LOCTEXT_NAMESPACE

struct FTextStyles
{
	/** Flags controlling which TTF or OTF font should be picked from the given font family */
	struct EFontStyle
	{
		typedef uint8 Flags;

		enum Flag
		{
			Regular = 0,
			Bold = 1 << 0,
			Italic = 1 << 1,
		};
	};

	/** This struct defines a font family, which combines multiple TTF or OTF fonts into a single group, allowing text to be styled as bold or italic */
	struct FFontFamily
	{
		FFontFamily(FText InDisplayName, FName InFamilyName, const TSharedRef<const FCompositeFont>& InCompositeFont);

		/** Name used for this font family in the UI */
		FText DisplayName;

		/** Named used to identify this family from the TextStyle decorator */
		FName FamilyName;

		/** Composite font to use (should contain at least a Regular, Bold, Italic, and BoldItalic style) */
		TSharedRef<const FCompositeFont> CompositeFont;
	};

	/** Get the font style name to use based on the requested style flags */
	static FString GetFontStyleString(const EFontStyle::Flags InFontStyle);


	static FRunInfo CreateRunInfo(const FName& InRowName);

	/** Explode some run meta-information back out into its component text style parts */
	void ExplodeRunInfo(const FRunInfo& InRunInfo, TSharedPtr<FFontFamily>& OutFontFamily, uint16& OutFontSize,
	                    EFontStyle::Flags& OutFontStyle, FLinearColor& OutFontColor) const;

	/** Convert the given text style into a text block style for use by Slate */
	static FTextBlockStyle CreateTextBlockStyle(const TSharedPtr<FFontFamily>& InFontFamily, const uint16 InFontSize,
	                                            const EFontStyle::Flags InFontStyle, const FLinearColor& InFontColor);

	/** Convert the given run meta-information into a text block style for use by Slate */
	FTextBlockStyle CreateTextBlockStyle(const FRunInfo& InRunInfo) const;

	/** Try and find a font family with the given name */
	TSharedPtr<FFontFamily> FindFontFamily(const FName InFamilyName) const;

	TArray<TSharedPtr<FFontFamily>> AvailableFontFamilies;
};

class JOINTEDITOR_API FJointTextStyleDecorator : public ITextDecorator
{
public:

	static TSharedRef<FJointTextStyleDecorator> Create(FTextStyles* const InTextStyles);

	virtual ~FJointTextStyleDecorator();

	virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override;

	virtual TSharedRef<ISlateRun> Create(const TSharedRef<FTextLayout>& TextLayout, const FTextRunParseResults& RunParseResult, const FString& OriginalText, const TSharedRef<FString>& InOutModelText, const ISlateStyle* Style) override;

private:

	FJointTextStyleDecorator(FTextStyles* const InTextStyles);

	FTextStyles* TextStyles;
};

DECLARE_DELEGATE_OneParam( FOnCursorMoved, const FTextLocation& );


class JOINTEDITOR_API SAdvancedMultiLineTextEditor : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS( SAdvancedMultiLineTextEditor );
		/** Sets the text content for this editable text box widget */
		SLATE_ATTRIBUTE( FText, Text )

		/** Hint text that appears when there is no text in the text box */
		SLATE_ATTRIBUTE( FText, HintText )
	
		/** Called whenever the text is changed programmatically or interactively by the user */
		SLATE_EVENT( FOnTextChanged, OnTextChanged )
	
	SLATE_END_ARGS()

public:

	// Slate Components Creation (Refactor)

	virtual TSharedRef<SWidget> BuildFontSelector();

	virtual TSharedRef<SWidget> BuildFontSizeSelector();

	virtual TSharedRef<SWidget> BuildColorPickerButton();

	virtual TSharedRef<SWidget> BuildBoldButton();

	virtual TSharedRef<SWidget> BuildItallicButton();
	
	virtual TSharedRef<SWidget> BuildHyperlinkButton();

public:

	FOnTextChanged OnTextChanged;

	FOnTextCommitted OnTextCommitted;

	FOnCursorMoved OnCursorMoved;

	
public:

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	FText GetRichEditableText() const;


	

	virtual void HandleRichEditableTextChanged(const FText& Text);
 
	virtual void HandleRichEditableTextCommitted(const FText& Text, ETextCommit::Type Type);

	virtual void HandleRichEditableTextCursorMoved(const FTextLocation& NewCursorPosition);
	
	virtual bool IsReadOnly() const;

	

	FText GetActiveFontFamilyName() const;
	
	void OnActiveFontFamilyChanged(TSharedPtr<FTextStyles::FFontFamily> NewValue, ESelectInfo::Type);

	TSharedRef<SWidget> GenerateFontFamilyComboEntry(TSharedPtr<FTextStyles::FFontFamily> SourceEntry);

	TOptional<uint16> GetFontSize() const;

	void SetFontSize(uint16 NewValue, ETextCommit::Type);
	
	ECheckBoxState IsFontStyleBold() const;

	void OnFontStyleBoldChanged(ECheckBoxState InState);

	ECheckBoxState IsFontStyleItalic() const;

	void OnFontStyleItalicChanged(ECheckBoxState InState);

	FLinearColor GetFontColor() const;

	void SetFontColor(FLinearColor NewValue);

	FReply OpenFontColorPicker();

	virtual void StyleSelectedText();
	
	void HandleHyperlinkComboOpened();

	FReply HandleInsertHyperlinkClicked();

	ECheckBoxState IsEnableSyntaxHighlightingChecked() const;

	void OnEnableSyntaxHighlightingChanged(ECheckBoxState InState);

	void HandleRichEditableTextCommitted(const FText& Text);

public:

	TSharedPtr<SMultiLineEditableTextBox> ContextTextBox;
	TSharedPtr<SMultiLineEditableTextBox> RawContextTextBox;

	//for some occasion when we need to directly access the instances of the marshaller or its components to process down some data.
	TSharedPtr<ITextLayoutMarshaller> RichTextMarshaller;
	TSharedPtr<FDefaultRichTextMarkupParser> RichTextMarkupParser;
	TSharedPtr<FDefaultRichTextMarkupWriter> RichTextMarkupWriter;

	TSharedPtr< ITextDecorator > ExecutionDecorator;

	
	TSharedPtr<FSyntaxHighlighterTextLayoutMarshaller> SyntaxHighlighterMarshaller;

	FSlateHyperlinkRun::FOnClick OnHyperlinkClicked;
	TSharedPtr<SComboButton> HyperlinkComboButton;
	TSharedPtr<SEditableTextBox> HyperlinkNameTextBox;
	TSharedPtr<SEditableTextBox> HyperlinkURLTextBox;

	
	FTextStyles TextStyles;

	TSharedPtr<FTextStyles::FFontFamily> ActiveFontFamily;
	uint16 FontSize;
	FTextStyles::EFontStyle::Flags FontStyle;
	FLinearColor FontColor;

public:

	
	virtual void RebuildMarshaller();

	virtual void RebuildWidget();
	
public:
	
	//Asset Picker Related

	//callback for the asset selection
	void OnTextStyleTableSelected(const FAssetData& SelectedAsset);
};


