//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Slate/SJointManagerViewer.h"

#include "JointAdvancedWidgets.h"
#include "JointEditorStyle.h"
#include "JointEditorToolkit.h"
#include "JointEdUtils.h"
#include "Filter/JointTreeFilter.h"
#include "Filter/JointTreeFilterItem.h"
#include "Framework/Application/SlateApplication.h"
#include "Item/JointTreeItem_Property.h"
#include "SearchTree/Builder/IJointTreeBuilder.h"
#include "Misc/MessageDialog.h"
#include "Styling/SlateStyleMacros.h"
#include "UObject/TextProperty.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"

#define LOCTEXT_NAMESPACE "JointPropertyList"


void SJointManagerViewer::Construct(const FArguments& InArgs)
{
	ToolKitPtr = InArgs._ToolKitPtr;
	JointManagers = InArgs._JointManagers;
	
	bShowOnlyCurrentGraph = InArgs._bInitialShowOnlyCurrentGraph;
	ShowOnlyCurrentGraphToggleButtonVisibilityAttr = InArgs._ShowOnlyCurrentGraphToggleButtonVisibility;

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

FJointPropertyTreeFilterArgs SJointManagerViewer::GetFilterArgs() const
{
	FJointPropertyTreeFilterArgs Args = FJointPropertyTreeFilterArgs();
	Args.TextFilter = Tree->TextFilterPtr;
	Args.bFlattenHierarchyOnFilter = false;

	if (bShowOnlyCurrentGraph && ToolKitPtr.IsValid())
	{
		if (UJointEdGraph* CurrentGraph = ToolKitPtr.Pin()->GetFocusedJointGraph())
		{
			Args.GraphsToShow.Add(CurrentGraph);
		}
	}

	return Args;
}

FJointPropertyTreeBuilderArgs SJointManagerViewer::GetBuilderArgs() const
{

	FJointPropertyTreeBuilderArgs Args = FJointPropertyTreeBuilderArgs();

	Args.bShowJointManagers = true;
	Args.bShowGraphs = true;
	Args.bShowNodes = true;
	Args.bShowProperties = true;

	return Args;
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
	
	return
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FJointEditorStyle::Margin_Normal)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[

			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.AutoWidth()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SJointOutlineButton)
				.ContentPadding(FJointEditorStyle::Margin_Normal)
				.OnPressed(this, &SJointManagerViewer::OnSearchModeButtonPressed)
				.IsEnabled(IsEnabled_Search_Attr)
				[
					SNew(SHorizontalBox)
					.Visibility(EVisibility::HitTestInvisible)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
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
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SearchBox", "Search"))
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h3")
						.ColorAndOpacity(FLinearColor(1, 1, 1))
					]
				]
			]
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.AutoWidth()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SJointOutlineButton)
				.ContentPadding(FJointEditorStyle::Margin_Normal)
				.OnPressed(this, &SJointManagerViewer::OnReplaceModeButtonPressed)
				.IsEnabled(IsEnabled_Replace_Attr)
				[
					SNew(SHorizontalBox)
					.Visibility(EVisibility::HitTestInvisible)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
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
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ReplaceBox", "Replace"))
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h3")
						.ColorAndOpacity(FLinearColor(1, 1, 1))
					]
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill)
		.Padding(FJointEditorStyle::Margin_Normal)
		[
			SNew(SHorizontalBox)
			.Visibility(ShowOnlyCurrentGraphToggleButtonVisibilityAttr)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("GraphDisplayTooltip", "Display Current Graph Only"))
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h4")
					.ColorAndOpacity(FLinearColor(1, 1, 1))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SJointOutlineToggleButton)
					.ToggleSize(8)
					.ButtonLength(10)
					.ToggleImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.RoundSolid")
					.IsChecked(this, &SJointManagerViewer::GetShowOnlyCurrentGraphState)
					.OnCheckStateChanged(this, &SJointManagerViewer::OnShowOnlyCurrentGraphChanged)
				]
		];
}


ECheckBoxState SJointManagerViewer::GetShowOnlyCurrentGraphState() const
{
	if (bShowOnlyCurrentGraph)
	{
		return ECheckBoxState::Checked;
	}
	else
	{
		return ECheckBoxState::Unchecked;
	}
}

void SJointManagerViewer::OnShowOnlyCurrentGraphChanged(ECheckBoxState CheckBoxState)
{
	switch (CheckBoxState)
	{
	case ECheckBoxState::Unchecked:
		bShowOnlyCurrentGraph = true;
		break;
	case ECheckBoxState::Checked:
		bShowOnlyCurrentGraph = false;
		break;
	case ECheckBoxState::Undetermined:
		break;
	}

	Tree->BuildFromJointManagers(JointManagers);

	//todo: change it to filtering
	//Builder->AbandonBuild();
	//BuildFromJointManagers();
}



