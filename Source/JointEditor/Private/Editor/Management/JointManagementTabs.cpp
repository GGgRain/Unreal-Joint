//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointManagementTabs.h"

#include "JointAdvancedWidgets.h"

#include "JointEdGraph.h"
#include "JointEditorSettings.h"
#include "JointEditorStyle.h"

#include "JointManager.h"

#include "Widgets/Layout/SScrollBox.h"

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
		.SetIcon(FSlateIcon(FJointEditorStyle::GetUEEditorSlateStyleSetName(), "ExternalImagePicker.GenerateImageButton"));
}

const FName FJointManagementTab_JointEditorUtilityTab::GetTabId()
{
	return "TAB_JointEditorUtility";
}

const ETabState::Type FJointManagementTab_JointEditorUtilityTab::GetInitialTabState()
{
	return IJointManagementSubTab::GetInitialTabState();
}

#if UE_VERSION_OLDER_THAN(5,1,0)

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
		.Padding(FJointEditorStyle::Margin_Border)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FJointEditorStyle::Margin_Border)
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
			.Padding(FJointEditorStyle::Margin_Border)
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
		.Padding(FJointEditorStyle::Margin_Border)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h5")
			.Text(LOCTEXT("DescText", "Joint, the conversation scripting plugin & Volt, the slate framework animation library"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FJointEditorStyle::Margin_Border)
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
		.Padding(FJointEditorStyle::Margin_Border)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			.Padding(FJointEditorStyle::Margin_Border)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[
				CreateProductSection()
			]
			+ SScrollBox::Slot()
			.Padding(FJointEditorStyle::Margin_Border)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			[
				CreateInvalidateSection()
			]
			+ SScrollBox::Slot()
			.Padding(FJointEditorStyle::Margin_Border)
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
		.NormalColor(FLinearColor(0.015,0.015,0.02))
		.HoverColor(FLinearColor(0.04,0.04,0.06))
		.OutlineNormalColor(FLinearColor(0.015,0.015,0.02))
		.OutlineHoverColor(FLinearColor(0.5,0.5,0.5))
		.ContentMargin(FJointEditorStyle::Margin_Border)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Border)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
				.Text(LOCTEXT("QuickFixStyleTip", "Quick-Fix"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Border)
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
					.ContentMargin(FJointEditorStyle::Margin_Button * 1.5)
					.OnClicked(this, &SJointEditorUtilityTab::ReconstructEveryNodeInOpenedJointManagerEditor)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ReconstructButton", "Reconstruct"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Border)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CleanUpNodes", "Clean-up orphened nodes in the projects. This doesn't need to be done twice, it's for the assets from old versions (before 2.8)"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineButton)
					.ContentMargin(FJointEditorStyle::Margin_Button * 1.5)
					.OnClicked(this, &SJointEditorUtilityTab::CleanUpUnnecessaryNodes)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CleanUpButton", "CleanUp"))
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
		.NormalColor(FLinearColor(0.015,0.015,0.02))
		.HoverColor(FLinearColor(0.04,0.04,0.06))
		.OutlineNormalColor(FLinearColor(0.015,0.015,0.02))
		.OutlineHoverColor(FLinearColor(0.5,0.5,0.5))
		.ContentMargin(FJointEditorStyle::Margin_Border)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Border)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h1")
				.Text(LOCTEXT("EditorStyleTip", "Editor Style"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Border)
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
					.ContentMargin(FJointEditorStyle::Margin_Button * 1.5)
					.OnClicked(this, &SJointEditorUtilityTab::ResetAllEditorStyle)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ResetButtonText", "Reset"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Border)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h2")
				.Text(LOCTEXT("GraphEditorStyleTip", "Graph Editor Style"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Border)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ResetGraphEditorBackgroundStyleText", "Reset Graph Editor Background Style (Grid, Color) to default"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineButton)
					.ContentMargin(FJointEditorStyle::Margin_Button * 1.5)
					.OnClicked(this, &SJointEditorUtilityTab::ResetGraphEditorStyle)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ResetButtonText", "Reset"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Border)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ResetGraphEditorBackgroundStyleText", "Reset Graph Editor Pin & Connection Style to default"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineButton)
					.ContentMargin(FJointEditorStyle::Margin_Button * 1.5)
					.OnClicked(this, &SJointEditorUtilityTab::ResetPinConnectionEditorStyle)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ResetButtonText", "Reset"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Border)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ResetGraphEditorBackgroundStyleText", "Reset Graph Editor Pin & Connection Style to default"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineButton)
					.ContentMargin(FJointEditorStyle::Margin_Button * 1.5)
					.OnClicked(this, &SJointEditorUtilityTab::ResetPinConnectionEditorStyle)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ResetButtonText", "Reset"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Border)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ResetDebuggerStyleText", "Reset Graph Editor Debugger Style (Color for playback states) to default"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SJointOutlineButton)
					.ContentMargin(FJointEditorStyle::Margin_Button * 1.5)
					.OnClicked(this, &SJointEditorUtilityTab::ResetDebuggerEditorStyle)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ResetButtonText", "Reset"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Border)
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
					.ContentMargin(FJointEditorStyle::Margin_Button * 1.5)
					.OnClicked(this, &SJointEditorUtilityTab::ResetNodeEditorStyle)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ResetButtonText", "Reset"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Border)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h2")
				.Text(LOCTEXT("ContextTextEditorStyleTip", "Context Text Style"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FJointEditorStyle::Margin_Border)
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
					.ContentMargin(FJointEditorStyle::Margin_Button * 1.5)
					.OnClicked(this, &SJointEditorUtilityTab::ResetContextTextEditorStyle)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ResetButtonText", "Reset"))
					]
				]
			]
		];
}

