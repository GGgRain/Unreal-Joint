//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointEditorNodePickingManager.h"

#include "JointEditorStyle.h"
#include "JointEditorToolkit.h"
#include "JointEditorGraphDocument.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "SGraphPanel.h"
#include "EditorWidget/JointGraphEditor.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/PlatformApplicationMisc.h"
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

FJointEditorNodePickingManagerResult::FJointEditorNodePickingManagerResult()
{
}

TSharedRef<FJointEditorNodePickingManagerResult> FJointEditorNodePickingManagerResult::MakeInstance()
{
	return MakeShareable(new FJointEditorNodePickingManagerResult);
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
	TSharedPtr<FJointEditorNodePickingManagerRequest> Request = FJointEditorNodePickingManagerRequest::MakeInstance();

	Request->NodePickingType = EJointNodePickingType::FromPropertyHandle;
	Request->TargetJointNodePointerNodePropertyHandle = InNodePickingJointNodePointerNodeHandle;
	Request->TargetJointNodePointerEditorNodePropertyHandle = InNodePickingJointNodePointerEditorNodeHandle;
	Request->ModifiedJointNodes.Empty();
	Request->TargetJointNodePointerStructures.Empty();

	return StartNodePicking(Request);
}

TWeakPtr<FJointEditorNodePickingManagerRequest> FJointEditorNodePickingManager::StartNodePicking(
	const TArray<UJointNodeBase*>& InNodePickingJointNodes,
	const TArray<FJointNodePointer*>&
	InNodePickingJointNodePointerStructures)
{
	TSharedPtr<FJointEditorNodePickingManagerRequest> Request = FJointEditorNodePickingManagerRequest::MakeInstance();

	Request->NodePickingType = EJointNodePickingType::FromJointNodePointerPtr;
	Request->TargetJointNodePointerNodePropertyHandle = nullptr;
	Request->TargetJointNodePointerEditorNodePropertyHandle = nullptr;
	Request->ModifiedJointNodes = InNodePickingJointNodes;
	Request->TargetJointNodePointerStructures = InNodePickingJointNodePointerStructures;

	return StartNodePicking(Request);
}

TWeakPtr<FJointEditorNodePickingManagerRequest> FJointEditorNodePickingManager::StartNodePicking(
	UJointNodeBase* InNode,
	FJointNodePointer* InNodePointerStruct)
{
	TSharedPtr<FJointEditorNodePickingManagerRequest> Request = FJointEditorNodePickingManagerRequest::MakeInstance();

	Request->NodePickingType = EJointNodePickingType::FromJointNodePointerPtr;
	Request->TargetJointNodePointerNodePropertyHandle = nullptr;
	Request->TargetJointNodePointerEditorNodePropertyHandle = nullptr;
	Request->ModifiedJointNodes.Empty();
	Request->ModifiedJointNodes.Add(InNode);
	Request->TargetJointNodePointerStructures.Empty();
	Request->TargetJointNodePointerStructures.Add(InNodePointerStruct);

	return StartNodePicking(Request);
}

TWeakPtr<FJointEditorNodePickingManagerRequest> FJointEditorNodePickingManager::StartNodePicking(TWeakPtr<FJointEditorNodePickingManagerRequest> InRequest)
{
	if (JointEditorToolkitPtr.IsValid())
	{
		JointEditorToolkitPtr.Pin()->PopulateNodePickingToastMessage();

		SavedSelectionSet = JointEditorToolkitPtr.Pin()->GetSelectedNodes();
	}

	bIsOnNodePickingMode = true;

	SetActiveRequest(InRequest.Pin());

	return GetActiveRequest();
}

TWeakPtr<FJointEditorNodePickingManagerRequest> FJointEditorNodePickingManager::StartQuickPicking()
{
	TSharedPtr<FJointEditorNodePickingManagerRequest> Request = FJointEditorNodePickingManagerRequest::MakeInstance();

	Request->NodePickingType = EJointNodePickingType::QuickPickSelection;
	Request->TargetJointNodePointerNodePropertyHandle = nullptr;
	Request->TargetJointNodePointerEditorNodePropertyHandle = nullptr;
	Request->ModifiedJointNodes.Empty();
	Request->TargetJointNodePointerStructures.Empty();

	if (JointEditorToolkitPtr.IsValid())
	{
		JointEditorToolkitPtr.Pin()->PopulateQuickNodePickingToastMessage();

		SavedSelectionSet = JointEditorToolkitPtr.Pin()->GetSelectedNodes();
	}

	bIsOnNodePickingMode = true;

	SetActiveRequest(Request);

	return GetActiveRequest();
}

void FJointEditorNodePickingManager::PerformNodePicking(TWeakPtr<FJointEditorNodePickingManagerResult> Result)
{
	if (!IsInNodePicking()) return;

	TSharedPtr<FJointEditorNodePickingManagerRequest> Request = GetActiveRequest().Pin();

	if (Result == nullptr || Result.Pin()->Node == nullptr || Request == nullptr) return;


	auto PerformNodePicking_FromPropertyHandle = [this, Result, Request]()
	{
		if (Result == nullptr || Request == nullptr) return;

		if (Request->TargetJointNodePointerNodePropertyHandle.IsValid())
		{
			Request->TargetJointNodePointerNodePropertyHandle->SetValue(Result.Pin()->Node);
			//It doesn't need to notify it manually since the handle take care of it.
		}

		if (Result.Pin()->OptionalEdNode)
		{
			if (Request->TargetJointNodePointerEditorNodePropertyHandle.IsValid())
			{
				Request->TargetJointNodePointerEditorNodePropertyHandle->SetValue(Result.Pin()->OptionalEdNode);
			}
		}

	};

	auto PerformNodePicking_FromJointNodePointerPtr = [this, Result, Request]()
	{
		for (UJointNodeBase* NodePickingJointNode : Request->ModifiedJointNodes)
		{
			if (NodePickingJointNode == nullptr) continue;

			NodePickingJointNode->Modify();
		}

		for (FJointNodePointer* NodePickingJointNodePointerStructure : Request->TargetJointNodePointerStructures)
		{
			if (NodePickingJointNodePointerStructure == nullptr) continue;


			if (FJointNodePointer::CanSetNodeOnProvidedJointNodePointer(
				*NodePickingJointNodePointerStructure, Result.Pin()->Node))
			{
				NodePickingJointNodePointerStructure->Node = Result.Pin()->Node;

				//Notify it to the owners.
				TArray<UObject*> Outers;

				for (UJointNodeBase* NodePickingJointNode : Request->ModifiedJointNodes)
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
	};

	switch (Request->NodePickingType)
	{
	case EJointNodePickingType::None:
		break;
	case EJointNodePickingType::FromPropertyHandle:
		PerformNodePicking_FromPropertyHandle();
		break;
	case EJointNodePickingType::FromJointNodePointerPtr:
		PerformNodePicking_FromJointNodePointerPtr();
		break;
	case EJointNodePickingType::QuickPickSelection:
		//Set to clipboard.
		FPlatformApplicationMisc::ClipboardCopy(*Result.Pin()->Node->GetPathName());
		break;
	}

	//Notify the request owner that the node picking is performed.
	Request->OnNodePickingPerformed.ExecuteIfBound(Result.Pin()->Node);


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

		JointEditorToolkitPtr.Pin()->StartHighlightingNode(Result.Pin()->OptionalEdNode, true);
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
	SavedSelectionSet.Empty();

	ClearActiveRequest();

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
