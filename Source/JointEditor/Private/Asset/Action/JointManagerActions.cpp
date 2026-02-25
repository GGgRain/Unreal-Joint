//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointManagerActions.h"

#include "ContentBrowserModule.h"
#include "FileHelpers.h"

#include "IContentBrowserSingleton.h"
#include "JointEditorStyle.h"
#include "JointManager.h"
#include "JointEditorToolkit.h"
#include "JointEdUtils.h"
#include "ObjectTools.h"

#include "Async/Async.h"
#include "EditorWidget/SJointGraphPreviewer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Markdown/SJointMDSlate_Admonitions.h"
#include "Script/JointScriptSettings.h"
#include "Thumbnail/JointManagerThumbnailRenderer.h"


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

	//auto Assets = GetTypedWeakObjectPtrs<UJointManager>(InObjects);
	
	// add capture thumbnail from graph action
	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("AssetTypeActions", "JointManager_CaptureThumbnailFromGraph", "Capture Thumbnail from Graph"),
		NSLOCTEXT("AssetTypeActions", "JointManager_CaptureThumbnailFromGraphTooltip", "Captures the thumbnail of this asset from its graph."),
		FSlateIcon(FJointEditorStyle::Get().GetStyleSetName(), "ClassThumbnail.JointManager"),
		FUIAction(
			FExecuteAction::CreateSP(this, &FJointManagerActions::CaptureThumbnailFromGraph, GetTypedWeakObjectPtrs<UJointManager>(InObjects))
		)
	);
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


// Static implementation: manually render the graph widget into a transient render target and cache thumbnail.
void FJointManagerActions::CaptureThumbnailFromGraph(
	TArray<TWeakObjectPtr<UJointManager>> Array)
{
	const int32 ThumbnailSizeX = 512;
	const int32 ThumbnailSizeY = 512;
	const float SleepPerPass = 0.03f;

	TSharedRef<FThreadSafeCounter> CompletedCount = MakeShared<FThreadSafeCounter>();

	const int32 TotalCount = Array.Num();

	for (TWeakObjectPtr<UJointManager> JointManagerPtr : Array)
	{
		if (!JointManagerPtr.IsValid()) 
		{
			CompletedCount->Increment();
			continue;
		}

		UJointManager* JointManager = JointManagerPtr.Get();
		if (!JointManager || !JointManager->GetJointGraph())
		{
			CompletedCount->Increment();
			continue;
		}

		TSharedPtr<FJointManagerThumbnailRendererResourcePoolElement> Elem = FJointManagerThumbnailRendererResourceManager::Get().FindOrAddResourcePoolElementFor(JointManager);

		if (!Elem)
		{
			CompletedCount->Increment();
			continue;
		}

		Elem->RequestUpdateGraphPreviewer();

		StartRenderChain(
			JointManager,
			Elem,
			ThumbnailSizeX,
			ThumbnailSizeY,
			SleepPerPass,
			CompletedCount,
			TotalCount,
			Array
		);
	}
}

void FJointManagerActions::StartRenderChain(
	UJointManager* JointManager,
	TSharedPtr<FJointManagerThumbnailRendererResourcePoolElement> Elem,
	int32 SizeX,
	int32 SizeY,
	float SleepPerPass,
	TSharedRef<FThreadSafeCounter> CompletedCount,
	int32 TotalCount,
	TArray<TWeakObjectPtr<UJointManager>> AllAssets)
{
	Async(EAsyncExecution::ThreadPool, [=, this]()
	{
		FPlatformProcess::Sleep(SleepPerPass);

		AsyncTask(ENamedThreads::GameThread, [=, this]()
		{
			Elem->CreateRenderTargetIfNeeded();

			if (!Elem->GetGraphPreviewer().IsValid())
			{
				Elem->SetGraphPreviewer(SNew(SJointGraphPreviewer, JointManager->GetJointGraph()));
				Elem->GetGraphPreviewer()->SetVisibility(EVisibility::SelfHitTestInvisible);
			}

			FJointManagerThumbnailRendererResourceManager::GetWidgetRenderer()->DrawWidget(
					Elem->GetRenderTarget(),
					Elem->GetGraphPreviewer().ToSharedRef(),
					FVector2D(SizeX, SizeY),
					30
				);

			Elem->TickGraphPreviewerUpdate();

			if (Elem->ShouldUpdateGraphPreviewer())
			{
				// if we have further chains to go through, do recursion.
				StartRenderChain(
					JointManager,
					Elem,
					SizeX,
					SizeY,
					SleepPerPass,
					CompletedCount,
					TotalCount,
					AllAssets
				);
			}
			else
			{
				FinalizeCapture(
					JointManager,
					Elem,
					SizeX,
					SizeY,
					CompletedCount,
					TotalCount,
					AllAssets
				);
			}
		});
	});
}