FReply SJointEditorUtilityTab::ReconstructEveryNodeInOpenedJointManagerEditor()
{
	TArray<UObject*> Assets = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->GetAllEditedAssets();
	
	for (UObject* AllEditedAsset : Assets)
	{
		if(!AllEditedAsset) continue;

		if(UJointManager* Manager = Cast<UJointManager>(AllEditedAsset))
		{
			if(!Manager || !Manager->JointGraph) continue;

			UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(Manager->JointGraph);
			
			CastedGraph->ReconstructAllNodes(true);
			
		}
	}
			

	return FReply::Handled();
}

FReply SJointEditorUtilityTab::CleanUpUnnecessaryNodes()
{

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	TArray<FAssetData> AssetData;

#if UE_VERSION_OLDER_THAN(5,1,0)

	AssetRegistryModule.Get().GetAssetsByClass(UJointManager::StaticClass()->GetFName(), AssetData);

#else

	AssetRegistryModule.Get().GetAssetsByClass(UJointManager::StaticClass()->GetClassPathName(), AssetData);

#endif
	
	for (const FAssetData& Data : AssetData) {

		if(!Data.GetAsset()) continue;
		
		if(UJointManager* Manager = Cast<UJointManager>(Data.GetAsset()))
		{
			if(!Manager || !Manager->JointGraph) continue;

			UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(Manager->JointGraph);
			
			CastedGraph->RemoveOrphanedNodes();
			
		}
	}

	return FReply::Handled();
}

FReply SJointEditorUtilityTab::ResetAllEditorStyle()
{
	if(UJointEditorSettings* Settings = UJointEditorSettings::Get())
	{
		
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
		Settings->ForwardSplineTangentFromHorizontalDelta = JointEditorDefaultSettings::ForwardSplineTangentFromHorizontalDelta;
		Settings->ForwardSplineTangentFromVerticalDelta = JointEditorDefaultSettings::ForwardSplineTangentFromVerticalDelta;
		
		Settings->BackwardSplineHorizontalDeltaRange = JointEditorDefaultSettings::BackwardSplineHorizontalDeltaRange;
		Settings->BackwardSplineVerticalDeltaRange = JointEditorDefaultSettings::BackwardSplineVerticalDeltaRange;
		Settings->BackwardSplineTangentFromHorizontalDelta = JointEditorDefaultSettings::BackwardSplineTangentFromHorizontalDelta;
		Settings->BackwardSplineTangentFromVerticalDelta = JointEditorDefaultSettings::BackwardSplineTangentFromVerticalDelta;

		Settings->SelfSplineHorizontalDeltaRange = JointEditorDefaultSettings::SelfSplineHorizontalDeltaRange;
		Settings->SelfSplineVerticalDeltaRange = JointEditorDefaultSettings::SelfSplineVerticalDeltaRange;
		Settings->SelfSplineTangentFromHorizontalDelta = JointEditorDefaultSettings::SelfSplineTangentFromHorizontalDelta;
		Settings->SelfSplineTangentFromVerticalDelta = JointEditorDefaultSettings::SelfSplineTangentFromVerticalDelta;



		
		Settings->DebuggerPlayingNodeColor = JointEditorDefaultSettings::DebuggerPlayingNodeColor;
		Settings->DebuggerPlayingNodeColor = JointEditorDefaultSettings::DebuggerPlayingNodeColor;
		Settings->DebuggerEndedNodeColor = JointEditorDefaultSettings::DebuggerEndedNodeColor;

		

		Settings->DefaultNodeColor = JointEditorDefaultSettings::DefaultNodeColor;
		Settings->NodeDepthAdditiveColor = JointEditorDefaultSettings::NodeDepthAdditiveColor;
		
		
		Settings->SaveConfig();
	}
	return FReply::Handled();
}

