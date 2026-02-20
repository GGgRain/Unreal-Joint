//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointManagerActions.h"

#include "JointEditorStyle.h"
#include "JointManager.h"
#include "JointEditorToolkit.h"
#include "EditorWidget/SJointGraphPreviewer.h"
#include "Script/JointScriptSettings.h"


FJointManagerActions::FJointManagerActions(EAssetTypeCategories::Type InAssetCategory) :
	Category(InAssetCategory)
{
}

bool FJointManagerActions::CanFilter()
{
	return true;
}


void FJointManagerActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	FAssetTypeActions_Base::GetActions(InObjects, MenuBuilder);

	auto Assets = GetTypedWeakObjectPtrs<UJointManager>(InObjects);
}


uint32 FJointManagerActions::GetCategories()
{
	return Category;
}


FText FJointManagerActions::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_JointManager", "Joint Manager");
}


UClass* FJointManagerActions::GetSupportedClass() const
{
	return UJointManager::StaticClass();
}


FColor FJointManagerActions::GetTypeColor() const
{
	return FColor::Turquoise;
}


bool FJointManagerActions::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}


#include "JointEditor.h"

void FJointManagerActions::StoreEditorModuleClassCache()
{
	FJointEditorModule& EditorModule = FModuleManager::GetModuleChecked<FJointEditorModule>(TEXT("JointEditor"));

	//Store the class caches here. This is due to the synchronization issue.
	EditorModule.StoreClassCaches();
}

void FJointManagerActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric: EToolkitMode::Standalone;

	//Store the editor's class caches here. This is due to the synchronization issue.
	//It's a little confusing but don't change the location where we call this function at. It's kinda traditional thing for the UE's editor modules.
	//Joint 2.3.0 : made it store the caches prior to the editor initialization.
	StoreEditorModuleClassCache();
	
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		const auto Asset = Cast<UJointManager>(*ObjIt);

		if (Asset == nullptr) continue;
		
		const TSharedRef<FJointEditorToolkit> EditorToolkit = MakeShareable(new FJointEditorToolkit());
		EditorToolkit->InitJointEditor(Mode, EditWithinLevelEditor, Asset);
	}

}

TSharedPtr<class SWidget> FJointManagerActions::GetThumbnailOverlay(const FAssetData& AssetData) const
{
	UObject* Asset = AssetData.GetAsset();
	
	if (Asset == nullptr || !Asset->IsA<UJointManager>()) return SNullWidget::NullWidget;
	
	UJointManager* JointManager = Cast<UJointManager>(Asset);
	
	if (JointManager->JointGraph == nullptr) return SNullWidget::NullWidget;
	
	//Check if this asset has a linked external script.
	
	struct FScriptLinkDescription
	{
		FJointScriptLinkerFileEntry FileEntry;
		FJointScriptLinkerParserData ParserData;
		TMap<FString, FJointScriptLinkerNodeSet> NodeMappings;
	};
	
	TArray<FScriptLinkDescription> LinkedScripts;
	
	for (FJointScriptLinkerDataElement& ScriptLink : UJointScriptSettings::Get()->ScriptLinkData.ScriptLinks)
	{
		for (FJointScriptLinkerMapping& Mapping : ScriptLink.Mappings)
		{
			if (Mapping.JointManager == JointManager)
			{
				FScriptLinkDescription LinkDescription;
				LinkDescription.FileEntry = ScriptLink.FileEntry;
				LinkDescription.ParserData = ScriptLink.ParserData;
				LinkDescription.NodeMappings = Mapping.NodeMappings;
				
				LinkedScripts.Add(LinkDescription);
			}
		}
	}
	
	TSharedPtr<SVerticalBox> VerticalBox = nullptr;
	
	TSharedPtr<SOverlay> Overlay = SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(FJointEditorStyle::Margin_Small)
		[
			SAssignNew(VerticalBox, SVerticalBox)
		];
	
	for (FScriptLinkDescription& LinkedScript : LinkedScripts)
	{
		VerticalBox->AddSlot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(FJointEditorStyle::Margin_Small)
		[
			SNew(SImage)
			.Image(FJointEditorStyle::Get().GetBrush("ClassThumbnail.JointScriptLinker"))
			.DesiredSizeOverride(FVector2D(32, 32))
			.ToolTipText(
				FText::FromString(
						FString::Printf(TEXT("This Joint Manager has nodes that are possibly linked with an external file:\nLinked file: %s\nParsed with: %s\nExisting mapping entries: %d"), 
						*LinkedScript.FileEntry.FileName, 
						LinkedScript.ParserData.ScriptParser.IsValid() ? *LinkedScript.ParserData.ScriptParser->GetName() : TEXT("None"), 
						LinkedScript.NodeMappings.Num()
					)
				)
			)
		];
	}
	
	
	return Overlay;
}
