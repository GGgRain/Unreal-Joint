﻿//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Slate/SJointManagerViewer.h"
#include "JointEditorStyle.h"
#include "JointEdUtils.h"
#include "Filter/JointTreeFilter.h"
#include "Filter/JointTreeFilterItem.h"
#include "Item/JointTreeItem_Property.h"
#include "SearchTree/Builder/IJointTreeBuilder.h"
#include "Misc/MessageDialog.h"
#include "Styling/SlateStyleMacros.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"

#define LOCTEXT_NAMESPACE "JointPropertyList"


void SJointManagerViewer::Construct(const FArguments& InArgs)
{
	ToolKitPtr = InArgs._ToolKitPtr;
	JointManagers = InArgs._JointManagers;
	BuilderArgs = InArgs._BuilderArgs;

	RebuildWidget();
}


void SJointManagerViewer::OnSearchModeButtonPressed()
{
	SetMode(EJointManagerViewerMode::Search);

	Tree->SetHighlightInlineFilterText(SearchFilterText);
	Tree->SetQueryInlineFilterText(FJointTreeFilter::ReplaceInqueryableCharacters(SearchFilterText));
}

void SJointManagerViewer::OnReplaceModeButtonPressed()
{
	SetMode(EJointManagerViewerMode::Replace);

	Tree->SetHighlightInlineFilterText(ReplaceFromText);
	Tree->SetQueryInlineFilterText(FJointTreeFilter::ReplaceInqueryableCharacters(ReplaceFromText));
}

void SJointManagerViewer::OnReplaceNextButtonPressed()
{
	Tree->ApplyFilter();

	ReplaceNextSrc();
}

void SJointManagerViewer::OnReplaceAllButtonPressed()
{
	Tree->ApplyFilter();

	ReplaceAllSrc();
}


TSharedRef<SWidget> SJointManagerViewer::CreateModeSelectionSection()
{
	TAttribute<bool> IsEnabled_Search_Attr = TAttribute<bool>::CreateLambda([this]
		{
			return Mode != EJointManagerViewerMode::Search ? true : false;
		});

	TAttribute<bool> IsEnabled_Replace_Attr = TAttribute<bool>::CreateLambda([this]
		{
			return Mode != EJointManagerViewerMode::Replace ? true : false;
		});

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.AutoWidth()
		.Padding(FJointEditorStyle::Margin_Frame)
		[
			SNew(SButton)
			.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round")
			.ContentPadding(FJointEditorStyle::Margin_Button)
			.OnPressed(this, &SJointManagerViewer::OnSearchModeButtonPressed)
			.IsEnabled(IsEnabled_Search_Attr)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::HitTestInvisible)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Frame)
				[
					SNew(SBox)
					.WidthOverride(16)
					.HeightOverride(16)
					[
						SNew(SImage)
						.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Search"))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Frame)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SearchBox", "Search"))
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h2")
					.ColorAndOpacity(FLinearColor(1, 1, 1))
				]
			]
		]
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.AutoWidth()
		.Padding(FJointEditorStyle::Margin_Frame)
		[
			SNew(SButton)
			.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round")
			.ContentPadding(FJointEditorStyle::Margin_Button)
			.OnPressed(this, &SJointManagerViewer::OnReplaceModeButtonPressed)
			.IsEnabled(IsEnabled_Replace_Attr)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::HitTestInvisible)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Frame)
				[
					SNew(SBox)
					.WidthOverride(16)
					.HeightOverride(16)
					[
						SNew(SImage)
						.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Convert"))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Frame)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ReplaceBox", "Replace"))
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h2")
					.ColorAndOpacity(FLinearColor(1, 1, 1))
				]
			]
		];
}

