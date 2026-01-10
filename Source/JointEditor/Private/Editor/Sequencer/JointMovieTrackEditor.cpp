// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/Sequencer/JointMovieTrackEditor.h"
#include "Widgets/SBoxPanel.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Modules/ModuleManager.h"
#include "Application/ThrottleManager.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SCheckBox.h"

#include "Sections/MovieSceneCameraCutSection.h"
#include "SequencerUtilities.h"

#include "LevelSequence.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "JointEditorLogChannels.h"
#include "JointManager.h"

#include "Editor/Sequencer/FJointMovieSection.h"

#include "Editor/Style/JointEditorStyle.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/Commands.h"
#include "Sequencer/MovieSceneJointTrack.h"

#include "Misc/EngineVersionComparison.h"

#define LOCTEXT_NAMESPACE "FJointMovieTrackEditor"


class FJointMovieTrackCommands
	: public TCommands<FJointMovieTrackCommands>
{
public:
	FJointMovieTrackCommands()
		: TCommands<FJointMovieTrackCommands>
		  (
			  "JointMovieTrack",
			  NSLOCTEXT("Contexts", "JointMovieTrack", "JointMovieTrack"),
			  NAME_None, // "MainFrame" // @todo Fix this crash
			  FJointEditorStyle::GetUEEditorSlateStyleSetName() // Icon Style Set
		  )
		  , BindingCount(0)
	{
	}

	/** Toggle the camera lock */
	TSharedPtr<FUICommandInfo> ToggleLockCamera;

	/**
	 * Initialize commands
	 */
	virtual void RegisterCommands() override;

	mutable uint32 BindingCount;
};


void FJointMovieTrackCommands::RegisterCommands()
{
	UI_COMMAND(ToggleLockCamera, "Toggle Lock Camera", "Toggle locking the viewport to the camera cut track.", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::L));
}


/* FJointMovieTrackEditor structors
 *****************************************************************************/

FJointMovieTrackEditor::FJointMovieTrackEditor(TSharedRef<ISequencer> InSequencer)
	: FMovieSceneTrackEditor(InSequencer)
{
	FJointMovieTrackCommands::Register();
}

TSharedRef<ISequencerTrackEditor> FJointMovieTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	return MakeShareable(new FJointMovieTrackEditor(InSequencer));
}

void FJointMovieTrackEditor::OnRelease()
{
	const FJointMovieTrackCommands& Commands = FJointMovieTrackCommands::Get();
	Commands.BindingCount--;

	if (Commands.BindingCount < 1)
	{
		FJointMovieTrackCommands::Unregister();
	}
}


/* ISequencerTrackEditor interface
 *****************************************************************************/

void FJointMovieTrackEditor::BindCommands(TSharedRef<FUICommandList> SequencerCommandBindings)
{
	const FJointMovieTrackCommands& Commands = FJointMovieTrackCommands::Get();

	/*
	SequencerCommandBindings->MapAction(
		Commands.ToggleLockCamera,
		FExecuteAction::CreateSP( this, &FJointMovieTrackEditor::ToggleLockCamera) );
	*/

	Commands.BindingCount++;
}

void FJointMovieTrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddSubMenu(
		LOCTEXT("AddJointMovieTrack", "Add Joint Movie Track"),
		LOCTEXT("AddJointMovieTooltip", "Adds a Joint Movie Track here. The track will be used to trigger fragments of the Joint Manager instance that created this Level Sequence."),
		FNewMenuDelegate::CreateRaw(this, &FJointMovieTrackEditor::AddJointMovieTrackMenuExtension),
		false /*bInOpenSubMenuOnClick*/,
		FSlateIcon(FJointEditorStyle::GetStyleSetName(), "ClassIcon.JointManager")
	);

	/*
	MenuBuilder.AddSubMenu(
		LOCTEXT("AddJointMovieTrack", "Add Joint Manager Binding"),
		LOCTEXT("AddJointMovieTooltip", "Adds a Joint Manager Binding Track here. The track will be used to trigger fragments of graph."),
		FNewMenuDelegate::CreateRaw(this, &FJointMovieTrackEditor::AddJointActorMenuExtension),
		false,
		FSlateIcon(FJointEditorStyle::GetStyleSetName(), "ClassIcon.JointManager")
	);
	*/
}

