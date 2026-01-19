// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneNameableTrack.h"
#include "Compilation/IMovieSceneTrackTemplateProducer.h"
#include "SharedType/JointSharedTypes.h"
#include "MovieSceneJointTrack.generated.h"


class UMovieSceneJointSection;
class FTrackEditorThumbnailPool;
class UJointManager;

UCLASS()
class JOINT_API UMovieSceneJointTrack
	: public UMovieSceneNameableTrack
	, public IMovieSceneTrackTemplateProducer
{
	GENERATED_UCLASS_BODY()
public:

	// UMovieSceneTrack interface
	virtual bool SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const override;
	virtual void RemoveAllAnimationData() override;
	
public:
	
	virtual bool HasSection(const UMovieSceneSection& Section) const override;
	virtual void AddSection(UMovieSceneSection& Section) override;
	virtual UMovieSceneSection* CreateNewSection() override;
	
	virtual void RemoveSection(UMovieSceneSection& Section) override;
	virtual void RemoveSectionAt(int32 SectionIndex) override;
	
	virtual bool IsEmpty() const override;
	virtual const TArray<UMovieSceneSection*>& GetAllSections() const override;
	virtual bool SupportsMultipleRows() const override;
	
public:
	
	
#if WITH_EDITORONLY_DATA
	
	virtual void SetTrackRowDisplayName(const FText& NewDisplayName, int32 TrackRowIndex) override;	
	virtual FText GetTrackRowDisplayName(int32 RowIndex) const override;

#endif 
	
public:
	
	// ~IMovieSceneTrackTemplateProducer interface
	virtual FMovieSceneEvalTemplatePtr CreateTemplateForSection(const UMovieSceneSection& InSection) const override;

#if WITH_EDITOR

public:

	UMovieSceneJointSection* AddNewSection(FJointNodePointer* InJointNodePointer, FFrameNumber Time);
	UMovieSceneJointSection* AddNewSectionOnRow(FJointNodePointer* InJointNodePointer, FFrameNumber Time, int32 RowIndex);

public:

	UJointManager* GetJointManager() const;
	void SetJointManager(UJointManager* InJointManager);
	
#endif
	
protected:

	/** All the sections in this track */
	UPROPERTY()
	TArray<TObjectPtr<UMovieSceneSection>> Sections;
	
	UPROPERTY()
	TObjectPtr<UJointManager> JointManager;

public:
	
	UFUNCTION(BlueprintPure, Category="Runtime")
	AJointActor* GetRuntimeJointActor() const;
	
	/**
	 * The Joint Actor used for runtime playback.
	 */
	UPROPERTY(Transient, BlueprintReadWrite, Category="Runtime", meta=(AllowPrivateAccess="true"))
	TObjectPtr<AJointActor> RuntimePlaybackActor;
	
#if WITH_EDITORONLY_DATA
	
public:

	/**
	 * Get the height of this track's rows
	 */
	int32 GetRowHeight() const
	{
		return RowHeight;
	}

	/**
	 * Set the height of this track's rows
	 */
	void SetRowHeight(int32 NewRowHeight)
	{
		RowHeight = FMath::Max(16, NewRowHeight);
	}

private:

	/** The height for each row of this track */
	UPROPERTY()
	int32 RowHeight;
	
#endif
	
};
