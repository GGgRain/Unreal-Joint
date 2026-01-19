//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointManagementTabs.h"

#include "ISettingsEditorModule.h"
#include "JointAdvancedWidgets.h"

#include "JointEdGraph.h"
#include "JointEditor.h"
#include "JointEditorLogChannels.h"
#include "JointEditorSettings.h"
#include "JointEditorStyle.h"

#include "JointManager.h"
#include "PropertyCustomizationHelpers.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/MessageDialog.h"
#include "UObject/CoreRedirects.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Images/SImage.h"

#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "JointManagementTab"


FJointManagementTab_JointEditorUtilityTab::FJointManagementTab_JointEditorUtilityTab() : IJointManagementSubTab()
{
}

FJointManagementTab_JointEditorUtilityTab::~FJointManagementTab_JointEditorUtilityTab()
{
}

TSharedRef<IJointManagementSubTab> FJointManagementTab_JointEditorUtilityTab::MakeInstance()
{
	return MakeShareable(new FJointManagementTab_JointEditorUtilityTab);
}

void FJointManagementTab_JointEditorUtilityTab::RegisterTabSpawner(const TSharedPtr<FTabManager>& TabManager)
{
	TSharedPtr<FWorkspaceItem> JointEditorGroup = GetParentTabHandler().Pin()->GetActiveGroupFor("JointEditor");

	if (!JointEditorGroup)
	{
		JointEditorGroup = TabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("JointEditorGroupName", "Joint Editor"));

		GetParentTabHandler().Pin()->AddActiveGroup("JointEditor", JointEditorGroup);
	}

	TabManager->RegisterTabSpawner(
			GetTabId()
			, FOnSpawnTab::CreateLambda(
				[=](const FSpawnTabArgs&)
				{
					return SNew(SDockTab)
						.TabRole(ETabRole::PanelTab)
						.Label(LOCTEXT("EditorUtility", "Editor Utility"))
						[
							SNew(SJointEditorUtilityTab)
						];
				}
			)
		)
		.SetDisplayName(LOCTEXT("EditorUtilityTabTitle", "Editor Utility"))
		.SetTooltipText(LOCTEXT("EditorUtilityTooltipText", "Open the Editor Utility tab."))
		.SetGroup(JointEditorGroup.ToSharedRef())
		.SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(),
		                    "ExternalImagePicker.GenerateImageButton"));
}

const FName FJointManagementTab_JointEditorUtilityTab::GetTabId()
{
	return "TAB_JointEditorUtility";
}

const ETabState::Type FJointManagementTab_JointEditorUtilityTab::GetInitialTabState()
{
	return IJointManagementSubTab::GetInitialTabState();
}

#if UE_VERSION_OLDER_THAN(5, 1, 0)

#include "AssetRegistryModule.h"
#include "ARFilter.h"

#else

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"


#endif


TSharedRef<SWidget> SJointEditorUtilityTab::CreateProductSection()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FJointEditorStyle::Margin_Normal)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FJointEditorStyle::Margin_Normal)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(156)
				.HeightOverride(74)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::Get().GetBrush("JointUI.Image.Joint"))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FJointEditorStyle::Margin_Normal)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(166)
				.HeightOverride(66)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::Get().GetBrush("JointUI.Image.Volt"))
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FJointEditorStyle::Margin_Normal)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h5")
			.Text(LOCTEXT(
				"DescText", "Joint, the conversation scripting plugin & Volt, the slate framework animation library"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FJointEditorStyle::Margin_Normal)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h5")
			.Text(LOCTEXT("CopyrightText", "Copyright 2022~2025 DevGrain. All Rights Reserved for Both Modules."))
		];
}

void SJointEditorUtilityTab::Construct(const FArguments& InArgs)
{
	ChildSlot.DetachWidget();

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Image.GraphBackground"))
		.BorderBackgroundColor(FJointEditorStyle::Color_Node_TabBackground)
		.Padding(FJointEditorStyle::Margin_Normal)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			.Padding(FJointEditorStyle::Margin_Normal)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[
				CreateProductSection()
			]
			+ SScrollBox::Slot()
			.Padding(FJointEditorStyle::Margin_Normal)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			[
				CreateInvalidateSection()
			]
			+ SScrollBox::Slot()
			.Padding(FJointEditorStyle::Margin_Normal)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			[
				CreateGraphSection()
			]
		]
	];
}

TSharedRef<SWidget> SJointEditorUtilityTab::CreateInvalidateSection()
{
	return SNew(SJointOutlineBorder)
		.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.NormalColor(FLinearColor(0.015, 0.015, 0.02))
		.HoverColor(FLinearColor(0.04, 0.04, 0.06))
		.OutlineNormalColor(FLinearColor(0.015, 0.015, 0.02))
		.OutlineHoverColor(FLinearColor(0.5, 0.5, 0.5))
		.ContentPadding(FJointEditorStyle::Margin_Normal)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
				.Text(LOCTEXT("DeveloperTip", "Developer Mode"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DeveloperModeExp",
					              "Enable / Disable Developer Mode - Show some additional data on the detail tabs and slates."))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineToggleButton)
					.IsChecked(this, &SJointEditorUtilityTab::GetIsDeveloperModeChecked)
					.OnCheckStateChanged(this, &SJointEditorUtilityTab::OnDeveloperModeToggled)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
				.Text(LOCTEXT("QuickFixStyleTip", "Quick-Fix"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ReconstructText", "Reconstruct Every Node In Opened Joint Manager Editor"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineButton)
					.ContentPadding(FJointEditorStyle::Margin_Large)
					.OnClicked(this, &SJointEditorUtilityTab::ReconstructEveryNodeInOpenedJointManagerEditor)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ReconstructButton", "Reconstruct"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CleanUpNodes",
					              "Clean-up orphened nodes in the projects. This doesn't need to be done twice, it's for the assets from old versions (before 2.8)"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineButton)
					.ContentPadding(FJointEditorStyle::Margin_Large)
					.OnClicked(this, &SJointEditorUtilityTab::CleanUpUnnecessaryNodes)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CleanUpButton", "CleanUp"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h2")
				.Text(LOCTEXT("2.9Tooltip", "Joint 2.9"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("UpdateEdSettingsNodes",
					              "Move the old editor node setting related properties to the new structure. This doesn't need to be done twice, it's for the assets from old versions. (before 2.9)"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineButton)
					.ContentPadding(FJointEditorStyle::Margin_Large)
					.OnClicked(this, &SJointEditorUtilityTab::UpdateBPNodeEdSettings)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("UpdateEdSettingsButton", "Update"))
					]
				]
			]
		];
}

