// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sequencer/MovieSceneJointSection.h"

#include "Tracks/MovieSceneAudioTrack.h"
#include "Sound/SoundBase.h"
#include "UObject/SequencerObjectVersion.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "MovieScene.h"
#include "MovieSceneCommonHelpers.h"
#include "Misc/FrameRate.h"
#include "Node/JointNodeBase.h"
#include "Sequencer/MovieSceneJointTrack.h"


#define LOCTEXT_NAMESPACE "UMovieSceneJointSection"

namespace
{
	float AudioDeprecatedMagicNumber = TNumericLimits<float>::Lowest();

	FFrameNumber GetStartOffsetAtTrimTime(FQualifiedFrameTime TrimTime, FFrameNumber StartOffset, FFrameNumber StartFrame)
	{
		return StartOffset + TrimTime.Time.FrameNumber - StartFrame;
	}
}

UMovieSceneJointSection::UMovieSceneJointSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
	//JointManager = nullptr;
	BlendType = EMovieSceneBlendType::Absolute;

	EvalOptions.EnableAndSetCompletionMode
		(GetLinkerCustomVersion(FSequencerObjectVersion::GUID) < FSequencerObjectVersion::WhenFinishedDefaultsToProjectDefault ? 
			EMovieSceneCompletionMode::RestoreState : 
			EMovieSceneCompletionMode::ProjectDefault);
	
}


void UMovieSceneJointSection::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading())
	{
		CacheChannelProxy();
	}
}

void UMovieSceneJointSection::PostEditImport()
{
	Super::PostEditImport();

	CacheChannelProxy();
}

EMovieSceneChannelProxyType  UMovieSceneJointSection::CacheChannelProxy()
{
	// Set up the channel proxy
	FMovieSceneChannelProxyData Channels;

	UMovieSceneJointTrack* JointTrack = Cast<UMovieSceneJointTrack>(GetOuter());

	//NodeTrack.SetPropertyClass(UJointNodeBase::StaticClass());
	
#if WITH_EDITOR

	//FAudioChannelEditorData EditorData;
	//Channels.Add(NodeTrack,     EditorData.Data[0], TMovieSceneExternalValue<FJointNodePointer>::Make());

	// quick fix for the overlapped layout issue - TODO: This is silly. Find a better way to handle this.
	//Channels.Add(NodeTrack,     EditorData.Data[0], TMovieSceneExternalValue<FJointNodePointer>::Make());

#else

	//Channels.Add(NodeTrack);

#endif
	
	ChannelProxy = MakeShared<FMovieSceneChannelProxy>(MoveTemp(Channels));

	return EMovieSceneChannelProxyType::Dynamic;
}

void UMovieSceneJointSection::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	// Notify that the section has changed
	OnPropertyChanged.ExecuteIfBound();
}


UMovieSceneJointTrack* UMovieSceneJointSection::GetTypedOuterJointTrack() const
{
	return Cast<UMovieSceneJointTrack>(GetOuter());
}


TOptional<FFrameTime> UMovieSceneJointSection::GetOffsetTime() const
{
	return TOptional<FFrameTime>(StartFrameOffset);
}

void UMovieSceneJointSection::MigrateFrameTimes(FFrameRate SourceRate, FFrameRate DestinationRate)
{
	if (StartFrameOffset.Value > 0)
	{
		FFrameNumber NewStartFrameOffset = ConvertFrameTime(FFrameTime(StartFrameOffset), SourceRate, DestinationRate).FloorToFrame();
		StartFrameOffset = NewStartFrameOffset;
	}
}

void UMovieSceneJointSection::PostLoad()
{
	Super::PostLoad();
}
	
TOptional<TRange<FFrameNumber> > UMovieSceneJointSection::GetAutoSizeRange() const
{

	float SoundDuration = 200; // magic number TODO
	//float SoundDuration = MovieSceneHelpers::GetSoundDuration(JointManager);

	FFrameRate FrameRate = GetTypedOuter<UMovieScene>()->GetTickResolution();

	// determine initial duration
	// @todo Once we have infinite sections, we can remove this
	// @todo ^^ Why? Infinte sections would mean there's no starting time?
	FFrameTime DurationToUse = 1.f * FrameRate; // if all else fails, use 1 second duration

	if (SoundDuration != INDEFINITELY_LOOPING_DURATION)
	{
		DurationToUse = SoundDuration * FrameRate;
	}

	return TRange<FFrameNumber>(GetInclusiveStartFrame(), GetInclusiveStartFrame() + DurationToUse.FrameNumber);
}

	
void UMovieSceneJointSection::TrimSection(FQualifiedFrameTime TrimTime, bool bTrimLeft, bool bDeleteKeys)
{
	SetFlags(RF_Transactional);

	if (TryModify())
	{
		if (bTrimLeft)
		{
			StartFrameOffset = HasStartFrame() ? GetStartOffsetAtTrimTime(TrimTime, StartFrameOffset, GetInclusiveStartFrame()) : 0;
		}

		Super::TrimSection(TrimTime, bTrimLeft, bDeleteKeys);
	}
}

UMovieSceneSection* UMovieSceneJointSection::SplitSection(FQualifiedFrameTime SplitTime, bool bDeleteKeys)
{
	const FFrameNumber InitialStartFrameOffset = StartFrameOffset;

	const FFrameNumber NewOffset = HasStartFrame() ? GetStartOffsetAtTrimTime(SplitTime, StartFrameOffset, GetInclusiveStartFrame()) : 0;

	UMovieSceneSection* NewSection = Super::SplitSection(SplitTime, bDeleteKeys);
	if (NewSection != nullptr)
	{
		UMovieSceneJointSection* NewAudioSection = Cast<UMovieSceneJointSection>(NewSection);
		NewAudioSection->StartFrameOffset = NewOffset;
	}

	// Restore original offset modified by splitting
	StartFrameOffset = InitialStartFrameOffset;

	return NewSection;
}

void UMovieSceneJointSection::SetJointNodePointer(const FJointNodePointer InJointNodePointer)
{
	NodePointer = InJointNodePointer;
	
	CacheChannelProxy();
}

FJointNodePointer UMovieSceneJointSection::GetJointNodePointer() const
{
	return NodePointer;
}

void UMovieSceneJointSection::SetJointMovieSectionType(EJointMovieSectionType InSectionType)
{
	SectionType = InSectionType;
}

EJointMovieSectionType UMovieSceneJointSection::GetJointMovieSectionType() const
{
	return SectionType;
}


#undef LOCTEXT_NAMESPACE
