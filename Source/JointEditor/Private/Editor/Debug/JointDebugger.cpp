// Fill out your copyright notice in the Description page of Project Settings.


#include "Editor/Debug/JointDebugger.h"

#include "JointActor.h"
#include "JointEdGraph.h"
#include "JointEditorToolkit.h"
#include "JointManager.h"
#include "Joint.h"

#include "JointEditor.h"
#include "JointEditorLogChannels.h"
#include "JointEditorSettings.h"
#include "JointEdUtils.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Framework/Application/SlateApplication.h"
#include "GraphNode/SJointGraphNodeBase.h"

#define LOCTEXT_NAMESPACE "UJointDebugger"

UJointDebugger::UJointDebugger()
{
	RegisterJointLifecycleEvents();

	FEditorDelegates::BeginPIE.AddUObject(this, &UJointDebugger::OnBeginPIE);
	FEditorDelegates::EndPIE.AddUObject(this, &UJointDebugger::OnEndPIE);
	FEditorDelegates::PausePIE.AddUObject(this, &UJointDebugger::OnPausePIE);
	FEditorDelegates::ResumePIE.AddUObject(this, &UJointDebugger::OnResumePIE);
	FEditorDelegates::SingleStepPIE.AddUObject(this, &UJointDebugger::OnSingleStepPIE);
}

UJointDebugger::~UJointDebugger()
{
	UnregisterJointLifecycleEvents();

	FEditorDelegates::BeginPIE.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);
	FEditorDelegates::PausePIE.RemoveAll(this);
	FEditorDelegates::ResumePIE.RemoveAll(this);
	FEditorDelegates::SingleStepPIE.RemoveAll(this);
}

void UJointDebugger::UnregisterJointLifecycleEvents()
{
#if WITH_EDITOR

	//Make sure to grab the module in this way if it is not in the Joint module itself,
	//because it is not always sure whether we can access the module anytime.
	//It indeed made a issue on the packaging stage with validation, even if it is on the editor module.

	if (IModuleInterface* Module = FModuleManager::Get().GetModule("Joint"); Module != nullptr)
	{
		if (FJointModule* CastedModule = static_cast<FJointModule*>(Module))
		{
			CastedModule->OnJointExecutionExceptionDelegate.Unbind();
			CastedModule->JointDebuggerJointBeginPlayNotification.Unbind();
			CastedModule->JointDebuggerJointEndPlayNotification.Unbind();
			CastedModule->JointDebuggerNodeBeginPlayNotification.Unbind();
			CastedModule->JointDebuggerNodeEndPlayNotification.Unbind();
			CastedModule->JointDebuggerNodePendingNotification.Unbind();
		}
	}

#endif
}

void UJointDebugger::RegisterJointLifecycleEvents()
{
#if WITH_EDITOR

	//Make sure to grab the module in this way if it is not in the Joint module itself,
	//because it is not always sure whether we can access the module anytime.
	//It indeed made a issue on the packaging stage with validation, even if it is on the editor module.

	if (IModuleInterface* Module = FModuleManager::Get().GetModule(FName("Joint")); Module != nullptr)
	{
		if (FJointModule* CastedModule = static_cast<FJointModule*>(Module))
		{
			CastedModule->OnJointExecutionExceptionDelegate.BindUObject(this, &UJointDebugger::CheckWhetherToBreakExecution);
			CastedModule->JointDebuggerJointBeginPlayNotification.BindUObject(this, &UJointDebugger::OnJointBegin);
			CastedModule->JointDebuggerJointEndPlayNotification.BindUObject(this, &UJointDebugger::OnJointEnd);
			CastedModule->JointDebuggerNodeBeginPlayNotification.BindUObject(this, &UJointDebugger::OnJointNodeBeginPlayed);
			CastedModule->JointDebuggerNodeEndPlayNotification.BindUObject(this, &UJointDebugger::OnJointNodeEndPlayed);
			CastedModule->JointDebuggerNodePendingNotification.BindUObject(this, &UJointDebugger::OnJointNodePending);
		}
	}

#endif
}

