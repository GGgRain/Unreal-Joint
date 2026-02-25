//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "EditorWidget/SJointManagerImportingPopup.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "JointAdvancedWidgets.h"
#include "JointEditorStyle.h"
#include "JointEdUtils.h"
#include "JointScriptParser.h"
#include "PropertyCustomizationHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Markdown/SJointMDSlate_Admonitions.h"
#include "Widgets/Input/SSegmentedControl.h"

#define LOCTEXT_NAMESPACE "SJointManagerImportingPopup"

SJointManagerImportingPopup::FParserCandidateItem::FParserCandidateItem()
{
	
}

SJointManagerImportingPopup::SParserCandidateRow::FArguments::FArguments()
{
}

void SJointManagerImportingPopup::SParserCandidateRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
{
	Item = InArgs._Item;

	SMultiColumnTableRow<TSharedPtr<FParserCandidateItem>>::Construct(
		FSuperRowType::FArguments()
		.Padding(FJointEditorStyle::Margin_Normal)
		, OwnerTable
	);
}

TSharedRef<SWidget> SJointManagerImportingPopup::SParserCandidateRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == "Name")
	{
		return SNew(STextBlock).Text(FText::FromString(Item->Name));
	}
	else if (ColumnName == "Path")
	{
		return SNew(STextBlock).Text(FText::FromString(Item->Path));
	}

	return SNew(STextBlock).Text(FText::FromString(TEXT("Unknown Column")));
}

SJointManagerImportingPopup::SJointManagerImportingPopup()
{
}