TSharedRef<SWidget> SJointEditorUtilityTab::CreateGraphSection()
{
	return SNew(SJointOutlineBorder)
		.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.NormalColor(FLinearColor(0.015, 0.015, 0.02))
		.HoverColor(FLinearColor(0.04, 0.04, 0.06))
		.OutlineNormalColor(FLinearColor(0.015, 0.015, 0.02))
		.OutlineHoverColor(FLinearColor(0.5, 0.5, 0.5))
		.ContentPadding(FJointEditorStyle::Margin_Normal)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
				.Text(LOCTEXT("EditorStyleTip", "Editor Style"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ResetAllStyleText", "Reset All Styles to default"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineButton)
					.ContentPadding(FJointEditorStyle::Margin_Large)
					.OnClicked(this, &SJointEditorUtilityTab::ResetAllEditorStyle)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ResetButtonText", "Reset"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h2")
				.Text(LOCTEXT("GraphEditorStyleTip", "Graph Editor Style"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ResetGraphEditorBackgroundStyleText",
					              "Reset Graph Editor Background Style (Grid, Color) to default"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineButton)
					.ContentPadding(FJointEditorStyle::Margin_Large)
					.OnClicked(this, &SJointEditorUtilityTab::ResetGraphEditorStyle)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ResetButtonText", "Reset"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ResetGraphEditorBackgroundStyleText",
					              "Reset Graph Editor Pin & Connection Style to default"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineButton)
					.ContentPadding(FJointEditorStyle::Margin_Large)
					.OnClicked(this, &SJointEditorUtilityTab::ResetPinConnectionEditorStyle)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ResetButtonText", "Reset"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ResetGraphEditorBackgroundStyleText",
					              "Reset Graph Editor Pin & Connection Style to default"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineButton)
					.ContentPadding(FJointEditorStyle::Margin_Large)
					.OnClicked(this, &SJointEditorUtilityTab::ResetPinConnectionEditorStyle)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ResetButtonText", "Reset"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ResetDebuggerStyleText",
					              "Reset Graph Editor Debugger Style (Color for playback states) to default"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineButton)
					.ContentPadding(FJointEditorStyle::Margin_Large)
					.OnClicked(this, &SJointEditorUtilityTab::ResetDebuggerEditorStyle)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ResetButtonText", "Reset"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ResetNodeStyleText", "Reset Graph Editor Node Style (Color) to default"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineButton)
					.ContentPadding(FJointEditorStyle::Margin_Large)
					.OnClicked(this, &SJointEditorUtilityTab::ResetNodeEditorStyle)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ResetButtonText", "Reset"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h2")
				.Text(LOCTEXT("ContextTextEditorStyleTip", "Context Text Style"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ResetContextTextEditorStyleText", "Reset Context Text Editor Style to default"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineButton)
					.ContentPadding(FJointEditorStyle::Margin_Large)
					.OnClicked(this, &SJointEditorUtilityTab::ResetContextTextEditorStyle)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ResetButtonText", "Reset"))
					]
				]
			]
		];
}

ECheckBoxState SJointEditorUtilityTab::GetIsDeveloperModeChecked() const
{
	if (UJointEditorSettings* Settings = UJointEditorSettings::Get())
	{
		return Settings->bEnableDeveloperMode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	return ECheckBoxState::Undetermined;
}

void SJointEditorUtilityTab::OnDeveloperModeToggled(ECheckBoxState CheckBoxState)
{
	switch (CheckBoxState)
	{
	case ECheckBoxState::Unchecked:
		if (UJointEditorSettings* Settings = UJointEditorSettings::Get())
		{
			Settings->bEnableDeveloperMode = true;
		}
		break;
	case ECheckBoxState::Checked:
		if (UJointEditorSettings* Settings = UJointEditorSettings::Get())
		{
			Settings->bEnableDeveloperMode = false;
		}
		break;
	case ECheckBoxState::Undetermined:
		break;
	}
}


FReply SJointEditorUtilityTab::ReconstructEveryNodeInOpenedJointManagerEditor()
{
	TArray<UObject*> Assets = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->GetAllEditedAssets();

	for (UObject* AllEditedAsset : Assets)
	{
		if (!AllEditedAsset) continue;

		if (UJointManager* Manager = Cast<UJointManager>(AllEditedAsset))
		{
			if (!Manager || !Manager->JointGraph) continue;

			UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(Manager->JointGraph);

			CastedGraph->ReconstructAllNodes(true);
		}
	}


	return FReply::Handled();
}

FReply SJointEditorUtilityTab::CleanUpUnnecessaryNodes()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<
		FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> AssetData;

#if UE_VERSION_OLDER_THAN(5, 1, 0)

	AssetRegistryModule.Get().GetAssetsByClass(UJointManager::StaticClass()->GetFName(), AssetData);

#else

	AssetRegistryModule.Get().GetAssetsByClass(UJointManager::StaticClass()->GetClassPathName(), AssetData);

#endif

	for (const FAssetData& Data : AssetData)
	{
		if (!Data.GetAsset()) continue;

		if (UJointManager* Manager = Cast<UJointManager>(Data.GetAsset()))
		{
			if (!Manager || !Manager->JointGraph) continue;

			UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(Manager->JointGraph);

			if (!CastedGraph) continue;

			CastedGraph->RemoveOrphanedNodes();
		}
	}

	return FReply::Handled();
}

FReply SJointEditorUtilityTab::UpdateBPNodeEdSettings()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<
		FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> AssetData;

#if UE_VERSION_OLDER_THAN(5, 1, 0)

	AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), AssetData);

#else

	AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), AssetData);

#endif

	int Count = 0;

	for (const FAssetData& Data : AssetData)
	{
		if (!Data.GetAsset()) continue;

		if (UBlueprint* Blueprint = Cast<UBlueprint>(Data.GetAsset()))
		{
			if (Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(UJointNodeBase::StaticClass()))
			{
				UE_LOG(LogJointEditor, Warning, TEXT("Modifying Blueprint: %s"), *Blueprint->GetName());

				// 1. Blueprint의 기본 CDO (Class Default Object) 가져오기
				UObject* DefaultObject = Blueprint->GeneratedClass->GetDefaultObject();
				if (DefaultObject)
				{
					if (UJointNodeBase* Node = Cast<UJointNodeBase>(DefaultObject))
					{
						Node->EdNodeSetting.UpdateFromNode(Node);
					}
				}

				// 3. Blueprint 변경 사항을 반영하기 위해 컴파일
				FKismetEditorUtilities::CompileBlueprint(Blueprint);

				Blueprint->MarkPackageDirty();

				Count++;
			}
		}
	}

	FNotificationInfo Info(
		FText::Format(
			LOCTEXT("UpdatedEdSettings",
			        "Updated {0} Joint Node Blueprint's Editor Settings. Save your project to apply the changes."),
			FText::FromString(FString::FromInt(Count)))
	);
	Info.ExpireDuration = 5.0f;
	Info.bUseLargeFont = false;
	Info.bUseThrobber = false;
	Info.bFireAndForget = true;

	FSlateNotificationManager::Get().AddNotification(Info);


	return FReply::Handled();
}