void UJointDebugger::OnBeginPIE(bool bArg)
{
}

void UJointDebugger::OnEndPIE(bool bArg)
{
	ClearDebugSessionData();
}

void UJointDebugger::OnPausePIE(bool bArg)
{
}

void UJointDebugger::OnResumePIE(bool bArg)
{
	RestartExecutionOfPausedJointActors();
}

void UJointDebugger::OnSingleStepPIE(bool bArg)
{
	RestartExecutionOfPausedJointActors();
}

void UJointDebugger::OnJointBegin(AJointActor* JointInstance, const FGuid& JointGuid)
{
	if (!IsPIESimulating()) return;

	if (JointInstance != nullptr)
	{
		AssignInstanceToKnownInstance(JointInstance);
	}
}

#include "Misc/EngineVersionComparison.h"

void UJointDebugger::OnJointEnd(AJointActor* JointInstance, const FGuid& JointGuid)
{
	if (!IsPIESimulating()) return;

	if (JointInstance != nullptr)
	{
		FJointEditorToolkit* ToolKit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(JointInstance->GetJointManager(), false);

		if (ToolKit != nullptr)
		{
			//Abandon the toolkit for the debugging.
#if UE_VERSION_OLDER_THAN(5, 3, 0)
			ToolKit->CloseWindow();
#else
			ToolKit->CloseWindow(EAssetEditorCloseReason::AssetUnloadingOrInvalid);
#endif
		}

		RemoveInstanceFromKnownInstance(JointInstance);
	}
}

UJointDebugger* UJointDebugger::Get()
{
	FJointEditorModule* Module = &FModuleManager::GetModuleChecked<FJointEditorModule>(
		"JointEditor");

	if (Module)
	{
		return Module->JointDebugger;
	}

	return nullptr;
}


static void ExecuteForEachGameWorld(const TFunction<void(UWorld*)>& Func)
{
	for (const FWorldContext& PieContext : GUnrealEd->GetWorldContexts())
	{
		UWorld* PlayWorld = PieContext.World();
		if (PlayWorld && PlayWorld->IsGameWorld())
		{
			Func(PlayWorld);
		}
	}
}


static bool AreAllGameWorldPaused()
{
	bool bPaused = true;
	ExecuteForEachGameWorld([&](UWorld* World)
	{
		bPaused = bPaused && World->bDebugPauseExecution;
	});
	return bPaused;
}


void UJointDebugger::GetMatchingInstances(UJointManager* JointManager,
                                          TArray<AJointActor*>& MatchingInstances)
{
	for (TWeakObjectPtr<AJointActor> KnownJointInstance : KnownJointInstances)
	{
		if (KnownJointInstance == nullptr) continue;

		if (KnownJointInstance->OriginalJointManager == JointManager)
		{
			MatchingInstances.Add(KnownJointInstance.Get());
		}
	}
}

