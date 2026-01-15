//Copyright 2022~2024 DevGrain. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FJointEditorStyle;
class FUICommandInfo;


class JOINTEDITOR_API FJointEditorCommands : public TCommands<FJointEditorCommands>
{
public:
	FJointEditorCommands();

public:
	virtual void RegisterCommands() override;

public:
	
	TSharedPtr<FUICommandInfo> CompileJoint;

public:
	TSharedPtr<FUICommandInfo> SetShowNormalConnection;
	TSharedPtr<FUICommandInfo> SetShowRecursiveConnection;
	TSharedPtr<FUICommandInfo> SetShowContextTextRawEditorBox;

	TSharedPtr<FUICommandInfo> OpenJointManagementTab;
	TSharedPtr<FUICommandInfo> OpenJointCompilerTab;
	TSharedPtr<FUICommandInfo> OpenJointBulkSearchReplaceTab;

	TSharedPtr<FUICommandInfo> OpenSearchTab;
	TSharedPtr<FUICommandInfo> OpenReplaceTab;

public:

	TSharedPtr<FUICommandInfo> JumpToSelection;

public:
	
	TSharedPtr<FUICommandInfo> QuickPickSelection;
	TSharedPtr<FUICommandInfo> EscapeNodePickingMode;

public:
	TSharedPtr<FUICommandInfo> RemoveAllBreakpoints;
	TSharedPtr<FUICommandInfo> EnableAllBreakpoints;
	TSharedPtr<FUICommandInfo> DisableAllBreakpoints;

	TSharedPtr<FUICommandInfo> ToggleDebuggerExecution;

public:

	TSharedPtr<FUICommandInfo> DissolveSubNodesIntoParentNode;
	TSharedPtr<FUICommandInfo> DissolveExactSubNodeIntoParentNode;
	TSharedPtr<FUICommandInfo> DissolveOnlySubNodesIntoParentNode;

	TSharedPtr<FUICommandInfo> SolidifySubNodesFromParentNode;

public:

	TSharedPtr<FUICommandInfo> ShowIndividualVisibilityButtonForSimpleDisplayProperty;

public:
	
	TSharedPtr<FUICommandInfo> CreateFoundation;

public:

	TSharedPtr<FUICommandInfo> DeleteEntry;

public:
	
	TSharedPtr<FUICommandList> PluginCommands;
	
};

class JOINTEDITOR_API FJointDebuggerCommands : public TCommands<FJointDebuggerCommands>
{
public:
	FJointDebuggerCommands();

public:
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> ForwardInto;
	TSharedPtr<FUICommandInfo> ForwardOver;
	TSharedPtr<FUICommandInfo> StepOut;

	TSharedPtr<FUICommandInfo> PausePlaySession;
	TSharedPtr<FUICommandInfo> ResumePlaySession;
	TSharedPtr<FUICommandInfo> StopPlaySession;

};
