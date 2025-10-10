// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "MovieSceneSection.h"
#include "Runtime/Engine/Classes/Components/AudioComponent.h"
#include "Sound/SoundAttenuation.h"

#include "Channels/MovieSceneFloatChannel.h"
#include "Channels/MovieSceneStringChannel.h"
#include "Channels/MovieSceneBoolChannel.h"
#include "Channels/MovieSceneIntegerChannel.h"
#include "Sections/MovieSceneActorReferenceSection.h"
#include "Channels/MovieSceneAudioTriggerChannel.h"

#include "Channels/MovieSceneObjectPathChannel.h"
#include "SharedType/JointSharedTypes.h"
#include "MovieSceneJointSection.generated.h"

class UMovieSceneJointTrack;
class UJointManager;
class USoundBase;

UENUM()
enum class EJointMovieSectionType : uint8
{
	Trigger = 0 UMETA(DisplayName="Trigger"),
	Range = 1 UMETA(DisplayName="Range"),
};





USTRUCT(BlueprintType)
struct JOINT_API FJointMovieSectionPayload
{
	GENERATED_BODY()

	FJointMovieSectionPayload() : SectionType(EJointMovieSectionType::Trigger) {}

	friend bool operator==(const FJointMovieSectionPayload& A, const FJointMovieSectionPayload& B)
	{
		return A.SectionRange == B.SectionRange && A.JointNodePointer == B.JointNodePointer && A.SectionType == B.SectionType;
	}

	friend bool operator!=(const FJointMovieSectionPayload& A, const FJointMovieSectionPayload& B)
	{
		return !(A == B);
	}
	
public:
	
	TRange<FFrameNumber> SectionRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Section Payload")
	FJointNodePointer JointNodePointer;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Section Payload")
	EJointMovieSectionType SectionType;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Section Payload")
	TWeakObjectPtr<const UMovieSceneJointTrack> ParentTrack = nullptr;

public:

	UPROPERTY()
	mutable bool bEverTriggered = false;
	
};



/**
 * An object for the section of a UMovieSceneJointTrack -
 */
UCLASS()
class JOINT_API UMovieSceneJointSection
	: public UMovieSceneSection
{
	GENERATED_UCLASS_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="Sequencer|Section")
	void SetJointNodePointer(FJointNodePointer InJointNodePointer);

	UFUNCTION(BlueprintPure, Category = "Sequencer|Section")
	FJointNodePointer GetJointNodePointer() const;

	UFUNCTION(BlueprintCallable, Category="Sequencer|Section")
	void SetJointMovieSectionType(EJointMovieSectionType InSectionType);

	UFUNCTION(BlueprintPure, Category = "Sequencer|Section")
	EJointMovieSectionType GetJointMovieSectionType() const;
	
public:

	UMovieSceneJointTrack* GetTypedOuterJointTrack() const;
	
public:
	
	/** ~UObject interface */
	virtual void PostLoad() override;
	
public:

	//~ UMovieSceneSection interface
	virtual TOptional<TRange<FFrameNumber> > GetAutoSizeRange() const override;
	virtual void TrimSection(FQualifiedFrameTime TrimTime, bool bTrimLeft, bool bDeleteKeys) override;
	virtual UMovieSceneSection* SplitSection(FQualifiedFrameTime SplitTime, bool bDeleteKeys) override;
	virtual TOptional<FFrameTime> GetOffsetTime() const override;
	virtual void MigrateFrameTimes(FFrameRate SourceRate, FFrameRate DestinationRate) override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostEditImport() override;
	virtual EMovieSceneChannelProxyType CacheChannelProxy() override;

private:
	
	template<typename ChannelType, typename ForEachFunction>
	FORCEINLINE static void ForEachInternal(ForEachFunction InFuncton, const TMap<FName, ChannelType>& InMapToIterate) 
	{
		for (auto& Item : InMapToIterate)
		{
			InFuncton(Item.Key, Item.Value);
		}	
	}

public:
	
	UPROPERTY(EditAnywhere, Category="JointManager")
	FJointNodePointer NodePointer;
	
public:

	/** Number of frames (in display rate) to skip at the beginning of the sub-sequence. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Clipping")
	FFrameNumber StartFrameOffset;

public:

	/* The section type. determine the behavior and look of the section. */
	UPROPERTY(EditAnywhere, Category="Joint")
	EJointMovieSectionType SectionType;
	
};