FText UJointDebugger::GetInstanceDescription(AJointActor* Instance) const
{
	FText ActorDesc = LOCTEXT("InstanceDescription_Default",
	                          "Select the instance you want to start debugging for.");

	if (Instance != nullptr && Instance->GetJointManager())
	{
		FString FullDesc = Instance->GetJointManager()->GetName() + "::" + Instance->GetActorLabel();

		switch (Instance->GetNetMode())
		{
		case ENetMode::NM_Standalone:
			{
				FullDesc += " ( ";
				FullDesc += NSLOCTEXT("BlueprintEditor", "DebugWorldStandalone", "Standalone").ToString();

				FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(Instance->GetWorld());
				if (WorldContext != nullptr)
				{
					FullDesc += TEXT(":");
					FullDesc += FString::FromInt(WorldContext->World()->GetUniqueID());
				}
				FullDesc += " )";
				break;
			}
		case ENetMode::NM_Client:
			{
				FullDesc += " ( ";
				FullDesc += NSLOCTEXT("BlueprintEditor", "DebugWorldClient", "Client").ToString();

				FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(Instance->GetWorld());
				if (WorldContext != nullptr && WorldContext->PIEInstance > 1)
				{
					FullDesc += TEXT(" ");
					FullDesc += FText::AsNumber(WorldContext->PIEInstance - 1).ToString();
				}
				FullDesc += " )";
				break;
			}
		case ENetMode::NM_ListenServer:
			{
				FullDesc += " ( ";
				FullDesc += NSLOCTEXT("BlueprintEditor", "DebugWorldServer", "Listen_Server").ToString();
				FullDesc += " )";
				break;
			}
		case ENetMode::NM_DedicatedServer:
			{
				FullDesc += " ( ";
				FullDesc += NSLOCTEXT("BlueprintEditor", "DebugWorldServer", "Dedicated_Server").ToString();
				if (Instance->IsSelectable())
					FullDesc += " )";
				break;
			}
		}

		ActorDesc = FText::FromString(FullDesc);
	}
	else if (Instance != nullptr)
	{
		ActorDesc = FText::FromString(FString("-INVALID-") + "::" + Instance->GetActorLabel());
	}

	return ActorDesc;
}

bool UJointDebugger::IsDebuggerEnabled()
{
	return UJointEditorSettings::Get()->bDebuggerEnabled;
}


void UJointDebugger::StopPlaySession()
{
	if (GUnrealEd->PlayWorld)
	{
		GEditor->RequestEndPlayMap();

		// @TODO: we need a unified flow to leave debugging mode from the different debuggers to prevent strong coupling between modules.
		// Each debugger (Blueprint & BehaviorTree for now) could then take the appropriate actions to resume the session.
		if (FSlateApplication::Get().InKismetDebuggingMode())
		{
			FSlateApplication::Get().LeaveDebuggingMode();
		}
	}
}

void UJointDebugger::PausePlaySession()
{
	if (GUnrealEd->SetPIEWorldsPaused(true))
	{
		GUnrealEd->PlaySessionPaused();
	}
}

void UJointDebugger::ResumePlaySession()
{
	if (GUnrealEd->SetPIEWorldsPaused(false))
	{
		// @TODO: we need a unified flow to leave debugging mode from the different debuggers to prevent strong coupling between modules.
		// Each debugger (Blueprint & BehaviorTree for now) could then take the appropriate actions to resume the session.
		if (FSlateApplication::Get().InKismetDebuggingMode())
		{
			FSlateApplication::Get().LeaveDebuggingMode();
		}

		GUnrealEd->PlaySessionResumed();
	}
}

bool UJointDebugger::IsPlaySessionPaused()
{
	return AreAllGameWorldPaused() && IsPIESimulating();
}

bool UJointDebugger::IsPlaySessionRunning()
{
	return !AreAllGameWorldPaused() && IsPIESimulating();
}

bool UJointDebugger::IsPIESimulating()
{
	return GEditor->bIsSimulatingInEditor || GEditor->PlayWorld;
}

bool UJointDebugger::IsPIENotSimulating()
{
	return !GEditor->bIsSimulatingInEditor && (GEditor->PlayWorld == NULL);
}

bool UJointDebugger::IsDebugging()
{
	return IsPlaySessionPaused();
}

void UJointDebugger::NotifyDebugDataChanged(UJointEdGraph* Graph)
{
	if (Graph == nullptr) return;

	Graph->UpdateDebugData();
}

void UJointDebugger::NotifyDebugDataChanged(const UJointManager* Manager)
{
	if (Manager == nullptr) return;

	if (Manager->JointGraph == nullptr) return;

	TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(Manager);

	for (UJointEdGraph* Graph : Graphs)
	{
		if (Graph == nullptr) continue;

		Graph->UpdateDebugData();
	}
	
	// If there are some debugging actor for the manager, we need to refresh their toolkits too.
	
	/*
	TArray<AJointActor*> MatchingInstances;
	UJointDebugger::Get()->GetMatchingInstances(const_cast<UJointManager*>(Manager), MatchingInstances);
	
	for (AJointActor* Instance : MatchingInstances)
	{
		if (FJointEditorToolkit* InToolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(Instance->GetJointManager(), false, false))
		{
			InToolkit->RequestManagerViewerRefresh();
		}
	}
	*/
	
}

