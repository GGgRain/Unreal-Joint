// Fill out your copyright notice in the Description page of Project Settings.


#include "Editor/Debug/JointDebugger.h"

#include "AssetViewUtils.h"
#include "JointActor.h"
#include "JointEdGraph.h"
#include "JointEditorToolkit.h"
#include "JointManager.h"
#include "Joint.h"

#include "JointEditor.h"
#include "JointEditorSettings.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
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
			CastedModule->OnJointExecutionExceptionDelegate.BindUObject(
				this, &UJointDebugger::CheckWhetherToBreakExecution);
			CastedModule->OnJointDebuggerMentionJointBeginPlay.
				BindUObject(this, &UJointDebugger::OnJointBegin);
			CastedModule->OnJointDebuggerMentionJointEndPlay.BindUObject(this, &UJointDebugger::OnJointEnd);
			CastedModule->OnJointDebuggerMentionNodeBeginPlay.BindUObject(
				this, &UJointDebugger::OnJointNodeBeginPlayed);
			CastedModule->OnJointDebuggerMentionNodeEndPlay.BindUObject(
				this, &UJointDebugger::OnJointNodeEndPlayed);
			CastedModule->OnJointDebuggerMentionNodePending.
				BindUObject(this, &UJointDebugger::OnJointNodePending);
		}
	}

#endif
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
			CastedModule->OnJointDebuggerMentionJointBeginPlay.Unbind();
			CastedModule->OnJointDebuggerMentionJointEndPlay.Unbind();
			CastedModule->OnJointDebuggerMentionNodeBeginPlay.Unbind();
			CastedModule->OnJointDebuggerMentionNodeEndPlay.Unbind();
			CastedModule->OnJointDebuggerMentionNodePending.Unbind();
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
	RestartRequestedBeginPlayExecutionWhileInDebuggingSession();
}

