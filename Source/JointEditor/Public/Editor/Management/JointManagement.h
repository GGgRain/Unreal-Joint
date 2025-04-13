//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class IJointManagementSubTab;

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
	
};


class JOINTEDITOR_API IJointManagementSubTab: public TSharedFromThis<IJointManagementSubTab>
{

public:
	
	IJointManagementSubTab();

	virtual ~IJointManagementSubTab() = default;

public:
	
	virtual void RegisterTabSpawner(const TSharedPtr<FTabManager>& TabManager) = 0;

public:

	virtual const FName GetTabId();

	virtual const ETabState::Type GetInitialTabState();

public:

	static const FName TAB_ID_INVALID;

};