FReply SJointEditorUtilityTab::ResetAllEditorStyle()
{
	if (UJointEditorSettings* Settings = UJointEditorSettings::Get())
	{
		Settings->bUseLODRenderingForSimplePropertyDisplay =
			JointEditorDefaultSettings::bUseLODRenderingForSimplePropertyDisplay;
		Settings->LODRenderingForSimplePropertyDisplayRetainerPeriod =
			JointEditorDefaultSettings::LODRenderingForSimplePropertyDisplayRetainerPeriod;

		Settings->bUseGrid = JointEditorDefaultSettings::bUseGrid;
		Settings->BackgroundColor = JointEditorDefaultSettings::BackgroundColor;
		Settings->RegularGridColor = JointEditorDefaultSettings::RegularGridColor;
		Settings->RuleGridColor = JointEditorDefaultSettings::RuleGridColor;
		Settings->CenterGridColor = JointEditorDefaultSettings::CenterGridColor;
		Settings->SmallestGridSize = JointEditorDefaultSettings::SmallestGridSize;
		Settings->GridSnapSize = JointEditorDefaultSettings::GridSnapSize;


		Settings->ContextTextEditorFontSizeMultiplier = JointEditorDefaultSettings::ContextTextEditorFontSizeMultiplier;
		Settings->ContextTextAutoTextWrapAt = JointEditorDefaultSettings::ContextTextAutoTextWrapAt;
		Settings->ContextTextEditorBackgroundColor = JointEditorDefaultSettings::ContextTextEditorBackgroundColor;


		Settings->NormalConnectionColor = JointEditorDefaultSettings::NormalConnectionColor;
		Settings->RecursiveConnectionColor = JointEditorDefaultSettings::RecursiveConnectionColor;
		Settings->HighlightedConnectionColor = JointEditorDefaultSettings::HighlightedConnectionColor;
		Settings->SelfConnectionColor = JointEditorDefaultSettings::SelfConnectionColor;
		Settings->PreviewConnectionColor = JointEditorDefaultSettings::PreviewConnectionColor;


		Settings->PinConnectionThickness = JointEditorDefaultSettings::PinConnectionThickness;
		Settings->HighlightedPinConnectionThickness = JointEditorDefaultSettings::HighlightedPinConnectionThickness;
		Settings->ConnectionHighlightFadeBias = JointEditorDefaultSettings::ConnectionHighlightFadeBias;
		Settings->ConnectionHighlightedFadeInPeriod = JointEditorDefaultSettings::ConnectionHighlightedFadeInPeriod;
		Settings->bDrawNormalConnection = JointEditorDefaultSettings::bDrawNormalConnection;
		Settings->bDrawRecursiveConnection = JointEditorDefaultSettings::bDrawRecursiveConnection;


		Settings->NotHighlightedConnectionOpacity = JointEditorDefaultSettings::NotHighlightedConnectionOpacity;
		Settings->NotReachableRouteConnectionOpacity = JointEditorDefaultSettings::NotReachableRouteConnectionOpacity;

		Settings->ForwardSplineHorizontalDeltaRange = JointEditorDefaultSettings::ForwardSplineHorizontalDeltaRange;
		Settings->ForwardSplineVerticalDeltaRange = JointEditorDefaultSettings::ForwardSplineVerticalDeltaRange;
		Settings->ForwardSplineTangentFromHorizontalDelta =
			JointEditorDefaultSettings::ForwardSplineTangentFromHorizontalDelta;
		Settings->ForwardSplineTangentFromVerticalDelta =
			JointEditorDefaultSettings::ForwardSplineTangentFromVerticalDelta;

		Settings->BackwardSplineHorizontalDeltaRange = JointEditorDefaultSettings::BackwardSplineHorizontalDeltaRange;
		Settings->BackwardSplineVerticalDeltaRange = JointEditorDefaultSettings::BackwardSplineVerticalDeltaRange;
		Settings->BackwardSplineTangentFromHorizontalDelta =
			JointEditorDefaultSettings::BackwardSplineTangentFromHorizontalDelta;
		Settings->BackwardSplineTangentFromVerticalDelta =
			JointEditorDefaultSettings::BackwardSplineTangentFromVerticalDelta;

		Settings->SelfSplineHorizontalDeltaRange = JointEditorDefaultSettings::SelfSplineHorizontalDeltaRange;
		Settings->SelfSplineVerticalDeltaRange = JointEditorDefaultSettings::SelfSplineVerticalDeltaRange;
		Settings->SelfSplineTangentFromHorizontalDelta =
			JointEditorDefaultSettings::SelfSplineTangentFromHorizontalDelta;
		Settings->SelfSplineTangentFromVerticalDelta = JointEditorDefaultSettings::SelfSplineTangentFromVerticalDelta;

		Settings->bUseWiggleWireForNormalConnection = JointEditorDefaultSettings::bUseWiggleWireForNormalConnection;
		Settings->bUseWiggleWireForRecursiveConnection = JointEditorDefaultSettings::bUseWiggleWireForRecursiveConnection;
		Settings->bUseWiggleWireForSelfConnection = JointEditorDefaultSettings::bUseWiggleWireForSelfConnection;
		Settings->bUseWiggleWireForPreviewConnection = JointEditorDefaultSettings::bUseWiggleWireForPreviewConnection;

		Settings->NormalConnectionWiggleWireConfig = JointEditorDefaultSettings::WiggleWireConfig;
		Settings->RecursiveConnectionWiggleWireConfig = JointEditorDefaultSettings::WiggleWireConfig;
		Settings->SelfConnectionWiggleWireConfig = JointEditorDefaultSettings::WiggleWireConfig;
		Settings->PreviewConnectionWiggleWireConfig = JointEditorDefaultSettings::WiggleWireConfig;
		
		Settings->DebuggerPlayingNodeColor = JointEditorDefaultSettings::DebuggerPlayingNodeColor;
		Settings->DebuggerPlayingNodeColor = JointEditorDefaultSettings::DebuggerPlayingNodeColor;
		Settings->DebuggerEndedNodeColor = JointEditorDefaultSettings::DebuggerEndedNodeColor;


		Settings->DefaultNodeColor = JointEditorDefaultSettings::DefaultNodeColor;
		Settings->NodeDepthAdditiveColor = JointEditorDefaultSettings::NodeDepthAdditiveColor;


		UJointEditorSettings::Save();
	}
	return FReply::Handled();
}

FReply SJointEditorUtilityTab::ResetGraphEditorStyle()
{
	if (UJointEditorSettings* Settings = UJointEditorSettings::Get())
	{
		Settings->bUseGrid = JointEditorDefaultSettings::bUseGrid;
		Settings->BackgroundColor = JointEditorDefaultSettings::BackgroundColor;
		Settings->RegularGridColor = JointEditorDefaultSettings::RegularGridColor;
		Settings->RuleGridColor = JointEditorDefaultSettings::RuleGridColor;
		Settings->CenterGridColor = JointEditorDefaultSettings::CenterGridColor;
		Settings->SmallestGridSize = JointEditorDefaultSettings::SmallestGridSize;
		Settings->GridSnapSize = JointEditorDefaultSettings::GridSnapSize;

		UJointEditorSettings::Save();
	}
	return FReply::Handled();
}


