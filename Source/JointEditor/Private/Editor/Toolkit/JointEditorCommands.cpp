//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Toolkit/JointEditorCommands.h"

#include "JointEditorStyle.h"
#include "Framework/Commands/UICommandInfo.h"

#define LOCTEXT_NAMESPACE "JointEditorCommands"


FJointEditorCommands::FJointEditorCommands()
	: TCommands<FJointEditorCommands>(
		TEXT("JointEditor"), // Context name for fast lookup
		NSLOCTEXT("Contexts", "JointEditor", "Joint Editor"), // Localized context name for displaying
		NAME_None, // Parent
		FJointEditorStyle::GetUEEditorSlateStyleSetName() // Icon Style Set
	)
{
}

void FJointEditorCommands::RegisterCommands()
{
	//Initializing a new command list.
	PluginCommands = MakeShareable(new FUICommandList);

	UI_COMMAND(CompileJoint, "Compile", "Compile the Joint manager to check out whether the Joint has any issues.", EUserInterfaceActionType::Button, FInputChord(EKeys::F7) );
	
	UI_COMMAND(SetShowNormalConnection, "Normal Connection", "Display the normal connections.", EUserInterfaceActionType::ToggleButton, FInputChord(EModifierKey::Control, EKeys::One));
	UI_COMMAND(SetShowRecursiveConnection, "Recursive Connection", "Displays the recursive connections.", EUserInterfaceActionType::ToggleButton, FInputChord(EModifierKey::Control, EKeys::Two));
	UI_COMMAND(SetShowContextTextRawEditorBox, "Context Text Raw Editor", "Displays the context text raw editor.", EUserInterfaceActionType::ToggleButton, FInputChord());
	
	UI_COMMAND(OpenJointManagementTab, "Open Joint Management Tab", "Open Joint management tab.",  EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control,EKeys::F1));
	UI_COMMAND(OpenJointCompilerTab, "Open Joint Compiler Tab", "Open Joint compiler tab.",  EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control,EKeys::F2));
	UI_COMMAND(OpenJointBulkSearchReplaceTab, "Open Joint Bulk Search & Replace Tab", "Open Joint Bulk Search & replace tab.",  EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control,EKeys::F3));

	UI_COMMAND(OpenSearchTab, "Search", "Open Search Tab",  EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::F));
	UI_COMMAND(OpenReplaceTab, "Replace", "Open Replace Tab",  EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::H));

	UI_COMMAND(JumpToSelection, "Jump To Selection", "Jump To Selected Node. It will boost your development speed, especially when you want to go back to the original node after node picking.", EUserInterfaceActionType::Button, FInputChord(EKeys::SpaceBar));

	UI_COMMAND(EscapeNodePickingMode, "Escape Node Picking Mode", "Escape node picking mode without selecting any of the nodes.",  EUserInterfaceActionType::Button, FInputChord(EKeys::Escape));

	//It's a bit mask with shift op - you can use + to combine multiple modifier keys.
	UI_COMMAND(RemoveAllBreakpoints, "Remove All Breakpoints", "Remove all Breakpoints",  EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control + EModifierKey::Shift, EKeys::F9));
	UI_COMMAND(EnableAllBreakpoints, "Enable All Breakpoints", "Enable all Breakpoints",  EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control + EModifierKey::Shift, EKeys::F10));
	UI_COMMAND(DisableAllBreakpoints, "Disable All Breakpoints", "Disable all Breakpoints",  EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control + EModifierKey::Shift, EKeys::F11));

	UI_COMMAND(CreateFoundation, "Create Foundation", "Create a foundation node", EUserInterfaceActionType::Button, FInputChord(EKeys::F));

	UI_COMMAND(ToggleDebuggerExecution, "Toggle Debugger Execution", "Toggle the debugger's execution to prevent any exection halting by the breakpoints on any Joint.",  EUserInterfaceActionType::ToggleButton, FInputChord(EModifierKey::FromBools(1,1,1,0), EKeys::F11));
	
	UI_COMMAND(DissolveSubNodeIntoParentNode, "Dissolve Sub Node", "Dissolve this sub node with its parent node.",  EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control + EModifierKey::Shift,EKeys::D));
	UI_COMMAND(SolidifySubNodesFromParentNode, "Solidify Sub Nodes", "Solidify all the sub nodes from this node.",  EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control + EModifierKey::Shift, EKeys::S));
	
	UI_COMMAND(ShowIndividualVisibilityButtonForSimpleDisplayProperty, "Show Individual Visibility Button For Simple Display Property", "Show Individual Visibility Button For Simple Display Property.",  EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::X));

	// SMyBlueprint ported commands for Joint Editor Outliner
	UI_COMMAND( DeleteEntry, "Delete", "Delete the selected entry.", EUserInterfaceActionType::Button, FInputChord(EKeys::Delete) );
}

FJointDebuggerCommands::FJointDebuggerCommands()
	: TCommands<FJointDebuggerCommands>(
		TEXT("JointDebuggerCommand"), // Context name for fast lookup
		NSLOCTEXT("Contexts", "JointDebugger", "Joint Debugger"), // Localized context name for displaying
		NAME_None, // Parent
		FJointEditorStyle::GetUEEditorSlateStyleSetName() // Icon Style Set
	)
{
}

void FJointDebuggerCommands::RegisterCommands()
{
	UI_COMMAND(ForwardInto, "Forward: Into", "Resume and break again when met the next node. Can go into the sub trees.", EUserInterfaceActionType::Button, FInputChord(EKeys::F10));
	UI_COMMAND(ForwardOver, "Forward: Over", "Resume and break again when met the next node. Can not go into the sub trees.", EUserInterfaceActionType::Button, FInputChord(EKeys::F11));
	UI_COMMAND(StepOut, "Step Out", "Resume and break again when met the next node. Must be another base node.", EUserInterfaceActionType::Button,FInputChord(EKeys::F12));

	UI_COMMAND(PausePlaySession, "Pause", "Pause simulation", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ResumePlaySession, "Resume", "Resume simulation", EUserInterfaceActionType::Button, FInputChord() );
	UI_COMMAND(StopPlaySession, "Stop", "Stop simulation", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
