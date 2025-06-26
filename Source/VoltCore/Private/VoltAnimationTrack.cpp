//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "VoltAnimationTrack.h"

const FVoltAnimationTrack FVoltAnimationTrack::NullTrack = FVoltAnimationTrack(FGuid());

FVoltAnimationTrack::~FVoltAnimationTrack()
{
	TargetSlateInterface = nullptr;
	TargetAnimation = nullptr;
}

FVoltAnimationTrack::FVoltAnimationTrack(const FGuid& SpecificGuid): Guid(SpecificGuid)
{}

FVoltAnimationTrack::FVoltAnimationTrack(): Guid(FGuid::NewGuid())
{}

FVoltAnimationTrack::FVoltAnimationTrack(TScriptInterface<IVoltInterface> InSlateInterface,
	const TSoftObjectPtr<UVoltAnimation>& InAnimation): TargetSlateInterface(InSlateInterface),TargetAnimation(InAnimation),Guid(FGuid::NewGuid())
{}

FVoltAnimationTrack::FVoltAnimationTrack(const FVoltAnimationTrack& From): TargetSlateInterface(From.TargetSlateInterface), TargetAnimation(From.TargetAnimation), Guid(From.GetGUID())
{}

bool FVoltAnimationTrack::IsNullTrack() const
{
	return *this == NullTrack;
}
