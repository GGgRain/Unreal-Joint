//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointManagerFactory.h"

#include "JointEdGraph.h"
#include "JointManager.h"

UJointManagerFactory::UJointManagerFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UJointManager::StaticClass();
}

UObject* UJointManagerFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UJointManager* Asset = NewObject<UJointManager>(InParent, Class, Name, Flags | RF_Transactional);
	
	CreateDefaultRootGraphForJointManager(Asset);
	
	return Asset;
}

bool UJointManagerFactory::ShouldShowInNewMenu() const {
	return true;
}

void UJointManagerFactory::CreateDefaultRootGraphForJointManager(UJointManager* Manager)
{
	if (!Manager) return;
	
	// If the manager already has a graph, do nothing.
	if (Manager->JointGraph) return;
	
	Manager->JointGraph = UJointEdGraph::CreateNewJointGraph(Manager, Manager, FName("MainGraph"));
	Manager->JointGraph->bAllowDeletion = false;
}
