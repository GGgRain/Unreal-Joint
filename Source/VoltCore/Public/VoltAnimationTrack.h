//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "VoltAnimationTrack.generated.h"

class IVoltInterface;
class UVoltAnimation;

/**
 * The animation track struct. It's for the storage action.
 * This struct use its Guid for the identification. If this Guid is same with the other track instance, then both will be recognized as the same tracks.
 */
USTRUCT(BlueprintType)
struct VOLTCORE_API FVoltAnimationTrack
{
	GENERATED_BODY()

public:
	
	virtual ~FVoltAnimationTrack();

public:
	
	FVoltAnimationTrack(const FGuid& SpecificGuid);

	FVoltAnimationTrack();

	FVoltAnimationTrack(TScriptInterface<IVoltInterface> InSlateInterface, const TSoftObjectPtr<UVoltAnimation>& InAnimation);

	//Copy-paste its Guid since both must direct the same track.
	FVoltAnimationTrack(const FVoltAnimationTrack& From);

public:
	
	UPROPERTY(BlueprintReadWrite, Category="Animation Track")
	TScriptInterface<IVoltInterface> TargetSlateInterface = nullptr;

	UPROPERTY(BlueprintReadWrite, Category="Animation Track")
	TSoftObjectPtr<UVoltAnimation> TargetAnimation = nullptr;

public:

	/**
	 * Guid for the track. If this Guid is same with the other track than both will be recognized as the same tracks.
	 */
	UPROPERTY(meta=(IgnoreForMemberInitializationTest) /* Added to silence false warning about uninitialized property */)
	FGuid Guid = FGuid();

public:

	virtual FGuid GetGUID() const;

public:
	
	bool IsNullTrack() const;

public:

	static const FVoltAnimationTrack NullTrack;
	
};

inline FGuid FVoltAnimationTrack::GetGUID() const
{
	return Guid;
}


FORCEINLINE uint32 GetTypeHash(const FVoltAnimationTrack& Struct)
{
	return FCrc::MemCrc32(&Struct.Guid, sizeof(FGuid));
}

FORCEINLINE bool operator==(const FVoltAnimationTrack& A, const FVoltAnimationTrack& B)
{
	return A.GetGUID() == B.GetGUID();
}

FORCEINLINE bool operator!=(const FVoltAnimationTrack& A, const FVoltAnimationTrack& B)
{
	return !(A == B);
}
