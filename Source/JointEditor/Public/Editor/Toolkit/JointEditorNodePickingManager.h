//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointEditorToolkit.h"

JOINTEDITOR_API DECLARE_DELEGATE_OneParam(FOnNodePickingPerformed, UJointNodeBase* PickedNode)

enum class EJointNodePickingType : uint8
{
	None = 0 UMETA(DisplayName="None"),
	FromPropertyHandle = 1 UMETA(DisplayName="From PropertyHandle"),
	FromJointNodePointerPtr = 2 UMETA(DisplayName="From JointNodePointer Ptr"),
	QuickPickSelection = 3 UMETA(DisplayName="Quick Pick Selection"),
};

class JOINTEDITOR_API FJointEditorNodePickingManagerRequest : public TSharedFromThis<FJointEditorNodePickingManagerRequest>
{

public:

	FJointEditorNodePickingManagerRequest();
	
public:

	// EJointNodePickingType::FromPropertyHandle
	
	TSharedPtr<IPropertyHandle> TargetJointNodePointerNodePropertyHandle;
	
	TSharedPtr<IPropertyHandle> TargetJointNodePointerEditorNodePropertyHandle;

public:

	// EJointNodePickingType::FromJointNodePointerPtr
	TArray<FJointNodePointer*> TargetJointNodePointerStructures;
	
	TArray<UJointNodeBase*> ModifiedJointNodes;

public:

	//Node picking type
	EJointNodePickingType NodePickingType = EJointNodePickingType::None;

public:
	
	/**
	 * Guid for the request. Use this to identify the request.
	 */
	FGuid RequestGuid;

public:

	static TSharedRef<FJointEditorNodePickingManagerRequest> MakeInstance();

public:

	FOnNodePickingPerformed OnNodePickingPerformed;
	
};

FORCEINLINE bool operator==(const FJointEditorNodePickingManagerRequest& A, const FJointEditorNodePickingManagerRequest& B)
{
	return A.RequestGuid == B.RequestGuid;
}

FORCEINLINE bool operator!=(const FJointEditorNodePickingManagerRequest& A, const FJointEditorNodePickingManagerRequest& B)
{
	return !(A == B);
}

class JOINTEDITOR_API FJointEditorNodePickingManagerResult : public TSharedFromThis<FJointEditorNodePickingManagerResult>
{

public:

	FJointEditorNodePickingManagerResult();
	
public:

	TObjectPtr<UJointNodeBase> Node = nullptr;
	
	TObjectPtr<UJointEdGraphNode> OptionalEdNode = nullptr;
	
public:

	static TSharedRef<FJointEditorNodePickingManagerResult> MakeInstance();
	
};


class JOINTEDITOR_API FJointEditorNodePickingManager : public TSharedFromThis<FJointEditorNodePickingManager>
{
public:
	
	FJointEditorNodePickingManager(TWeakPtr<FJointEditorToolkit> InJointEditorToolkitPtr);

	virtual ~FJointEditorNodePickingManager() = default;

public:

	static TSharedRef<FJointEditorNodePickingManager> MakeInstance(TWeakPtr<FJointEditorToolkit> InJointEditorToolkitPtr);
	
public:
	
	/**
	 * Start the node picking mode for the provided FJointNodePointer property handle.
	 * @param InNodePickingJointNodePointerNodeHandle The property handle of the object pointer property in the parent FJointNodePointer structure.
	 */
	TWeakPtr<FJointEditorNodePickingManagerRequest> StartNodePicking(const TSharedPtr<IPropertyHandle>& InNodePickingJointNodePointerNodeHandle, const TSharedPtr<IPropertyHandle>& InNodePickingJointNodePointerEditorNodeHandle);
	
	/**
	 * Start the node picking mode for the provided FJointNodePointer structures.
	 * @param InNodePickingJointNodePointerStructures Provided Structures.
	 */
	TWeakPtr<FJointEditorNodePickingManagerRequest> StartNodePicking(const TArray<UJointNodeBase*>& InNodePickingJointNodes, const TArray<FJointNodePointer*>& InNodePickingJointNodePointerStructures);
	
	/**
	 * Start the node picking mode for the provided FJointNodePointer structure pointer
	 * @param InNodePointerStruct The direct pointer to the FJointNodePointer structure to fill out
	 */
	TWeakPtr<FJointEditorNodePickingManagerRequest> StartNodePicking(UJointNodeBase* InNode, FJointNodePointer* InNodePointerStruct);

	/**
	 * Start the node picking mode for the provided request.
	 * @param InRequest The request to start the node picking mode.
	 */
	TWeakPtr<FJointEditorNodePickingManagerRequest> StartNodePicking(TWeakPtr<FJointEditorNodePickingManagerRequest> InRequest);
	
	
	/**
	 * Start Quick Picking mode.
	 * Quick Picking mode allows users to pick the selected node on the graph panel directly to the OS clipboard.
	 * Users can paste it to the target property handle after that.
	 * (This doesn't require any request setup.)
	 */
	TWeakPtr<FJointEditorNodePickingManagerRequest> StartQuickPicking();
	
	/**
	 * Pick and feed the provided node instance in the activating picking property handle.
	 * @param Node Object to pick up in this action.
	 */
	void PerformNodePicking(TWeakPtr<FJointEditorNodePickingManagerResult> Result);

	/**
	 * End node picking action of the editor.
	 */
	void EndNodePicking();

	/**
	 * Check whether the editor is performing the node picking action.
	 * @return whether the editor is performing the node picking action
	 */
	bool IsInNodePicking();

public:

	TWeakPtr<FJointEditorNodePickingManagerRequest> GetActiveRequest() const;

private:

	void ClearActiveRequest();

	void SetActiveRequest(const TSharedPtr<FJointEditorNodePickingManagerRequest>& Request);

private:
	
	TSharedPtr<FJointEditorNodePickingManagerRequest> ActiveRequest = nullptr;

public:

	FGraphPanelSelectionSet SavedSelectionSet;

private:

	bool bIsOnNodePickingMode = false;

private:

	/**
	 * Pointer to the joint editor toolkit that owns this manager. (Node picking manager is per editor instance.)
	 */
	TWeakPtr<FJointEditorToolkit> JointEditorToolkitPtr;
};