FReply SJointEditorUtilityTab::ResetContextTextEditorStyle()
{
	if (UJointEditorSettings* Settings = UJointEditorSettings::Get())
	{
		Settings->ContextTextEditorFontSizeMultiplier = JointEditorDefaultSettings::ContextTextEditorFontSizeMultiplier;
		Settings->ContextTextAutoTextWrapAt = JointEditorDefaultSettings::ContextTextAutoTextWrapAt;
		Settings->ContextTextEditorBackgroundColor = JointEditorDefaultSettings::ContextTextEditorBackgroundColor;

		UJointEditorSettings::Save();
	}
	return FReply::Handled();
}


FReply SJointEditorUtilityTab::ResetPinConnectionEditorStyle()
{
	if (UJointEditorSettings* Settings = UJointEditorSettings::Get())
	{
		Settings->NormalConnectionColor = JointEditorDefaultSettings::NormalConnectionColor;
		Settings->RecursiveConnectionColor = JointEditorDefaultSettings::RecursiveConnectionColor;
		Settings->HighlightedConnectionColor = JointEditorDefaultSettings::HighlightedConnectionColor;
		Settings->SelfConnectionColor = JointEditorDefaultSettings::SelfConnectionColor;
		Settings->PreviewConnectionColor = JointEditorDefaultSettings::PreviewConnectionColor;

		Settings->PinConnectionThickness = JointEditorDefaultSettings::PinConnectionThickness;
		Settings->HighlightedPinConnectionThickness = JointEditorDefaultSettings::HighlightedPinConnectionThickness;
		Settings->ConnectionHighlightFadeBias = JointEditorDefaultSettings::ConnectionHighlightFadeBias;
		Settings->ConnectionHighlightedFadeInPeriod = JointEditorDefaultSettings::ConnectionHighlightedFadeInPeriod;
		Settings->bDrawNormalConnection = JointEditorDefaultSettings::bDrawNormalConnection;
		Settings->bDrawRecursiveConnection = JointEditorDefaultSettings::bDrawRecursiveConnection;

		Settings->NotHighlightedConnectionOpacity = JointEditorDefaultSettings::NotHighlightedConnectionOpacity;
		Settings->NotReachableRouteConnectionOpacity = JointEditorDefaultSettings::NotReachableRouteConnectionOpacity;

		Settings->ForwardSplineHorizontalDeltaRange = JointEditorDefaultSettings::ForwardSplineHorizontalDeltaRange;
		Settings->ForwardSplineVerticalDeltaRange = JointEditorDefaultSettings::ForwardSplineVerticalDeltaRange;
		Settings->ForwardSplineTangentFromHorizontalDelta =
			JointEditorDefaultSettings::ForwardSplineTangentFromHorizontalDelta;
		Settings->ForwardSplineTangentFromVerticalDelta =
			JointEditorDefaultSettings::ForwardSplineTangentFromVerticalDelta;

		Settings->BackwardSplineHorizontalDeltaRange = JointEditorDefaultSettings::BackwardSplineHorizontalDeltaRange;
		Settings->BackwardSplineVerticalDeltaRange = JointEditorDefaultSettings::BackwardSplineVerticalDeltaRange;
		Settings->BackwardSplineTangentFromHorizontalDelta =
			JointEditorDefaultSettings::BackwardSplineTangentFromHorizontalDelta;
		Settings->BackwardSplineTangentFromVerticalDelta =
			JointEditorDefaultSettings::BackwardSplineTangentFromVerticalDelta;

		Settings->SelfSplineHorizontalDeltaRange = JointEditorDefaultSettings::SelfSplineHorizontalDeltaRange;
		Settings->SelfSplineVerticalDeltaRange = JointEditorDefaultSettings::SelfSplineVerticalDeltaRange;
		Settings->SelfSplineTangentFromHorizontalDelta =
			JointEditorDefaultSettings::SelfSplineTangentFromHorizontalDelta;
		Settings->SelfSplineTangentFromVerticalDelta = JointEditorDefaultSettings::SelfSplineTangentFromVerticalDelta;

		UJointEditorSettings::Save();
	}
	return FReply::Handled();
}

FReply SJointEditorUtilityTab::ResetDebuggerEditorStyle()
{
	if (UJointEditorSettings* Settings = UJointEditorSettings::Get())
	{
		Settings->DebuggerPlayingNodeColor = JointEditorDefaultSettings::DebuggerPlayingNodeColor;
		Settings->DebuggerPlayingNodeColor = JointEditorDefaultSettings::DebuggerPlayingNodeColor;
		Settings->DebuggerEndedNodeColor = JointEditorDefaultSettings::DebuggerEndedNodeColor;

		UJointEditorSettings::Save();
	}
	return FReply::Handled();
}

FReply SJointEditorUtilityTab::ResetNodeEditorStyle()
{
	if (UJointEditorSettings* Settings = UJointEditorSettings::Get())
	{
		Settings->DefaultNodeColor = JointEditorDefaultSettings::DefaultNodeColor;
		Settings->NodeDepthAdditiveColor = JointEditorDefaultSettings::NodeDepthAdditiveColor;

		UJointEditorSettings::Save();
	}
	return FReply::Handled();
}


FJointManagementTab_NodeClassManagementTab::FJointManagementTab_NodeClassManagementTab()
{
}

FJointManagementTab_NodeClassManagementTab::~FJointManagementTab_NodeClassManagementTab()
{
}

TSharedRef<IJointManagementSubTab> FJointManagementTab_NodeClassManagementTab::MakeInstance()
{
	return MakeShareable(new FJointManagementTab_NodeClassManagementTab);
}

void FJointManagementTab_NodeClassManagementTab::RegisterTabSpawner(const TSharedPtr<FTabManager>& TabManager)
{
	TSharedPtr<FWorkspaceItem> JointEditorGroup = GetParentTabHandler().Pin()->GetActiveGroupFor("JointEditor");

	if (!JointEditorGroup)
	{
		JointEditorGroup = TabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("JointEditorGroupName", "Joint Editor"));

		GetParentTabHandler().Pin()->AddActiveGroup("JointEditor", JointEditorGroup);
	}

	TabManager->RegisterTabSpawner(
			GetTabId()
			, FOnSpawnTab::CreateLambda(
				[=](const FSpawnTabArgs&)
				{
					return SNew(SDockTab)
						.TabRole(ETabRole::PanelTab)
						.Label(LOCTEXT("NodeClassManagementTab", "Node Class Management"))
						[
							SNew(SJointEditorNodeClassManagementTab)
						];
				}
			)
		)
		.SetDisplayName(LOCTEXT("NodeClassManagementTabTitle", "Node Class Management"))
		.SetTooltipText(LOCTEXT("NodeClassManagementTabText", "Open the Node Class Management tab."))
		.SetGroup(JointEditorGroup.ToSharedRef())
		.SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(),
		                    "ExternalImagePicker.GenerateImageButton"));
}

const FName FJointManagementTab_NodeClassManagementTab::GetTabId()
{
	return "TAB_NodeClassManagementTab";
}

const ETabState::Type FJointManagementTab_NodeClassManagementTab::GetInitialTabState()
{
	return IJointManagementSubTab::GetInitialTabState();
}


