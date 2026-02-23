//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointEdGraphSchemaActions.h"
#include "SGraphActionMenu.h"
#include "Widgets/SCompoundWidget.h"

class SJointGraphEditorActionMenu;
class SScrollBox;
class UJointEdGraphNode;
class FJointEditorToolkit;

//////////////////////////////////////////////////////////////////////////
// SJointFragmentPalette

class JOINTEDITOR_API SJointNodePalette : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SJointNodePalette){}
		SLATE_ARGUMENT(TWeakPtr<FJointEditorToolkit>, ToolKitPtr)
	SLATE_END_ARGS()
public:
	
	void Construct(const FArguments& InArgs);

public:
	void RebuildWidget();

private:

	TWeakPtr<FJointEditorToolkit> ToolKitPtr;

public:
	
	void OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& Shareds, ESelectInfo::Type Arg);
	
public:

	TArray< TSharedPtr<struct FJointFragmentPaletteAction> > ActionEntries;
	
public:
	
	TSharedPtr<SJointGraphEditorActionMenu> ActionMenu;

};
