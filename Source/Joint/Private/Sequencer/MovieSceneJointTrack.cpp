// Fill out your copyright notice in the Description page of Project Settings.


#include "Sequencer/MovieSceneJointTrack.h"

#include "JointManager.h"

#include "Sequencer/MovieSceneJointSection.h"
#include "Sequencer/MovieSceneJointSectionTemplate.h"
#include "MovieScene.h"
#include "Evaluation/MovieSceneEvalTemplate.h"


#define LOCTEXT_NAMESPACE "UMovieSceneJointTrack"

UMovieSceneJointTrack::UMovieSceneJointTrack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	TrackTint = FColor::Turquoise;
	TrackTint.A = 80;
	bSupportsDefaultSections = false;
#endif
	
	//SupportedBlendTypes.Add(EMovieSceneBlendType::);

	EvalOptions.bEvaluateNearestSection_DEPRECATED = EvalOptions.bCanEvaluateNearestSection = true;
	DisplayOptions.bShowVerticalFrames = false;
}

bool UMovieSceneJointTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
	return SectionClass == UMovieSceneJointSection::StaticClass();
}

void UMovieSceneJointTrack::RemoveAllAnimationData()
{
	Sections.Empty();
}

bool UMovieSceneJointTrack::HasSection(const UMovieSceneSection& Section) const
{
	return Sections.Contains(&Section);
}

void UMovieSceneJointTrack::AddSection(UMovieSceneSection& Section)
{
	Sections.Add(&Section);
}

void UMovieSceneJointTrack::RemoveSection(UMovieSceneSection& Section)
{
	Sections.RemoveAll([&](const UMovieSceneSection* In) { return In == &Section; });
}

void UMovieSceneJointTrack::RemoveSectionAt(int32 SectionIndex)
{
	Sections.RemoveAt(SectionIndex);
}

bool UMovieSceneJointTrack::IsEmpty() const
{
	return Sections.IsEmpty();
}

const TArray<UMovieSceneSection*>& UMovieSceneJointTrack::GetAllSections() const
{
	return Sections;
}

bool UMovieSceneJointTrack::SupportsMultipleRows() const
{
	return Super::SupportsMultipleRows();
}

UMovieSceneSection* UMovieSceneJointTrack::CreateNewSection()
{
	return NewObject<UMovieSceneJointSection>(this, NAME_None, RF_Transactional);
}


FMovieSceneEvalTemplatePtr UMovieSceneJointTrack::CreateTemplateForSection(const UMovieSceneSection& InSection) const
{
	return FMovieSceneJointSectionTemplate(*CastChecked<UMovieSceneJointSection>(&InSection), *this);
}

UMovieSceneJointSection* UMovieSceneJointTrack::AddNewSection(FJointNodePointer* InJointNodePointer, FFrameNumber Time)
{
	return AddNewSectionOnRow(InJointNodePointer, Time, INDEX_NONE);
}

UMovieSceneJointSection* UMovieSceneJointTrack::AddNewSectionOnRow(FJointNodePointer* InJointNodePointer, FFrameNumber Time, int32 RowIndex)
{
	FFrameRate FrameRate = GetTypedOuter<UMovieScene>()->GetTickResolution();

	// determine initial duration
	FFrameTime DurationToUse = 1.f * FrameRate; // if all else fails, use 1 second duration

	//Todo : get duration from nodes
	
	// add the section
	UMovieSceneJointSection* NewSection = Cast<UMovieSceneJointSection>(CreateNewSection());
	Sections.Add(NewSection);
	NewSection->InitialPlacementOnRow( Sections, Time, DurationToUse.FrameNumber.Value, RowIndex );

	if (InJointNodePointer) NewSection->SetJointNodePointer(*InJointNodePointer);
	
	return NewSection;
}

UJointManager* UMovieSceneJointTrack::GetJointManager() const
{
	return JointManager;
}

void UMovieSceneJointTrack::SetJointManager(UJointManager* InJointManager)
{
	JointManager = InJointManager;

	//Update display name
	if (JointManager)
	{
		FText NewDisplayName = FText::FromString(JointManager->GetName());
		SetDisplayName(NewDisplayName);
	}
}

AJointActor* UMovieSceneJointTrack::GetJointActor() const
{
	return RuntimePlaybackActor.Get();
}

#undef LOCTEXT_NAMESPACE