namespace JointEditorNodeClassManagementTabs
{
	static const FName JointListTab("JointListID");
}


void SJointEditorNodeClassManagementTab::Construct(const FArguments& InArgs)
{
	auto NomadTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("SJointEditorNodeClassManagementTabTitle", "Node Class Management"));

	TabManager = FGlobalTabmanager::Get()->NewTabManager(NomadTab);

	SetCanTick(false);


	InitializeMissingClassesMapTab();

	RegisterTabSpawners(TabManager.ToSharedRef());


	TSharedPtr<FTabManager::FLayout> DebuggerLayout = FTabManager::NewLayout("JointNodeClassManagementTab_V1.1")
		->AddArea(FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.25f)
				->AddTab(JointEditorNodeClassManagementTabs::JointListTab, ETabState::OpenedTab)
			)
			// ->Split(
			// 	FTabManager::NewStack()
			// 	->SetSizeCoefficient(0.75f)
			// 	->AddTab(BulkSearchReplaceTapIDs::JointTreeID, ETabState::OpenedTab)
			// )
		);

	DebuggerLayout = FLayoutSaveRestore::LoadFromConfig(GEditorLayoutIni, DebuggerLayout.ToSharedRef());

	TSharedRef<SWidget> TabContents = TabManager->RestoreFrom(DebuggerLayout.ToSharedRef(), TSharedPtr<SWindow>()).
		ToSharedRef();

	// create & initialize main menu
	FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(TSharedPtr<FUICommandList>());

	MenuBarBuilder.AddPullDownMenu(
		LOCTEXT("SubTabsMenuLabel", "Sub Tabs")
		, FText::GetEmpty()
		, FNewMenuDelegate::CreateSP(this, &SJointEditorNodeClassManagementTab::FillWindowMenu)
		, "Sub Tabs"
	);

	// Tell tab-manager about the multi-box for platforms with a global menu bar

#if UE_VERSION_OLDER_THAN(5, 0, 0)
	TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox());
#else
	TSharedRef<SWidget> MenuWidget = MenuBarBuilder.MakeWidget();
	MenuWidget->SetClipping(EWidgetClipping::ClipToBoundsWithoutIntersecting);

	TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox(), MenuWidget);
#endif
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MenuBarBuilder.MakeWidget()
		]
		+ SVerticalBox::Slot()
		.Padding(2.0)
		[
			TabContents
		]
	];
}

void SJointEditorNodeClassManagementTab::FillWindowMenu(FMenuBuilder& MenuBuilder)
{
	if (!TabManager.IsValid()) { return; }

#if !WITH_EDITOR
	FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, WorkspaceMenu::GetMenuStructure().GetStructureRoot());
#endif //!WITH_EDITOR

	TabManager->PopulateLocalTabSpawnerMenu(MenuBuilder);
}

void SJointEditorNodeClassManagementTab::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	//Unregister first
	TabManager->UnregisterTabSpawner(JointEditorNodeClassManagementTabs::JointListTab);

	TabManager->RegisterTabSpawner(JointEditorNodeClassManagementTabs::JointListTab,
	                               FOnSpawnTab::CreateSP(
		                               this, &SJointEditorNodeClassManagementTab::SpawnMissingClassesMapTab))
		.SetDisplayName(LOCTEXT("JointListTabTitle", "Joint List"))
		.SetTooltipText(LOCTEXT("JointListTabTooltipText", "Open Joint list tab."))
		.SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "ContentBrowser.ShowSourcesView"));
}

void SJointEditorNodeClassManagementTab::InitializeMissingClassesMapTab()
{
	MissingClassesMapWidget = SNew(SJointEditorTap_MissingClassesMap);
}

TSharedRef<SDockTab> SJointEditorNodeClassManagementTab::SpawnMissingClassesMapTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> TabPtr = SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.Label(LOCTEXT("JointListTabTitle", "Joint List"));

	if (MissingClassesMapWidget.IsValid())
	{
		TabPtr->SetContent(MissingClassesMapWidget.ToSharedRef());
	}

	return TabPtr;
}