void SJointManagerViewer::RebuildWidget()
{
	if (!Tree.IsValid())
	{
		SAssignNew(Tree, SJointTree)
		.BuilderArgs(this, &SJointManagerViewer::GetBuilderArgs)
		.FilterArgs(this, &SJointManagerViewer::GetFilterArgs);
	}
	
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
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.AutoHeight()
					[
						SAssignNew(ReplaceFromSearchBox, SSearchBox)
						.HintText(LOCTEXT("FilterReplaceFrom", "Replace from..."))
						.ToolTipText(LOCTEXT("FilterReplaceFromHint", "Type a text to search and replace from"))
						.OnTextChanged(this, &SJointManagerViewer::OnFilterReplaceFromTextChanged)
					]
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.AutoHeight()
					[
						SAssignNew(ReplaceToSearchBox, SSearchBox)
						.HintText(LOCTEXT("FilterReplaceTo", "Replace to..."))
						.ToolTipText(LOCTEXT("FilterReplaceToHint", "Type a text to search and replace to"))
						.OnTextChanged(this, &SJointManagerViewer::OnFilterReplaceToTextChanged)
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Top)
					.AutoHeight()
					[
						SNew(SJointOutlineButton)
						.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.RoundSolid")
						.ContentPadding(FJointEditorStyle::Margin_Normal)
						.OnPressed(this, &SJointManagerViewer::OnReplaceNextButtonPressed)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ReplaceNextBoxText", "Replace Next"))
							.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h4")
						]
					]
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Top)
					.AutoHeight()
					[
						SNew(SJointOutlineButton)
						.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.RoundSolid")
						.ContentPadding(FJointEditorStyle::Margin_Normal)
						.OnPressed(this, &SJointManagerViewer::OnReplaceAllButtonPressed)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ReplaceAllBoxText", "Replace All"))
							.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h4")
						]
					]
				]
			]
		]
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
		Tree->BuildFromJointManagers(JointManagers);
	}
}

void PerformReplaceAction(const FString& ReplaceFrom, const FString& ReplaceTo, TSharedPtr<IJointTreeItem>& ItemRef,
                          int& OccurrenceCount, bool bShouldStopOnFirstOccurrence, TSet<UObject*>& VisitedObjects)
{
	
	if (bShouldStopOnFirstOccurrence && OccurrenceCount > 0)
	{
		return;
	}

	for (TSharedPtr<IJointTreeItem> JointPropertyTreeItem : ItemRef->GetChildren())
	{
		PerformReplaceAction(ReplaceFrom, ReplaceTo, JointPropertyTreeItem, OccurrenceCount,
		                     bShouldStopOnFirstOccurrence, VisitedObjects);
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
			if(CastedPtr.Pin()->PropertyOuter.Get() != nullptr && !VisitedObjects.Contains(CastedPtr.Pin()->PropertyOuter.Get()))
			{
				VisitedObjects.Add(CastedPtr.Pin()->PropertyOuter.Get());
				CastedPtr.Pin()->PropertyOuter.Get()->Modify();
			}
			
			if (FStrProperty* StrProperty = CastField<FStrProperty>(CastedPtr.Pin()->Property))
			{
				const FString String = StrProperty->GetPropertyValue(
					StrProperty->ContainerPtrToValuePtr<void>(CastedPtr.Pin()->PropertyOuter.Get()));

				const uint32 Index = String.Find(ReplaceFrom);

				if (Index != INDEX_NONE)
				{
					const FString NewTextString = String.Left(Index) + ReplaceTo + String.Right(
						String.Len() - Index - ReplaceFrom.Len());

					StrProperty->SetPropertyValue(
						StrProperty->ContainerPtrToValuePtr<void>(CastedPtr.Pin()->PropertyOuter.Get()),
						NewTextString);

					++OccurrenceCount;
				}
			}
			else if (FNameProperty* NameProperty = CastField<FNameProperty>(CastedPtr.Pin()->Property))
			{
				const FString String = NameProperty->GetPropertyValue(
					NameProperty->ContainerPtrToValuePtr<void>(CastedPtr.Pin()->PropertyOuter.Get())).ToString();

				const uint32 Index = String.Find(ReplaceFrom);

				if (Index != INDEX_NONE)
				{
					const FString NewTextString = String.Left(Index) + ReplaceTo + String.Right(
						String.Len() - Index - ReplaceFrom.Len());

					NameProperty->SetPropertyValue(
						NameProperty->ContainerPtrToValuePtr<void>(CastedPtr.Pin()->PropertyOuter.Get()),
						FName(NewTextString));

					++OccurrenceCount;
				}
			}
			else if (FTextProperty* TextProperty = CastField<FTextProperty>(CastedPtr.Pin()->Property))
			{
				const FText Text = TextProperty->GetPropertyValue(
					TextProperty->ContainerPtrToValuePtr<void>(CastedPtr.Pin()->PropertyOuter.Get()));

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
						CastedPtr.Pin()->PropertyOuter.Get(),
						IEditableTextProperty::ETextPropertyEditAction::EditedSource,
						NewTextString,
						Namespace,
						Key,
						OutNamespace,
						OutKey);

					FText FinalText = FText::ChangeKey(FTextKey(OutNamespace), FTextKey(OutKey),
					                                   FText::FromString(NewTextString));


					TextProperty->SetPropertyValue(
						TextProperty->ContainerPtrToValuePtr<void>(CastedPtr.Pin()->PropertyOuter.Get()), FinalText);

					++OccurrenceCount;
				}
			}
		}
	}
}

