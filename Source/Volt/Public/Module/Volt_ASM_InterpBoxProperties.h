//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoltDecl.h"
#include "VoltModuleItem.h"
#include "Kismet/KismetMathLibrary.h"
#include "Shared/VoltSharedTypes.h"
#include "Slate/WidgetTransform.h"
#include "Volt_ASM_InterpBoxProperties.generated.h"

UCLASS(EditInlineNew, Blueprintable, BlueprintType)
class VOLT_API UVolt_ASM_InterpBoxProperties : public UVoltModuleItem
{
	
	GENERATED_BODY()

public:

	VOLT_MODULE_BEGIN_ARGS(UVolt_ASM_InterpBoxProperties) :
		_InterpolationMode(EVoltInterpMode::RateBased),

		_StartHeightOverride(0),
		_StartWidthOverride(0),
		_StartMinDesiredWidth(0),
		_StartMinDesiredHeight(0),
		_StartMaxDesiredWidth(0),
		_StartMaxDesiredHeight(0),
		_StartMinAspectRatio(0),
		_StartMaxAspectRatio(0),
		
		_TargetHeightOverride(0),
		_TargetWidthOverride(0),
		_TargetMinDesiredWidth(0),
		_TargetMinDesiredHeight(0),
		_TargetMaxDesiredWidth(0),
		_TargetMaxDesiredHeight(0),
		_TargetMinAspectRatio(0),
		_TargetMaxAspectRatio(0),
	
		_bUseStartHeightOverride(false),
		_bUseStartWidthOverride(false),
		_bUseStartMinDesiredWidth(false),
		_bUseStartMinDesiredHeight(false),
		_bUseStartMaxDesiredWidth(false),
		_bUseStartMaxDesiredHeight(false),
		_bUseStartMinAspectRatio(false),
		_bUseStartMaxAspectRatio(false),

		_bOverride_WidthOverride(false),
		_bOverride_HeightOverride(false),
		_bOverride_MinDesiredWidth(false),
		_bOverride_MinDesiredHeight(false),
		_bOverride_MaxDesiredWidth(false),
		_bOverride_MaxDesiredHeight(false),
		_bOverride_MinAspectRatio(false),
		_bOverride_MaxAspectRatio(false),
	
		_RateBasedInterpSpeed(5),
		_bRateBasedUseConstant(false),
		_bRateBasedNeverFinish(false),
		
		_AlphaBasedEasingFunction(EEasingFunc::Linear),
		_AlphaBasedDuration(1),
		_AlphaBasedBlendExp(2),
		_AlphaBasedSteps(2)
	{}
		VOLT_MODULE_ARGUMENT( EVoltInterpMode, InterpolationMode )
	
		VOLT_MODULE_ARGUMENT( float, StartHeightOverride )
		VOLT_MODULE_ARGUMENT( float, StartWidthOverride )
		VOLT_MODULE_ARGUMENT( float, StartMinDesiredWidth )
		VOLT_MODULE_ARGUMENT( float, StartMinDesiredHeight )
		VOLT_MODULE_ARGUMENT( float, StartMaxDesiredWidth )
		VOLT_MODULE_ARGUMENT( float, StartMaxDesiredHeight )
		VOLT_MODULE_ARGUMENT( float, StartMinAspectRatio )
		VOLT_MODULE_ARGUMENT( float, StartMaxAspectRatio )
	
		VOLT_MODULE_ARGUMENT( float, TargetHeightOverride )
		VOLT_MODULE_ARGUMENT( float, TargetWidthOverride )
		VOLT_MODULE_ARGUMENT( float, TargetMinDesiredWidth )
		VOLT_MODULE_ARGUMENT( float, TargetMinDesiredHeight )
		VOLT_MODULE_ARGUMENT( float, TargetMaxDesiredWidth )
		VOLT_MODULE_ARGUMENT( float, TargetMaxDesiredHeight )
		VOLT_MODULE_ARGUMENT( float, TargetMinAspectRatio )
		VOLT_MODULE_ARGUMENT( float, TargetMaxAspectRatio )
	
		VOLT_MODULE_ARGUMENT( bool, bUseStartHeightOverride )
		VOLT_MODULE_ARGUMENT( bool, bUseStartWidthOverride )
		VOLT_MODULE_ARGUMENT( bool, bUseStartMinDesiredWidth )
		VOLT_MODULE_ARGUMENT( bool, bUseStartMinDesiredHeight )
		VOLT_MODULE_ARGUMENT( bool, bUseStartMaxDesiredWidth )
		VOLT_MODULE_ARGUMENT( bool, bUseStartMaxDesiredHeight )
		VOLT_MODULE_ARGUMENT( bool, bUseStartMinAspectRatio )
		VOLT_MODULE_ARGUMENT( bool, bUseStartMaxAspectRatio )
			
		VOLT_MODULE_ARGUMENT( bool, bOverride_WidthOverride )
		VOLT_MODULE_ARGUMENT( bool, bOverride_HeightOverride )
		VOLT_MODULE_ARGUMENT( bool, bOverride_MinDesiredWidth )
		VOLT_MODULE_ARGUMENT( bool, bOverride_MinDesiredHeight )
		VOLT_MODULE_ARGUMENT( bool, bOverride_MaxDesiredWidth )
		VOLT_MODULE_ARGUMENT( bool, bOverride_MaxDesiredHeight )
		VOLT_MODULE_ARGUMENT( bool, bOverride_MinAspectRatio )
		VOLT_MODULE_ARGUMENT( bool, bOverride_MaxAspectRatio )
	
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
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float StartHeightOverride;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float StartWidthOverride;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float StartMinDesiredWidth;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float StartMinDesiredHeight;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float StartMaxDesiredWidth;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float StartMaxDesiredHeight;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float StartMinAspectRatio;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float StartMaxAspectRatio;

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float TargetHeightOverride;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float TargetWidthOverride;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float TargetMinDesiredWidth;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float TargetMinDesiredHeight;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float TargetMaxDesiredWidth;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float TargetMaxDesiredHeight;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float TargetMinAspectRatio;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	float TargetMaxAspectRatio;

public:
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bUseStartHeightOverride = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bUseStartWidthOverride = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bUseStartMinDesiredWidth = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bUseStartMinDesiredHeight = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bUseStartMaxDesiredWidth = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bUseStartMaxDesiredHeight = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bUseStartMinAspectRatio = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bUseStartMaxAspectRatio = false;

public:
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bOverride_WidthOverride = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bOverride_HeightOverride = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bOverride_MinDesiredWidth = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bOverride_MinDesiredHeight = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bOverride_MaxDesiredWidth = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bOverride_MaxDesiredHeight = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bOverride_MinAspectRatio = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Module Property")
	bool bOverride_MaxAspectRatio = false;
	
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