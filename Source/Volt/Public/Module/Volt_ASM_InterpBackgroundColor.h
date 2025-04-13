//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoltDecl.h"
#include "VoltModuleItem.h"
#include "Kismet/KismetMathLibrary.h"
#include "Layout/Margin.h"
#include "Shared/VoltSharedTypes.h"
#include "Volt_ASM_InterpBackgroundColor.generated.h"

UCLASS(EditInlineNew, Blueprintable, BlueprintType)
class VOLT_API UVolt_ASM_InterpBackgroundColor : public UVoltModuleItem
{
	
	GENERATED_BODY()

public:

	VOLT_MODULE_BEGIN_ARGS(UVolt_ASM_InterpBackgroundColor) :
		_InterpolationMode(EVoltInterpMode::RateBased),
	
		_StartColor(FLinearColor::Black),
		_bUseStartColor(false),
		_TargetColor(FLinearColor::Black),
	
		_RateBasedInterpSpeed(5),
		_bRateBasedUseConstant(false),
		_bRateBasedNeverFinish(false),

		_AlphaBasedEasingFunction(EEasingFunc::Linear),
		_AlphaBasedDuration(-1),
		_AlphaBasedBlendExp(2),
		_AlphaBasedSteps(2)
	{}
		VOLT_MODULE_ARGUMENT( EVoltInterpMode, InterpolationMode )
			
		VOLT_MODULE_ARGUMENT( FLinearColor, StartColor )
		VOLT_MODULE_ARGUMENT( bool, bUseStartColor )
		VOLT_MODULE_ARGUMENT( FLinearColor, TargetColor )

		VOLT_MODULE_ARGUMENT( float, RateBasedInterpSpeed )
		VOLT_MODULE_ARGUMENT( bool, bRateBasedUseConstant )
		VOLT_MODULE_ARGUMENT( bool, bRateBasedNeverFinish )

		VOLT_MODULE_ARGUMENT( TEnumAsByte<EEasingFunc::Type>, AlphaBasedEasingFunction )
		VOLT_MODULE_ARGUMENT( double, AlphaBasedDuration )
		VOLT_MODULE_ARGUMENT( double, AlphaBasedBlendExp )
		VOLT_MODULE_ARGUMENT( int32, AlphaBasedSteps )
	
	VOLT_MODULE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

public:

	virtual void ModifySlateVariable(const float DeltaTime, const TScriptInterface<IVoltInterface>& Volt) override;

public:

	virtual void OnModuleBeginPlay_Implementation() override;
	
	virtual void OnModuleEndPlay_Implementation() override;
	
	virtual bool IsActive() override;

public:

	/**
	 * The interpolation mode of the module.
	 * EVoltInterpMode::RateBased: Interpolate animation with specified update rate. This mode is useful when you don't know the duration or alpha of the interpolation, but it supports only 2 interp functions (const, linear).
	 * EVoltInterpMode::AlphaBased: Interpolate animation with specified alpha (or by duration) between A to B. This mode is useful when you want to animate something with specific duration. This mode supports much more interp functions (16~) but you must take more care on the animation duration.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Action")
	EVoltInterpMode InterpolationMode = EVoltInterpMode::RateBased;

public:

	/**
	 * Start Color of the animation. The color of the slate will be overrided with it if bUseStartColor was true.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Common")
	FLinearColor StartColor;

	/**
	 * Whether to override the slate color with StartColor when the module has been started.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Common")
	bool bUseStartColor = false;

	/**
	 * Target color to interpolate to.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Common")
	FLinearColor TargetColor;
	
public:
	
	//RateBased Relate
	
	/**
	 * RateBased interpolation speed (rate). bigger value means faster interp.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RateBased")
	float RateBasedInterpSpeed = 5;

	/**
	 * If true, it will use FInterpConstantTo instead of FInterpTo.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RateBased")
	bool bRateBasedUseConstant = false;

	/**
	 * If true, it will never stop updating even if it reaches the target value!
	 * You must stop the module manually by executing Stop() or setting bNeverFinish to true again.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RateBased")
	bool bRateBasedNeverFinish = false;
	
	
public:

	//AlphaBased
	
	/**
	 * Easing function to use on the interpolation.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AlphaBased")
	TEnumAsByte<EEasingFunc::Type> AlphaBasedEasingFunction = EEasingFunc::Linear;

	/**
	 * Alpha of the interpolation. You can also manually update the alpha externally.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AlphaBased")
	double AlphaBasedEaseAlpha = 0;
	
	/**
	 * Duration of the interpolation.
	 * If not specified (-1), it will not do anything. You must update the alpha manually.
	 * Please notice when you update the alpha manually, you must stop the module's update manually.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AlphaBased")
	double AlphaBasedDuration = -1;

	/**
	 * BlendExp of the interpolation. (used for EEasingFunc::EaseIn, EEasingFunc::EaseOut, EEasingFunc::EaseInOut)
	 * If not specified (-1), it will not do anything. You must update the alpha manually.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AlphaBased")
	double AlphaBasedBlendExp = 2;

	/**
	 * Steps of the interpolation. It's used for EEasingFunc::Step.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AlphaBased")
	int32 AlphaBasedSteps = 2;
	
private:
	
	/**
	 * For the duration calculation.
	 */
	UPROPERTY(Transient)
	double AccumulatedTime = 0;
	
};