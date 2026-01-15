// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JointEdGraphNode.h"
#include "JointNodeDebugData.h"
#include "SubSystem/JointSubsystem.h"
#include "JointDebugger.generated.h"

class FJointEditorToolkit;
class UJointManager;
/**
 * 
 */

USTRUCT()
struct JOINTEDITOR_API FJointDebuggerNodeSet
{
	GENERATED_BODY()
	
	UPROPERTY()
	TSet<TObjectPtr<UJointNodeBase>> JointNodes;

public:

	bool CheckHasNode(UJointNodeBase* Node)
	{
		return JointNodes.Contains(Node);
	}
};

/**
 * A Debugger object for the Joint system.
 * It provides step in & step out features for the Joint system.
 */
UCLASS()
class JOINTEDITOR_API UJointDebugger : public UObject
{
	GENERATED_BODY()

public:
	UJointDebugger();

	virtual ~UJointDebugger() override;

public:

	void UnregisterJointLifecycleEvents();
	void RegisterJointLifecycleEvents();

public:
	
	void OnBeginPIE(bool bArg);
	void OnEndPIE(bool bArg);
	void OnPausePIE(bool bArg);
	void OnResumePIE(bool bArg);
	void OnSingleStepPIE(bool bArg);

public:

	void OnJointBegin(AJointActor* JointInstance, const FGuid& JointGuid);

	void OnJointEnd(AJointActor* JointInstance, const FGuid& JointGuid);

	void OnJointNodeBeginPlayed(AJointActor* JointActor, UJointNodeBase* JointNodeBase);
	
	void OnJointNodeEndPlayed(AJointActor* JointActor, UJointNodeBase* JointNodeBase);
	
	void OnJointNodePending(AJointActor* JointActor, UJointNodeBase* JointNodeBase);
	
	bool CheckWhetherToBreakExecution(AJointActor* Instance, const FJointActorExecutionElement& Element);

public:
	
	void PlayPauseNodeAnimation(const UJointNodeBase* Node);

public:

	/**
	 * Currently debugging Joint Instances.
	 * The instances in the array will be used to update the animations or provide cached data for the instance.
	 */
	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<class AJointActor>> DebuggingJointInstances;
	
	/**
	 * all known Joint instances in the world, cached for dropdown list.
	 */
	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<class AJointActor>> KnownJointInstances;
	
	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<AJointActor>, FJointActorExecutionElement> JointActorToLastPausedExecutedMap;

public:
	
	/**
	 * Assign a Joint instance to look up in the debug session.
	 * @param Instance The Joint instance to add.
	 */
	void AssignDebuggingInstance(AJointActor* Instance);

	/**
	 * Remove a Joint instance from look up in the debug session.
	 * @param Instance The Joint instance to remove.
	 */
	void RemoveDebuggingInstance(AJointActor* Instance);
	
	/**
	 * Check whether we have that instance in the look up set.
	 * @param Instance The Joint instance to check.
	 */
	bool IsInstanceDebugging(AJointActor* Instance);
	
public:
	
	void SetLastPausedExecutionForJointActor(AJointActor* Instance, const FJointActorExecutionElement& Execution);
	
	void ClearLastPausedExecutionForJointActor(AJointActor* Instance);
	
public:

	void OnInstanceAddedToLookUp(AJointActor* Instance);
	
	void OnInstanceRemovedFromLookUp(AJointActor* Instance);

public:
	
	void AssignInstanceToKnownInstance(AJointActor* Instance);
	
	void RemoveInstanceFromKnownInstance(AJointActor* Instance);
	
public:

	void OnInstanceAddedToKnownInstance(AJointActor* Instance);
	
	void OnInstanceRemovedFromKnownInstance(AJointActor* Instance);

private:

	/**
	 * Clear cached data and session related data.
	 */
	void ClearDebugSessionData();

public:

	/**
	 * Focus asset editor's graph with provided node. This action will fail if the editor is not opened yet.
	 * @param NodeToFocus runtime instance of the node to focus on the graph.
	 */
	void FocusGraphOnNode(UJointNodeBase* NodeToFocus);

public:

	//Debugger actions related to the steps. basically what it does is simply resuming the session and wait until the next node has been executed.

	/**
	 * Step the debug process into the tree. This will stop on any node execution next to this function.
	 */
	void StepForwardInto();
	bool CanStepForwardInto() const;

