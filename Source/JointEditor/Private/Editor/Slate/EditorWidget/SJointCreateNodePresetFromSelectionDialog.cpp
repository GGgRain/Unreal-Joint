//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "EditorWidget/SJointCreateNodePresetFromSelectionDialog.h"

SJointCreateNodePresetFromSelectionDialog::SJointCreateNodePresetFromSelectionDialog() 
	: GraphObj(nullptr)
{
}

void SJointCreateNodePresetFromSelectionDialog::Construct( const FArguments& InArgs )
{
	GraphObj = InArgs._GraphObj;
	GraphNodes = InArgs._GraphNodes;
}