void UJointDebugger::NotifyDebugDataChangedToGraphNodeWidget(UJointEdGraphNode* Changed, FJointNodeDebugData* Data)
{
	if (Changed == nullptr) return;
	
	UJointManager* ChangedManager = Changed->GetJointManager();
	UJointManager* OriginalJointManager = FJointEdUtils::GetOriginalJointManager(Changed->GetJointManager());
	
	// when changed node is from an asset...
		
	// change the original asset node's widget first.
	if (UJointEdGraphNode* FoundEdGraphNode = FJointEdUtils::GetCorrespondingJointGraphNodeForJointManager(Changed, OriginalJointManager))
	{
		if (TSharedPtr<SJointGraphNodeBase> GraphNodeSlate = FoundEdGraphNode->GetGraphNodeSlate().Pin())
		{
			GraphNodeSlate->OnDebugDataChanged(Data);
		}
	}
	
	// find joint actor instances that are debugging this node and notify their toolkits too.
	
	TArray<AJointActor*> MatchingInstances;
	UJointDebugger::Get()->GetMatchingInstances(OriginalJointManager, MatchingInstances);
	
	for (AJointActor* Instance : MatchingInstances)
	{
		UJointEdGraphNode* FoundEdGraphNode = FJointEdUtils::GetCorrespondingJointGraphNodeForJointManager(Changed, Instance->GetJointManager());
		
		if (!FoundEdGraphNode) continue;
		
		if (TSharedPtr<SJointGraphNodeBase> GraphNodeSlate = FoundEdGraphNode->GetGraphNodeSlate().Pin())
		{
			GraphNodeSlate->OnDebugDataChanged(Data);
		}
	}
}

TArray<FJointNodeDebugData>* UJointDebugger::GetCorrespondingDebugDataForGraph(UJointEdGraph* Graph)
{
	TArray<FJointNodeDebugData>* OutDebugData = nullptr;

	if (Graph == nullptr) return OutDebugData;

	UJointManager* JointManagerToSearchFrom = FJointEdUtils::GetOriginalJointManager(Graph->GetJointManager());
	
	//find the corresponding graph from the original Joint manager - probably via path name comparison.

	if (JointManagerToSearchFrom == nullptr) return OutDebugData;
	
	TArray<UJointEdGraph*> AllGraphs = UJointEdGraph::GetAllGraphsFrom(JointManagerToSearchFrom);

	const FString& InGraphPath = Graph->GetPathName(Graph->GetJointManager());
	
	for (UJointEdGraph* IterGraph : AllGraphs)
	{
		if (IterGraph == nullptr) continue;

		if (IterGraph->GetPathName(IterGraph->GetJointManager()) != InGraphPath) continue;

		OutDebugData = &IterGraph->DebugData;

		break;
	}

	return OutDebugData;
}

FJointNodeDebugData* UJointDebugger::GetDebugDataForInstance(UJointEdGraphNode* Node)
{
	if (!Node) return nullptr;

	UJointEdGraph* Graph = Node->GetCastedGraph();

	if (!Graph) return nullptr;
	
	return GetDebugDataForInstanceFrom(GetCorrespondingDebugDataForGraph(Graph), Node);
}

FJointNodeDebugData* UJointDebugger::GetDebugDataForInstance(UJointNodeBase* Node)
{
	if (!Node) return nullptr;

	UJointEdGraph* Graph = FJointEdUtils::FindGraphForNodeInstance(Node);

	if (!Graph) return nullptr;
	
	return GetDebugDataForInstanceFrom(GetCorrespondingDebugDataForGraph(Graph), Node);
}

