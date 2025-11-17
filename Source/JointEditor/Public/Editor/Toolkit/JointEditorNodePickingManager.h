//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointEditorToolkit.h"

class JOINTEDITOR_API FJointEditorNodePickingManagerRequest : public TSharedFromThis<FJointEditorNodePickingManagerRequest>
{

public:

	FJointEditorNodePickingManagerRequest();

public:

	static TSharedRef<FJointEditorNodePickingManagerRequest> MakeInstance();

public:

	/**
	 * Guid for the request. Use this to identify the request.
	 */
	FGuid RequestGuid;

};

FORCEINLINE bool operator==(const FJointEditorNodePickingManagerRequest& A, const FJointEditorNodePickingManagerRequest& B)
{
	return A.RequestGuid == B.RequestGuid;
}

FORCEINLINE bool operator!=(const FJointEditorNodePickingManagerRequest& A, const FJointEditorNodePickingManagerRequest& B)
{
	return !(A == B);
}

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
	TWeakPtr<FJointEditorNodePickingManagerRequest>StartNodePicking(const TArray<UJointNodeBase*>& InNodePickingJointNodes, const TArray<FJointNodePointer*>& InNodePickingJointNodePointerStructures);
	
	/**
	 * Start the node picking mode for the provided FJointNodePointer structure pointer
	 * @param InNodePointerStruct The direct pointer to the FJointNodePointer structure to fill out
	 */
	TWeakPtr<FJointEditorNodePickingManagerRequest> StartNodePicking(UJointNodeBase* InNode, FJointNodePointer* InNodePointerStruct);
	
	/**
	 * Pick and feed the provided node instance in the activating picking property handle.
	 * @param Node Object to pick up in this action.
	 */
	void PerformNodePicking(UJointNodeBase* Node, UJointEdGraphNode* OptionalEdNode = nullptr);

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

	TWeakPtr<IPropertyHandle> NodePickingJointNodePointerNodeHandle;
	TWeakPtr<IPropertyHandle> NodePickingJointNodePointerEditorNodeHandle;
	TArray<FJointNodePointer*> NodePickingJointNodePointerStructures;
	TArray<UJointNodeBase*> NodePickingJointNodes;

	UJointEdGraphNode* LastSelectedNode = nullptr;
private:

	TWeakPtr<FJointEditorToolkit> JointEditorToolkitPtr;
};
