//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointEditorNodePickingManager.h"

#include "JointEditorStyle.h"
#include "JointEditorToolkit.h"
#include "JointEditorGraphDocument.h"
#include "ScopedTransaction.h"
#include "SGraphPanel.h"
#include "EditorWidget/JointGraphEditor.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "JointEditorNodePickingManager"


FJointEditorNodePickingManagerRequest::FJointEditorNodePickingManagerRequest()
{
	//Empty One
	RequestGuid = FGuid::NewGuid();
}

TSharedRef<FJointEditorNodePickingManagerRequest> FJointEditorNodePickingManagerRequest::MakeInstance()
{
	return MakeShareable(new FJointEditorNodePickingManagerRequest);
}

FJointEditorNodePickingManager::FJointEditorNodePickingManager(TWeakPtr<FJointEditorToolkit> InJointEditorToolkitPtr)
{
	JointEditorToolkitPtr = InJointEditorToolkitPtr;
}

TSharedRef<FJointEditorNodePickingManager> FJointEditorNodePickingManager::MakeInstance(
	TWeakPtr<FJointEditorToolkit> InJointEditorToolkitPtr)
{
	return MakeShareable(new FJointEditorNodePickingManager(InJointEditorToolkitPtr));
}


TWeakPtr<FJointEditorNodePickingManagerRequest> FJointEditorNodePickingManager::StartNodePicking(
	const TSharedPtr<IPropertyHandle>& InNodePickingJointNodePointerNodeHandle,
	const TSharedPtr<IPropertyHandle>& InNodePickingJointNodePointerEditorNodeHandle)
{
	NodePickingJointNodePointerNodeHandle = InNodePickingJointNodePointerNodeHandle;
	NodePickingJointNodePointerEditorNodeHandle = InNodePickingJointNodePointerEditorNodeHandle;

	NodePickingJointNodePointerStructures.Empty();

	NodePickingJointNodes.Empty();

	if (JointEditorToolkitPtr.IsValid())
	{
		JointEditorToolkitPtr.Pin()->PopulateNodePickingToastMessage();

		SavedSelectionSet = JointEditorToolkitPtr.Pin()->GetSelectedNodes();
	}

	bIsOnNodePickingMode = true;
	
	SetActiveRequest(FJointEditorNodePickingManagerRequest::MakeInstance());

	return GetActiveRequest();
}

TWeakPtr<FJointEditorNodePickingManagerRequest> FJointEditorNodePickingManager::StartNodePicking(
	const TArray<UJointNodeBase*>& InNodePickingJointNodes,
	const TArray<FJointNodePointer*>&
	InNodePickingJointNodePointerStructures)
{
	NodePickingJointNodePointerNodeHandle = nullptr;
	NodePickingJointNodePointerEditorNodeHandle = nullptr;

	NodePickingJointNodes.Empty();

	NodePickingJointNodes = InNodePickingJointNodes;

	NodePickingJointNodePointerStructures.Empty();

	NodePickingJointNodePointerStructures = InNodePickingJointNodePointerStructures;

	if (JointEditorToolkitPtr.IsValid())
	{
		JointEditorToolkitPtr.Pin()->PopulateNodePickingToastMessage();

		SavedSelectionSet = JointEditorToolkitPtr.Pin()->GetSelectedNodes();
	}

	bIsOnNodePickingMode = true;

	SetActiveRequest(FJointEditorNodePickingManagerRequest::MakeInstance());

	return GetActiveRequest();
}

TWeakPtr<FJointEditorNodePickingManagerRequest> FJointEditorNodePickingManager::StartNodePicking(
	UJointNodeBase* InNode,
	FJointNodePointer* InNodePointerStruct)
{
	NodePickingJointNodePointerNodeHandle = nullptr;
	NodePickingJointNodePointerEditorNodeHandle = nullptr;

	NodePickingJointNodes.Empty();

	NodePickingJointNodes.Add(InNode);

	NodePickingJointNodePointerStructures.Empty();

	NodePickingJointNodePointerStructures.Add(InNodePointerStruct);

	if (JointEditorToolkitPtr.IsValid())
	{
		JointEditorToolkitPtr.Pin()->PopulateNodePickingToastMessage();

		SavedSelectionSet = JointEditorToolkitPtr.Pin()->GetSelectedNodes();
	}

	bIsOnNodePickingMode = true;
	
	SetActiveRequest(FJointEditorNodePickingManagerRequest::MakeInstance());

	return GetActiveRequest();
}