void FJointMovieTrackEditor::BuildTrackContextMenu(FMenuBuilder& MenuBuilder, UMovieSceneTrack* Track)
{
	/*
	UMovieSceneCameraCutTrack* CameraCutTrack = Cast<UMovieSceneCameraCutTrack>(Track);
	MenuBuilder.AddMenuEntry(
		LOCTEXT("CanBlendShots", "Can Blend"),
		LOCTEXT("CanBlendShotsTooltip", "Enable shot blending on this track, making it possible to overlap sections."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FJointMovieTrackEditor::HandleToggleCanBlendExecute, CameraCutTrack),
			FCanExecuteAction::CreateLambda([=]() { return CameraCutTrack != nullptr; }),
			FIsActionChecked::CreateLambda([=]() { return CameraCutTrack->bCanBlend; })
			),
		"Edit",
		EUserInterfaceActionType::ToggleButton
	);
	*/
}

void FJointMovieTrackEditor::BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const TArray<FGuid>& ObjectBindings, const UClass* ObjectClass)
{
}

TSharedPtr<SWidget> FJointMovieTrackEditor::BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params)
{
	return SNew(SHorizontalBox)
			// Add the audio combo box
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				FSequencerUtilities::MakeAddButton(
					LOCTEXT("NodeSectionText", "Node Section"),
					FOnGetContent::CreateSP(
						this,
						&FJointMovieTrackEditor::BuildAddSectionSubMenu,
						Track
					),
					Params.NodeIsHovered,
					GetSequencer()
				)
			];
}

TSharedRef<ISequencerSection> FJointMovieTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	check(SupportsType(SectionObject.GetOuter()->GetClass()));

	return MakeShareable(new FJointMovieSection(GetSequencer(), SectionObject));
}

bool FJointMovieTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const
{
	return (Type == UMovieSceneJointTrack::StaticClass());
}


bool FJointMovieTrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
	ETrackSupport TrackSupported = InSequence ? InSequence->IsTrackSupported(UMovieSceneJointTrack::StaticClass()) : ETrackSupport::NotSupported;

	if (TrackSupported == ETrackSupport::Supported)
	{
		return true;
	}
	else if (TrackSupported == ETrackSupport::Default)
	{
		// By default, we only support sequences that are level sequence.
		// This prevents adding camera cut tracks to animation sequences, for example.

		// If you want to add camera cut tracks to other sequence types, well... just change it as you need.
		return InSequence->IsA<ULevelSequence>();
	}

	return false;
}

void FJointMovieTrackEditor::Tick(float DeltaTime)
{
	/*
	TSharedPtr<ISequencer> SequencerPin = GetSequencer();
	if (!SequencerPin.IsValid())
	{
		return;
	}
	
	EMovieScenePlayerStatus::Type PlaybackState = SequencerPin->GetPlaybackStatus();

	if (FSlateThrottleManager::Get().IsAllowingExpensiveTasks() && PlaybackState != EMovieScenePlayerStatus::Playing && PlaybackState != EMovieScenePlayerStatus::Scrubbing)
	{
		SequencerPin->EnterSilentMode();

		FQualifiedFrameTime SavedTime = SequencerPin->GetLocalTime();

		//if (DeltaTime > 0.f && ThumbnailPool->DrawThumbnails())
		{
			SequencerPin->SetLocalTimeDirectly(SavedTime.Time);
		}

		SequencerPin->ExitSilentMode();
	}
	*/
}

const FSlateBrush* FJointMovieTrackEditor::GetIconBrush() const
{		
	return FJointEditorStyle::Get().GetBrush("ClassIcon.JointManager");
}


bool FJointMovieTrackEditor::OnAllowDrop(const FDragDropEvent& DragDropEvent, FSequencerDragDropParams& DragDropParams)
{
	UE_LOG(LogJointEditor, Warning, TEXT("FJointMovieTrackEditor::OnAllowDrop called"));

	/*
	if (!DragDropParams.Track.IsValid() || !DragDropParams.Track.Get()->IsA(UMovieSceneCameraCutTrack::StaticClass()))
	{
		return false;
	}

	TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();

	if (!Operation.IsValid() || !Operation->IsOfType<FActorDragDropGraphEdOp>() )
	{
		return false;
	}
	
	UMovieSceneCameraCutTrack* CameraCutTrack = Cast<UMovieSceneCameraCutTrack>(DragDropParams.Track.Get());

	TSharedPtr<FActorDragDropGraphEdOp> DragDropOp = StaticCastSharedPtr<FActorDragDropGraphEdOp>( Operation );

	for (auto& ActorPtr : DragDropOp->Actors)
	{
		if (ActorPtr.IsValid())
		{
			AActor* Actor = ActorPtr.Get();
				
			UCameraComponent* CameraComponent = MovieSceneHelpers::CameraComponentFromActor(Actor);
			if (CameraComponent)
			{
				FFrameNumber EndFrameNumber = CameraCutTrack->FindEndTimeForCameraCut(DragDropParams.FrameNumber);
				DragDropParams.FrameRange = TRange<FFrameNumber>(DragDropParams.FrameNumber, EndFrameNumber);
				return true;
			}
		}
	}
	
	*/
	return false;
}