void SJointManagerViewer::ReplaceNextSrc()
{
	int OccurrenceCount = 0;

	int32 TransactionID = GEditor->BeginTransaction(
		FText::Format(NSLOCTEXT("JointEdTransaction", "TransactionTitle_ReplaceNext", "Replace Next Text From \'{0}\' To \'{1}\'"),
			FText::FromString(ReplaceFromText.ToString()),
			FText::FromString(ReplaceToText.ToString())));

	TSet<UObject*> VisitedObjects;
	
	for (TSharedPtr<IJointTreeItem> JointPropertyTreeItem : Tree->FilteredItems)
	{
		PerformReplaceAction(ReplaceFromText.ToString(), ReplaceToText.ToString(), JointPropertyTreeItem,
		                     OccurrenceCount, true ,VisitedObjects);
	}
	
	if (OccurrenceCount == 0)
	{
		GEditor->CancelTransaction(TransactionID);

		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No occurrence founded."));

	}else
	{
		GEditor->EndTransaction();
	}
}

void SJointManagerViewer::ReplaceAllSrc()
{
	int OccurrenceCount = 0;

	int32 TransactionID = GEditor->BeginTransaction(
	FText::Format(NSLOCTEXT("JointEdTransaction", "TransactionTitle_ReplaceAll", "Replace All Text From \'{0}\' To \'{1}\'"),
		FText::FromString(ReplaceFromText.ToString()),
		FText::FromString(ReplaceToText.ToString())));

	TSet<UObject*> VisitedObjects;

	for (TSharedPtr<IJointTreeItem> JointPropertyTreeItem : Tree->FilteredItems)
	{
		PerformReplaceAction(ReplaceFromText.ToString(), ReplaceToText.ToString(), JointPropertyTreeItem,
		                     OccurrenceCount, false,VisitedObjects);
	}

	if (OccurrenceCount == 0)
	{

		GEditor->CancelTransaction(TransactionID);
		
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No occurrence founded."));
	}
	else
	{
		GEditor->EndTransaction();
		
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
	if (!(Tree && Tree->Filter)) return;

	Mode = InMode;

	switch (InMode)
	{
	case EJointManagerViewerMode::Search:

		if (TSharedPtr<FJointTreeFilterItem> Item = Tree->Filter->FindEqualItem(
			MakeShareable(new FJointTreeFilterItem("Tag:FName"))))
			Item->SetIsEnabled(false);
		if (TSharedPtr<FJointTreeFilterItem> Item = Tree->Filter->FindEqualItem(
			MakeShareable(new FJointTreeFilterItem("Tag:FString"))))
			Item->SetIsEnabled(false);
		if (TSharedPtr<FJointTreeFilterItem> Item = Tree->Filter->FindEqualItem(
			MakeShareable(new FJointTreeFilterItem("Tag:FText"))))
			Item->SetIsEnabled(false);

		if (SearchSearchBox.IsValid()) FSlateApplication::Get().SetKeyboardFocus(SearchSearchBox.ToSharedRef());

		break;

	case EJointManagerViewerMode::Replace:

		if (TSharedPtr<FJointTreeFilterItem> Item = Tree->Filter->FindOrAddItem(
			MakeShareable(new FJointTreeFilterItem("Tag:FName"))))
			Item->SetIsEnabled(true);
		if (TSharedPtr<FJointTreeFilterItem> Item = Tree->Filter->FindOrAddItem(
			MakeShareable(new FJointTreeFilterItem("Tag:FString"))))
			Item->SetIsEnabled(true);
		if (TSharedPtr<FJointTreeFilterItem> Item = Tree->Filter->FindOrAddItem(
			MakeShareable(new FJointTreeFilterItem("Tag:FText"))))
			Item->SetIsEnabled(true);

		if (ReplaceFromSearchBox.IsValid())
			FSlateApplication::Get().SetKeyboardFocus(
				ReplaceFromSearchBox.ToSharedRef());

		break;
	}
}

void SJointManagerViewer::SetTargetManager(TArray<TWeakObjectPtr<UJointManager>> NewManagers)
{
	JointManagers = NewManagers;
}

#undef LOCTEXT_NAMESPACE