void UJointDebugger::OnJointNodeBeginPlayed(AJointActor* JointActor, UJointNodeBase* JointNodeBase)
{
	if (IsInstanceDebugging(JointActor))
	{
		if (const FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(JointActor->GetJointManager(), false, false); !Toolkit) return;

		if (UJointEdGraphNode* OriginalNode = FJointEdUtils::FindGraphNodeWithProvidedNodeInstanceGuid(JointActor->GetJointManager(), JointNodeBase->GetNodeGuid()))
		{
			if (const UJointEditorSettings* EditorSettings = UJointEditorSettings::Get())
			{
				if (OriginalNode && OriginalNode->GetGraphNodeSlate().IsValid())
				{
					TSharedPtr<SJointGraphNodeBase> GraphNodeSlate = OriginalNode->GetGraphNodeSlate().Pin();
					
					GraphNodeSlate->PlayDebuggerAnimation(false,true, false, false);
				}
			}
		}
	}
}

void UJointDebugger::OnJointNodeEndPlayed(AJointActor* JointActor, UJointNodeBase* JointNodeBase)
{
	if (IsInstanceDebugging(JointActor))
	{
		if (const FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(JointActor->GetJointManager(), false, false); !Toolkit) return;

		if (UJointEdGraphNode* OriginalNode = FJointEdUtils::FindGraphNodeWithProvidedNodeInstanceGuid(JointActor->GetJointManager(), JointNodeBase->GetNodeGuid()))
		{
			if (const UJointEditorSettings* EditorSettings = UJointEditorSettings::Get())
			{
				if (OriginalNode && OriginalNode->GetGraphNodeSlate().IsValid())
				{
					TSharedPtr<SJointGraphNodeBase> GraphNodeSlate = OriginalNode->GetGraphNodeSlate().Pin();
					
					GraphNodeSlate->PlayDebuggerAnimation(false,false, false, true);
				}
			}
		}
	}
}

void UJointDebugger::OnJointNodePending(AJointActor* JointActor, UJointNodeBase* JointNodeBase)
{
	if (IsInstanceDebugging(JointActor))
	{
		if (const FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(JointActor->GetJointManager(), false, false); !Toolkit) return;
		
		if (UJointEdGraphNode* OriginalNode = FJointEdUtils::FindGraphNodeWithProvidedNodeInstanceGuid(JointActor->GetJointManager(), JointNodeBase->GetNodeGuid()))
		{
			if (const UJointEditorSettings* EditorSettings = UJointEditorSettings::Get())
			{
				if (OriginalNode && OriginalNode->GetGraphNodeSlate().IsValid())
				{
					TSharedPtr<SJointGraphNodeBase> GraphNodeSlate = OriginalNode->GetGraphNodeSlate().Pin();
					
					if (UJointNodeBase* Node = OriginalNode->GetCastedNodeInstance())
					{
						/**
						 * WE ONLY show pending animation if the node is not ended yet.
						 * 
						 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
						 * !!!!!!!Don't delete this comment block - it is here to remind why this check is needed.!!!!!!!!!!
						 * I spend 3 hours debugging why the nodes are not showing the ended play animation again....
						 * It happened like, 3th time. for every refactoring I do (shh)
						 * 
						 * 
						 * Nodes can be marked as pending after the endplay event. Because the pending state is a state that represent that the node is done doing its job, so basically it still make sense to mark it as pending after ended play.
						 * But for the debugger animation, we don't want to show pending state after ended play, because it looks like as if the node is still running, while it is ended already. (which is confusing).
						 * (This happens because pending state is more like a, flag, rather than a state - and here we are representing it as a state for the better visualization in the debugger)
						 */
						
						// dumbass, check this always. nodes can be marked as pending after the endplay event.
						if (!Node->IsNodeEndedPlay()) 
						{
							GraphNodeSlate->PlayDebuggerAnimation(false,false, true, false);
						}
					}
				}
			}
		}
	}
}


