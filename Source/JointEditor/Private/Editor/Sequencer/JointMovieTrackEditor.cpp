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
}


TSharedPtr<SWidget> FJointMovieTrackEditor::BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params)
{
	
	TSharedPtr<ISequencer> SequencerPtr = GetSequencer();
	if (!SequencerPtr.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	TWeakObjectPtr<UMovieSceneTrack> WeakTrack = Track;
	const int32 RowIndex = Params.TrackInsertRowIndex;
	auto SubMenuCallback = [this, WeakTrack, RowIndex]
	{
		FMenuBuilder MenuBuilder(true, nullptr);

		UMovieSceneTrack* TrackPtr = WeakTrack.Get();
		if (TrackPtr)
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("AddNewBeginPlayTrack", "Begin Play"),
				LOCTEXT("AddNewBeginPlayTrackTooltip", "Adds a new section that triggers the begin play of a node."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FJointMovieTrackEditor::CreateNewSection, TrackPtr, RowIndex, UMovieSceneJointSection::StaticClass(), EJointMovieSectionType::BeginPlay, true))
			);
			
			MenuBuilder.AddMenuEntry(
				LOCTEXT("AddNewActiveForRangeTrack", "Active For Range"),
				LOCTEXT("AddNewRangeTrackTooltip", "Adds a new section that starts off a node, and force it to stop after the duration."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FJointMovieTrackEditor::CreateNewSection, TrackPtr, RowIndex, UMovieSceneJointSection::StaticClass(), EJointMovieSectionType::EndPlay, true))
			);
			
			MenuBuilder.AddMenuEntry(
				LOCTEXT("AddNewEndPlayTrack", "End Play"),
				LOCTEXT("AddNewEndPlayTrackTooltip", "Adds a new section that triggers the end play of a node."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FJointMovieTrackEditor::CreateNewSection, TrackPtr, RowIndex, UMovieSceneJointSection::StaticClass(), EJointMovieSectionType::EndPlay, true))
			);
			
			MenuBuilder.AddMenuEntry(
				LOCTEXT("AddNewMarkAsPendingTrack", "Mark As Pending"),
				LOCTEXT("AddNewMarkAsPendingTrackTooltip", "Adds a new section that marks the node as pending."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FJointMovieTrackEditor::CreateNewSection, TrackPtr, RowIndex, UMovieSceneJointSection::StaticClass(), EJointMovieSectionType::MarkAsPending, true))
			);
			
		}
		else
		{
			MenuBuilder.AddWidget(SNew(STextBlock).Text(LOCTEXT("InvalidTrack", "Track is no longer valid")), FText(), true);
		}

		return MenuBuilder.MakeWidget();
	};
	
	return SNew(SHorizontalBox)
		// Add the audio combo box
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			
			FSequencerUtilities::MakeAddButton(
				LOCTEXT("NodeSectionText", "Node Section"),
				FOnGetContent::CreateLambda(SubMenuCallback),
				true,
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


const FSlateBrush* FJointMovieTrackEditor::GetIconBrush() const
{		
	return FJointEditorStyle::Get().GetBrush("ClassIcon.JointManager");
}

bool FJointMovieTrackEditor::IsResizable(UMovieSceneTrack* InTrack) const
{
	return true;
}

void FJointMovieTrackEditor::Resize(float NewSize, UMovieSceneTrack* InTrack)
{
	if (UMovieSceneJointTrack* Track = Cast<UMovieSceneJointTrack>(InTrack))
	{
		Track->Modify();

		const int32 MaxNumRows = Track->GetMaxRowIndex() + 1;
		Track->SetRowHeight(FMath::RoundToInt(NewSize) / MaxNumRows);
	}
}

bool FJointMovieTrackEditor::OnAllowDrop(const FDragDropEvent& DragDropEvent, FSequencerDragDropParams& DragDropParams)
{
	
	return FMovieSceneTrackEditor::OnAllowDrop(DragDropEvent, DragDropParams);
}

FReply FJointMovieTrackEditor::OnDrop(const FDragDropEvent& DragDropEvent, const FSequencerDragDropParams& DragDropParams)
{
	return FMovieSceneTrackEditor::OnDrop(DragDropEvent, DragDropParams);
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
				
				GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);

		
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

void FJointMovieTrackEditor::CreateNewSection(UMovieSceneTrack* Track, int32 RowIndex, UClass* SectionType, EJointMovieSectionType JointSectionType, bool bSelect)
{
	TSharedPtr<ISequencer> SequencerPtr = GetSequencer();
	if (SequencerPtr.IsValid())
	{
		UMovieScene* FocusedMovieScene = GetFocusedMovieScene();
		FQualifiedFrameTime CurrentTime = SequencerPtr->GetLocalTime();

		FScopedTransaction Transaction(LOCTEXT("CreateNewSectionTransactionText", "Add Section"));

		UMovieSceneSection* NewSection = NewObject<UMovieSceneSection>(Track, SectionType, NAME_None, RF_Transactional);
		check(NewSection);

		int32 OverlapPriority = 0;
		for (UMovieSceneSection* Section : Track->GetAllSections())
		{
			if (Section->GetRowIndex() >= RowIndex)
			{
				Section->SetRowIndex(Section->GetRowIndex() + 1);
			}
			OverlapPriority = FMath::Max(Section->GetOverlapPriority() + 1, OverlapPriority);
		}

		Track->Modify();

		if (SectionType == UMovieSceneJointSection::StaticClass())
		{
			TRange<FFrameNumber> NewSectionRange;

			const float DefaultLengthInSeconds = 1.f;
			NewSectionRange = TRange<FFrameNumber>(CurrentTime.Time.FrameNumber, CurrentTime.Time.FrameNumber + (DefaultLengthInSeconds * SequencerPtr->GetFocusedTickResolution()).FloorToFrame());

			NewSection->SetRange(NewSectionRange);
		}
		
		if (UMovieSceneJointSection* JointSection = Cast<UMovieSceneJointSection>(NewSection))
		{
			JointSection->SectionType = JointSectionType;
		}

		NewSection->SetOverlapPriority(OverlapPriority);
		NewSection->SetRowIndex(RowIndex);

		Track->AddSection(*NewSection);
		Track->UpdateEasing();

		if (bSelect)
		{
			SequencerPtr->EmptySelection();
			SequencerPtr->SelectSection(NewSection);
			SequencerPtr->ThrobSectionSelection();
		}

		SequencerPtr->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
	}
}


#undef LOCTEXT_NAMESPACE