void UJointDebugger::OnSingleStepPIE(bool bArg)
{
	RestartRequestedBeginPlayExecutionWhileInDebuggingSession();
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
		FJointEditorToolkit* ToolKit = FJointEditorToolkit::FindOrOpenEditorInstanceFor(JointInstance->GetJointManager(), false);

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


void UJointDebugger::NotifyDebugDataChanged(const UJointManager* Manager)
{
	if (Manager == nullptr) return;

	if (Manager->JointGraph == nullptr) return;

	if (UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(Manager->JointGraph))
	{
		CastedGraph->UpdateDebugData();
	}
}

TArray<FJointNodeDebugData>* UJointDebugger::GetDebugDataFromJointManager(UJointManager* Manager)
{
	TArray<FJointNodeDebugData>* OutDebugData = nullptr;

	if (Manager == nullptr) return OutDebugData;

	UJointManager* JointManagerToSearchFrom = GetOriginalJointManager(Manager);

	if (JointManagerToSearchFrom == nullptr) return OutDebugData;

	if (JointManagerToSearchFrom->JointGraph == nullptr) return OutDebugData;

	if (UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(JointManagerToSearchFrom->JointGraph))
	{
		OutDebugData = &CastedGraph->DebugData;
	}

	return OutDebugData;
}

FJointNodeDebugData* UJointDebugger::GetDebugDataFor(UJointEdGraphNode* Node)
{
	if (Node == nullptr || Node->GetCastedNodeInstance() == nullptr) return nullptr;

	return GetDebugDataFor(Node->GetCastedNodeInstance());
}

FJointNodeDebugData* UJointDebugger::GetDebugDataFor(UJointNodeBase* NodeInstance)
{
	FJointNodeDebugData* OutDebugData = nullptr;

	if (NodeInstance == nullptr) return nullptr;

	TArray<FJointNodeDebugData>* DebugData = GetDebugDataFromJointManager(NodeInstance->GetJointManager());

	if (DebugData == nullptr) return nullptr;

	FString RelPath = NodeInstance->GetPathName(NodeInstance->GetJointManager());

	for (FJointNodeDebugData& Data : *DebugData)
	{
		if (Data.Node == nullptr || Data.Node->GetCastedNodeInstance() == nullptr || Data.Node->GetCastedNodeInstance()
			->GetJointManager() == nullptr) continue;

		//UE_LOG(LogTemp,Log,TEXT("Path1 %s, Path2 %s"),*Data.Node->GetCastedNodeInstance()->GetPathName(Data.Node->GetJointManager()), *RelPath);

		if (Data.Node->GetCastedNodeInstance()->GetPathName(Data.Node->GetJointManager()) != RelPath) continue;

		OutDebugData = &Data;

		break;
	}

	return OutDebugData;
}

UJointManager* UJointDebugger::GetOriginalJointManager(
	UJointManager* InJointManager)
{
	if (InJointManager != nullptr && !InJointManager->IsAsset())
	{
		if (UObject* Outer = InJointManager->GetOuter())
		{
			if (AJointActor* JointActor = Cast<AJointActor>(Outer))
			{
				if (JointActor->OriginalJointManager && JointActor->OriginalJointManager->IsValidLowLevel())
				{
					return JointActor->OriginalJointManager;
				}
			}
		}
	}
	else if (InJointManager != nullptr && InJointManager->IsAsset())
	{
		return InJointManager;
	}

	return nullptr;
}

UJointEdGraphNode* UJointDebugger::GetOriginalJointGraphNodeFromJointGraphNode(
	UJointEdGraphNode* InJointEdGraphNode)
{
	if (InJointEdGraphNode != nullptr && InJointEdGraphNode->GetJointManager() != nullptr)
	{
		UJointManager* OriginalAssetJointManager = GetOriginalJointManager(
			InJointEdGraphNode->GetJointManager());

		//if this node is from the original Joint manager : return itself.
		if (InJointEdGraphNode->GetJointManager() == OriginalAssetJointManager) return InJointEdGraphNode;

		if (OriginalAssetJointManager && OriginalAssetJointManager->JointGraph)
		{
			if (UJointEdGraph* JointEdGraph = Cast<UJointEdGraph>(OriginalAssetJointManager->JointGraph))
			{
				for (TObjectPtr<UEdGraphNode> EdGraphNode : JointEdGraph->Nodes)
				{
					if (!EdGraphNode) continue;

					UJointEdGraphNode* CastedJointEdGraphNode = Cast<UJointEdGraphNode>(EdGraphNode);

					if (!CastedJointEdGraphNode) continue;

					if (InJointEdGraphNode->GetPathName(InJointEdGraphNode->GetJointManager()) !=
						CastedJointEdGraphNode->GetPathName(CastedJointEdGraphNode->GetJointManager()))
						continue;

					return CastedJointEdGraphNode;
				}
			}
		}
	}

	return nullptr;
}

bool UJointDebugger::CheckWhetherToBreakExecution(AJointActor* Instance, UJointNodeBase* NodeToCheck)
{
	UJointNodeBase* NodeToCheckInOriginalAsset = NodeToCheck;

	FString NodeToCheckPath = NodeToCheck->GetPathName(Instance->GetJointManager());

	if (Instance && Instance->OriginalJointManager)
	{
		for (UJointNodeBase* Node : Instance->OriginalJointManager->Nodes)
		{
			if (Node->GetPathName(Instance->OriginalJointManager) != NodeToCheckPath) continue;

			NodeToCheckInOriginalAsset = Node;

			break;
		}
	}

	if (!UJointEditorSettings::Get()->bDebuggerEnabled) return false;

	bool Result = false;

	if (Instance != nullptr)
	{
		if (IsDebugging())
		{
			//If it was already debugging, then store that node and return true to prevent the execution.

			AssignNodeToDelayedNodeCache(NodeToCheck);

			Result = true;
		}
		else
		{
			//Do not interrupt if we already broke with the node.
			if (PausedJointNodeBase == NodeToCheck) return false;

			//Check whether we have any debug data for the node that can cause the pause action.
			if (FJointNodeDebugData* DebugData = GetDebugDataFor(NodeToCheckInOriginalAsset); DebugData != nullptr)
			{
				if (DebugData->bDisabled)
				{
					//This node is disabled, so notify that this node will not be played.
					Result = true;

					return Result;
				}
				else if (DebugData->bHasBreakpoint && DebugData->bIsBreakpointEnabled)
				{
					Result = true;

					ClearStepActionRequest();

					AssignInstanceToLookUp(Instance);

					PausedJointNodeBase = NodeToCheck;

					FocusGraphOnNode(NodeToCheck);

					AssignNodeToDelayedNodeCache(NodeToCheck);

					PausePlaySession();

					UE_LOG(LogTemp, Warning,
					       TEXT(
						       "Session paused due to the Joint debugger requested the pause for the the Joint node '%s' in Joint instance '%s'"
					       ), *PausedJointNodeBase->GetName(), *Instance->GetName());

					return Result;
				}
			}


			//Check for the steps
			if (CheckHasStepForwardIntoRequest())
			{
				UJointNodeBase* CurrentParentNode = NodeToCheck;

				if (CurrentParentNode->GetParentNode() == PausedJointNodeBase
					|| PausedJointNodeBase->SubNodes.IsEmpty()
					|| PausedJointNodeBase->IsNodeEndedPlay()) // for the conditions and possible reverting nodes.
				{
					ClearStepActionRequest();

					Result = true;

					AssignInstanceToLookUp(Instance);

					PausedJointNodeBase = NodeToCheck;

					FocusGraphOnNode(NodeToCheck);

					AssignNodeToDelayedNodeCache(NodeToCheck);

					PausePlaySession();

					UE_LOG(LogTemp, Warning,
					       TEXT(
						       "Session paused due to the Joint debugger requested the pause for the the Joint node '%s' in Joint instance '%s' due to the continue to step forward into action"
					       ), *PausedJointNodeBase->GetName(), *Instance->GetName());
				}
			}
			else if (CheckHasStepForwardOverRequest())
			{
				UJointNodeBase* CurrentParentNode = NodeToCheck;

				bool bShouldBreak = false;

				while (CurrentParentNode != nullptr && !bShouldBreak)
				{
					if (CurrentParentNode == PausedJointNodeBase)
					{
						bShouldBreak = false;

						break;
					}

					if (CurrentParentNode == PausedJointNodeBase->GetParentNode()) bShouldBreak = true;

					CurrentParentNode = CurrentParentNode->GetParentNode();
				}

				if (bShouldBreak)
				{
					ClearStepActionRequest();

					Result = true;

					AssignInstanceToLookUp(Instance);

					PausedJointNodeBase = NodeToCheck;

					FocusGraphOnNode(NodeToCheck);

					AssignNodeToDelayedNodeCache(NodeToCheck);

					PausePlaySession();

					UE_LOG(LogTemp, Warning,
					       TEXT(
						       "Session paused due to the Joint debugger requested the pause for the the Joint node '%s' in Joint instance '%s' due to the continue to step forward into action"
					       ), *PausedJointNodeBase->GetName(), *Instance->GetName());
				}
			}
			else if (CheckHasStepOutRequest())
			{
				if (NodeToCheck->GetParentNode() == nullptr)
				{
					ClearStepActionRequest();

					Result = true;

					AssignInstanceToLookUp(Instance);

					PausedJointNodeBase = NodeToCheck;

					FocusGraphOnNode(NodeToCheck);

					AssignNodeToDelayedNodeCache(NodeToCheck);

					PausePlaySession();

					UE_LOG(LogTemp, Warning,
					       TEXT(
						       "Session paused due to the Joint debugger requested the pause for the the Joint node '%s' in Joint instance '%s' due to the continue to step forward into action"
					       ), *PausedJointNodeBase->GetName(), *Instance->GetName());
				}
			}
		}
	}

	return Result;
}


void UJointDebugger::OnJointNodeBeginPlayed(AJointActor* JointActor, UJointNodeBase* JointNodeBase)
{
	if (CheckHasInstanceInLookUp(JointActor))
	{
		if (const FJointEditorToolkit* Toolkit = FJointEditorToolkit::FindOrOpenEditorInstanceFor(JointActor->GetJointManager(), false); !Toolkit) return;
		
		if (UJointEdGraphNode* OriginalNode = GetEditorNodeFor(JointNodeBase, JointActor->GetJointManager()))
		{
				
			if(const UJointEditorSettings* EditorSettings = UJointEditorSettings::Get())
			{
				if (OriginalNode && OriginalNode->GetGraphNodeSlate().IsValid())
				{
					OriginalNode->GetGraphNodeSlate().Pin()->PlayHighlightAnimation(true);
					OriginalNode->GetGraphNodeSlate().Pin()->PlayNodeBodyColorAnimation(EditorSettings->DebuggerPlayingNodeColor, true);
				}
			}
				
		}
	}
}

void UJointDebugger::OnJointNodePending(AJointActor* JointActor, UJointNodeBase* JointNodeBase)
{
	if (CheckHasInstanceInLookUp(JointActor))
	{
		FJointEditorToolkit* Toolkit = nullptr;

		FJointEditorToolkit::FindOrOpenEditorInstanceFor(JointActor->GetJointManager(), false);

		if (!Toolkit) return;
		
		if (UJointEdGraphNode* OriginalNode = GetEditorNodeFor(JointNodeBase, JointActor->GetJointManager()))
		{
				
			if(const UJointEditorSettings* EditorSettings = UJointEditorSettings::Get())
			{
				if (OriginalNode && OriginalNode->GetGraphNodeSlate().IsValid())
					OriginalNode->GetGraphNodeSlate().Pin()->PlayNodeBodyColorAnimation(EditorSettings->DebuggerPendingNodeColor, true);
			}
				
		}
	}
}

void UJointDebugger::OnJointNodeEndPlayed(AJointActor* JointActor, UJointNodeBase* JointNodeBase)
{
	if (CheckHasInstanceInLookUp(JointActor))
	{
		if (const FJointEditorToolkit* Toolkit = FJointEditorToolkit::FindOrOpenEditorInstanceFor(JointActor->GetJointManager(), false); !Toolkit) return;
		
		if (UJointEdGraphNode* OriginalNode = GetEditorNodeFor(JointNodeBase, JointActor->GetJointManager()))
		{
				
			if(const UJointEditorSettings* EditorSettings = UJointEditorSettings::Get())
			{
				if (OriginalNode && OriginalNode->GetGraphNodeSlate().IsValid())
					OriginalNode->GetGraphNodeSlate().Pin()->PlayNodeBodyColorAnimation(EditorSettings->DebuggerEndedNodeColor, false);
			}
				
		}
	}
}


void UJointDebugger::AssignNodeToDelayedNodeCache(TWeakObjectPtr<UJointNodeBase> Node)
{
	//Reallocate the elements.
	DelayedBeginPlayedJointNodeBases.RemoveAll([](const TWeakObjectPtr<UJointNodeBase>& Elem)
	{
		return Elem.Get() == nullptr;
	});

	if (Node == nullptr) return;

	//If it is already assigned, then ignore it.
	if (DelayedBeginPlayedJointNodeBases.Contains(Node)) return;

	if (Node->GetParentNode() == nullptr)
	{
		// If the parent node of the provided node was not present, then attach it to the tail because it means that this node is another base node on the graph.

		UE_LOG(LogTemp, Warning,
		       TEXT(
			       "The begin play action of the Joint node '%s' has been delayed due to the active debug session, and it will be triggered by the debugger when you resume the PIE"
		       ), *Node->GetName());

		DelayedBeginPlayedJointNodeBases.Add(Node);
	}
	else if (Node->GetParentNode() != nullptr && DelayedBeginPlayedJointNodeBases.Contains(Node->GetParentNode()))
	{
		// If the parent node of the provided node exists in the delayed execution array, then insert it to that point since it must be played prior to the other nodes in higher hierarchy level.

		const int ParentNodeIndex = DelayedBeginPlayedJointNodeBases.Find(Node->GetParentNode());

		//Try to find the nodes that are sharing the same parent node and attach our node on the tail of those nodes.

		int SameParentNodeOffset = 1;

		while (DelayedBeginPlayedJointNodeBases.IsValidIndex(ParentNodeIndex + SameParentNodeOffset))
		{
			if (DelayedBeginPlayedJointNodeBases[ParentNodeIndex + SameParentNodeOffset] != nullptr &&
				DelayedBeginPlayedJointNodeBases[ParentNodeIndex + SameParentNodeOffset].Get()->GetParentNode() ==
				Node->GetParentNode())
			{
				SameParentNodeOffset++;
			}
			else
			{
				break;
			}
		}

		DelayedBeginPlayedJointNodeBases.Insert(Node, ParentNodeIndex + SameParentNodeOffset);

		UE_LOG(LogTemp, Warning,
		       TEXT(
			       "The begin play action of the Joint node '%s' has been delayed due to the active debug session, and it will be triggered by the debugger when you resume the PIE"
		       ), *Node->GetName());
	}
	else if (Node->GetParentNode() != nullptr && !DelayedBeginPlayedJointNodeBases.Contains(Node->GetParentNode()))
	{
		DelayedBeginPlayedJointNodeBases.Add(Node);

		UE_LOG(LogTemp, Warning,
		       TEXT(
			       "The begin play action of the Joint node '%s' has been delayed due to the active debug session, and it will be triggered by the debugger when you resume the PIE"
		       ), *Node->GetName());
	}
}

void UJointDebugger::AssignInstanceToLookUp(AJointActor* Instance)
{
	if (!DebuggingJointInstances.Contains(Instance))
	{
		DebuggingJointInstances.Add(Instance);

		OnInstanceAddedToLookUp(Instance);
	}
}

void UJointDebugger::RemoveInstanceFromLookUp(AJointActor* Instance)
{
	if (DebuggingJointInstances.Contains(Instance))
	{
		DebuggingJointInstances.Remove(Instance);

		OnInstanceRemovedFromLookUp(Instance);
	}
}

bool UJointDebugger::CheckHasInstanceInLookUp(AJointActor* Instance)
{
	return DebuggingJointInstances.Contains(Instance);
}

void UJointDebugger::OnInstanceAddedToLookUp(AJointActor* Instance)
{
	if (FJointEditorToolkit* Toolkit = FJointEditorToolkit::FindOrOpenEditorInstanceFor(Instance->GetJointManager(), true))
	{
		Toolkit->SetDebuggingJointInstance(Instance);
	}
}

void UJointDebugger::OnInstanceRemovedFromLookUp(AJointActor* Instance)
{
	if (const FJointEditorToolkit* Toolkit = FJointEditorToolkit::FindOrOpenEditorInstanceFor(Instance->GetJointManager(), false); Toolkit != nullptr)
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
	PausedJointNodeBase.Reset();

	ClearStepActionRequest();
}


UJointEdGraphNode* UJointDebugger::GetEditorNodeFor(UJointNodeBase* JointNodeBase,
                                                       UJointManager* JointManager)
{
	if (JointManager == nullptr) return nullptr;
	if (JointManager->JointGraph == nullptr) return nullptr;

	UJointEdGraph* CastedGraph = nullptr;

	CastedGraph = Cast<UJointEdGraph>(JointManager->JointGraph);

	if (CastedGraph == nullptr) return nullptr;

	TSet<TSoftObjectPtr<UJointEdGraphNode>> GraphNodes = CastedGraph->GetCacheJointGraphNodes();

	for (TSoftObjectPtr<UJointEdGraphNode> CachedJointGraphNode : GraphNodes)
	{
		if (CachedJointGraphNode == nullptr) continue;

		if (CachedJointGraphNode->GetCastedNodeInstance() != nullptr)
		{
			if (CachedJointGraphNode->GetCastedNodeInstance()->NodeGuid == JointNodeBase->NodeGuid)
				return
					CachedJointGraphNode.Get();
		}
	}

	return nullptr;
}


void UJointDebugger::FocusGraphOnNode(UJointNodeBase* NodeToFocus)
{
	FJointEditorToolkit* Toolkit = FJointEditorToolkit::FindOrOpenEditorInstanceFor(NodeToFocus->GetJointManager(), true);

	//Cancel the action if the toolkit was not present.
	if (Toolkit == nullptr) return;

	if (UJointEdGraphNode* OriginalNode = GetEditorNodeFor(NodeToFocus, NodeToFocus->GetJointManager()))
	{
		TSet<UObject*> ObjectsToSelect;

		ObjectsToSelect.Add(OriginalNode);

		Toolkit->StartHighlightingNode(OriginalNode, true);

		Toolkit->JumpToNode(OriginalNode);
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


void UJointDebugger::RestartRequestedBeginPlayExecutionWhileInDebuggingSession()
{
	if (DelayedBeginPlayedJointNodeBases.IsEmpty()) return;

	TArray<TWeakObjectPtr<UJointNodeBase>> SavedDelayedBeginPlayedJointNodeBases =
		DelayedBeginPlayedJointNodeBases;

	UE_LOG(LogTemp, Warning, TEXT("Joint debugger is restarting the delayed nodes in the debugger session..."));


	for (TWeakObjectPtr<UJointNodeBase> SavedDelayedBeginPlayedJointNodeBase :
	     SavedDelayedBeginPlayedJointNodeBases)
	{
		if (!SavedDelayedBeginPlayedJointNodeBase.IsValid()) continue;

		UE_LOG(LogTemp, Warning, TEXT("Joint debugger is requesting the restart of the Joint node '%s'.... : "),
		       *SavedDelayedBeginPlayedJointNodeBase->GetName());


		SavedDelayedBeginPlayedJointNodeBase->RequestNodeBeginPlay(
			SavedDelayedBeginPlayedJointNodeBase->GetHostingJointInstance());

		//Remove it from the list if it has been played successfully.
		//Have to check with IsNodeBegunPlay() because IsPlaySessionPaused() can be triggered by the other nodes that has been started while this node's RequestNodeBeginPlay().
		if (SavedDelayedBeginPlayedJointNodeBase->IsNodeBegunPlay())
		{
			UE_LOG(LogTemp, Warning,
			       TEXT(
				       "Joint node '%s' has been successfully restarted. The node has been removed from the debugger cache."
			       ), *SavedDelayedBeginPlayedJointNodeBase->GetName());

			DelayedBeginPlayedJointNodeBases.Remove(SavedDelayedBeginPlayedJointNodeBase);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
			       TEXT(
				       "Joint node '%s' has been failed to be restarted. This might be caused by some of sub nodes's interruption."
			       ), *SavedDelayedBeginPlayedJointNodeBase->GetName());
		}

		//While the execution of the node the debugging has been started again. Stop the iteration in this case.
		if (IsDebugging())
		{
			UE_LOG(LogTemp, Warning,
			       TEXT("Restart iteration canceled with %s because the sub node requested another debug session."),
			       *SavedDelayedBeginPlayedJointNodeBase.Get()->GetName());

			break;
		}
	}
}

#undef LOCTEXT_NAMESPACE