bool UJointDebugger::CheckWhetherToBreakExecution(AJointActor* Instance, const FJointActorExecutionElement& Element)
{
	// If bDebuggerEnabled is false, we do not break execution at all.
	if (!UJointEditorSettings::Get()->bDebuggerEnabled) return false;
	
	// We only break on BeginPlay execution types currently. (not sure if anything else makes sense)
	if (Element.ExecutionType != EJointActorExecutionType::PreBeginPlay) return false;
	
	UJointNodeBase* NodeToCheck = Element.TargetNode.Get();
	UJointNodeBase* NodeToCheckInOriginalAsset = FJointEdUtils::GetOriginalJointNodeFromJointNode(NodeToCheck);
	const FJointActorExecutionElement* LastPausedExecution = JointActorToLastPausedExecutedMap.Find(Instance);
	
	// If we are re-executing the last paused node, do not break again.
	if (LastPausedExecution != nullptr && *LastPausedExecution == Element) return false;
	
	// If no instance, we can't debug.
	if (Instance == nullptr) return false;
	
	// If node information is missing we can't evaluate break conditions.
	if (NodeToCheck == nullptr || NodeToCheckInOriginalAsset == nullptr) return false;
	
	
	// Helper to perform the common pause actions and return true (break execution).
	auto DoPause = [&](UJointNodeBase* Node) -> bool
	{
		ClearStepActionRequest();
		AssignDebuggingInstance(Instance);
		SetLastPausedExecutionForJointActor(Instance, Element);
		FocusGraphOnNode(Node);
		PausePlaySession();
		PlayPauseNodeAnimation(Node);
		return true;
	};
	
	
	UJointEdGraph* EdGraph = FJointEdUtils::FindGraphForNodeInstance(NodeToCheckInOriginalAsset);
	TArray<FJointNodeDebugData>* DebugDataArr = UJointDebugger::GetCorrespondingDebugDataForGraph(EdGraph);

	// Check whether we have any debug data for the node that can cause the pause action.
	if (FJointNodeDebugData* DebugData = GetDebugDataForInstanceFrom(DebugDataArr, NodeToCheckInOriginalAsset); DebugData != nullptr)
	{
		if (DebugData->bDisabled)
		{
			// This node is disabled, so notify that this node will not be played.
			return true;
		}
		if (DebugData->bHasBreakpoint && DebugData->bIsBreakpointEnabled)
		{
			// Hit a breakpoint: always stop here.
			return DoPause(NodeToCheck);
		}
	}

	// Check step requests.
	
	// Check step into requests. If present, always stop.
	if (CheckHasStepForwardIntoRequest())
	{
		return DoPause(NodeToCheck);
	}

	// Check step over requests. If present, stop only if we are not entering a child node.
	if (CheckHasStepForwardOverRequest())
	{
		// Stop if the current node is not a child of the last paused node.
		UJointNodeBase* LastPaused = nullptr;
		if (const FJointActorExecutionElement* Found = JointActorToLastPausedExecutedMap.Find(Instance))
		{
			LastPaused = Found->TargetNode.Get();
		}
		if (NodeToCheck->GetParentNode() != LastPaused)
		{
			return DoPause(NodeToCheck);
		}
	}

	// Check step out requests. If present, stop when we reach a base node.
	if (CheckHasStepOutRequest())
	{
		// Stop when we reach a base node (no parent).
		if (NodeToCheck->GetParentNode() == nullptr)
		{
			return DoPause(NodeToCheck);
		}
	}

	// No break conditions met.
	return false;
}

void UJointDebugger::PlayPauseNodeAnimation(const UJointNodeBase* Node)
{
	AJointActor* JointActor = Node->GetHostingJointInstance();
	
	if (IsInstanceDebugging(JointActor))
	{
		if (const FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(JointActor->GetJointManager(), false, false); !Toolkit) return;

		if (UJointEdGraphNode* OriginalNode = FJointEdUtils::FindGraphNodeWithProvidedNodeInstanceGuid(JointActor->GetJointManager(), Node->GetNodeGuid()))
		{
			if (const UJointEditorSettings* EditorSettings = UJointEditorSettings::Get())
			{
				if (OriginalNode && OriginalNode->GetGraphNodeSlate().IsValid())
				{
					TSharedPtr<SJointGraphNodeBase> GraphNodeSlate = OriginalNode->GetGraphNodeSlate().Pin();
					
					GraphNodeSlate->PlayDebuggerAnimation(true,false, false, false);
				}
			}
		}
	}
}