void SJointManagerViewer::RebuildWidget()
{
	if (!Tree.IsValid())
	{
		Tree = SNew(SJointTree)
			.BuilderArgs(BuilderArgs);
	}

	Tree->JointManagerToShow = JointManagers;
	Tree->BuildFromJointManagers();

	TAttribute<FSlateColor> NodeVisibilityCheckBoxColor_Attr = TAttribute<FSlateColor>::CreateLambda([this]
		{
			if (!NodeVisibilityCheckbox.IsValid()) { return FLinearColor(0.5, 0.2, 0.2); }
			return (NodeVisibilityCheckbox->IsChecked())
				       ? FLinearColor(0.7, 0.4, 0.02)
				       : FLinearColor(0.03, 0.03, 0.03);
		});

	TAttribute<ECheckBoxState> NodeVisibilityIsChecked_Attr = TAttribute<ECheckBoxState>::CreateLambda([this]
		{
			return (Tree->Builder->IsShowingNodes()) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		});


	TAttribute<EVisibility> VisibilityByMode_Search_Attr = TAttribute<EVisibility>::CreateLambda([this]
		{
			return (Mode == EJointManagerViewerMode::Search) ? EVisibility::Visible : EVisibility::Collapsed;
		});

	TAttribute<EVisibility> VisibilityByMode_Replace_Attr = TAttribute<EVisibility>::CreateLambda([this]
		{
			return (Mode == EJointManagerViewerMode::Replace) ? EVisibility::Visible : EVisibility::Collapsed;
		});


	this->ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			//SAssignNew(SearchBox, SSearchBox)
			CreateModeSelectionSection()
		]
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(SVerticalBox)
			.Visibility(VisibilityByMode_Search_Attr)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				//SAssignNew(SearchBox, SSearchBox)
				SAssignNew(SearchSearchBox, SSearchBox)
				.HintText(LOCTEXT("FilterSearch", "Search..."))
				.ToolTipText(LOCTEXT("FilterSearchHint", "Type here to search"))
				.OnTextChanged(this, &SJointManagerViewer::OnFilterTextChanged)
				.OnTextCommitted(this, &SJointManagerViewer::OnFilterTextCommitted)
			]
		]
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(SVerticalBox)
			.Visibility(VisibilityByMode_Replace_Attr)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.AutoHeight()
			.Padding(4.0f, 0.0f, 0.0f, 4.0f)
			[
				//SAssignNew(SearchBox, SSearchBox)
				SNew(STextBlock)
				.Text(LOCTEXT("ReplaceHelperText", "Replace texts. Works for the literal type only."))
			]
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[

				SAssignNew(ReplaceFromSearchBox, SSearchBox)
				.HintText(LOCTEXT("FilterReplaceFrom", "Replace from..."))
				.ToolTipText(LOCTEXT("FilterReplaceFromHint", "Type a text to search and replace from"))
				.OnTextChanged(this, &SJointManagerViewer::OnFilterReplaceFromTextChanged)
			]
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SAssignNew(ReplaceToSearchBox, SSearchBox)
				.HintText(LOCTEXT("FilterReplaceTo", "Replace to..."))
				.ToolTipText(LOCTEXT("FilterReplaceToHint", "Type a text to search and replace to"))
				.OnTextChanged(this, &SJointManagerViewer::OnFilterReplaceToTextChanged)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(SButton)
					.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round")
					.ContentPadding(FJointEditorStyle::Margin_Button)
					.OnPressed(this, &SJointManagerViewer::OnReplaceNextButtonPressed)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ReplaceNextBoxText", "Replace Next"))
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h5")
						.ColorAndOpacity(FLinearColor(1, 1, 1))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(SButton)
					.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round")
					.ContentPadding(FJointEditorStyle::Margin_Button)
					.OnPressed(this, &SJointManagerViewer::OnReplaceAllButtonPressed)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ReplaceAllBoxText", "Replace All"))
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h5")
						.ColorAndOpacity(FLinearColor(1, 1, 1))
					]
				]
			]
		]
		/*
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			//SAssignNew(SearchBox, SSearchBox)
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.AutoWidth()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(SBorder)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(NodeVisibilityCheckBoxColor_Attr)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.Padding(FMargin(2))
					.AutoWidth()
					[
						SAssignNew(NodeVisibilityCheckbox, SCheckBox)
						.IsChecked(NodeVisibilityIsChecked_Attr)
						.OnCheckStateChanged(this, &SJointManagerViewer::OnNodeVisibilityCheckBoxCheckStateChanged)
						.BorderBackgroundColor(FLinearColor(1, 1, 1, 1))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("NodeVisibilityBoxText", "Node"))
							.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h5")
							.ColorAndOpacity(FLinearColor(1, 1, 1))
						]
						// sorry, but god's sake, I was really lazy to handle it more cleverly
					]
	
				]
			]
		]
		*/
		+ SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			Tree.ToSharedRef()
		]
	];
}

void SJointManagerViewer::RequestTreeRebuild()
{
	if (Tree)
	{
		Tree->BuildFromJointManagers();
	}
}

