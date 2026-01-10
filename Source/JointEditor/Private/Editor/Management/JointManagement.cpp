//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointManagement.h"

#include "JointEditorLogChannels.h"
#include "JointEditorStyle.h"
#include "Framework/Docking/LayoutService.h"
#include "Framework/Docking/TabManager.h"

#include "Slate/SJointManagement.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "JointManagementTab"

FJointManagementTabHandler::FJointManagementTabHandler()
{
}

TSharedRef<FJointManagementTabHandler> FJointManagementTabHandler::MakeInstance()
{
	return MakeShareable(new FJointManagementTabHandler());
}

TSharedRef<SDockTab> FJointManagementTabHandler::SpawnJointManagementTab()
{
	//Flush down previous group.
	ActiveGroups.Empty();

	TSharedRef<SDockTab> NomadTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("JointManagementTabTitle", "Joint Management"));

	const TSharedRef<FTabManager> NewSubTabManager = FGlobalTabmanager::Get()->NewTabManager(NomadTab);

	NewSubTabManager->SetOnPersistLayout(
		FTabManager::FOnPersistLayout::CreateStatic(
			[](const TSharedRef<FTabManager::FLayout>& InLayout)
			{
				if (InLayout->GetPrimaryArea().Pin().IsValid())
				{
					FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, InLayout);
				}
			}
		)
	);

	NomadTab->SetTabIcon(FJointEditorStyle::Get().GetBrush("ClassIcon.JointManager"));

	NomadTab->SetOnTabClosed(
		SDockTab::FOnTabClosedCallback::CreateStatic(
			[](TSharedRef<SDockTab> Self, TWeakPtr<FTabManager> TabManager)
			{
				TSharedPtr<FTabManager> OwningTabManager = TabManager.Pin();
				if (OwningTabManager.IsValid())
				{
					FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, OwningTabManager->PersistLayout());
					OwningTabManager->CloseAllAreas();
				}
			}
			, TWeakPtr<FTabManager>(NewSubTabManager)
		)
	);

	auto MainWidget = SNew(SJointManagement)
		.TabManager(NewSubTabManager)
		.JointManagementTabHandler(SharedThis(this));

	NomadTab->SetContent(MainWidget);

	return NomadTab;
}

bool FJointManagementTabHandler::ContainsSubTabForId(const FName& TabId)
{
	for (TSharedPtr<IJointManagementSubTab> JointManagementSubTab : SubTabs)
	{
		if (!JointManagementSubTab.IsValid()) continue;

		if (JointManagementSubTab->GetTabId() == TabId)
		{
			return true;
		}
	}
	return false;
}

const TArray<TSharedPtr<IJointManagementSubTab>>& FJointManagementTabHandler::GetSubTabs() const
{
	return SubTabs;
}

void FJointManagementTabHandler::AddActiveGroup(const FName Name, const TSharedPtr<FWorkspaceItem>& GroupWorkspaceItem)
{
	if(!ActiveGroups.Contains(Name))
	{
		ActiveGroups.Add(Name, GroupWorkspaceItem);
	}
}

TSharedPtr<FWorkspaceItem> FJointManagementTabHandler::GetActiveGroupFor(const FName Key) const
{
	if(ActiveGroups.Contains(Key))
	{
		return ActiveGroups.FindRef(Key);
	}
	
	return nullptr;
}

const FName IJointManagementSubTab::TAB_ID_INVALID("TAB_ID_INVALID");

IJointManagementSubTab::IJointManagementSubTab()
{
}

void IJointManagementSubTab::SetParentTabHandler(const TWeakPtr<FJointManagementTabHandler>& InParentTabHandler)
{
	ParentTabHandler = InParentTabHandler;
}

TWeakPtr<FJointManagementTabHandler> IJointManagementSubTab::GetParentTabHandler()
{
	return ParentTabHandler;
}

void IJointManagementSubTab::RegisterTabSpawner(const TSharedPtr<FTabManager>& TabManager)
{
}

const FName IJointManagementSubTab::GetTabId()
{
	return TAB_ID_INVALID;
}

const ETabState::Type IJointManagementSubTab::GetInitialTabState()
{
	return ETabState::OpenedTab;
}

const FName IJointManagementSubTab::GetTabGroup()
{
	return ""; // Empty group by default.
}

void FJointManagementTabHandler::AddSubTab(const TSharedRef<IJointManagementSubTab>& SubTab)
{
	const FName& TabId = SubTab->GetTabId();

	if (TabId == IJointManagementSubTab::TAB_ID_INVALID)
	{
		UE_LOG(LogJointEditor, Error,
		       TEXT(
			       "FJointManagementTabHandler : Detected a sub tab with TAB_ID_INVALID id. You must specify its id to be unique one. This tab will not be added."
		       ));

		return;
	}

	if (ContainsSubTabForId(TabId)) return;

	SubTab->SetParentTabHandler(AsShared());

	SubTabs.Add(SubTab);
}

#undef LOCTEXT_NAMESPACE