FReply FJointMovieTrackEditor::OnDrop(const FDragDropEvent& DragDropEvent, const FSequencerDragDropParams& DragDropParams)
{

	//UE_LOG(LogJointEditor, Warning, TEXT("FJointMovieTrackEditor::OnDrop called"));
	/*
	if (!DragDropParams.Track.IsValid() || !DragDropParams.Track.Get()->IsA(UMovieSceneCameraCutTrack::StaticClass()))
	{
		return FReply::Unhandled();
	}

	TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();

	if (!Operation.IsValid() || !Operation->IsOfType<FActorDragDropGraphEdOp>() )
	{
		return FReply::Unhandled();
	}
	
	TSharedPtr<FActorDragDropGraphEdOp> DragDropOp = StaticCastSharedPtr<FActorDragDropGraphEdOp>( Operation );

	FMovieSceneTrackEditor::BeginKeying(DragDropParams.FrameNumber);

	bool bAnyDropped = false;
	for (auto& ActorPtr : DragDropOp->Actors)
	{
		if (ActorPtr.IsValid())
		{
			AActor* Actor = ActorPtr.Get();
				
			FGuid ObjectGuid = FindOrCreateHandleToObject(Actor).Handle;
	
			if (ObjectGuid.IsValid())
			{
				//AnimatablePropertyChanged(FOnKeyProperty::CreateRaw(this, &FJointMovieTrackEditor::AddKeyInternal, ObjectGuid));
				
				bAnyDropped = true;
			}
		}
	}

	FMovieSceneTrackEditor::EndKeying();
	
	return bAnyDropped ? FReply::Handled() : FReply::Unhandled();
	*/

	return FReply::Unhandled();
}

void FJointMovieTrackEditor::AddJointMovieTrackMenuExtension(FMenuBuilder& MenuBuilder)
{
	TSet<UObject*> ExistingPossessedObjects;
	if (GetSequencer().IsValid())
	{
		UMovieSceneSequence* MovieSceneSequence = GetSequencer()->GetFocusedMovieSceneSequence();
		UMovieScene* MovieScene = MovieSceneSequence->GetMovieScene();
		if (MovieScene)
		{
			for (int32 Index = 0; Index < MovieScene->GetPossessableCount(); Index++)
			{
				FMovieScenePossessable& Possessable = MovieScene->GetPossessable(Index);
#if UE_VERSION_OLDER_THAN(5,5,0)
				// A possession guid can apply to more than one object, so we get all bound objects for the GUID and add them to our set.
				ExistingPossessedObjects.Append(MovieSceneSequence->LocateBoundObjects(Possessable.GetGuid(), GetSequencer()->GetPlaybackContext()));
#else
				
				TWeakPtr<ISequencer> InSequencer = GetSequencer();
				
				TArray<UObject*, TInlineAllocator<1>> BoundObjects;
				MovieSceneSequence->LocateBoundObjects(
					Possessable.GetGuid(),
					UE::UniversalObjectLocator::FResolveParams(InSequencer.Pin()->GetPlaybackContext()),
					InSequencer.Pin()->GetSharedPlaybackState(),
					BoundObjects);

				// A possession guid can apply to more than one object, so we get all bound objects for the GUID and add them to our set.
				ExistingPossessedObjects.Append(BoundObjects);
#endif
			}
		}
	}

	auto IsActorValidForPossession = [=](const AActor* InActor, TSet<UObject*> InPossessedObjectSet)
	{
		return !InPossessedObjectSet.Contains((UObject*)InActor);
	};

	// Set up a menu entry to add the selected actor(s) to the sequencer
	TArray<AActor*> ActorsValidForPossession;

	// TODO : DO SHITS HERE

	MenuBuilder.BeginSection("ChooseJointManagerSection", LOCTEXT("ChooseJointManager", "Choose Joint Manager:"));

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

#if UE_VERSION_OLDER_THAN(5,1,0)
	TArray<FName> ClassNames;
	ClassNames.Add(UJointManager::StaticClass()->GetFName());

	TSet<FName> DerivedClassNames;
	AssetRegistryModule.Get().GetDerivedClassNames(ClassNames, TSet<FName>(), DerivedClassNames);
#else
	TArray<FTopLevelAssetPath> ClassNames;
	ClassNames.Add(UJointManager::StaticClass()->GetClassPathName());

	TSet<FTopLevelAssetPath> DerivedClassNames;
	AssetRegistryModule.Get().GetDerivedClassNames(ClassNames, TSet<FTopLevelAssetPath>(), DerivedClassNames);
#endif
	
	FAssetPickerConfig AssetPickerConfig;
	{
		AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateRaw(this, &FJointMovieTrackEditor::OnJointManagerTrackAssetSelected);
		AssetPickerConfig.OnAssetEnterPressed = FOnAssetEnterPressed::CreateRaw(this, &FJointMovieTrackEditor::OnJointManagerTrackAssetPressed);
		AssetPickerConfig.bAllowNullSelection = false;
		AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
		for (auto ClassName : DerivedClassNames)
		{
#if UE_VERSION_OLDER_THAN(5,1,0)
			AssetPickerConfig.Filter.ClassNames.Add(ClassName);
#else
			AssetPickerConfig.Filter.ClassPaths.Add(ClassName);
#endif
			
		}
		AssetPickerConfig.SaveSettingsName = TEXT("SequencerAssetPicker");
	}

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	TSharedPtr<SBox> MenuEntry = SNew(SBox)
		.WidthOverride(300.0f)
		.HeightOverride(300.f)
		[
			ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
		];

	MenuBuilder.AddWidget(MenuEntry.ToSharedRef(), FText::GetEmpty(), true);
	MenuBuilder.EndSection();
}