void SJointEditorTap_MissingClassesMap::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Image.GraphBackground"))
		.BorderBackgroundColor(FJointEditorStyle::Color_Node_TabBackground)
		.Padding(FJointEditorStyle::Margin_Normal)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(STextBlock)
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
					.Text(LOCTEXT("EditorTitle_ReallocateMissingNodeClasses", "Reallocate missing node classes (BETA)"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SJointOutlineButton)
					.ContentPadding(FJointEditorStyle::Margin_Normal)
					.OnClicked(this, &SJointEditorTap_MissingClassesMap::MissingClassRefresh)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("RefreshButton", "Refresh List"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(MissingClassScrollBox, SScrollBox)
				+ SScrollBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SJointOutlineBorder)
					.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.NormalColor(FLinearColor(0.015, 0.015, 0.02))
					.HoverColor(FLinearColor(0.04, 0.04, 0.06))
					.OutlineNormalColor(FLinearColor(0.015, 0.015, 0.02))
					.OutlineHoverColor(FLinearColor(0.5, 0.5, 0.5))
					.ContentPadding(FJointEditorStyle::Margin_Normal * 2)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(STextBlock)
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
						.Text(LOCTEXT("MissingNodeClassRefreshDescription",
						              "Please press refresh button to audit all assets."))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h2")
				.AutoWrapText(true)
				.Text(LOCTEXT("RedirectionTitle", "Allocated Redirections"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(RedirectionScrollBox, SScrollBox)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h1")
				.AutoWrapText(true)
				.ColorAndOpacity(FLinearColor::Red)
				.Text(LOCTEXT("DangerZoneTitle", "Danger Zone (BETA)"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				SNew(SJointOutlineBorder)
				.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.NormalColor(FLinearColor(0.015, 0.015, 0.02))
				.HoverColor(FLinearColor(0.04, 0.04, 0.06))
				.OutlineNormalColor(FLinearColor(0.015, 0.015, 0.02))
				.OutlineHoverColor(FLinearColor(0.5, 0.5, 0.5))
				.ContentPadding(FJointEditorStyle::Margin_Normal)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(STextBlock)
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h2")
						.AutoWrapText(true)
						.Text(LOCTEXT("NodeClassSwapHintTextTitle", "Swap Node Instances' Class"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[

						SNew(STextBlock)
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
						.AutoWrapText(true)
						.Text(LOCTEXT("NodeClassDataSwapHintText",
						              "Swap the existing node's class on the assets on the system into the other one. Be careful! This action is not revertiable, you must backup your project before proceed."))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FJointEditorStyle::Margin_Normal)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SClassPropertyEntryBox)
							.AllowNone(false)
							.AllowAbstract(false)
							.MetaClass(UJointNodeBase::StaticClass())
							.OnSetClass(this, &SJointEditorTap_MissingClassesMap::OnSetClass_NodeClassLeftSelectedClass)
							.SelectedClass(TAttribute<const UClass*>::CreateLambda([this]
							{
								return NodeClassLeftSelectedClass;
							}))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FJointEditorStyle::Margin_Normal)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
							.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("MeshPaint.NextTexture"))
							.DesiredSizeOverride(FVector2D(18, 18))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FJointEditorStyle::Margin_Normal)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SClassPropertyEntryBox)
							.AllowNone(false)
							.AllowAbstract(false)
							.MetaClass(UJointNodeBase::StaticClass())
							.OnSetClass(
								this, &SJointEditorTap_MissingClassesMap::OnSetClass_NodeClassRightSelectedClass)
							.SelectedClass(TAttribute<const UClass*>::CreateLambda([this]
							{
								return NodeClassRightSelectedClass;
							}))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FJointEditorStyle::Margin_Normal)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SJointOutlineButton)
							.ContentPadding(FJointEditorStyle::Margin_Large)
							.OnClicked(this, &SJointEditorTap_MissingClassesMap::OnNodeClassChangeButtonClicked)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("ReconstructButton", "Apply"))
							]
						]
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Normal)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				SNew(SJointOutlineBorder)
				.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.NormalColor(FLinearColor(0.015, 0.015, 0.02))
				.HoverColor(FLinearColor(0.04, 0.04, 0.06))
				.OutlineNormalColor(FLinearColor(0.015, 0.015, 0.02))
				.OutlineHoverColor(FLinearColor(0.5, 0.5, 0.5))
				.ContentPadding(FJointEditorStyle::Margin_Normal)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(STextBlock)
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h2")
						.AutoWrapText(true)
						.Text(LOCTEXT("EditorNodeClassSwapHintTextTitle", "Swap Editor Node Instances' Class"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(STextBlock)
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
						.AutoWrapText(true)
						.Text(LOCTEXT("EditorNodeClassDataSwapHintText",
						              "Swap the existing editor node's class on the assets on the system into the other one. Be careful! This action is not revertiable, you must backup your project before proceed."))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FJointEditorStyle::Margin_Normal)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SClassPropertyEntryBox)
							.AllowNone(false)
							.AllowAbstract(false)
							.MetaClass(UJointEdGraphNode::StaticClass())
							.OnSetClass(
								this, &SJointEditorTap_MissingClassesMap::OnSetClass_EditorNodeClassLeftSelectedClass)
							.SelectedClass(TAttribute<const UClass*>::CreateLambda([this]
							{
								return EditorNodeClassLeftSelectedClass;
							}))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FJointEditorStyle::Margin_Normal)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
							.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("MeshPaint.NextTexture"))
							.DesiredSizeOverride(FVector2D(18, 18))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FJointEditorStyle::Margin_Normal)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SClassPropertyEntryBox)
							.AllowNone(false)
							.AllowAbstract(false)
							.MetaClass(UJointEdGraphNode::StaticClass())
							.OnSetClass(
								this, &SJointEditorTap_MissingClassesMap::OnSetClass_EditorNodeClassRightSelectedClass)
							.SelectedClass(TAttribute<const UClass*>::CreateLambda([this]
							{
								return EditorNodeClassRightSelectedClass;
							}))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FJointEditorStyle::Margin_Normal)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SJointOutlineButton)
							.ContentPadding(FJointEditorStyle::Margin_Large)
							.OnClicked(this, &SJointEditorTap_MissingClassesMap::OnEditorNodeClassChangeButtonClicked)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("ReconstructButton", "Apply"))
							]
						]
					]
				]
			]
		]
	];


	AllocatedRedirectionRefresh();
}

FReply SJointEditorTap_MissingClassesMap::AllocatedRedirectionRefresh()
{
	int Count = 0;

	RedirectionScrollBox->ClearChildren();

	UJointEditorSettings* Settings = UJointEditorSettings::Get();

	for (FJointCoreRedirect& JointCoreRedirect : Settings->JointCoreRedirects)
	{
		++Count;

		RedirectionScrollBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(FJointEditorTap_RedirectionInstance)
				.Redirection(JointCoreRedirect)
				.Owner(SharedThis(this))
			];
	}


	if (Count == 0)
	{
		RedirectionScrollBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SJointOutlineBorder)
				.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.NormalColor(FLinearColor(0.015, 0.015, 0.02))
				.HoverColor(FLinearColor(0.04, 0.04, 0.06))
				.OutlineNormalColor(FLinearColor(0.015, 0.015, 0.02))
				.OutlineHoverColor(FLinearColor(0.5, 0.5, 0.5))
				.ContentPadding(FJointEditorStyle::Margin_Normal * 2)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(STextBlock)
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
					.Text(LOCTEXT("NoRedirectionDescription", "No Redirection is allocated."))
				]
			];
	}

	return FReply::Handled();
}

FReply SJointEditorTap_MissingClassesMap::MissingClassRefresh()
{
	if (MissingClassScrollBox.IsValid()) MissingClassScrollBox->ClearChildren();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<
		FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> AssetData;

#if UE_VERSION_OLDER_THAN(5, 1, 0)

	AssetRegistryModule.Get().GetAssetsByClass(UJointManager::StaticClass()->GetFName(), AssetData);

#else

	AssetRegistryModule.Get().GetAssetsByClass(UJointManager::StaticClass()->GetClassPathName(), AssetData);

#endif

	for (const FAssetData& Data : AssetData)
	{
		if (!Data.GetAsset()) continue;

		UJointManager* Manager = Cast<UJointManager>(Data.GetAsset());
		
		TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(Manager);

		for (UJointEdGraph* Graph : Graphs)
		{
			if (!Graph) continue;
			
			Graph->UpdateClassData();

			Graph->GrabUnknownClassDataFromGraph();
		}
	}
	

	if (FJointEditorModule* Module = FJointEditorModule::Get(); Module && Module->GetClassCache().IsValid())
	{
		bool bEverCreated = false;

		for (const FJointGraphNodeClassData& UnknownPackage : Module->GetClassCache()->UnknownPackages)
		{
			bEverCreated = true;

			MissingClassScrollBox->AddSlot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(FJointEditorTap_MissingClassInstance)
					.ClassData(UnknownPackage)
					.Owner(SharedThis(this))
				];
		}

		if (!bEverCreated)
		{
			MissingClassScrollBox->AddSlot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SJointOutlineBorder)
					.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.NormalColor(FLinearColor(0.015, 0.015, 0.02))
					.HoverColor(FLinearColor(0.04, 0.04, 0.06))
					.OutlineNormalColor(FLinearColor(0.015, 0.015, 0.02))
					.OutlineHoverColor(FLinearColor(0.5, 0.5, 0.5))
					.ContentPadding(FJointEditorStyle::Margin_Normal * 2)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(STextBlock)
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
						.Text(LOCTEXT("MissingNodeClassFixDescription", "No Missing Node Class. Hooray!"))
					]
				];
		}
	}
	else
	{
		MissingClassScrollBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SJointOutlineBorder)
				.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.NormalColor(FLinearColor(0.015, 0.015, 0.02))
				.HoverColor(FLinearColor(0.04, 0.04, 0.06))
				.OutlineNormalColor(FLinearColor(0.015, 0.015, 0.02))
				.OutlineHoverColor(FLinearColor(0.5, 0.5, 0.5))
				.ContentPadding(FJointEditorStyle::Margin_Normal* 2)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(STextBlock)
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
					.Text(LOCTEXT("RefreshError",
					              "Something went wrong. Please close the tab and reopen it, and try this again."))
				]
			];
	}

	return FReply::Handled();
}

