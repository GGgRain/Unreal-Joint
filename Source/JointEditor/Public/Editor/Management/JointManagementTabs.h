//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "JointManagement.h"

class JOINTEDITOR_API FJointManagementTab_JointEditorUtilityTab: public IJointManagementSubTab
{

public:
	
	FJointManagementTab_JointEditorUtilityTab();

	virtual ~FJointManagementTab_JointEditorUtilityTab() override;

public:

	static TSharedRef<IJointManagementSubTab> MakeInstance();

public:
	
	virtual void RegisterTabSpawner(const TSharedPtr<FTabManager>& TabManager) override;

public:

	virtual const FName GetTabId() override;

	virtual const ETabState::Type GetInitialTabState() override;
	
};





/**
 * Content widget for the FJointManagementTab_JointEditorUtilityTab. it's just a pure slate. make something like this for your own extension.
 */
class JOINTEDITOR_API SJointEditorUtilityTab : public SCompoundWidget
{
	
public:

	SLATE_BEGIN_ARGS(SJointEditorUtilityTab) {}
	SLATE_END_ARGS();
	
	void Construct(const FArguments& InArgs);

public:

	TSharedRef<SWidget> CreateProductSection();

	TSharedRef<SWidget> CreateInvalidateSection();

	TSharedRef<SWidget> CreateGraphSection();

public:

	FReply ReconstructEveryNodeInOpenedJointManagerEditor();

	FReply CleanUpUnnecessaryNodes();
	
public:

	FReply ResetAllEditorStyle();
	FReply ResetGraphEditorStyle();
	FReply ResetPinConnectionEditorStyle();
	FReply ResetDebuggerEditorStyle();
	FReply ResetNodeEditorStyle();

	FReply ResetContextTextEditorStyle();

};