void FJointMovieTrackEditor::OnJointManagerTrackAssetSelected(const FAssetData& AssetData)
{
	FSlateApplication::Get().DismissAllMenus();

	UObject* SelectedObject = AssetData.GetAsset();

	if (SelectedObject)
	{
		UJointManager* NewManager = CastChecked<UJointManager>(AssetData.GetAsset());
		if (NewManager != nullptr)
		{
			auto CreateNewSection = [this, NewManager](FFrameNumber KeyTime)
			{
				FKeyPropertyResult KeyPropertyResult;

				UMovieSceneSection* NewSection = FindOrCreateJointTrackFor(NewManager)->AddNewSection(nullptr, KeyTime);
				KeyPropertyResult.bTrackModified = true;
				KeyPropertyResult.SectionsCreated.Add(NewSection);

				GetSequencer()->EmptySelection();
				GetSequencer()->SelectSection(NewSection);
				GetSequencer()->ThrobSectionSelection();
		
				return KeyPropertyResult;
			};
			
			
			AnimatablePropertyChanged(FOnKeyProperty::CreateLambda(CreateNewSection));
		}
	}
}



void FJointMovieTrackEditor::OnJointManagerTrackAssetPressed(const TArray<FAssetData>& AssetDatas)
{
	if (AssetDatas.Num() > 0)
	{
		OnJointManagerBindingAssetSelected(AssetDatas[0].GetAsset());
	}
}