	/**
	 * Step the debug process over the tree. This will stop when any node has been executed, except for the nodes on the sub node tree of the last PausedJointNodeBase.
	 */
	void StepForwardOver();
	bool CanStepForwardOver() const;

	/**
	 * Step out the debug process. This will stop when a new base node has been executed.
	 */
	void StepOut();
	bool CanStepOut() const;


	bool CheckHasStepForwardIntoRequest() const;
	bool CheckHasStepForwardOverRequest() const;
	bool CheckHasStepOutRequest() const;

	
	bool IsStepActionRequested() const;

	void ClearStepActionRequest();
	
private:

	bool bRequestedStepForwardInto = false;
	bool bRequestedStepForwardOver = false;
	bool bRequestedStepOut = false;

public:
	
	void RestartExecutionOfPausedJointActors();

public:


	/**
	 * Get existing instances that uses the provided Joint manager from KnownJointInstances.
	 * @param JointManager Original Joint manager asset to search the instances with.
	 * @param MatchingInstances Found Instances.
	 */
	void GetMatchingInstances(UJointManager* JointManager, TArray<AJointActor*>& MatchingInstances);

	/**
	 * Get proper description for the provided Joint instance.
	 * @param Instance The Joint instance to describe.
	 * @return Description for the provided instance.
	 */
	FText GetInstanceDescription(AJointActor* Instance) const;


	/**
	 * Get whether the debugger is enabled or not.
	 */
	bool IsDebuggerEnabled();


public:

	static void StopPlaySession();
	static void PausePlaySession();
	static void ResumePlaySession();
	static bool IsPlaySessionPaused();
	static bool IsPlaySessionRunning();
	static bool IsPIESimulating();
	static bool IsPIENotSimulating();

public:
	
	static bool IsDebugging();
public:

	static void NotifyDebugDataChanged(UJointEdGraph* Graph);


	static void NotifyDebugDataChanged(const UJointManager* Manager);
	
public:

	static void NotifyDebugDataChangedToGraphNodeWidget(UJointEdGraphNode* Changed, FJointNodeDebugData* Data);
	
	/**
	 * Get the debug data for the provided graph.
	 * In Joint 2.3.0, This function now returns the original asset's debug data since now the debug data only available on the stored asset side, not on transient objects.
	 * @param Graph The Graph that it will find debug data from.
	 * @return found debug data
	 */
	static TArray<FJointNodeDebugData>* GetCorrespondingDebugDataForGraph(UJointEdGraph* Graph);

public:
	
  	/**
	 * Get the debug data for the provided graph node.
	 * In Joint 2.3.0, This function now returns the original asset's debug data that is corresponding to the provided graph node since now the debug data only available on the stored asset side, not on transient objects.
	 * @param Node The node that it will find debug data for.
	 * @return found debug data. If not present, returns nullptr. 
	 */
	static FJointNodeDebugData* GetDebugDataForInstance(UJointEdGraphNode* Node);

	/**
	* Get the debug data for the provided graph node.
	* In Joint 2.3.0, This function now returns the original asset's debug data that is corresponding to the provided graph node since now the debug data only available on the stored asset side, not on transient objects.
	* @param Node The node that it will find debug data for.
	* @return found debug data. If not present, returns nullptr. 
	*/
	static FJointNodeDebugData* GetDebugDataForInstance(UJointNodeBase* Node);
	
	/**
	 * Get the debug data for the provided graph node from the provided debug data array.
	 * @param Node The node that it will find debug data for.
	 * @return found debug data. If not present, returns nullptr. 
	 */
	static FJointNodeDebugData* GetDebugDataForInstanceFrom(TArray<FJointNodeDebugData>* TargetDataArrayPtr, UJointEdGraphNode* Node);

	/**
	 * Get the debug data for the provided node instance from the provided debug data array.
	 * @param NodeInstance The node that it will find debug data for.
	 * @return found debug data. If not present, returns nullptr. 
	 */
	static FJointNodeDebugData* GetDebugDataForInstanceFrom(TArray<FJointNodeDebugData>* TargetDataArrayPtr, UJointNodeBase* NodeInstance);


public:
	
	/** PIE worlds that we can debug */
	TArray< TWeakObjectPtr<UWorld> > DebugWorlds;
	TArray< TSharedPtr<FString> > DebugWorldNames;
	
public:
	
	/**
	 * Get the singleton object of the debugger.
	 * You can also get the instance from the Editor Module. (It relies on the editor module.)
	 */
	static UJointDebugger* Get();
	
};