void UJointDebugger::AssignDebuggingInstance(AJointActor* Instance)
{
	if (!DebuggingJointInstances.Contains(Instance))
	{
		DebuggingJointInstances.Add(Instance);

		OnInstanceAddedToLookUp(Instance);
	}
}

void UJointDebugger::RemoveDebuggingInstance(AJointActor* Instance)
{
	if (DebuggingJointInstances.Contains(Instance))
	{
		DebuggingJointInstances.Remove(Instance);

		OnInstanceRemovedFromLookUp(Instance);
	}
}


bool UJointDebugger::IsInstanceDebugging(AJointActor* Instance)
{
	return DebuggingJointInstances.Contains(Instance);
}

void UJointDebugger::SetLastPausedExecutionForJointActor(AJointActor* Instance, const FJointActorExecutionElement& Execution)
{
	JointActorToLastPausedExecutedMap.Add(Instance, Execution);
}

void UJointDebugger::ClearLastPausedExecutionForJointActor(AJointActor* Instance)
{
	JointActorToLastPausedExecutedMap.Remove(Instance);
}

void UJointDebugger::OnInstanceAddedToLookUp(AJointActor* Instance)
{
	if (FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(Instance->GetJointManager(), true))
	{
		Toolkit->SetDebuggingJointInstance(Instance);
	}
}

void UJointDebugger::OnInstanceRemovedFromLookUp(AJointActor* Instance)
{
	if (const FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(Instance->GetJointManager(), false); Toolkit != nullptr)
	{
		//TODO: Notify the toolkit that we eliminated the debugging session in the debugger. Make it display the original normal editor asset again.
	}
}

void UJointDebugger::AssignInstanceToKnownInstance(AJointActor* Instance)
{
	if (!KnownJointInstances.Contains(Instance))
	{
		KnownJointInstances.Add(Instance);

		OnInstanceAddedToKnownInstance(Instance);
	}
}

void UJointDebugger::RemoveInstanceFromKnownInstance(AJointActor* Instance)
{
	if (KnownJointInstances.Contains(Instance))
	{
		KnownJointInstances.Remove(Instance);

		OnInstanceRemovedFromKnownInstance(Instance);
	}
}

void UJointDebugger::OnInstanceAddedToKnownInstance(AJointActor* Instance)
{
}

void UJointDebugger::OnInstanceRemovedFromKnownInstance(AJointActor* Instance)
{
}

void UJointDebugger::ClearDebugSessionData()
{
	DebuggingJointInstances.Empty();
	KnownJointInstances.Empty();

	ClearStepActionRequest();
}

void UJointDebugger::FocusGraphOnNode(UJointNodeBase* NodeToFocus)
{
	FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(NodeToFocus->GetJointManager(), true);

	//Cancel the action if the toolkit was not present.
	if (Toolkit == nullptr) return;

	if (UJointEdGraphNode* OriginalNode = FJointEdUtils::FindGraphNodeWithProvidedNodeInstanceGuid(NodeToFocus->GetJointManager(), NodeToFocus->GetNodeGuid()))
	{
		TSet<UObject*> ObjectsToSelect;

		ObjectsToSelect.Add(OriginalNode);

		Toolkit->StartHighlightingNode(OriginalNode, true);

		Toolkit->JumpToHyperlink(OriginalNode);
	}
}

void UJointDebugger::StepForwardInto()
{
	bRequestedStepForwardInto = true;

	ResumePlaySession();
}


bool UJointDebugger::CanStepForwardInto() const
{
	return IsPIESimulating() && AreAllGameWorldPaused() && !IsStepActionRequested();
}