void FJointEditorNodePickingManager::PerformNodePicking(UJointNodeBase* Node, UJointEdGraphNode* OptionalEdNode)
{
	if (!IsInNodePicking()) return;

	if (Node == nullptr) return;

	//Only for the slate picking
	if (!NodePickingJointNodePointerStructures.IsEmpty())
	{
		const FScopedTransaction Transaction(NSLOCTEXT("JointEdTransaction", "TransactionTitle_PerformNodePicking", "Perform node picking"));

		for (UJointNodeBase* NodePickingJointNode : NodePickingJointNodes)
		{
			if (NodePickingJointNode == nullptr) continue;

			NodePickingJointNode->Modify();
		}

		for (FJointNodePointer* NodePickingJointNodePointerStructure : NodePickingJointNodePointerStructures)
		{
			if (NodePickingJointNodePointerStructure == nullptr) continue;


			if (FJointNodePointer::CanSetNodeOnProvidedJointNodePointer(
				*NodePickingJointNodePointerStructure, Node))
			{
				NodePickingJointNodePointerStructure->Node = Node;

				//Notify it to the owners.
				TArray<UObject*> Outers;

				for (UJointNodeBase* NodePickingJointNode : NodePickingJointNodes)
				{
					if (NodePickingJointNode == nullptr) continue;

					NodePickingJointNode->PostEditChange();
				}
			}
			else
			{
				//Notify the action failure

				FString AllowedTypeStr;
				for (TSubclassOf<UJointNodeBase> AllowedType : NodePickingJointNodePointerStructure->AllowedType)
				{
					if (AllowedType == nullptr) continue;
					if (!AllowedTypeStr.IsEmpty()) AllowedTypeStr.Append(", ");
					AllowedTypeStr.Append(AllowedType.Get()->GetName());
				}
				FString DisallowedTypeStr;
				for (TSubclassOf<UJointNodeBase> DisallowedType : NodePickingJointNodePointerStructure->
				     DisallowedType)
				{
					if (DisallowedType == nullptr) continue;
					if (!DisallowedTypeStr.IsEmpty()) DisallowedTypeStr.Append(", ");
					DisallowedTypeStr.Append(DisallowedType.Get()->GetName());
				}

				FText FailedNotificationText = LOCTEXT("NotJointNodeInstanceType", "Node Pick Up Canceled");
				FText FailedNotificationSubText = FText::Format(
					LOCTEXT("NotJointNodeInstanceType_Sub",
					        "Current structure can not have the provided node type. Pointer reseted.\n\nAllowd Types: {0}\nDisallowed Types: {1}"),
					FText::FromString(AllowedTypeStr),
					FText::FromString(DisallowedTypeStr));

				FNotificationInfo NotificationInfo(FailedNotificationText);
				NotificationInfo.SubText = FailedNotificationSubText;
				NotificationInfo.Image = FJointEditorStyle::Get().GetBrush("JointUI.Image.JointManager");
				NotificationInfo.bFireAndForget = true;
				NotificationInfo.FadeInDuration = 0.2f;
				NotificationInfo.FadeOutDuration = 0.2f;
				NotificationInfo.ExpireDuration = 2.5f;
				NotificationInfo.bUseThrobber = true;

				FSlateNotificationManager::Get().AddNotification(NotificationInfo);
			}
		}
	}
	else
	{
		if (NodePickingJointNodePointerNodeHandle.IsValid())
		{
			NodePickingJointNodePointerNodeHandle.Pin()->SetValue(Node);

			//It doesn't need to notify it manually since the handle take care of it.
		}
	}

	//Clear this at this time, it will prevent the toolkit to notify the node picking action again.
	//TODO: make it more clear and clean.
	bIsOnNodePickingMode = false;

	if (JointEditorToolkitPtr.IsValid())
	{
		if (JointEditorToolkitPtr.Pin()
			&& JointEditorToolkitPtr.Pin()->GetFocusedGraphEditor()
			&& JointEditorToolkitPtr.Pin()->GetFocusedGraphEditor()->GetGraphPanel()
			)
			JointEditorToolkitPtr.Pin()->GetFocusedGraphEditor()->GetGraphPanel()->SelectionManager.SetSelectionSet(SavedSelectionSet);
	}

	if (OptionalEdNode)
	{
		LastSelectedNode = OptionalEdNode;
		
		if (JointEditorToolkitPtr.IsValid()) JointEditorToolkitPtr.Pin()->StartHighlightingNode(OptionalEdNode, true);

		if (NodePickingJointNodePointerEditorNodeHandle.IsValid())
		{
			NodePickingJointNodePointerEditorNodeHandle.Pin()->SetValue(OptionalEdNode);
		}
	}

	if (JointEditorToolkitPtr.IsValid())
	{
		JointEditorToolkitPtr.Pin()->CompileAllJointGraphs();
	}

	EndNodePicking();
}

void FJointEditorNodePickingManager::EndNodePicking()
{
	if (JointEditorToolkitPtr.IsValid())
	{
		JointEditorToolkitPtr.Pin()->ClearNodePickingToastMessage();

		// fuck this feature. This is so dumb. I will update the slate system a little in the next update.
		// //if both aren't same, it means that the user selected something else than the previous one.
		// //In this case, we need to notify the previous one to stop highlighting.
		// if(OptionalPreviousNode && (!LastSelectedNode || LastSelectedNode != OptionalPreviousNode))
		// {
		// 	JointEditorToolkitPtr.Pin()->StopHighlightingNode(OptionalPreviousNode);
		// }
	}
	NodePickingJointNodePointerNodeHandle.Reset();
	NodePickingJointNodePointerEditorNodeHandle.Reset();
	NodePickingJointNodePointerStructures.Empty();
	NodePickingJointNodes.Empty();
	SavedSelectionSet.Empty();

	ClearActiveRequest();
	
	LastSelectedNode = nullptr;

	bIsOnNodePickingMode = false;
}

bool FJointEditorNodePickingManager::IsInNodePicking()
{
	return bIsOnNodePickingMode;
}

TWeakPtr<FJointEditorNodePickingManagerRequest> FJointEditorNodePickingManager::GetActiveRequest() const
{
	return ActiveRequest;
}

void FJointEditorNodePickingManager::ClearActiveRequest()
{
	ActiveRequest = nullptr;
}

void FJointEditorNodePickingManager::SetActiveRequest(const TSharedPtr<FJointEditorNodePickingManagerRequest>& Request)
{
	ActiveRequest = Request;
}

#undef LOCTEXT_NAMESPACE