FReply SJointEditorUtilityTab::ResetGraphEditorStyle()
{
	if(UJointEditorSettings* Settings = UJointEditorSettings::Get())
	{
		Settings->bUseGrid = JointEditorDefaultSettings::bUseGrid;
		Settings->BackgroundColor = JointEditorDefaultSettings::BackgroundColor;
		Settings->RegularGridColor = JointEditorDefaultSettings::RegularGridColor;
		Settings->RuleGridColor = JointEditorDefaultSettings::RuleGridColor;
		Settings->CenterGridColor = JointEditorDefaultSettings::CenterGridColor;
		Settings->SmallestGridSize = JointEditorDefaultSettings::SmallestGridSize;
		Settings->GridSnapSize = JointEditorDefaultSettings::GridSnapSize;

		Settings->SaveConfig();
	}
	return FReply::Handled();
}


FReply SJointEditorUtilityTab::ResetContextTextEditorStyle()
{
	if(UJointEditorSettings* Settings = UJointEditorSettings::Get())
	{
		Settings->ContextTextEditorFontSizeMultiplier = JointEditorDefaultSettings::ContextTextEditorFontSizeMultiplier;
		Settings->ContextTextAutoTextWrapAt = JointEditorDefaultSettings::ContextTextAutoTextWrapAt;
		Settings->ContextTextEditorBackgroundColor = JointEditorDefaultSettings::ContextTextEditorBackgroundColor;
		
		Settings->SaveConfig();
	}
	return FReply::Handled();
}

FReply SJointEditorUtilityTab::ResetPinConnectionEditorStyle()
{
	if(UJointEditorSettings* Settings = UJointEditorSettings::Get())
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
		Settings->ForwardSplineTangentFromHorizontalDelta = JointEditorDefaultSettings::ForwardSplineTangentFromHorizontalDelta;
		Settings->ForwardSplineTangentFromVerticalDelta = JointEditorDefaultSettings::ForwardSplineTangentFromVerticalDelta;
		
		Settings->BackwardSplineHorizontalDeltaRange = JointEditorDefaultSettings::BackwardSplineHorizontalDeltaRange;
		Settings->BackwardSplineVerticalDeltaRange = JointEditorDefaultSettings::BackwardSplineVerticalDeltaRange;
		Settings->BackwardSplineTangentFromHorizontalDelta = JointEditorDefaultSettings::BackwardSplineTangentFromHorizontalDelta;
		Settings->BackwardSplineTangentFromVerticalDelta = JointEditorDefaultSettings::BackwardSplineTangentFromVerticalDelta;

		Settings->SelfSplineHorizontalDeltaRange = JointEditorDefaultSettings::SelfSplineHorizontalDeltaRange;
		Settings->SelfSplineVerticalDeltaRange = JointEditorDefaultSettings::SelfSplineVerticalDeltaRange;
		Settings->SelfSplineTangentFromHorizontalDelta = JointEditorDefaultSettings::SelfSplineTangentFromHorizontalDelta;
		Settings->SelfSplineTangentFromVerticalDelta = JointEditorDefaultSettings::SelfSplineTangentFromVerticalDelta;
		
		Settings->SaveConfig();
	}
	return FReply::Handled();
}

FReply SJointEditorUtilityTab::ResetDebuggerEditorStyle()
{
	if(UJointEditorSettings* Settings = UJointEditorSettings::Get())
	{
		Settings->DebuggerPlayingNodeColor = JointEditorDefaultSettings::DebuggerPlayingNodeColor;
		Settings->DebuggerPlayingNodeColor = JointEditorDefaultSettings::DebuggerPlayingNodeColor;
		Settings->DebuggerEndedNodeColor = JointEditorDefaultSettings::DebuggerEndedNodeColor;
		
		Settings->SaveConfig();
	}
	return FReply::Handled();
}

FReply SJointEditorUtilityTab::ResetNodeEditorStyle()
{
	if(UJointEditorSettings* Settings = UJointEditorSettings::Get())
	{
		Settings->DefaultNodeColor = JointEditorDefaultSettings::DefaultNodeColor;
		Settings->NodeDepthAdditiveColor = JointEditorDefaultSettings::NodeDepthAdditiveColor;
		
		Settings->SaveConfig();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