void PerformReplaceAction(const FString& ReplaceFrom, const FString& ReplaceTo, TSharedPtr<IJointTreeItem>& ItemRef,
                          int& OccurrenceCount, bool bShouldStopOnFirstOccurrence)
{
	//UE_LOG(LogTemp, Log, TEXT("%s"), *ItemRef->GetRowItemName().ToString());

	if (bShouldStopOnFirstOccurrence && OccurrenceCount > 0)
	{
		return;
	}

	for (TSharedPtr<IJointTreeItem> JointPropertyTreeItem : ItemRef->GetChildren())
	{
		PerformReplaceAction(ReplaceFrom, ReplaceTo, JointPropertyTreeItem, OccurrenceCount,
		                     bShouldStopOnFirstOccurrence);
	}

	//Check it again since it can be changed when the children iteration has been finished. 
	if (bShouldStopOnFirstOccurrence && OccurrenceCount > 0)
	{
		return;
	}

	if (ItemRef->IsOfType<FJointTreeItem_Property>())
	{
		TWeakPtr<FJointTreeItem_Property> CastedPtr = StaticCastSharedPtr<FJointTreeItem_Property>(ItemRef);

		if (CastedPtr.Pin()->Property != nullptr)
		{
			if (FStrProperty* StrProperty = CastField<FStrProperty>(CastedPtr.Pin()->Property))
			{
				const FString String = StrProperty->GetPropertyValue(
					StrProperty->ContainerPtrToValuePtr<void>(CastedPtr.Pin()->PropertyOuter));

				const uint32 Index = String.Find(ReplaceFrom);

				if (Index != INDEX_NONE)
				{
					const FString NewTextString = String.Left(Index) + ReplaceTo + String.Right(
						String.Len() - Index - ReplaceFrom.Len());

					StrProperty->SetPropertyValue(
						StrProperty->ContainerPtrToValuePtr<void>(CastedPtr.Pin()->PropertyOuter),
						NewTextString);

					++OccurrenceCount;
				}
			}
			else if (FNameProperty* NameProperty = CastField<FNameProperty>(CastedPtr.Pin()->Property))
			{
				const FString String = NameProperty->GetPropertyValue(
					NameProperty->ContainerPtrToValuePtr<void>(CastedPtr.Pin()->PropertyOuter)).ToString();

				const uint32 Index = String.Find(ReplaceFrom);

				if (Index != INDEX_NONE)
				{
					const FString NewTextString = String.Left(Index) + ReplaceTo + String.Right(
						String.Len() - Index - ReplaceFrom.Len());

					NameProperty->SetPropertyValue(
						NameProperty->ContainerPtrToValuePtr<void>(CastedPtr.Pin()->PropertyOuter),
						FName(NewTextString));

					++OccurrenceCount;
				}
			}
			else if (FTextProperty* TextProperty = CastField<FTextProperty>(CastedPtr.Pin()->Property))
			{
				const FText Text = TextProperty->GetPropertyValue(
					TextProperty->ContainerPtrToValuePtr<void>(CastedPtr.Pin()->PropertyOuter));

				const FString TextToString = Text.ToString();

				const uint32 Index = TextToString.Find(ReplaceFrom);

				if (Index != INDEX_NONE)
				{
					const FString NewTextString = TextToString.Left(Index) + ReplaceTo + TextToString.Right(
						TextToString.Len() - Index - ReplaceFrom.Len());

					FString OutKey, OutNamespace;

					const FString Namespace = FTextInspector::GetNamespace(Text).Get(FString());
					const FString Key = FTextInspector::GetKey(Text).Get(FString());

					FJointEdUtils::JointText_StaticStableTextIdWithObj(
						CastedPtr.Pin()->PropertyOuter,
						IEditableTextProperty::ETextPropertyEditAction::EditedSource,
						NewTextString,
						Namespace,
						Key,
						OutNamespace,
						OutKey);

					FText FinalText = FText::ChangeKey(FTextKey(OutNamespace), FTextKey(OutKey),
					                                   FText::FromString(NewTextString));


					TextProperty->SetPropertyValue(
						TextProperty->ContainerPtrToValuePtr<void>(CastedPtr.Pin()->PropertyOuter), FinalText);

					++OccurrenceCount;
				}
			}
		}
	}
}

void SJointManagerViewer::ReplaceNextSrc()
{
	int OccurrenceCount = 0;

	for (TSharedPtr<IJointTreeItem> JointPropertyTreeItem : Tree->FilteredItems)
	{
		PerformReplaceAction(ReplaceFromText.ToString(), ReplaceToText.ToString(), JointPropertyTreeItem,
		                     OccurrenceCount, true);
	}

	if (OccurrenceCount == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No occurrence founded."));
	}
}