void SJointManagerImportingPopup::ConstructParserCandidateItems()
{
	ParserCandidateItems.Empty();
	
	// Gather all the parsers in the project, and find out which one can import the file.
	
	if (FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
	{
		FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		FARFilter Filter;
#if UE_VERSION_OLDER_THAN(5, 1, 0)
		Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
#else
		Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
#endif
		
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> AssetDataList;
		AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

		for (const FAssetData& AD : AssetDataList)
		{
			if (!AD.GetClass()->IsChildOf(UBlueprint::StaticClass()))
			{
				continue;
			}
			
			UBlueprint* Blueprint = Cast<UBlueprint>(AD.GetAsset());
			if (!Blueprint || !Blueprint->GeneratedClass || !Blueprint->GeneratedClass->IsChildOf(UJointScriptParser::StaticClass()))
			{
				continue;
			}

			//Make a new object from the parser class and store it for the candidate list.
			//We need to use an object that is not CDO to avoid the default properties are overridden. (we will save the serialized properties of the parser and use them for reimporting)
			UJointScriptParser* Parser = NewObject<UJointScriptParser>(GetTransientPackage(), Blueprint->GeneratedClass);
			
			// Add to candidate list			
			TSharedPtr<FParserCandidateItem> NewItem = MakeShared<FParserCandidateItem>();
			NewItem->Parser = Parser;
			NewItem->Name = AD.AssetName.ToString();
#if UE_VERSION_OLDER_THAN(5, 1, 0)
			NewItem->Path = AD.ObjectPath.ToString();
#else
			NewItem->Path = AD.GetSoftObjectPath().ToString();
#endif
				
			ParserCandidateItems.Add(NewItem);
		}
	}
}

void SJointManagerImportingPopup::ConstructLayout()
{
	FContentBrowserModule& ContentBrowserModule =FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.DefaultPath = SelectedContentPath;
	PathPickerConfig.bAllowContextMenu = true;
	PathPickerConfig.bAllowClassesFolder = false;
	PathPickerConfig.bAllowReadOnlyFolders = false;

	// 경로 변경 시 콜백
	PathPickerConfig.OnPathSelected = FOnPathSelected::CreateLambda(
		[this](const FString& NewPath)
		{
			SelectedContentPath = NewPath;
		}
	);

	TSharedRef<SWidget> PathPicker =
		ContentBrowserModule.Get().CreatePathPicker(PathPickerConfig);
	
	
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FJointEditorStyle::Margin_Normal)
		[
			SNew(STextBlock)
			.Text( bIsReimporting ? LOCTEXT("ReimportingTitle", "Reimport Joint Manager") : LOCTEXT("ImportingTitle", "Import Joint Manager") )
			.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h3")
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FJointEditorStyle::Margin_Normal)
		[
			SNew(SJointOutlineBorder)
			.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OutlineNormalColor(FLinearColor(0.04, 0.04, 0.04))
			.OutlineHoverColor(FJointEditorStyle::Color_Selected)
			.ContentPadding(FJointEditorStyle::Margin_Large)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Files: ")))
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(STextBlock)
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
					.ColorAndOpacity(FLinearColor(0.6, 0.6, 0.6))
					.Text_Lambda([this]()
					{
						return ExternalFilePaths.Num() > 0
							? FText::FromString(FString::Join(ExternalFilePaths, TEXT("\n")))
							: LOCTEXT("NoFileSelected", "No file selected");
					})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SJointOutlineButton)
					.IsEnabled(!bIsReimporting)
					.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
					.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.OutlineNormalColor(FLinearColor::Transparent)
					.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
					.ContentPadding(FJointEditorStyle::Margin_Normal)
					.ToolTipText(LOCTEXT("SelectFileButtonTooltip", "Select a file to import"))
					.OnClicked(this, &SJointManagerImportingPopup::OnSelectFileButtonClicked)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SelectFileButtonLabel", "Select..."))
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h3")
					]
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FJointEditorStyle::Margin_Normal)
		[
			SNew(SSegmentedControl<EJointImportMode>)
			.IsEnabled(!bIsReimporting)
			.OnValueChanged(this, &SJointManagerImportingPopup::OnImportModeChanged)
			.Value(this, &SJointManagerImportingPopup::GetCurrentImportMode)
			+ SSegmentedControl<EJointImportMode>::Slot(EJointImportMode::AsIndividualJointManagers)
			.Text(LOCTEXT("ImportMode_AsIndividual", "As New Individual Joint Managers"))
			.Icon(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("LevelEditor.Tabs.Layers") )
			+ SSegmentedControl<EJointImportMode>::Slot(EJointImportMode::ToSpecifiedJointManager)
			.Text(LOCTEXT("ImportMode_ToSpecified", "To Specified Joint Manager"))
			.Icon(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("LevelEditor.Tabs.Details") )
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FJointEditorStyle::Margin_Normal)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda([this]()
			{
				return CurrentImportMode == EJointImportMode::ToSpecifiedJointManager ? EVisibility::Visible : EVisibility::Collapsed;
			})
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("OptionalJointManagerLabel", "Joint Manager: "))
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
			]
			+ SHorizontalBox::Slot() // asset picker for OptionalJointManagerToImport
			.FillWidth(1)
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SObjectPropertyEntryBox)
				.IsEnabled(!bIsReimporting)
				.AllowedClass(UJointManager::StaticClass())
				.ObjectPath(this, &SJointManagerImportingPopup::GetOptionalJointManagerPath)
				.OnObjectChanged(this, &SJointManagerImportingPopup::OnOptionalJointManagerChanged)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FJointEditorStyle::Margin_Normal)
		[
			SNew(SVerticalBox)
			.Visibility_Lambda([this]()
			{
				return CurrentImportMode == EJointImportMode::AsIndividualJointManagers ? EVisibility::Visible : EVisibility::Collapsed;
			})
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("LocationForNewJointManagersLabel", "Location for new Joint Managers: "))
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				PathPicker
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FJointEditorStyle::Margin_Normal)
		[
			SNew(SBox)
			.MinDesiredHeight(100)
			[
				SAssignNew(ParserListView, SListView<TSharedPtr<FParserCandidateItem>>)
				.IsEnabled(!bIsReimporting)
				.ListItemsSource(&ParserCandidateItems)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow_Lambda([](TSharedPtr<FParserCandidateItem> InItem, const TSharedRef<STableViewBase>& Owner)
				{
					return SNew(SParserCandidateRow, Owner).Item(InItem);
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<FParserCandidateItem> NewSelection, ESelectInfo::Type)
				{
					SelectedParserItem = NewSelection;
					UpdateParserSettingsWidget();
					UpdateInfoWidget();
				})
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column("Name")
					.DefaultLabel(LOCTEXT("Column_Name", "Name"))
					.FillWidth(0.3f)
					.OnSort(this, &SJointManagerImportingPopup::OnColumnSort)
					.SortMode(
						TAttribute<EColumnSortMode::Type>::Create(
							TAttribute<EColumnSortMode::Type>::FGetter::CreateSP(
								this,
								&SJointManagerImportingPopup::GetColumnSortMode,
								FName("Name")
							)
						)
					)
					+ SHeaderRow::Column("Path")
					.DefaultLabel(LOCTEXT("Column_Path", "Path"))
					.FillWidth(0.5f)
					.OnSort(this, &SJointManagerImportingPopup::OnColumnSort)
					.SortMode(
						TAttribute<EColumnSortMode::Type>::Create(
							TAttribute<EColumnSortMode::Type>::FGetter::CreateSP(
								this,
								&SJointManagerImportingPopup::GetColumnSortMode,
								FName("Path")
							)
						)
					)
				)
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1)
		.VAlign(VAlign_Bottom)
		.HAlign(HAlign_Fill)
		.Padding(FJointEditorStyle::Margin_Normal)
		[
			SAssignNew(ParserSettingsBox, SBox)
			[
				CreateParserSettingsWidget()
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Bottom)
		.HAlign(HAlign_Fill)
		.Padding(FJointEditorStyle::Margin_Normal)
		[
			SAssignNew(InfoBox, SBox)
			[
				CreateInfoWidget()
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.Padding(FJointEditorStyle::Margin_Normal)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SButton)
				.Text(LOCTEXT("Cancel", "Cancel"))
				.OnClicked_Lambda([this]() -> FReply
				{
					if (ParentWindow.IsValid()) ParentWindow.Pin()->RequestDestroyWindow();
					
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SButton)
				.Text(LOCTEXT("Proceed", "Proceed"))
				.ButtonColorAndOpacity(FLinearColor(0.5, 0.7, 1.0))
				.IsEnabled_Lambda([this]() -> bool
				{
					return CurrentImportMode == EJointImportMode::ToSpecifiedJointManager
						? OptionalJointManagerToImport.IsValid() && SelectedParserItem.IsValid() && ExternalFilePaths.Num() > 0
						: SelectedParserItem.IsValid() && ExternalFilePaths.Num() > 0;
				})
				.OnClicked(this, &SJointManagerImportingPopup::OnProceedButtonClicked)
			]
		]
	];
}

