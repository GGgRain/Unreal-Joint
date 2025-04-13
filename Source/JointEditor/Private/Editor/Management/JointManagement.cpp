//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointManagement.h"

#include "JointEditorStyle.h"

#include "Slate/SJointManagement.h"

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
	auto NomadTab = SNew(SDockTab)
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
		if(!JointManagementSubTab.IsValid()) continue;

		if(JointManagementSubTab->GetTabId() == TabId)
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


const FName IJointManagementSubTab::TAB_ID_INVALID("TAB_ID_INVALID");

IJointManagementSubTab::IJointManagementSubTab() 
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

void FJointManagementTabHandler::AddSubTab(const TSharedRef<IJointManagementSubTab>& SubTab)
{
	const FName& TabId = SubTab->GetTabId();

	if(TabId == IJointManagementSubTab::TAB_ID_INVALID)
	{
		UE_LOG(LogTemp,Error,TEXT("FJointManagementTabHandler : Detected a sub tab with TAB_ID_INVALID id. You must specify its id to be unique one. This tab will not be added."));
		
		return;
	}
	
	if (ContainsSubTabForId(TabId)) return;

	SubTabs.Add(SubTab);
}

#undef LOCTEXT_NAMESPACE

