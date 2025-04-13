#pragma once

#include "CoreMinimal.h"
#include "VoltSharedTypes.generated.h"

UENUM(BlueprintType)
enum class EVoltInterpMode : uint8
{
	//Interpolate animation with specified update rate. This mode is useful when you don't know the duration or alpha of the interpolation, but it supports only 2 interp functions (const, linear).
	RateBased UMETA(DisplayName="Rate Based Interpolation"),
	//Interpolate animation with specified alpha (or by duration) between A to B. This mode is useful when you want to animate something with specific duration. This mode supports much more interp functions (16~) but you must take more care on the animation duration.
	AlphaBased UMETA(DisplayName="Alpha Based Interpolation"),
};