void SJointManagerImportingPopup::Construct(const FArguments& InArgs)
{
	bIsReimporting = InArgs._bIsReimporting;
	OptionalJointManagerToImport = InArgs._OptionalJointManagerToImport;
	ParentWindow = InArgs._ParentWindow;
	ExternalFilePaths = InArgs._ExternalFilePaths;
	CurrentImportMode = InArgs._InitialImportMode;
	
	ConstructParserCandidateItems();
	
	if ( InArgs._InitiallySelectedParserClass )
	{
		for (TSharedPtr<FParserCandidateItem>& ParserCandidateItem : ParserCandidateItems)
		{
			if (ParserCandidateItem->Parser->GetClass() == InArgs._InitiallySelectedParserClass)
			{
				if (!InArgs._InitiallySelectedParserInstanceData.IsEmpty())
				{
					// if there is initial parser class specified, try to find it from the candidate list and select it.
					FJointScriptLinkerParserData::DeserializeParserInstanceFromDataToExistingParser(ParserCandidateItem->Parser, InArgs._InitiallySelectedParserInstanceData);
				}

				SelectedParserItem = ParserCandidateItem;
				break;
			}
		}
	}
	
	// We have to ask:
	// 1. to select the file to import.
	// 2. to select OptionalJointManagerToImport, if user want.
	// 3. show the list of the parsers that can import the file, and ask user to select one of them.
	// 4. show the import option of the selected parser, and ask user to confirm the import.
	
	ConstructLayout();
	
	if (SelectedParserItem.IsValid())
	{
		if (ParserListView) ParserListView->SetSelection(SelectedParserItem.Pin());
	}
}

FString SJointManagerImportingPopup::GetOptionalJointManagerPath() const
{
	return OptionalJointManagerToImport.IsValid() ? OptionalJointManagerToImport.ToString() : FString("");
}

void SJointManagerImportingPopup::OnOptionalJointManagerChanged(const FAssetData& AssetData)
{
	OptionalJointManagerToImport = AssetData.ToSoftObjectPath();
	UpdateInfoWidget(); 
}

void SJointManagerImportingPopup::UpdateParserSettingsWidget()
{
	if (ParserSettingsBox.IsValid())
	{
		ParserSettingsBox->SetContent(
			CreateParserSettingsWidget()
		);
	}
}

