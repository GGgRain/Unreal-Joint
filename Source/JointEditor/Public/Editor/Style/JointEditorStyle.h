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


#define Joint_IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define Joint_BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define Joint_BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define Joint_DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

#define Joint_AssignTextBlockStyle(Signifier, StyleName, FontName, Size, Color)\
	const FTextBlockStyle Signifier = FTextBlockStyle()\
	.SetFont(Joint_DEFAULT_FONT(FontName, Size))\
	.SetColorAndOpacity(Color)\
	.SetShadowOffset(FVector2D::ZeroVector)\
	.SetShadowColorAndOpacity(FLinearColor::Black)\
	.SetHighlightColor(FLinearColor(1.f, 1.f, 1.f));\
	Style->Set(StyleName, Signifier);

#define Joint_AssignInlineEditableTextBlockStyle(Signifier, StyleName, TextStyle)\
	const FInlineEditableTextBlockStyle Signifier = FInlineEditableTextBlockStyle()\
	.SetTextStyle(TextStyle)\
	.SetEditableTextBoxStyle(GetUEEditorSlateStyleSet().GetWidgetStyle<FEditableTextBoxStyle>("Graph.Node.NodeTitleEditableText"));\
	Style->Set(StyleName, Signifier);


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
	
	static const FMargin Margin_Large;
	static const FMargin Margin_Normal;
	static const FMargin Margin_Small;
	static const FMargin Margin_Tiny;

	static const FMargin Margin_Shadow;
	static const FMargin Margin_SubNode;

public:
	static const FLinearColor Color_Normal;
	static const FLinearColor Color_Hover;
	static const FLinearColor Color_Selected;
	static const FLinearColor Color_Disabled;

	static const FLinearColor Color_SolidNormal;
	static const FLinearColor Color_SolidHover;
	static const FLinearColor Color_SolidSelected;

	static const FLinearColor Color_Node_TabBackground;
	static const FLinearColor Color_Node_Inactive;
	static const FLinearColor Color_Node_Selected;
	static const FLinearColor Color_Node_Invalid;
	static const FLinearColor Color_Node_DragMarker;
	static const FLinearColor Color_Node_Shadow;

};