void FJointMovieTrackEditor::AddJointActorMenuExtension(FMenuBuilder& MenuBuilder)
{
	TSet<UObject*> ExistingPossessedObjects;
	if (GetSequencer().IsValid())
	{
		UMovieSceneSequence* MovieSceneSequence = GetSequencer()->GetFocusedMovieSceneSequence();
		UMovieScene* MovieScene = MovieSceneSequence->GetMovieScene();
		if (MovieScene)
		{
			for (int32 Index = 0; Index < MovieScene->GetPossessableCount(); Index++)
			{
				FMovieScenePossessable& Possessable = MovieScene->GetPossessable(Index);

				// A possession guid can apply to more than one object, so we get all bound objects for the GUID and add them to our set.
#if UE_VERSION_OLDER_THAN(5,5,0)
				ExistingPossessedObjects.Append(MovieSceneSequence->LocateBoundObjects(Possessable.GetGuid(), GetSequencer()->GetPlaybackContext()));
#else
				
				TWeakPtr<ISequencer> InSequencer = GetSequencer();
				
				TArray<UObject*, TInlineAllocator<1>> BoundObjects;
				MovieSceneSequence->LocateBoundObjects(
					Possessable.GetGuid(),
					UE::UniversalObjectLocator::FResolveParams(InSequencer.Pin()->GetPlaybackContext()),
					InSequencer.Pin()->GetSharedPlaybackState(),
					BoundObjects);

				// A possession guid can apply to more than one object, so we get all bound objects for the GUID and add them to our set.
				ExistingPossessedObjects.Append(BoundObjects);
#endif
			}
		}
	}

	auto IsActorValidForPossession = [=](const AActor* InActor, TSet<UObject*> InPossessedObjectSet)
	{
		return !InPossessedObjectSet.Contains((UObject*)InActor);
	};

	// Set up a menu entry to add the selected actor(s) to the sequencer
	TArray<AActor*> ActorsValidForPossession;

	// TODO : DO SHITS HERE

	MenuBuilder.BeginSection("ChooseJointManagerSection", LOCTEXT("ChooseJointManager", "Choose Joint Manager:"));

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	
#if UE_VERSION_OLDER_THAN(5,1,0)
	TArray<FName> ClassNames;
	ClassNames.Add(UJointManager::StaticClass()->GetFName());

	TSet<FName> DerivedClassNames;
	AssetRegistryModule.Get().GetDerivedClassNames(ClassNames, TSet<FName>(), DerivedClassNames);
#else
	TArray<FTopLevelAssetPath> ClassNames;
	ClassNames.Add(UJointManager::StaticClass()->GetClassPathName());

	TSet<FTopLevelAssetPath> DerivedClassNames;
	AssetRegistryModule.Get().GetDerivedClassNames(ClassNames, TSet<FTopLevelAssetPath>(), DerivedClassNames);
#endif
	
	FAssetPickerConfig AssetPickerConfig;
	{
		AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateRaw(this, &FJointMovieTrackEditor::OnJointManagerBindingAssetSelected);
		AssetPickerConfig.OnAssetEnterPressed = FOnAssetEnterPressed::CreateRaw(this, &FJointMovieTrackEditor::OnJointManagerBindingAssetPressed);
		AssetPickerConfig.bAllowNullSelection = false;
		AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
		for (auto ClassName : DerivedClassNames)
		{
#if UE_VERSION_OLDER_THAN(5,1,0)
			AssetPickerConfig.Filter.ClassNames.Add(ClassName);
#else
			AssetPickerConfig.Filter.ClassPaths.Add(ClassName);
#endif
		}
		AssetPickerConfig.SaveSettingsName = TEXT("SequencerAssetPicker");
	}

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	TSharedPtr<SBox> MenuEntry = SNew(SBox)
		.WidthOverride(300.0f)
		.HeightOverride(300.f)
		[
			ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
		];

	MenuBuilder.AddWidget(MenuEntry.ToSharedRef(), FText::GetEmpty(), true);
	MenuBuilder.EndSection();
}


UMovieSceneJointTrack* FJointMovieTrackEditor::FindOrCreateJointTrackFor(UJointManager* NewManager)
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();
	if (FocusedMovieScene->IsReadOnly())
	{
		return nullptr;
	}

#if UE_VERSION_OLDER_THAN(5,2,0)

	TArray<UMovieSceneTrack*> Tracks = FocusedMovieScene->GetMasterTracks();

#else
	
	TArray<UMovieSceneTrack*> Tracks = FocusedMovieScene->GetTracks();

#endif

	// See if we already have a Joint Track for the Joint Manager
	UMovieSceneTrack* JointTrack = nullptr;
	
	for (UMovieSceneTrack* Track : Tracks)
	{
		if (Track == nullptr) continue;
		
		if (UMovieSceneJointTrack* TestJointTrack = Cast<UMovieSceneJointTrack>(Track); 
			TestJointTrack && TestJointTrack->GetJointManager() == NewManager)
		{
			JointTrack = TestJointTrack;
			break;
		}
	}

	if (JointTrack == nullptr)
	{
		const FScopedTransaction Transaction(LOCTEXT("AddJointMovieTrack_Transaction", "Add Joint Movie Track"));
		FocusedMovieScene->Modify();

#if UE_VERSION_OLDER_THAN(5,2,0)

		JointTrack = FocusedMovieScene->AddMasterTrack<UMovieSceneJointTrack>();
		ensure(JointTrack);

#else
		
		JointTrack = FocusedMovieScene->AddTrack<UMovieSceneJointTrack>();
		ensure(JointTrack);

#endif

		Cast<UMovieSceneJointTrack>(JointTrack)->SetJointManager(NewManager);
	}

	return CastChecked<UMovieSceneJointTrack>(JointTrack);
}