void FJointManagerActions::FinalizeCapture(
	UJointManager* JointManager,
	TSharedPtr<FJointManagerThumbnailRendererResourcePoolElement> Elem,
	int32 SizeX,
	int32 SizeY,
	TSharedRef<FThreadSafeCounter> CompletedCount,
	int32 TotalCount,
	TArray<TWeakObjectPtr<UJointManager>> AllAssets)
{
	AsyncTask(ENamedThreads::GameThread, [=, this]()
	{
		TArray<FColor> Pixels;

		if (Elem->GetRenderTarget() && Elem->GetRenderTarget()->GameThread_GetRenderTargetResource())
		{
			Elem->GetRenderTarget()->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
		}

		if (Pixels.Num() > 0)
		{
			FObjectThumbnail CachedThumbnail;
			CachedThumbnail.SetImageSize(SizeX, SizeY);

			const int32 ByteCount = Pixels.Num() * sizeof(FColor);
			CachedThumbnail.AccessImageData().SetNumUninitialized(ByteCount);

			FMemory::Memcpy(
				CachedThumbnail.AccessImageData().GetData(),
				Pixels.GetData(),
				ByteCount
			);

			UPackage* Package = JointManager->GetOutermost();

			FObjectThumbnail* NewThumbnail = ThumbnailTools::CacheThumbnail(
				JointManager->GetFullName(), 
				&CachedThumbnail,
				Package
			);
			
			// Let the content browser know that we've changed the thumbnail
			NewThumbnail->MarkAsDirty();
									
			// Signal that the asset was changed if it is loaded so thumbnail pools will update
			Package->PostEditChange();
			JointManager->PostEditChange();

			//Set that thumbnail as a valid custom thumbnail so it'll be saved out
			NewThumbnail->SetCreatedAfterCustomThumbsEnabled();

			Package->MarkPackageDirty();
		}

		// Increment the completed count and check if all captures are done. If so, prompt to save all assets that were captured.
		int32 Done = CompletedCount->Increment();

		if (Done == TotalCount)
		{
			TArray<UPackage*> PackagesToSave;
			TArray<UObject*> AssetsToSync;

			for (auto Ptr : AllAssets)
			{
				if (Ptr.IsValid())
				{
					PackagesToSave.AddUnique(Ptr->GetOutermost());
					AssetsToSync.AddUnique(Ptr.Get());
				}
			}

			FEditorFileUtils::PromptForCheckoutAndSave(
				PackagesToSave,
				false,
				false
			);
			
			FJointEdUtils::FireNotification(	
				NSLOCTEXT("JointManagerActions", "ThumbnailCaptureComplete", "Thumbnail capture complete!"),
                 				FText::Format(NSLOCTEXT("JointManagerActions", "ThumbnailCaptureCompleteTooltip", "The thumbnail capture process has completed for {0} asset(s).\nPlease save the assets to keep the captured thumbnails."), TotalCount),
                 				EJointMDAdmonitionType::Mention,
                 				8
			);
		}
	});
}

void FJointManagerActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric: EToolkitMode::Standalone;

	//Store the editor's class caches here. This is due to the synchronization issue.
	//It's a little confusing but don't change the location where we call this function at. It's kinda traditional thing for the UE's editor modules.
	//Joint 2.3.0 : made it store the caches prior to the editor initialization.
	FJointEdUtils::StoreEditorModuleClassCache();
	
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
		.Padding(FJointEditorStyle::Margin_Tiny)
		[
			SAssignNew(VerticalBox, SVerticalBox)
		];
	
	for (FScriptLinkDescription& LinkedScript : LinkedScripts)
	{
		VerticalBox->AddSlot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(FJointEditorStyle::Margin_Tiny)
		[
			SNew(SImage)
			.Image(FJointEditorStyle::Get().GetBrush("ClassThumbnail.JointScriptLinker"))
			.DesiredSizeOverride(FVector2D(32, 32))
			.ToolTipText(
				FText::FromString(
						FString::Printf(TEXT("This Joint Manager has nodes that are possibly linked with an external file:\nLinked file Path: %s\nParsed with: %s\nExisting mapping entries: %d"), 
						*LinkedScript.FileEntry.FilePath,
						LinkedScript.ParserData.ScriptParser.IsValid() ? *LinkedScript.ParserData.ScriptParser->GetName() : TEXT("None"), 
						LinkedScript.NodeMappings.Num()
					)
				)
			)
		];
	}
	
	VerticalBox->AddSlot()
	.HAlign(HAlign_Right)
	.VAlign(VAlign_Bottom)
	.Padding(FJointEditorStyle::Margin_Tiny)
	[
		SNew(SImage)
			.Image(FJointEditorStyle::Get().GetBrush("ClassThumbnail.JointManager"))
			.DesiredSizeOverride(FVector2D(32, 32))
	];
	
	return Overlay;
}
