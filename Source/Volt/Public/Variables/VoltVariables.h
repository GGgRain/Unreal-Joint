//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoltDecl.h"
#include "VoltVariableBase.h"
#include "Layout/Margin.h"
#include "Slate/WidgetTransform.h"
#include "VoltVariables.generated.h"

/**
 * Basic slate render related variable. override UVoltVariableBase to create a new variable.
 */

/**
 * A variable that affects the opacity of the slate.
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVar_Opacity : public UVoltVariableBase
{
	GENERATED_BODY()

public:
	UVoltVar_Opacity();

public:
	UPROPERTY(BlueprintReadWrite, Category="Slate - Opacity")
	float Value;

public:
	MACRO_DEF_GETTER_FUNCTION(float, Value)
};

/**
 * A variable that affects the widget transform of the slate.
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVar_WidgetTransform : public UVoltVariableBase
{
	GENERATED_BODY()

public:
	UVoltVar_WidgetTransform();

public:
	UPROPERTY(BlueprintReadWrite, Category="Slate - Widget Transform")
	FWidgetTransform Value;

	TOptional<FSlateRenderTransform> InternalValue;

public:
	MACRO_DEF_GETTER_FUNCTION(FWidgetTransform, Value)
};


/**
 * A variable that affects the pivot of widget transform of the slate.
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVar_WidgetTransformPivot : public UVoltVariableBase
{
	GENERATED_BODY()

public:
	UVoltVar_WidgetTransformPivot();

public:
	UPROPERTY(BlueprintReadWrite, Category="Slate - Widget Transform Pivot")
	FVector2D Value;

public:
	MACRO_DEF_GETTER_FUNCTION(FVector2D, Value)
};

/**
 * A variable that affects the color and opacity.
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVar_ColorAndOpacity : public UVoltVariableBase
{
	GENERATED_BODY()

public:
	UVoltVar_ColorAndOpacity();

public:
	UPROPERTY(BlueprintReadWrite, Category="Slate - Color")
	FLinearColor Value;

public:
	MACRO_DEF_GETTER_FUNCTION(FLinearColor, Value)
};

/**
 * A variable that affects the background color of the border.
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVar_BackgroundColor : public UVoltVariableBase
{
	GENERATED_BODY()

public:
	UVoltVar_BackgroundColor();

public:
	UPROPERTY(BlueprintReadWrite, Category="Slate - Color")
	FLinearColor Value;

public:
	MACRO_DEF_GETTER_FUNCTION(FLinearColor, Value)
};

/**
 * A variable that affects the Foreground color of the border.
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVar_ForegroundColor : public UVoltVariableBase
{
	GENERATED_BODY()

public:
	UVoltVar_ForegroundColor();

public:
	UPROPERTY(BlueprintReadWrite, Category="Slate - Color")
	FLinearColor Value;

public:
	MACRO_DEF_GETTER_FUNCTION(FLinearColor, Value)
};


/**
 * A variable that affects the padding of the slate's child slots.
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVar_ChildSlotPadding : public UVoltVariableBase
{
	GENERATED_BODY()

public:
	UVoltVar_ChildSlotPadding();

public:
	UPROPERTY(BlueprintReadWrite, Category="Slate - Slot Padding")
	FMargin Value;

public:
	MACRO_DEF_GETTER_FUNCTION(FMargin, Value)
};

/**
 * A variable that affects the padding of the slot that contains the animated slate.
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVar_ParentSlotPadding : public UVoltVariableBase
{
	GENERATED_BODY()

public:
	UVoltVar_ParentSlotPadding();

public:
	UPROPERTY(BlueprintReadWrite, Category="Slate - Slot Padding")
	FMargin Value;

public:
	MACRO_DEF_GETTER_FUNCTION(FMargin, Value)
};

/**
 * A variable that affects the box size setting of SBox.
 */
UCLASS(BlueprintType)
class VOLT_API UVoltVar_Box : public UVoltVariableBase
{
	GENERATED_BODY()

public:
	UVoltVar_Box();

public:
	/** When specified, ignore the content's desired size and report the WidthOverride as the Box's desired width. */
	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	double WidthOverride = 0;

	/** When specified, ignore the content's desired size and report the HeightOverride as the Box's desired height. */
	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	double HeightOverride = 0;

	/** When specified, will report the MinDesiredWidth if larger than the content's desired width. */
	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	double MinDesiredWidth = 0;

	/** When specified, will report the MinDesiredHeight if larger than the content's desired height. */
	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	double MinDesiredHeight = 0;

	/** When specified, will report the MaxDesiredWidth if smaller than the content's desired width. */
	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	double MaxDesiredWidth = 0;

	/** When specified, will report the MaxDesiredHeight if smaller than the content's desired height. */
	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	double MaxDesiredHeight = 0;

	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	double MinAspectRatio = 0;

	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	double MaxAspectRatio = 0;


	/**  */
	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	bool bOverride_WidthOverride = false;

	/**  */
	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	bool bOverride_HeightOverride = false;

	/**  */
	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	bool bOverride_MinDesiredWidth = false;

	/**  */
	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	bool bOverride_MinDesiredHeight = false;

	/**  */
	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	bool bOverride_MaxDesiredWidth = false;

	/**  */
	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	bool bOverride_MaxDesiredHeight = false;

	/**  */
	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	bool bOverride_MinAspectRatio = false;

	/**  */
	UPROPERTY(BlueprintReadWrite, Category="Slate - Box Size")
	bool bOverride_MaxAspectRatio = false;
};