void FJointMovieTrackEditor::OnAddSectionSubMenuSelected(UMovieSceneTrack* Track, EJointMovieSectionType SectionType)
{
	auto CreateNewSection = [this, Track, SectionType](FFrameNumber KeyTime)
	{
		FKeyPropertyResult KeyPropertyResult;

		UMovieSceneJointSection* NewSection = Cast<UMovieSceneJointTrack>(Track)->AddNewSection(nullptr, KeyTime);
		NewSection->SectionType = SectionType;
		KeyPropertyResult.bTrackModified = true;
		KeyPropertyResult.SectionsCreated.Add(NewSection);

		GetSequencer()->EmptySelection();
		GetSequencer()->SelectSection(NewSection);
		GetSequencer()->ThrobSectionSelection();
		
		return KeyPropertyResult;
	};

	AnimatablePropertyChanged(FOnKeyProperty::CreateLambda(CreateNewSection));
}


TSharedRef<SWidget> FJointMovieTrackEditor::BuildAddSectionSubMenu(UMovieSceneTrack* InTrack)
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddNewBeginPlayTrack", "Begin Play"),
		LOCTEXT("AddNewBeginPlayTrackTooltip", "Adds a new section that triggers the begin play of a node."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FJointMovieTrackEditor::OnAddSectionSubMenuSelected, InTrack, EJointMovieSectionType::BeginPlay)
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddNewActiveForRangeTrack", "Active For Range"),
		LOCTEXT("AddNewRangeTrackTooltip", "Adds a new section that starts off a node, and force it to stop after the duration."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FJointMovieTrackEditor::OnAddSectionSubMenuSelected, InTrack, EJointMovieSectionType::ActiveForRange)
		)
	);
	
	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddNewEndPlayTrack", "End Play"),
		LOCTEXT("AddNewEndPlayTrackTooltip", "Adds a new section that triggers the end play of a node."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FJointMovieTrackEditor::OnAddSectionSubMenuSelected, InTrack, EJointMovieSectionType::EndPlay)
		)
	);
	
	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddNewMarkAsPendingTrack", "Mark As Pending"),
		LOCTEXT("AddNewMarkAsPendingTrackTooltip", "Adds a new section that marks the node as pending."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FJointMovieTrackEditor::OnAddSectionSubMenuSelected, InTrack, EJointMovieSectionType::MarkAsPending)
		)
	);
	
	return MenuBuilder.MakeWidget();
}


FMovieSceneBinding* FJointMovieTrackEditor::AddJointManagerBinding(UJointManager* NewManager)
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();

	if (FocusedMovieScene == nullptr || NewManager == nullptr) return nullptr;
	if (FocusedMovieScene->IsReadOnly()) return nullptr;

	const FScopedTransaction Transaction(NSLOCTEXT("Sequencer", "AddJointSpawnable_Transaction", "Add Joint Actor Spawnable"));
	FocusedMovieScene->Modify();

	FMovieSceneBinding* NewBinding = nullptr;

	FGuid SpawnableGuid = FocusedMovieScene->AddSpawnable(NewManager->GetName(), *NewManager);
	NewBinding = FocusedMovieScene->FindBinding(SpawnableGuid);

	return NewBinding;
}

void FJointMovieTrackEditor::OnJointManagerBindingAssetSelected(const FAssetData& AssetData)
{
	FSlateApplication::Get().DismissAllMenus();

	UObject* SelectedObject = AssetData.GetAsset();

	if (SelectedObject)
	{
		UJointManager* NewManager = CastChecked<UJointManager>(AssetData.GetAsset());
		if (NewManager != nullptr)
		{
			auto JointTrack = AddJointManagerBinding(NewManager);

			GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
		}
	}
}

void FJointMovieTrackEditor::OnJointManagerBindingAssetPressed(const TArray<FAssetData>& AssetDatas)
{
	if (AssetDatas.Num() > 0)
	{
		OnJointManagerBindingAssetSelected(AssetDatas[0].GetAsset());
	}
}


#undef LOCTEXT_NAMESPACE
