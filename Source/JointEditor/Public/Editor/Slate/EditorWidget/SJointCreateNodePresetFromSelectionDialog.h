//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphEditor.h"
#include "SGraphActionMenu.h"
#include "Misc/EngineVersionComparison.h"


class SEditableTextBox;
class SGraphActionMenu;
class UJointEdGraphNode;
class UJointNodeBase;


class JOINTEDITOR_API SJointCreateNodePresetFromSelectionDialog: public SGraphActionMenu
{
	// a dialog widget to create a node preset from the selected nodes in the graph editor
	
public:
	
	SJointCreateNodePresetFromSelectionDialog();

public:
	
	SLATE_BEGIN_ARGS(SJointCreateNodePresetFromSelectionDialog)
		: _GraphObj(nullptr)
		, _GraphNodes()
		{}
		SLATE_ARGUMENT(UEdGraph*, GraphObj)
		SLATE_ARGUMENT(TArray<UJointEdGraphNode*>, GraphNodes)
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );
	
private:
	
	UEdGraph* GraphObj;
	
	TArray<UJointEdGraphNode*> GraphNodes;
	
};
