// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "Templates/SubclassOf.h"
#include "Widgets/SWidget.h"
#include "ISequencer.h"
#include "MovieSceneTrack.h"
#include "ISequencerSection.h"
#include "ISequencerTrackEditor.h"
#include "MovieSceneTrackEditor.h"
#include "Sequencer/MovieSceneJointSection.h"

class UJointManager;
class UMovieSceneJointTrack;
class AActor;
class FMenuBuilder;
class FTrackEditorThumbnailPool;
class UFactory;
class UMovieSceneCameraCutTrack;
class FCameraCutTrackEditor;
class FTrackEditorBindingIDPicker;
struct FMovieSceneObjectBindingID;
class FCameraCutSection;

/**
 * Tools for camera cut tracks.
 */
class JOINTEDITOR_API FJointMovieTrackEditor
	: public FMovieSceneTrackEditor
{
public:

	/**
	 * Constructor
	 *
	 * @param InSequencer The sequencer instance to be used by this tool.
	 */
	FJointMovieTrackEditor(TSharedRef<ISequencer> InSequencer);

	/** Virtual destructor. */
	virtual ~FJointMovieTrackEditor() { }

	/**
	 * Creates an instance of this class.  Called by a sequencer .
	 *
	 * @param OwningSequencer The sequencer instance to be used by this tool.
	 * @return The new instance of this class.
	 */
	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer);

public:

	// ISequencerTrackEditor interface

	virtual void BindCommands(TSharedRef<FUICommandList> SequencerCommandBindings) override;
	virtual void BuildAddTrackMenu(FMenuBuilder& MenuBuilder) override;
	virtual TSharedPtr<SWidget> BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params) override;
	virtual TSharedRef<ISequencerSection> MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding) override;
	virtual void OnRelease() override;
	virtual bool SupportsType(TSubclassOf<UMovieSceneTrack> Type) const override;
	virtual bool SupportsSequence(UMovieSceneSequence* InSequence) const override;
	virtual const FSlateBrush* GetIconBrush() const override;
	
	
	virtual bool IsResizable(UMovieSceneTrack* InTrack) const override;
	virtual void Resize(float NewSize, UMovieSceneTrack* InTrack) override;
	
	virtual bool OnAllowDrop(const FDragDropEvent& DragDropEvent, FSequencerDragDropParams& DragDropParams) override;
	virtual FReply OnDrop(const FDragDropEvent& DragDropEvent, const FSequencerDragDropParams& DragDropParams) override;

public:
	
	void AddJointMovieTrackMenuExtension(FMenuBuilder& MenuBuilder);
	
	// Track Addition
	void OnJointManagerTrackAssetSelected(const FAssetData& AssetData);
	void OnJointManagerTrackAssetPressed(const TArray<FAssetData>& AssetDatas);
	
	void AddJointActorMenuExtension(FMenuBuilder& MenuBuilder);
	UMovieSceneJointTrack* FindOrCreateJointTrackFor(UJointManager* NewManager);

public:

	// Binding
	
	FMovieSceneBinding* AddJointManagerBinding(UJointManager* NewManager);
	
	void OnJointManagerBindingAssetSelected(const FAssetData& AssetData);
	void OnJointManagerBindingAssetPressed(const TArray<FAssetData>& AssetDatas);
	
public:
	
	void CreateNewSection(UMovieSceneTrack* Track, int32 RowIndex, UClass* SectionType, EJointMovieSectionType JointSectionType, bool bSelect);

};
