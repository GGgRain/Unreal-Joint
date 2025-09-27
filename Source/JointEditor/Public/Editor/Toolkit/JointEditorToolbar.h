//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointEditorSettings.h"

class FJointEditorToolkit;
class FExtender;
class FToolBarBuilder;
class SWidget;
class FMenuBuilder;

class JOINTEDITOR_API FJointEditorToolbar : public TSharedFromThis<FJointEditorToolbar>
{
public:
	FJointEditorToolbar(const TSharedPtr<FJointEditorToolkit>& InJointEditor)
		: JointEditor(InJointEditor) {}

	void AddDebuggerToolbar(TSharedPtr<FExtender> Extender);
	void AddJointToolbar(TSharedPtr<FExtender> Extender);
	void AddEditorModuleToolbar(TSharedPtr<FExtender> Extender);

public:

	void FillJointToolbar_BeforeAsset(FToolBarBuilder& ToolbarBuilder);
	void FillJointToolbar_AfterAsset(FToolBarBuilder& ToolbarBuilder);
	void FillEditorModuleToolbar(FToolBarBuilder& ToolbarBuilder);

public:

	TSharedRef<SWidget> GenerateDebuggerMenu() const;

	TSharedRef<SWidget> GenerateVisibilityMenu() const;

	TSharedRef<SWidget> GenerateJointToolsMenu() const;


protected:
	/** Pointer back to the blueprint editor tool that owns us */
	TWeakPtr<FJointEditorToolkit> JointEditor;

public:
	
	TSharedRef<SWidget> Task_HandleCreateNewJointAsset() const;
	bool Task_CanCreateNewJointAsset();
	
	void Task_HandleOpenSearchReplaceTab();
	
};