void SJointManagerViewer::ReplaceAllSrc()
{
	int OccurrenceCount = 0;

	for (TSharedPtr<IJointTreeItem> JointPropertyTreeItem : Tree->FilteredItems)
	{
		PerformReplaceAction(ReplaceFromText.ToString(), ReplaceToText.ToString(), JointPropertyTreeItem,
		                     OccurrenceCount, false);
	}

	if (OccurrenceCount == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No occurrence founded."));
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok,
		                     FText::FromString(
			                     "Total " + FString::FromInt(OccurrenceCount) + " occurrences has been replaced."));
	}
}

EJointManagerViewerMode SJointManagerViewer::GetMode() const
{
	return Mode;
}

void SJointManagerViewer::OnNodeVisibilityCheckBoxCheckStateChanged(ECheckBoxState CheckBoxState)
{
	switch (CheckBoxState)
	{
	case ECheckBoxState::Unchecked:
		break;
	case ECheckBoxState::Checked:

		break;
	case ECheckBoxState::Undetermined:
		break;
	}
}


void SJointManagerViewer::OnFilterTextChanged(const FText& Text)
{
	SearchFilterText = Text;

	Tree->SetHighlightInlineFilterText(SearchFilterText);
	Tree->SetQueryInlineFilterText(FJointTreeFilter::ReplaceInqueryableCharacters(SearchFilterText));
}

void SJointManagerViewer::OnFilterTextCommitted(const FText& Text, ETextCommit::Type Arg)
{
	SearchFilterText = Text;
	
	Tree->SetHighlightInlineFilterText(SearchFilterText);
	Tree->SetQueryInlineFilterText(FJointTreeFilter::ReplaceInqueryableCharacters(SearchFilterText));
}

void SJointManagerViewer::OnFilterReplaceFromTextChanged(const FText& Text)
{
	ReplaceFromText = Text;
	
	Tree->SetHighlightInlineFilterText(ReplaceFromText);
	Tree->SetQueryInlineFilterText(FJointTreeFilter::ReplaceInqueryableCharacters(ReplaceFromText));
	
	Tree->ApplyFilter();
}

void SJointManagerViewer::OnFilterReplaceToTextChanged(const FText& Text)
{
	ReplaceToText = Text;

	Tree->ApplyFilter();
}

void SJointManagerViewer::SetMode(const EJointManagerViewerMode InMode)
{
	if(!(Tree && Tree->Filter)) return;

	Mode = InMode;

	switch (InMode)
	{
	case EJointManagerViewerMode::Search:

		if(TSharedPtr<FJointTreeFilterItem> Item = Tree->Filter->FindEqualItem(MakeShareable(new FJointTreeFilterItem("Tag:FName")))) Item->SetIsEnabled(false);
		if(TSharedPtr<FJointTreeFilterItem> Item = Tree->Filter->FindEqualItem(MakeShareable(new FJointTreeFilterItem("Tag:FString")))) Item->SetIsEnabled(false);
		if(TSharedPtr<FJointTreeFilterItem> Item = Tree->Filter->FindEqualItem(MakeShareable(new FJointTreeFilterItem("Tag:FText")))) Item->SetIsEnabled(false);
		
		if(SearchSearchBox.IsValid()) FSlateApplication::Get().SetKeyboardFocus( SearchSearchBox.ToSharedRef() );
		
		break;
		
	case EJointManagerViewerMode::Replace:
		
		if(TSharedPtr<FJointTreeFilterItem> Item = Tree->Filter->FindOrAddItem(MakeShareable(new FJointTreeFilterItem("Tag:FName")))) Item->SetIsEnabled(true);	
		if(TSharedPtr<FJointTreeFilterItem> Item = Tree->Filter->FindOrAddItem(MakeShareable(new FJointTreeFilterItem("Tag:FString")))) Item->SetIsEnabled(true);	
		if(TSharedPtr<FJointTreeFilterItem> Item = Tree->Filter->FindOrAddItem(MakeShareable(new FJointTreeFilterItem("Tag:FText")))) Item->SetIsEnabled(true);	
		
		if(ReplaceFromSearchBox.IsValid()) FSlateApplication::Get().SetKeyboardFocus( ReplaceFromSearchBox.ToSharedRef() );
		
		break;
	}
}

void SJointManagerViewer::SetTargetManager(TArray<UJointManager*> NewManagers)
{
	JointManagers = NewManagers;

	RebuildWidget();
}

#undef LOCTEXT_NAMESPACE