void SJointEditorTap_MissingClassesMap::OnSetClass_NodeClassLeftSelectedClass(const UClass* Class)
{
	NodeClassLeftSelectedClass = Class ? const_cast<UClass*>(Class) : nullptr;
}

void SJointEditorTap_MissingClassesMap::OnSetClass_NodeClassRightSelectedClass(const UClass* Class)
{
	NodeClassRightSelectedClass = Class ? const_cast<UClass*>(Class) : nullptr;
}

void SJointEditorTap_MissingClassesMap::OnSetClass_EditorNodeClassLeftSelectedClass(const UClass* Class)
{
	EditorNodeClassLeftSelectedClass = Class ? const_cast<UClass*>(Class) : nullptr;
}

void SJointEditorTap_MissingClassesMap::OnSetClass_EditorNodeClassRightSelectedClass(const UClass* Class)
{
	EditorNodeClassRightSelectedClass = Class ? const_cast<UClass*>(Class) : nullptr;
}

FReply SJointEditorTap_MissingClassesMap::OnNodeClassChangeButtonClicked()
{
	if (NodeClassLeftSelectedClass == nullptr || NodeClassRightSelectedClass == nullptr)
	{
		FNotificationInfo Info = FNotificationInfo(LOCTEXT("CanNotProceedWarning", "Can not proceed class swapping"));
		Info.SubText = LOCTEXT("CanNotProceedWarning1", "Provided invalid classes.");
		Info.bFireAndForget = true;
		Info.FadeInDuration = 0.2f;
		Info.FadeOutDuration = 0.2f;
		Info.ExpireDuration = 2.5f;

		FSlateNotificationManager::Get().AddNotification(Info);

		return FReply::Handled();
	}

	if (NodeClassLeftSelectedClass == NodeClassRightSelectedClass)
	{
		FNotificationInfo Info = FNotificationInfo(LOCTEXT("CanNotProceedWarning", "Can not proceed class swapping"));
		Info.SubText = LOCTEXT("CanNotProceedWarning2",
		                       "The selected classes are same. Please select different classes.");
		Info.bFireAndForget = true;
		Info.FadeInDuration = 0.2f;
		Info.FadeOutDuration = 0.2f;
		Info.ExpireDuration = 2.5f;

		FSlateNotificationManager::Get().AddNotification(Info);

		return FReply::Handled();
	}


	switch (
		FMessageDialog::Open(EAppMsgType::OkCancel,
		                     FText::Format(
			                     LOCTEXT("NodeClassChangeWarning",
			                             "You are about to change {0} to {1} class in the whole Joint Manager Assets. Be careful! This action is not revertible, you must backup your project before proceed."),
			                     FText::FromName(NodeClassLeftSelectedClass->GetFName()),
			                     FText::FromName(NodeClassRightSelectedClass->GetFName()))))
	{
	case EAppReturnType::Ok:
		{
			//Cache again.

			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
				"AssetRegistry");

			TArray<FAssetData> AssetData;

#if UE_VERSION_OLDER_THAN(5, 1, 0)

			AssetRegistryModule.Get().GetAssetsByClass(UJointManager::StaticClass()->GetFName(), AssetData);

#else

			AssetRegistryModule.Get().GetAssetsByClass(UJointManager::StaticClass()->GetClassPathName(), AssetData);

#endif

			for (const FAssetData& Data : AssetData)
			{
				UObject* Asset = Data.GetAsset();
				if (!Asset) continue;
				
				UJointManager* Manager = Cast<UJointManager>(Asset);
				TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(Manager);

				for (UJointEdGraph* Graph : Graphs)
				{
					if (!Graph) continue;
					Graph->UpdateClassData();

					TSet<TWeakObjectPtr<UJointEdGraphNode>> EditorNodes = Graph->GetCachedJointGraphNodes(true);
					for (TWeakObjectPtr<UJointEdGraphNode> JointEdGraphNode : EditorNodes)
					{
						UJointNodeBase* NodeInstance = JointEdGraphNode->GetCastedNodeInstance();

						if (!NodeInstance) continue;

						if (NodeInstance->GetClass() == NodeClassLeftSelectedClass)
						{
							JointEdGraphNode->ReplaceNodeClassTo(NodeClassRightSelectedClass);

							Manager->MarkPackageDirty();
						}
					}
				}
			}

			break;
		}
	case EAppReturnType::Cancel:

		break;

	default:

		break;
	}


	return FReply::Handled();
}

FReply SJointEditorTap_MissingClassesMap::OnEditorNodeClassChangeButtonClicked()
{
	if (NodeClassLeftSelectedClass == nullptr || NodeClassRightSelectedClass == nullptr)
	{
		FNotificationInfo Info = FNotificationInfo(LOCTEXT("CanNotProceedWarning", "Can not proceed class swapping"));
		Info.SubText = LOCTEXT("CanNotProceedWarning1", "Provided invalid classes.");
		Info.bFireAndForget = true;
		Info.FadeInDuration = 0.2f;
		Info.FadeOutDuration = 0.2f;
		Info.ExpireDuration = 2.5f;

		FSlateNotificationManager::Get().AddNotification(Info);

		return FReply::Handled();
	}

	if (NodeClassLeftSelectedClass == NodeClassRightSelectedClass)
	{
		FNotificationInfo Info = FNotificationInfo(LOCTEXT("CanNotProceedWarning", "Can not proceed class swapping"));
		Info.SubText = LOCTEXT("CanNotProceedWarning2",
		                       "The selected classes are same. Please select different classes.");
		Info.bFireAndForget = true;
		Info.FadeInDuration = 0.2f;
		Info.FadeOutDuration = 0.2f;
		Info.ExpireDuration = 2.5f;

		FSlateNotificationManager::Get().AddNotification(Info);

		return FReply::Handled();
	}


	switch (
		FMessageDialog::Open(EAppMsgType::OkCancel,
		                     FText::Format(
			                     LOCTEXT("NodeClassChangeWarning",
			                             "You are about to change {0} to {1} class in the whole Joint Manager Assets. Be careful! This action is not revertible, you must backup your project before proceed."),
			                     FText::FromName(NodeClassLeftSelectedClass->GetFName()),
			                     FText::FromName(NodeClassRightSelectedClass->GetFName()))))
	{
	case EAppReturnType::Ok:
		{
			//Cache again.

			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
				"AssetRegistry");

			TArray<FAssetData> AssetData;

#if UE_VERSION_OLDER_THAN(5, 1, 0)

			AssetRegistryModule.Get().GetAssetsByClass(UJointManager::StaticClass()->GetFName(), AssetData);

#else

			AssetRegistryModule.Get().GetAssetsByClass(UJointManager::StaticClass()->GetClassPathName(), AssetData);

#endif

			for (const FAssetData& Data : AssetData)
			{
				UObject* Asset = Data.GetAsset();
				if (!Asset) continue;
				
				UJointManager* Manager = Cast<UJointManager>(Asset);
				TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(Manager);

				for (UJointEdGraph* Graph : Graphs)
				{
					if (!Graph) continue;
					Graph->UpdateClassData();

					TSet<TWeakObjectPtr<UJointEdGraphNode>> EditorNodes = Graph->GetCachedJointGraphNodes(true);
					for (TWeakObjectPtr<UJointEdGraphNode> JointEdGraphNode : EditorNodes)
					{
						if (JointEdGraphNode->GetClass() == EditorNodeClassLeftSelectedClass)
						{
							JointEdGraphNode->ReplaceEditorNodeClassTo(EditorNodeClassRightSelectedClass);

							Manager->MarkPackageDirty();
						}
					}
				}
			}

			break;
		}
	case EAppReturnType::Cancel:

		break;

	default:

		break;
	}

	return FReply::Handled();
}