void UJointDebugger::StepForwardOver()
{
	bRequestedStepForwardOver = true;

	ResumePlaySession();
}

bool UJointDebugger::CanStepForwardOver() const
{
	return IsPIESimulating() && AreAllGameWorldPaused() && !IsStepActionRequested();
}

void UJointDebugger::StepOut()
{
	bRequestedStepOut = true;

	ResumePlaySession();
}

bool UJointDebugger::CanStepOut() const
{
	return IsPIESimulating() && AreAllGameWorldPaused() && !IsStepActionRequested();
}

bool UJointDebugger::CheckHasStepForwardIntoRequest() const
{
	return bRequestedStepForwardInto;
}

bool UJointDebugger::CheckHasStepForwardOverRequest() const
{
	return bRequestedStepForwardOver;
}

bool UJointDebugger::CheckHasStepOutRequest() const
{
	return bRequestedStepOut;
}

bool UJointDebugger::IsStepActionRequested() const
{
	return bRequestedStepForwardInto || bRequestedStepForwardOver || bRequestedStepOut;
}

void UJointDebugger::ClearStepActionRequest()
{
	bRequestedStepForwardInto = false;
	bRequestedStepForwardOver = false;
	bRequestedStepOut = false;
}

void UJointDebugger::RestartExecutionOfPausedJointActors()
{
	for (TWeakObjectPtr<class AJointActor> JointActor : DebuggingJointInstances)
	{
		if (!JointActor.IsValid()) continue;
		
		JointActor->ProcessExecutionQueue();
	}
}

FJointNodeDebugData* UJointDebugger::GetDebugDataForInstanceFrom(TArray<FJointNodeDebugData>* TargetDataArrayPtr, UJointEdGraphNode* Node)
{
	FJointNodeDebugData* OutDebugData = nullptr;

	if (!Node || !TargetDataArrayPtr) return nullptr;

	UJointNodeBase* InNode = Node->GetCastedNodeInstance();

	if (!InNode) return nullptr;

	FString RelPath = InNode->GetPathName(InNode->GetJointManager());

	for (FJointNodeDebugData& Data : *TargetDataArrayPtr)
	{
		if (Data.Node == nullptr || Data.Node->GetCastedNodeInstance() == nullptr || Data.Node->GetCastedNodeInstance()->GetJointManager() == nullptr) continue;

		//UE_LOG(LogJointEditor,Log,TEXT("Path1 %s, Path2 %s"),*Data.Node->GetCastedNodeInstance()->GetPathName(Data.Node->GetJointManager()), *RelPath);

		if (Data.Node->GetCastedNodeInstance()->GetPathName(Data.Node->GetJointManager()) != RelPath) continue;

		OutDebugData = &Data;

		break;
	}
	
	return OutDebugData;
}


FJointNodeDebugData* UJointDebugger::GetDebugDataForInstanceFrom(TArray<FJointNodeDebugData>* TargetDataArrayPtr, UJointNodeBase* NodeInstance)
{
	FJointNodeDebugData* OutDebugData = nullptr;
	
	if (!NodeInstance || !TargetDataArrayPtr) return nullptr;

	FString RelPath = NodeInstance->GetPathName(NodeInstance->GetJointManager());

	for (FJointNodeDebugData& Data : *TargetDataArrayPtr)
	{
		if (Data.Node == nullptr || Data.Node->GetCastedNodeInstance() == nullptr || Data.Node->GetCastedNodeInstance()->GetJointManager() == nullptr)
			continue;

		//UE_LOG(LogJointEditor,Log,TEXT("Path1 %s, Path2 %s"),*Data.Node->GetCastedNodeInstance()->GetPathName(Data.Node->GetJointManager()), *RelPath);

		if (Data.Node->GetCastedNodeInstance()->GetPathName(Data.Node->GetJointManager()) != RelPath) continue;

		OutDebugData = &Data;

		break;
	}
	
	return OutDebugData;
}

#undef LOCTEXT_NAMESPACE