void SJointManagerImportingPopup::UpdateInfoWidget()
{
	if (InfoBox.IsValid())
	{
		InfoBox->SetContent(
			CreateInfoWidget()
		);
	}
}

TSharedRef<SWidget> SJointManagerImportingPopup::CreateParserSettingsWidget()
{
	if (SelectedParserItem.IsValid())
	{
		UJointScriptParser* Parser = SelectedParserItem.Pin()->Parser.Get();
		
		if (Parser)
		{
			// iterate properties mentioned in Parser->OptionPropertyNames, and create a details panel for them.
			
			TArray<FProperty*> PropertiesToShow;
			TArray<FName> PropertyNamesToShow;
			
			// get SaveGame properties
			for (TFieldIterator<FProperty> PropIt(Parser->GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
			{
				FProperty* Property = *PropIt;
				if (Property->HasAnyPropertyFlags(CPF_SaveGame))
				{
					PropertiesToShow.Add(Property);
				}
			}
			
			if (PropertiesToShow.Num() == 0)
			{
				return SNew(SJointMDSlate_Admonitions)
					.AdmonitionType(EJointMDAdmonitionType::Info)
					.bUseDescriptionText(true)
					.DescriptionText(LOCTEXT("NoOptions", "This parser has no customizable options. (If you are a developer and can't find the options you added for your parser, make sure to mark your properties for the options as 'SaveGame (IsSaveGame for Blueprint properties)' in your parser class.)"));
			}
			
			for (FProperty*& ToShow : PropertiesToShow)
			{
				PropertyNamesToShow.Add(ToShow->GetFName());
				ToShow = nullptr; // set to nullptr to avoid showing them in details panel defaultly. we will show them with custom filter.
			}
			
			FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(
			"PropertyEditor");

			FDetailsViewArgs DetailsViewArgs;
			DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ENameAreaSettings::HideNameArea;
			DetailsViewArgs.bForceHiddenPropertyVisibility = false;
			DetailsViewArgs.bHideSelectionTip = true;
			DetailsViewArgs.bAllowSearch = true;
			DetailsViewArgs.bShowScrollBar = true;
			//DetailsViewArgs.ViewIdentifier = FName(FGuid::NewGuid().ToString());
		
			
			TSharedPtr<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
			DetailsView->SetIsPropertyVisibleDelegate(
				FIsPropertyVisible::CreateLambda([PropertyNamesToShow](const FPropertyAndParent& PropertyAndParent)
				{
					return PropertyNamesToShow.Contains(PropertyAndParent.Property.GetFName());
				})
			);
			DetailsView->SetObject(Parser);
			
			return DetailsView.ToSharedRef();
		}
	}
	
	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SJointManagerImportingPopup::CreateInfoWidget()
{
	// 1. if there is no parser selected, show a message asking user to select a parser.

	if (!SelectedParserItem.IsValid())
	{
		return SNew(SJointMDSlate_Admonitions)
			.AdmonitionType(EJointMDAdmonitionType::Note)
			.CustomHeaderText(LOCTEXT("ParserHeader", "No parser selected"))
			.bUseDescriptionText(true)
			.DescriptionText(LOCTEXT("ParserDescription", "Select a parser from the list above."));
	}
	
	// 2. if no file is selected, show a message asking user to select a file.
	
	if (ExternalFilePaths.Num() == 0)
	{
		return SNew(SJointMDSlate_Admonitions)
			.AdmonitionType(EJointMDAdmonitionType::Note)
			.CustomHeaderText(LOCTEXT("NoFileSelectedHeader", "No file selected"))
			.bUseDescriptionText(true)
			.DescriptionText(LOCTEXT("NoFileSelectedDescription", "Select a file to import by clicking the 'Select...' button above."));
	}
	
	// 3 - 1. (AsIndividualJointManagers mode) if OptionalJointManagerToImport is not valid, show a message asking user to select a Joint Manager to import to.
	// 3 - 2. (ToSpecifiedJointManager mode) if there is no valid path, show a message asking user to select a location for the new Joint Managers.
	
	
	if (CurrentImportMode == EJointImportMode::ToSpecifiedJointManager && !OptionalJointManagerToImport.IsValid())
	{
		return SNew(SJointMDSlate_Admonitions)
			.AdmonitionType(EJointMDAdmonitionType::Note)
			.CustomHeaderText(LOCTEXT("NoTargetManagerHeader", "No target Joint Manager selected"))
			.bUseDescriptionText(true)
			.DescriptionText(LOCTEXT("NoTargetManagerDescription", "Select a target Joint Manager to import to by clicking the box next to 'Joint Manager' label above."));
	}else if (CurrentImportMode == EJointImportMode::AsIndividualJointManagers && SelectedContentPath.IsEmpty())
	{
		return SNew(SJointMDSlate_Admonitions)
			.AdmonitionType(EJointMDAdmonitionType::Note)
			.CustomHeaderText(LOCTEXT("NoTargetLocationHeader", "No target location selected"))
			.bUseDescriptionText(true)
			.DescriptionText(LOCTEXT("NoTargetLocationDescription", "Select a target location for the new Joint Managers by clicking the box next to 'Location for new Joint Managers' label above."));
	}
	
	// 4. if every condition is satisfied, don't show any message.
	return SNullWidget::NullWidget;
}

SJointManagerImportingPopup::EJointImportMode SJointManagerImportingPopup::GetCurrentImportMode() const
{
	return CurrentImportMode;
}

void SJointManagerImportingPopup::OnImportModeChanged(EJointImportMode JointImportMode)
{
	CurrentImportMode = JointImportMode;
	UpdateInfoWidget();
}

void SJointManagerImportingPopup::OnColumnSort(
	EColumnSortPriority::Type,
	const FName& ColumnId,
	EColumnSortMode::Type SortMode)
{
	ParserCurrentSortColumn = ColumnId;
	ParserCurrentSortMode = SortMode;

	ParserCandidateItems.Sort([&](const TSharedPtr<FParserCandidateItem>& A, const TSharedPtr<FParserCandidateItem>& B)
	{
		int32 Result = 0;

		if (ColumnId == "Name")
			Result = A->Name.Compare(B->Name);
		else if (ColumnId == "Path")
			Result = A->Path.Compare(B->Path);

		return SortMode == EColumnSortMode::Ascending ? Result < 0 : Result > 0;
	});

	if (ParserListView.IsValid())
	{
		ParserListView->RequestListRefresh();
	}
}

EColumnSortMode::Type SJointManagerImportingPopup::GetColumnSortMode(FName ColumnId) const
{
	return (ParserCurrentSortColumn == ColumnId) ? ParserCurrentSortMode : EColumnSortMode::None;
}

FReply SJointManagerImportingPopup::OnSelectFileButtonClicked()
{
	
	//1. Open file dialog to select the file
	TArray<FString> OutFiles;
	
	FJointEdUtils::OpenJointScriptFileSelectionWindow(OutFiles, true);
	
	ExternalFilePaths = OutFiles;
	
	UpdateInfoWidget();
	
	return FReply::Handled();
}

FReply SJointManagerImportingPopup::OnProceedButtonClicked()
{
	
	if (CurrentImportMode == EJointImportMode::ToSpecifiedJointManager)
	{
		UObject* LoadedObject = OptionalJointManagerToImport.TryLoad();
		UJointManager* TargetManager = Cast<UJointManager>(LoadedObject);
		
		FJointEdUtils::ImportFileToJointManager(
			TargetManager,
			ExternalFilePaths.Num() > 0 ? ExternalFilePaths[0] : FString(""),
			SelectedParserItem.Pin()->Parser.Get()
		);
	}
	else if (CurrentImportMode == EJointImportMode::AsIndividualJointManagers)
	{
		for (const FString& FilePath : ExternalFilePaths)
		{
			// Create a new Joint Manager asset for each file, and import the file to it.
			
			UJointManager* DM = FJointEdUtils::CreateNewAssetForClass<UJointManager>(UJointManager::StaticClass(), SelectedContentPath);
			if (!DM) continue;
			
			FJointEdUtils::ImportFileToJointManager(
				DM,
				FilePath,
				SelectedParserItem.Pin()->Parser.Get()
			);
			
			DM->MarkPackageDirty();
		}
	}
	

	if (ParentWindow.IsValid()) ParentWindow.Pin()->RequestDestroyWindow();
	
	return FReply::Handled();
}
#undef LOCTEXT_NAMESPACE
