//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "JointManagement.h"

#include "SCheckBoxList.h"

#include "Input/Reply.h"
#include "Framework/Docking/TabManager.h"

#include "JointEdGraphNode.h"
#include "Node/JointNodeBase.h"

#include "SharedType/JointEditorSharedTypes.h"

class UJointManager;

class FTabManager;

namespace JointEditorTabs
{
}

class JOINTEDITOR_API FJointManagementTab_JointEditorUtilityTab : public IJointManagementSubTab
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
	SLATE_BEGIN_ARGS(SJointEditorUtilityTab)
		{
		}

	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);

public:
	TSharedRef<SWidget> CreateProductSection();

	TSharedRef<SWidget> CreateInvalidateSection();

	TSharedRef<SWidget> CreateGraphSection();

public:
	ECheckBoxState GetIsDeveloperModeChecked() const;

	void OnDeveloperModeToggled(ECheckBoxState CheckBoxState);

public:
	FReply ReconstructEveryNodeInOpenedJointManagerEditor();

	FReply CleanUpUnnecessaryNodes();

	FReply UpdateBPNodeEdSettings();

public:
	FReply ResetAllEditorStyle();
	FReply ResetGraphEditorStyle();
	FReply ResetPinConnectionEditorStyle();
	FReply ResetDebuggerEditorStyle();
	FReply ResetNodeEditorStyle();

	FReply ResetContextTextEditorStyle();
};


class JOINTEDITOR_API FJointManagementTab_NodeClassManagementTab : public IJointManagementSubTab
{
public:
	FJointManagementTab_NodeClassManagementTab();

	virtual ~FJointManagementTab_NodeClassManagementTab() override;

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
class JOINTEDITOR_API SJointEditorNodeClassManagementTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SJointEditorNodeClassManagementTab)
		{
		}

	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);
	void FillWindowMenu(FMenuBuilder& MenuBuilder);

public:
	void InitializeMissingClassesMapTab();

public:
	TSharedRef<SDockTab> SpawnMissingClassesMapTab(const FSpawnTabArgs& Args);

public:
	void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager);

public:
	TSharedPtr<class FTabManager> TabManager;

public:
	TSharedPtr<class SJointEditorTap_MissingClassesMap> MissingClassesMapWidget;
};


/**
 * Content widget for the FJointManagementTab_JointEditorUtilityTab. it's just a pure slate. make something like this for your own extension.
 */
class JOINTEDITOR_API SJointEditorTap_MissingClassesMap : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SJointEditorTap_MissingClassesMap)
		{
		}

	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);

public:
	FReply AllocatedRedirectionRefresh();

	FReply MissingClassRefresh();

public:
	TSharedPtr<class SScrollBox> MissingClassScrollBox;
	TSharedPtr<class SScrollBox> RedirectionScrollBox;

	TSharedPtr<class SScrollBox> ReallocateScrollBox;

public:
	void OnSetClass_NodeClassLeftSelectedClass(const UClass* Class);
	void OnSetClass_NodeClassRightSelectedClass(const UClass* Class);
	void OnSetClass_EditorNodeClassLeftSelectedClass(const UClass* Class);
	void OnSetClass_EditorNodeClassRightSelectedClass(const UClass* Class);

public:
	FReply OnNodeClassChangeButtonClicked();
	FReply OnEditorNodeClassChangeButtonClicked();

public:
	TSubclassOf<UJointNodeBase> NodeClassLeftSelectedClass;
	TSubclassOf<UJointNodeBase> NodeClassRightSelectedClass;
	TSubclassOf<UJointEdGraphNode> EditorNodeClassLeftSelectedClass;
	TSubclassOf<UJointEdGraphNode> EditorNodeClassRightSelectedClass;
};

class JOINTEDITOR_API FJointEditorTap_RedirectionInstance : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(FJointEditorTap_RedirectionInstance)
		{
		}

		SLATE_ARGUMENT(FJointCoreRedirect, Redirection)
		SLATE_ARGUMENT(TSharedPtr<SJointEditorTap_MissingClassesMap>, Owner)
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);

public:
	
	FReply RemoveRedirection();

public:
	FJointCoreRedirect Redirection;

	TWeakPtr<SJointEditorTap_MissingClassesMap> Owner;
};


class JOINTEDITOR_API FJointEditorTap_MissingClassInstance : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(FJointEditorTap_MissingClassInstance)
		{
		}

		SLATE_ARGUMENT(FJointGraphNodeClassData, ClassData)
		SLATE_ARGUMENT(TSharedPtr<SJointEditorTap_MissingClassesMap>, Owner)
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);

public:
	void OnChangeNodeSetClass(const UClass* Class);

	const UClass* GetSelectedClass() const;

public:
	FReply Apply();

public:
	FJointGraphNodeClassData ClassData;

	TSubclassOf<UJointNodeBase> SelectedClass;

	TWeakPtr<SJointEditorTap_MissingClassesMap> Owner;

};
