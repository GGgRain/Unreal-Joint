//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Docking/TabManager.h"

class FTabManager;
class IJointManagementSubTab;
class FWorkspaceItem;
class SDockTab;

class JOINTEDITOR_API FJointManagementTabHandler : public TSharedFromThis<FJointManagementTabHandler>
{

public:
	
	FJointManagementTabHandler();

public:

	static TSharedRef<FJointManagementTabHandler> MakeInstance();

public:
	
	TSharedRef<SDockTab> SpawnJointManagementTab();

public:

	void AddSubTab(const TSharedRef<IJointManagementSubTab>& SubTab);

	bool ContainsSubTabForId(const FName& TabId);

public:

	const TArray<TSharedPtr<IJointManagementSubTab>>& GetSubTabs() const;
	
protected:

	/**
	 * Sub tabs for the Joint Management tab.
	 * All the tabs will be accessible through the toolbar of the management tab.
	 * Add your own sub tab definition on here to implement that as well. (This is useful when you are working on the external modules.
	 */
	TArray<TSharedPtr<IJointManagementSubTab>> SubTabs;

public:
	
	void AddActiveGroup(FName Name, const TSharedPtr<FWorkspaceItem>& GroupWorkspaceItem);
	
	TSharedPtr<FWorkspaceItem> GetActiveGroupFor(const FName Key) const;


protected:

	//Grouping & Category - Use this to group your tabs in the management tab.

	TMap<FName, TSharedPtr<FWorkspaceItem>> ActiveGroups;

	
};


class JOINTEDITOR_API IJointManagementSubTab: public TSharedFromThis<IJointManagementSubTab>
{

public:
	
	IJointManagementSubTab();

	virtual ~IJointManagementSubTab() = default;

public:

	void SetParentTabHandler(const TWeakPtr<FJointManagementTabHandler>& InParentTabHandler);

	TWeakPtr<FJointManagementTabHandler> GetParentTabHandler();

public:
	
	virtual void RegisterTabSpawner(const TSharedPtr<FTabManager>& TabManager);

public:

	virtual const FName GetTabId();

	virtual const ETabState::Type GetInitialTabState();

public:
	
	virtual const FName GetTabGroup();

public:
	
	TWeakPtr<FJointManagementTabHandler> ParentTabHandler;
	
public:

	static const FName TAB_ID_INVALID;
	
};