void FJointEditorTap_RedirectionInstance::Construct(const FArguments& InArgs)
{
	Redirection = InArgs._Redirection;
	Owner = InArgs._Owner;

	ChildSlot[
		SNew(SJointOutlineBorder)
		.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.NormalColor(FLinearColor(0.015, 0.015, 0.02))
		.HoverColor(FLinearColor(0.04, 0.04, 0.06))
		.OutlineNormalColor(FLinearColor(0.015, 0.015, 0.02))
		.OutlineHoverColor(FLinearColor(0.5, 0.5, 0.5))
		.ContentPadding(FJointEditorStyle::Margin_Normal * 2)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
				.Text(FText::FromString(
					FJointCoreRedirectObjectName::ConvertToCoreRedirectObjectName(Redirection.OldName).ToString()))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("MeshPaint.NextTexture"))
				.DesiredSizeOverride(FVector2D(18, 18))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
				.Text(FText::FromString(
					FJointCoreRedirectObjectName::ConvertToCoreRedirectObjectName(Redirection.NewName).ToString()))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SJointOutlineButton)
				.ContentPadding(FJointEditorStyle::Margin_Large)
				.OnClicked(this, &FJointEditorTap_RedirectionInstance::RemoveRedirection)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Remove", "Remove Redirection"))
				]
			]
		]
	];
}

FReply FJointEditorTap_RedirectionInstance::RemoveRedirection()
{
	switch (
		FMessageDialog::Open(EAppMsgType::OkCancel,LOCTEXT("RemoveRedirectionWarning",
		                                                   "Are you sure you want to remove this redirection?\nThis will cause the redirected node to be unrecognized again by the system.")))
	{
	case EAppReturnType::Ok:
		{
			if (UJointEditorSettings* Settings = UJointEditorSettings::Get())
			{
				Settings->RemoveCoreRedirect(Redirection);
			}

			if (Owner.IsValid()) Owner.Pin()->AllocatedRedirectionRefresh();

			break;
		}
	case EAppReturnType::Cancel:

		break;

	default:

		break;
	}

	return FReply::Handled();
}

void FJointEditorTap_MissingClassInstance::Construct(const FArguments& InArgs)
{
	Owner = InArgs._Owner;
	ClassData = InArgs._ClassData;

	ChildSlot[
		SNew(SJointOutlineBorder)
		.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.NormalColor(FLinearColor(0.015, 0.015, 0.02))
		.HoverColor(FLinearColor(0.04, 0.04, 0.06))
		.OutlineNormalColor(FLinearColor(0.015, 0.015, 0.02))
		.OutlineHoverColor(FLinearColor(0.5, 0.5, 0.5))
		.ContentPadding(FJointEditorStyle::Margin_Normal * 2)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
				.Text(FText::Format(LOCTEXT("MissingNodeClassFixDescription", "Unknown Package: {0}"),
				                    FText::FromString(ClassData.GetPackageName())))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SClassPropertyEntryBox)
				.AllowNone(false)
				.AllowAbstract(false)
				.MetaClass(UJointNodeBase::StaticClass())
				.SelectedClass(this, &FJointEditorTap_MissingClassInstance::GetSelectedClass)
				.OnSetClass(this, &FJointEditorTap_MissingClassInstance::OnChangeNodeSetClass)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SJointOutlineButton)
				.ContentPadding(FJointEditorStyle::Margin_Large)
				.OnClicked(this, &FJointEditorTap_MissingClassInstance::Apply)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ReconstructButton", "Apply"))
				]
			]
		]
	];
}

void FJointEditorTap_MissingClassInstance::OnChangeNodeSetClass(const UClass* Class)
{
	SelectedClass = Class ? const_cast<UClass*>(Class) : nullptr;
}

const UClass* FJointEditorTap_MissingClassInstance::GetSelectedClass() const
{
	return SelectedClass;
}

FReply FJointEditorTap_MissingClassInstance::Apply()
{
	if (SelectedClass == nullptr)
	{
		FNotificationInfo Info = FNotificationInfo(
			LOCTEXT("CanNotProceedWarning", "Can not proceed class reallocation"));
		Info.SubText = LOCTEXT("CanNotProceedWarning1", "Provided invalid classes.");
		Info.bFireAndForget = true;
		Info.FadeInDuration = 0.2f;
		Info.FadeOutDuration = 0.2f;
		Info.ExpireDuration = 2.5f;

		FSlateNotificationManager::Get().AddNotification(Info);

		return FReply::Handled();
	}

	switch (
		FMessageDialog::Open(EAppMsgType::OkCancel,
		                     FText::Format(
			                     LOCTEXT("NodeClassChangeWarning",
			                             "You are about to feed {0} to lastly known \'{1}\' class in the whole Joint Manager Assets. This action is revertible but we recommend you to backup your project before proceed."),
			                     FText::FromName(SelectedClass->GetFName()),
			                     FText::FromString(ClassData.GetPackageName()))))
	{
	case EAppReturnType::Ok:
		{
			if (UJointEditorSettings* Settings = UJointEditorSettings::Get())
			{
				FJointCoreRedirect Redirect = FJointCoreRedirect(
					FCoreRedirectObjectName(FName(ClassData.GetClassName()), NAME_None,
					                        FName(ClassData.GetPackageName())),
					FCoreRedirectObjectName(SelectedClass)
				);

				Settings->AddCoreRedirect(Redirect);
			}

			//Clear unknown SelectedClass

			if (ISettingsEditorModule* SettingsEditorModule = FModuleManager::GetModulePtr<
				ISettingsEditorModule>("SettingsEditor"))
				SettingsEditorModule->OnApplicationRestartRequired();

			if (Owner.IsValid()) Owner.Pin()->AllocatedRedirectionRefresh();

			FMessageDialog::Open(EAppMsgType::Ok,
			                     LOCTEXT("NodeClassChangeFinished",
			                             "Added the redirection. Restart the editor to apply the change."));

			break;
		}
	case EAppReturnType::Cancel:

		break;

	default:

		break;
	}

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
