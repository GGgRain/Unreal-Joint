//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Misc/EngineVersionComparison.h"

#if UE_VERSION_OLDER_THAN(5,0,0)
#include "EditorStyleSet.h"
#else
#include "Styling/AppStyle.h"
#endif

#if UE_VERSION_OLDER_THAN(5,0,0)
	#include "EditorStyleSet.h"
#else
	#include "Styling/AppStyle.h"
#endif

class JOINTEDITOR_API FJointEditorStyle
{
public:
	static TSharedRef<class ISlateStyle> Create();

	/** @return the singleton instance. */
	static const ISlateStyle& Get();

	static void ResetToDefault();

	static FName GetStyleSetName();

	/** @return the singleton instance. */
	static const ISlateStyle& GetUEEditorSlateStyleSet();

	static const FName GetUEEditorSlateStyleSetName();

private:
	static void SetStyleSet(const TSharedRef<class ISlateStyle>& NewStyle);


private:
	/** Singleton instances of this style. */
	static TSharedPtr<class ISlateStyle> Instance;

public:
	
	static const FMargin Margin_Button;
	static const FMargin Margin_Border;
	static const FMargin Margin_Tag;
	static const FMargin Margin_Shadow;
	static const FMargin Margin_Frame;
	static const FMargin Margin_Name;
	static const FMargin Margin_Pin;
	static const FMargin Margin_PinGap;
	static const FMargin Margin_Subnode;

public:
	static const FLinearColor Color_Normal;
	static const FLinearColor Color_Hover;
	static const FLinearColor Color_Selected;
	static const FLinearColor Color_Disabled;

	static const FLinearColor Color_SolidNormal;
	static const FLinearColor Color_SolidHover;
	static const FLinearColor Color_SolidSelected;


	static const FLinearColor Color_Node_Inactive;
	static const FLinearColor Color_Node_Selected;
	static const FLinearColor Color_Node_Invalid;
	static const FLinearColor Color_Node_DragMarker;
	static const FLinearColor Color_Node_Shadow;

};
