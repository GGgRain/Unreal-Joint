//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Node/JointEdGraphNode.h"

#include "JointEdGraph.h"
#include "JointEdGraphSchema.h"
#include "JointEditorStyle.h"
#include "JointManager.h"

#include "IMessageLogListing.h"
#include "JointEditorSettings.h"
#include "JointEdUtils.h"

#include "Node/JointNodeBase.h"

#include "ToolMenu.h"
#include "Engine/Blueprint.h"

#include "EditorWidget/SJointGraphEditorActionMenu.h"
#include "GraphNode/SJointGraphNodeBase.h"

#include "EdGraph/EdGraph.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "Math/UnrealMathUtility.h"
#include "Misc/UObjectToken.h"

#include "Logging/TokenizedMessage.h"
#include "Misc/EngineVersionComparison.h"


#define LOCTEXT_NAMESPACE "JointGraphNode"

UJointEdGraphNode::UJointEdGraphNode():
	NodeInstance(nullptr),
	ParentNode(nullptr)
{
	NodeWidth = JointGraphNodeResizableDefs::MinNodeSize.X;
	NodeHeight = JointGraphNodeResizableDefs::MinNodeSize.Y;

	bUseFixedNodeSize = true;
	bCanRenameNode = true;

	CompileMessages.Empty();
}

FText UJointEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return (NodeInstance)
		       ? FText::FromName(NodeInstance->GetFName())
		       : FText::FromName(FName("ERROR: NO NODE INSTANCE"));
}

FLinearColor UJointEdGraphNode::GetNodeTitleColor() const
{
	return CheckShouldUseNodeInstanceSpecifiedBodyColor()
		       ? GetCastedNodeInstance()->NodeBodyColor
		       : UJointEditorSettings::Get()->DefaultNodeColor;
}

bool UJointEdGraphNode::CheckShouldUseNodeInstanceSpecifiedBodyColor() const
{
	if (GetCastedNodeInstance())
	{
		return GetCastedNodeInstance()->bUseSpecifiedGraphNodeBodyColor;
	}

	return false;
}


FText UJointEdGraphNode::GetTooltipText() const
{
	//If this graph node doesn't have valid node instance, show the error message for it.
	if (!NodeInstance)
	{
		FString StoredClassName = ClassData.GetClassName();
		StoredClassName.RemoveFromEnd(TEXT("_C"));

		return FText::Format(
			LOCTEXT("NodeClassError", "Class {0} not found, make sure it's saved!"),
			FText::FromString(StoredClassName));
	}

	//If this graph node has error message, display it instead.
	if (ErrorMsg.Len() > 0)
	{
		return FText::FromString(ErrorMsg);
	}

	//If this node class is compiled from the blueprint, try to get the blueprint's description.
	if (NodeInstance->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		const FAssetData AssetData(NodeInstance->GetClass()->ClassGeneratedBy);
		FString Description = AssetData.GetTagValueRef<FString>(
			GET_MEMBER_NAME_CHECKED(UBlueprint, BlueprintDescription));

		if (!Description.IsEmpty())
		{
			Description.ReplaceInline(TEXT("\\n"), TEXT("\n"));
			return FText::FromString(MoveTemp(Description));
		}
	}

	//If this node class is not compiled from the blueprint, display the native description.
	return NodeInstance->GetClass()->GetToolTipText();
}

void UJointEdGraphNode::AllocateDefaultPins()
{
	PinData.Empty();

	PinData.Add(FJointEdPinData("In", EEdGraphPinDirection::EGPD_Input));
}

void UJointEdGraphNode::UpdatePins()
{
	ReallocatePins();

	RemoveUnnecessaryPins();

	UpdatePinsFromPinData();

	ImplementPinDataPins();

	ReplicateSubNodePins();

	RequestPopulationOfPinWidgets();

	if (ParentNode)
	{
		ParentNode->UpdatePins();
	}
}

bool UJointEdGraphNode::CheckProvidedPinIsOnPinData(const UEdGraphPin* Pin)
{
	if (!Pin) return false;

	for (const FJointEdPinData& Data : PinData)
	{
		if (Data.ImplementedPinId == Pin->PinId) return true;
	}

	return false;
}

void UJointEdGraphNode::UpdatePinsFromPinData()
{
	TArray<UEdGraphPin*> SavedPins = Pins;

	//Fix already implemented pins if data for the pin is recognizable.

	for (const FJointEdPinData& JointEdPinData : PinData)
	{
		UEdGraphPin* ImplementedPin = FindImplementedPinFromPinData(JointEdPinData);

		if (!ImplementedPin) continue;

		if (!(ImplementedPin->PinName == JointEdPinData.PinName
			&& ImplementedPin->Direction == JointEdPinData.Direction
			&& ImplementedPin->PinType == JointEdPinData.Type))
		{
			ImplementedPin->PinName = JointEdPinData.PinName;
			ImplementedPin->Direction = JointEdPinData.Direction;
			ImplementedPin->PinType = JointEdPinData.Type;
		}
	}
}

void UJointEdGraphNode::ImplementPinDataPins()
{
	TArray<UEdGraphPin*> SavedPins = Pins;

	const TArray<UEdGraphPin*> SubNodePins = GetAllPinsFromChildren();

	//Implement new pins if it is not implemented from the pin data list yet.
	for (FJointEdPinData& Data : PinData)
	{
		if (FindImplementedPinFromPinData(Data) == nullptr)
		{
			const UEdGraphPin* NewPin = CreatePin(Data.Direction, Data.Type, Data.PinName);

			Data.ImplementedPinId = NewPin->PinId;
		}
	}
}

void UJointEdGraphNode::RemoveUnnecessaryPins()
{
	TArray<UEdGraphPin*> SavedPins = Pins;

	const TArray<UEdGraphPin*> SubNodePins = GetAllPinsFromChildren();

	//Remove pins if we don't need.
	for (UEdGraphPin* Pin : SavedPins)
	{
		//Revert if the pin is implemented by this node and still have some data for it.
		if (CheckProvidedPinIsOnPinData(Pin)) continue;

		//Revert if it is a replicated pin and we don't need to hold it anymore.
		if (SubNodePins.ContainsByPredicate([Pin](const UEdGraphPin* SubNodePin)
		{
			if (!SubNodePin) return false;

			return Pin->PinId == SubNodePin->PinId;
		}))
			continue;

		//If it turned out we don't need to hold it any longer, remove it.
		Pin->BreakAllPinLinks();

		RemovePin(Pin);
	}
}

UEdGraphPin* UJointEdGraphNode::FindImplementedPinFromPinData(const FJointEdPinData& InPinData)
{
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->PinId == InPinData.ImplementedPinId)
		{
			return Pin;
		}
	}

	return nullptr;
}

FJointEdPinData* UJointEdGraphNode::FindPinDataForImplementedPin(const UEdGraphPin* Pin)
{
	if (!Pin) return nullptr;

	for (FJointEdPinData& Data : PinData)
	{
		if (Data.ImplementedPinId == Pin->PinId) return &Data;
	}

	return nullptr;
}

FJointEdPinData* UJointEdGraphNode::FindPinDataFromChildren(const UEdGraphPin* Pin)
{
	const TArray<FJointEdPinData*>& SubNodePinData = GetAllPinDataFromChildren();

	for (FJointEdPinData* InPinData : SubNodePinData)
	{
		if(InPinData == nullptr) continue;

		if(InPinData->ImplementedPinId == Pin->PinId) return InPinData;
	}

	return nullptr;
}

void CollectAllPinsFromNodeAndSubNodes(UJointEdGraphNode* Node, TArray<UEdGraphPin*>& CollectedPins)
{
	if (Node == nullptr) return;

	CollectedPins.Append(Node->GetAllPins());

	for (UJointEdGraphNode* SubNode : Node->SubNodes)
	{
		if (SubNode == nullptr) continue;

		CollectAllPinsFromNodeAndSubNodes(SubNode, CollectedPins);
	}
}

TArray<UEdGraphPin*> UJointEdGraphNode::GetAllPinsFromChildren()
{
	TArray<UEdGraphPin*> CollectedPins;

	for (UJointEdGraphNode* SubNode : SubNodes)
	{
		CollectAllPinsFromNodeAndSubNodes(SubNode, CollectedPins);
	}

	return CollectedPins;
}

TArray<UEdGraphPin*> UJointEdGraphNode::GetAllPinsFromThis()
{
	TArray<UEdGraphPin*> CollectedPins;

	for (UEdGraphPin* Pin : Pins)
	{
		if (CheckProvidedPinIsOnPinData(Pin)) CollectedPins.Add(Pin);
	}
	
	return CollectedPins;
}

TArray<FJointEdPinData>& UJointEdGraphNode::GetPinData()
{
	return PinData;
}

TArray<FJointEdPinData*> UJointEdGraphNode::GetAllPinDataFromChildren() const
{
	TArray<FJointEdPinData*> OutData;
	
	TArray<UJointEdGraphNode*> InSubNodes = GetAllSubNodesInHierarchy();
	
	for (UJointEdGraphNode* SubNode : InSubNodes)
	{
		if(!SubNode) continue;

		TArray<FJointEdPinData>* Array = &SubNode->GetPinData();

		if(Array == nullptr) continue;
		
		OutData.Reserve(OutData.Num() + Array->Num());
		
		for (FJointEdPinData& JointEdPinData : *Array)
		{
			OutData.Emplace(&JointEdPinData);
		}
	}

	return OutData;
}

UObject* UJointEdGraphNode::JointEdNodeInterface_GetNodeInstance()
{
	return NodeInstance;
}

TArray<FJointEdPinData> UJointEdGraphNode::JointEdNodeInterface_GetPinData()
{
	return GetPinData();
}

FPinConnectionResponse UJointEdGraphNode::CanAttachThisAtParentNode(const UJointEdGraphNode* InParentNode) const
{
	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("AllowedAttachmentMessage", "Allow Attaching"));
}

FPinConnectionResponse UJointEdGraphNode::CanAttachSubNodeOnThis(const UJointEdGraphNode* InSubNode) const
{
	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("AllowedAttachmentMessage", "Allow Attaching"));
}

bool UJointEdGraphNode::CanHaveSubNode() const
{
	return true;
}


void UJointEdGraphNode::Lock()
{
	bIsLocked = true;
}

void UJointEdGraphNode::Unlock()
{
	bIsLocked = false;
}

void UJointEdGraphNode::ReplicateSubNodePins()
{
	//Logic has been changed.
	//Now we iterate through the sub node pins and find one that has the same pin Guid.
	//This is safe since we are not going to implement one of the pin (specifically the one at the fragment side) in the graph so there will be no same pins with same guid.

	//Replication action will be allowed only if this node is parent-most node.
	if (GetParentmostNode() != this) return;

	TArray<UEdGraphPin*> SubNodePins = GetAllPinsFromChildren();

	//Make sure the pins are ordered in the same order with the sub nodes.
	TArray<UEdGraphPin*> OrderedPinArray;
	OrderedPinArray.Reserve(SubNodePins.Num());

	TArray<UEdGraphPin*> ThisPinArray = GetAllPinsFromThis();

	//Implement new pins & update the implemented pins
	for (UEdGraphPin* SubNodePin : SubNodePins)
	{
		if (!SubNodePin) continue;

		//Check we already have the replicated one.
		if (UEdGraphPin** ReplicatedPin = Pins.FindByPredicate([SubNodePin](const UEdGraphPin* ReplicatedSubNodePin)
		{
			return ReplicatedSubNodePin->PinId == SubNodePin->PinId;
		}))
		{
			(*ReplicatedPin)->PinName = SubNodePin->PinName;
			(*ReplicatedPin)->Direction = SubNodePin->Direction;
			(*ReplicatedPin)->PinType = SubNodePin->PinType;

			OrderedPinArray.Emplace(*ReplicatedPin);
		}
		else
		{
			UEdGraphPin* NewPin = CreatePin(SubNodePin->Direction,FJointEdPinData::PinType_Joint_Normal, SubNodePin->PinName);

			NewPin->PinId = SubNodePin->PinId;

			OrderedPinArray.Emplace(NewPin);
		}
	}

	//Append all the sub node pins at the ordered pin array.
	ThisPinArray.Append(OrderedPinArray);

	Pins = ThisPinArray;
}

UEdGraphPin* UJointEdGraphNode::FindOriginalSubNodePin(UEdGraphPin* InReplicatedSubNodePin)
{
	TArray<UEdGraphPin*> SubNodePins = GetAllPinsFromChildren();

	for (UEdGraphPin* SubNodePin : SubNodePins)
	{
		if (SubNodePin->PinId == InReplicatedSubNodePin->PinId) return SubNodePin;
	}

	return nullptr;
}

UJointEdGraphNode* GetParentmostNodeOf(UJointEdGraphNode* InNode)
{
	if (!InNode) return nullptr;

	if (InNode->ParentNode != nullptr)
	{
		return GetParentmostNodeOf(InNode->ParentNode);
	}

	return InNode;
}


UJointEdGraphNode* UJointEdGraphNode::GetParentmostNode()
{
	return GetParentmostNodeOf(this);
}

void UJointEdGraphNode::RequestUpdateSlate()
{
	if (!GetGraphNodeSlate().IsValid()) return;

	const TSharedPtr<SJointGraphNodeBase> NodeSlate = GetGraphNodeSlate().Pin();

	if (NodeSlate.IsValid())
	{
		NodeSlate->UpdateGraphNode();
	}
}

void UJointEdGraphNode::RequestPopulationOfPinWidgets()
{
	if (!GetGraphNodeSlate().IsValid()) return;

	const TSharedPtr<SJointGraphNodeBase> NodeSlate = GetGraphNodeSlate().Pin();

	if (NodeSlate.IsValid())
	{
		NodeSlate->PopulatePinWidgets();
	}
}

void UJointEdGraphNode::NotifyNodeInstancePropertyChangeToGraphNodeWidget(const FPropertyChangedEvent& PropertyChangedEvent, const FString& PropertyName)
{
	if (!GetGraphNodeSlate().IsValid()) return;

	const TSharedPtr<SJointGraphNodeBase> NodeSlate = GetGraphNodeSlate().Pin();

	if (NodeSlate.IsValid())
	{
		NodeSlate->OnNodeInstancePropertyChanged(PropertyChangedEvent,PropertyName);
	}
}

void UJointEdGraphNode::NotifyGraphNodePropertyChangeToGraphNodeWidget(
	const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!GetGraphNodeSlate().IsValid()) return;

	const TSharedPtr<SJointGraphNodeBase> NodeSlate = GetGraphNodeSlate().Pin();

	if (NodeSlate.IsValid())
	{
		NodeSlate->OnGraphNodePropertyChanged(PropertyChangedEvent);
	}
}

void UJointEdGraphNode::NotifyDebugDataChangedToGraphNodeWidget(const FJointNodeDebugData* Data)
{
	if (!GetGraphNodeSlate().IsValid()) return;

	const TSharedPtr<SJointGraphNodeBase> NodeSlate = GetGraphNodeSlate().Pin();

	if (NodeSlate.IsValid())
	{
		NodeSlate->OnDebugDataChanged(Data);
	}
}

bool UJointEdGraphNode::CheckCanPerformReplaceEditorNodeClassTo(const UClass* Class)
{
	//If the node instance is nullptr, Don't perform the action.

	if (NodeInstance == nullptr) return false;

	if (Class == nullptr) return false;

	if (!Class->IsChildOf(UJointEdGraphNode::StaticClass()) || Class == this->GetClass()) return false;

	if (UClass* SupportedClass = Class->GetDefaultObject<UJointEdGraphNode>()->SupportedNodeClass())
	{
		if (NodeInstance->GetClass()->IsChildOf(SupportedClass) || NodeInstance->GetClass() == SupportedClass)
		{
			return true;
		}
	}

	return false;
}

void UJointEdGraphNode::ReplaceEditorNodeClassTo(const UClass* Class)
{
	if (CheckCanPerformReplaceEditorNodeClassTo(Class))
	{
		UJointEdGraphNode* OpNode = NewObject<UJointEdGraphNode>(GetGraph(), Class);

		OpNode->ClassData = ClassData;

		OpNode->NodeInstance = NodeInstance;

		ParentNode->AddSubNode(OpNode);

		ParentNode->RemoveSubNode(this);
	}
	else
	{
		FText NotificationText = FText::FromString("Replace Editor Node Class Action Has Been Denied");
		FText NotificationSubText = FText::FromString(
			"Provided class doesn't support the node instance type or it was invalid class.");

		FNotificationInfo NotificationInfo(NotificationText);
		NotificationInfo.SubText = NotificationSubText;
		NotificationInfo.Image = FJointEditorStyle::Get().GetBrush("JointUI.Image.JointManager");
		NotificationInfo.bFireAndForget = true;
		NotificationInfo.FadeInDuration = 0.3f;
		NotificationInfo.FadeOutDuration = 1.3f;
		NotificationInfo.ExpireDuration = 4.5f;

		FSlateNotificationManager::Get().AddNotification(NotificationInfo);
	}
}

void UJointEdGraphNode::NotifyGraphChanged()
{
	if (UJointEdGraph* MyGraph = GetCastedGraph()) MyGraph->NotifyGraphChanged();
}


void UJointEdGraphNode::NotifyGraphRequestUpdate()
{
	if (UJointEdGraph* MyGraph = GetCastedGraph()) MyGraph->NotifyGraphRequestUpdate();
}


UEdGraphPin* UJointEdGraphNode::FindReplicatedSubNodePin(const UEdGraphPin* InOriginalSubNodePin)
{
	return GetParentmostNodeOf(this)->FindPinById(InOriginalSubNodePin->PinId);
}


void UJointEdGraphNode::PostPlacedNewNode()
{
	PatchNodeInstanceFromClassDataIfNeeded();

	UpdateNodeInstance();

	GrabSlateDetailLevelFromNodeInstance();
}

UJointManager* UJointEdGraphNode::GetJointManager() const
{
	UJointManager* Manager = nullptr;

	if (GetGraph() && GetGraph()->GetOuter()) Manager = Cast<UJointManager>(GetGraph()->GetOuter());
	
	return Manager;
}

void UJointEdGraphNode::DestroyNode()
{
	UnbindNodeInstance();
	
	//Notify the removal of this sub node to the parent node.
	if (this->IsSubNode())
	{
		if (ParentNode) ParentNode->RemoveSubNode(this);
	}
	else
	{
		//Remove this node from the parent Joint manager.
		if (UJointNodeBase* CastedNodeInstance = GetCastedNodeInstance())
		{
			if (UJointManager* Manager = GetCastedNodeInstance()->GetJointManager())
			{
				Manager->Nodes.Remove(CastedNodeInstance);
				Manager->ManagerFragments.Remove(CastedNodeInstance);
			}
		}
	}

	//Mark the instance's outer to transient package, to make sure it will be GCed
	if (UJointNodeBase* CastedNodeInstance = GetCastedNodeInstance())
	{
		CastedNodeInstance->Rename(nullptr, GetTransientPackage(),REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
	}

	//Propagate the action to the children nodes as well.
	TArray<UJointEdGraphNode*> CachedSubNodes = SubNodes;
	
	for (UJointEdGraphNode* SubNode : CachedSubNodes)
	{
		if(!SubNode) continue;
		
		SubNode->DestroyNode();
	}

	Super::DestroyNode();
}

void UJointEdGraphNode::ImportCustomProperties(const TCHAR* SourceText, FFeedbackContext* Warn)
{
	//It won't copy-paste the old pins.
}

void UJointEdGraphNode::ReconstructNodeInHierarchy()
{
	Lock();
	
	ReconstructNode();

	Unlock();

	for (UJointEdGraphNode* SubNode : SubNodes)
	{
		if(!SubNode) continue;
		
		SubNode->ReconstructNodeInHierarchy();
	}
}

bool UJointEdGraphNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
	return Schema->GetClass() == UJointEdGraphSchema::StaticClass();
}


void UJointEdGraphNode::OnRenameNode(const FString& NewName)
{
	if (!GetCanRenameNode()) return;

	FString ProcessedName = NewName;

	FText FailedNotificationText = LOCTEXT("NodeRenameFailedText", "Rename Task Failed");
	FText SucceedNotificationText = LOCTEXT("NodeRenameSucceedText", "Rename Task Succeeded");

	FText InstanceConditionFailNotificationSubText = LOCTEXT(
		"NodeRenameFailed_InstanceConditionFailNotificationSubText",
		"The node instance was in a condition that can not be modified.");
	FText NoInstanceFailNotificationSubText = LOCTEXT("NodeRenameFailed_NoInstanceFailNotificationSubText",
	                                                  "There is no node instance for this graph node.");
	FText SameNameFailNotificationSubText = LOCTEXT("NodeRenameFailed_SameNameSubText",
	                                                "There might be a node that has the same name.");
	FText NodeNameEmptySubText = LOCTEXT("NodeRenameFailed_EmptyNameSubText",
	                                     "Node name can not be empty");

	FNotificationInfo NotificationInfo(FailedNotificationText);
	NotificationInfo.SubText = NoInstanceFailNotificationSubText;
	NotificationInfo.Image = FJointEditorStyle::Get().GetBrush("JointUI.Image.JointManager");
	NotificationInfo.bFireAndForget = true;
	NotificationInfo.FadeInDuration = 0.2f;
	NotificationInfo.FadeOutDuration = 0.2f;
	NotificationInfo.ExpireDuration = 2.5f;
	NotificationInfo.bUseThrobber = true;


	if (ProcessedName.IsEmpty())
	{
		NotificationInfo.Text = FailedNotificationText;
		NotificationInfo.SubText = NodeNameEmptySubText;
		FSlateNotificationManager::Get().AddNotification(NotificationInfo);

		return;
	}

	if (!GetCastedNodeInstance())
	{
		NotificationInfo.Text = FailedNotificationText;
		NotificationInfo.SubText = NoInstanceFailNotificationSubText;
		FSlateNotificationManager::Get().AddNotification(NotificationInfo);


		return;
	}

	if (GetCastedNodeInstance()->HasAnyFlags(RF_ClassDefaultObject)
		|| GetCastedNodeInstance()->HasAnyFlags(RF_ArchetypeObject)
		|| GetCastedNodeInstance()->HasAnyFlags(RF_FinishDestroyed)
		|| GetCastedNodeInstance()->HasAnyFlags(RF_BeginDestroyed))
	{
		NotificationInfo.Text = FailedNotificationText;
		NotificationInfo.SubText = InstanceConditionFailNotificationSubText;
		FSlateNotificationManager::Get().AddNotification(NotificationInfo);

		return;
	}

	const FString SavedName = GetCastedNodeInstance()->GetName();

	if (SavedName == ProcessedName)
	{
		return;
	}

	//Start rename action.

	FScopedTransaction Transaction(LOCTEXT("RenameNodeTransaction", "Rename Node"));
	{
		GetCastedNodeInstance()->Modify();

		if (UObject* ExistingObject = StaticFindObject(
			/*Class=*/ NULL, GetCastedNodeInstance()->GetOuter(),
			           *ProcessedName, true))
		{
			NotificationInfo.Text = FailedNotificationText;
			NotificationInfo.SubText = SameNameFailNotificationSubText;

			Transaction.Cancel();
		}
		else if (GetCastedNodeInstance()->Rename(*ProcessedName))
		{
			NotificationInfo.Text = SucceedNotificationText;
			NotificationInfo.SubText = FText::Format(
				LOCTEXT("NodeRenameSucceeded_NameSubText", "{0} -> {1}"), FText::FromString(SavedName),
				FText::FromString(ProcessedName));

			FSlateNotificationManager::Get().AddNotification(NotificationInfo);
		}
	}
}

void UJointEdGraphNode::RequestRenameOnGraphNodeSlate()
{
	if (!GetGraphNodeSlate().IsValid()) return;

	const TSharedPtr<SJointGraphNodeBase> NodeSlate = GetGraphNodeSlate().Pin();

	if (NodeSlate)
	{
		NodeSlate->RequestRename();
		NodeSlate->ApplyRename();
	}
}

void UJointEdGraphNode::FeedEditorNodeToNodeInstance()
{
	if (UJointNodeBase* NodeBaseInstance = GetCastedNodeInstance())
	{
		NodeBaseInstance->EdGraphNode = TObjectPtr<UEdGraphNode>(this);
	}
}

void UJointEdGraphNode::BindNodeInstance()
{
	FeedEditorNodeToNodeInstance();
	
	BindNodeInstancePropertyChangeEvents();
}

void UJointEdGraphNode::UnbindNodeInstance()
{
	UnbindNodeInstancePropertyChangeEvents();
}

UJointEdGraph* UJointEdGraphNode::GetCastedGraph() const
{
	return GetGraph() ? Cast<UJointEdGraph>(GetGraph()) : nullptr;
}

FText UJointEdGraphNode::GetPriorityIndicatorTooltipText() const
{
	return LOCTEXT("MultiNodeIndexTooltIp",
	               "There are a number of nodes connected at the pin. When it played, it will try to play a node from smaller index to higher.");
}


EVisibility UJointEdGraphNode::GetPriorityIndicatorVisibility() const
{
	TArray<UEdGraphPin*> AllPins = this->GetAllPins();

	UEdGraphPin* MyInputPin = nullptr;

	for (UEdGraphPin* Pin : AllPins)
	{
		if (!Pin) continue;

		if (Pin->Direction != EEdGraphPinDirection::EGPD_Input) continue;

		MyInputPin = Pin;

		break;
	}

	const UEdGraphPin* MyParentOutputPin = nullptr;

	if (MyInputPin != nullptr && MyInputPin->LinkedTo.Num() > 0) { MyParentOutputPin = MyInputPin->LinkedTo[0]; }

	if (MyParentOutputPin)
	{
		if (MyParentOutputPin->LinkedTo.Num() > 1) { return EVisibility::Visible; }
		else { return EVisibility::Collapsed; }
	}

	return EVisibility::Collapsed;
}

FText UJointEdGraphNode::GetPriorityIndicatorText() const
{
	TArray<UEdGraphPin*> AllPins = this->GetAllPins();

	UEdGraphPin* MyInputPin = nullptr;

	for (UEdGraphPin* Pin : AllPins)
	{
		if (Pin->Direction == EEdGraphPinDirection::EGPD_Input)
		{
			MyInputPin = Pin;

			break;
		}
	}

	UEdGraphPin* MyParentOutputPin = nullptr;

	if (MyInputPin != nullptr && MyInputPin->LinkedTo.Num() > 0) { MyParentOutputPin = MyInputPin->LinkedTo[0]; }

	int32 Index = 0;

	CA_SUPPRESS(6235);
	if (MyParentOutputPin != nullptr)
	{
		for (Index = 0; Index < MyParentOutputPin->LinkedTo.Num(); ++Index)
		{
			if (MyParentOutputPin->LinkedTo[Index] == MyInputPin) { break; }
		}
	}

	return FText::AsNumber(Index + 1);
}

FVector2D UJointEdGraphNode::GetSize() const
{
	return FVector2D(NodeWidth, NodeHeight);
}

void UJointEdGraphNode::PrepareForCopying()
{
	//Hold the node instance to make sure it is copied as well.
	SetNodeInstanceOuterTo(this);

	//Cache the node instance's parent / sub nodes data prior to the copy action, and clear it to prevent the hash corruption.
	//Ah, It also happens to the editor node too.
	if (UJointNodeBase* CastedNodeInstance = GetCastedNodeInstance())
	{
		CachedParentNodeForCopyPaste = ParentNode;
		CachedSubNodesForCopyPaste = SubNodes;

		ParentNode = nullptr;
		SubNodes.Reset();

		CachedNodeInstanceParentNodeForCopyPaste = CastedNodeInstance->ParentNode;
		CachedNodeInstanceSubNodesForCopyPaste = CastedNodeInstance->SubNodes;

		CastedNodeInstance->ParentNode = nullptr;
		CastedNodeInstance->SubNodes.Reset();
	}
}

void UJointEdGraphNode::ResetPinDataGuid()
{
	for (FJointEdPinData& Data : PinData)
	{
		Data.ImplementedPinId = FGuid();
	}
}

void UJointEdGraphNode::PostCopyNode()
{
	UpdateNodeInstanceOuter();

	//Restore the node instance's parent / sub nodes properties with the cached data.
	if (UJointNodeBase* CastedNodeInstance = GetCastedNodeInstance())
	{
		ParentNode = CachedParentNodeForCopyPaste;
		SubNodes = CachedSubNodesForCopyPaste;

		CastedNodeInstance->ParentNode = CachedNodeInstanceParentNodeForCopyPaste;
		CastedNodeInstance->SubNodes = CachedNodeInstanceSubNodesForCopyPaste;
	}
}

void UJointEdGraphNode::PostEditImport()
{
	ReallocateNodeInstanceGuid();

	ResetPinDataGuid();

	UpdateNodeInstance();
}

void UJointEdGraphNode::PostPasteNode()
{
	ReallocateNodeInstanceGuid();

	ResetPinDataGuid();

	UpdateNodeInstance();

	//Since it has been allocated in different location, refresh the connection.
	NodeConnectionListChanged();
}

void UJointEdGraphNode::PostEditUndo()
{
	UpdateNodeInstance();

	return Super::PostEditUndo();
}


void UJointEdGraphNode::GetPinDataToConnectionMap(TMap<FJointEdPinData, FJointNodes>& PinToConnection)
{
	//Get all pins that has been populated by this node.
	TArray<UEdGraphPin*> PinsFromThisNode = GetAllPinsFromThis();

	for (UEdGraphPin* PinFromThisNode : PinsFromThisNode)
	{
		if (UEdGraphPin* ReplicatedPin = FindReplicatedSubNodePin(PinFromThisNode))
		{
			FJointEdPinData* ReplicatedPinData = FindPinDataForImplementedPin(ReplicatedPin);

			if (ReplicatedPinData == nullptr) continue;

			TArray<UJointNodeBase*> ConnectedNodes;

			for (const UEdGraphPin* LinkedTo : ReplicatedPin->LinkedTo)
			{
				if (LinkedTo == nullptr) continue;

				if (LinkedTo->GetOwningNode() == nullptr) continue;

				UEdGraphNode* ConnectedNode = LinkedTo->GetOwningNode();

				if (UJointEdGraphNode* GraphNode = Cast<UJointEdGraphNode>(ConnectedNode); GraphNode)
				{
					TArray<UJointNodeBase*> ActualNodes;

					GraphNode->AllocateReferringNodeInstancesOnConnection(ActualNodes);

					ConnectedNodes.Append(ActualNodes);
				}
			}

			PinToConnection.Add(*ReplicatedPinData, ConnectedNodes);
		}
	}
}

void UJointEdGraphNode::NodeConnectionListChanged()
{
	//Notify that the connection has been changed to the node instance.
	if (UJointNodeBase* CastedNodeInstance = GetCastedNodeInstance(); CastedNodeInstance && CastedNodeInstance->
		GetAllowNodeInstancePinControl())
	{
		TMap<FJointEdPinData, FJointNodes> PinToConnection;

		GetPinDataToConnectionMap(PinToConnection);

		CastedNodeInstance->OnPinConnectionChanged(PinToConnection);
	}

	//Propagate this function to the sub nodes.
	for (UJointEdGraphNode* SubNode : SubNodes)
	{
		if (!SubNode) continue;

		if (!SubNode->IsValidLowLevel()) continue;

		SubNode->NodeConnectionListChanged();
	}

	return;

	//Like this.

	/*
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin == nullptr) continue;

		UEdGraphPin* ReplicatedPin = FindReplicatedSubNodePin(Pin);

		if (ReplicatedPin == nullptr) continue;

		for (UEdGraphPin* LinkedTo : ReplicatedPin->LinkedTo)
		{
			if (LinkedTo == nullptr) continue;

			if (LinkedTo->GetOwningNode() == nullptr) continue;

			UEdGraphNode* ConnectedNode = LinkedTo->GetOwningNode();

			//Use ConnectedNode as you need.
		}
	}
	*/
}

void UJointEdGraphNode::AllocateReferringNodeInstancesOnConnection(TArray<UJointNodeBase*>& Nodes)
{
	Nodes.Add(GetCastedNodeInstance());
}

/*
void UJointEdGraphNode::FeedChangedSubNodeConnectionList()
{

	TArray<UEdGraphPin*> SubNodePins = GetAllPinsFromChildren();

	for (UEdGraphPin* Pin : Pins)
	{
		//Revert if it is not a pin from sub node.
		if(CheckProvidedPinIsOnPinData(Pin)) continue;

		if (UEdGraphPin** FoundSubNodePin = SubNodePins.FindByPredicate([Pin] (const UEdGraphPin* SubNodePin)
		{
			return Pin->PinId == SubNodePin->PinId;
		}))
		{
			(*FoundSubNodePin)->LinkedTo = Pin->LinkedTo;
		}
	}
}
*/

void UJointEdGraphNode::ReallocatePins()
{
	if (UJointNodeBase* CastedNodeInstance = GetCastedNodeInstance(); CastedNodeInstance && CastedNodeInstance->
		GetAllowNodeInstancePinControl())
	{
		CastedNodeInstance->OnUpdatePinData(PinData);
	}

	//Use this function to dynamically update the pin data.
	//PinData.Add()...
}

void UJointEdGraphNode::UpdateNodeInstanceOuter() const
{
	UJointManager* Manager = GetJointManager();

	SetNodeInstanceOuterTo(Manager);

	//Propagate the execution to the children sub nodes to make sure all the sub nodes' instances are correctly assigned to its parent node.
	UpdateSubNodesInstanceOuter();
}

void UJointEdGraphNode::UpdateSubNodesInstanceOuter() const
{
	if (this->NodeInstance == nullptr) return;

	UJointManager* Manager = GetJointManager();

	for (const UJointEdGraphNode* SubNode : SubNodes)
	{
		if (SubNode == nullptr) continue;

		if (!SubNode->IsValidLowLevel()) continue;

		SubNode->SetNodeInstanceOuterTo(Manager);

		SubNode->UpdateSubNodesInstanceOuter();
	}
}

void SanitizeName(FString& InOutName)
{
	// Sanitize the name
	for (int32 i = 0; i < InOutName.Len(); ++i)
	{
		TCHAR& C = InOutName[i];

		const bool bGoodChar = FChar::IsAlpha(C) || // Any letter
			(C == '_') || (C == '-') || (C == '.') || // _  - . anytime
			((i > 0) && (FChar::IsDigit(C))) || // 0-9 after the first character
			((i > 0) && (C == ' ')); // Space after the first character to support virtual bones

		if (!bGoodChar)
		{
			C = '_';
		}
	}

	if (InOutName.Len() > FKismetNameValidator::GetMaximumNameLength())
	{
		InOutName.LeftChopInline(InOutName.Len() - FKismetNameValidator::GetMaximumNameLength());
	}
}

FName UJointEdGraphNode::GetSafeNewName(const FString& InPotentialNewName, UObject* Outer) const
{
	FString SanitizedName = InPotentialNewName;
	SanitizeName(SanitizedName);
	FString Name = SanitizedName;

	int32 Suffix = 1;
	while (StaticFindObject(nullptr, Outer, *Name, true))
	{
		FString BaseString = SanitizedName;


		FString LString, RString;

		//if we found it as numeric, then trim it.
		if (BaseString.Split("_", &LString, &RString, ESearchCase::IgnoreCase, ESearchDir::FromEnd) && RString.
			IsNumeric())
		{
			BaseString = LString;
		}

		if (BaseString.Len() > FKismetNameValidator::GetMaximumNameLength() - 4)
		{
			BaseString.LeftChopInline(BaseString.Len() - (FKismetNameValidator::GetMaximumNameLength() - 4));
		}
		Name = *FString::Printf(TEXT("%s_%d"), *BaseString, ++Suffix);
	}

	return *Name;
}

bool UJointEdGraphNode::SetNodeInstanceOuterTo(UObject* NewOuter) const
{
	//You can not null out the outer with this function.
	if (NewOuter == nullptr) return false;

	//Revert if the node instance was nullptr.
	if (!NodeInstance) return false;

	//Revert if the new outer is the same with the object.
	if (NewOuter == NodeInstance) return false;

	//Revert if the node instance has the same outer as the given outer.
	if (NodeInstance->GetOuter() == NewOuter) return false;

	FName NewName = GetSafeNewName(NodeInstance->GetName(), NewOuter);

	NodeInstance->Rename(*NewName.ToString(), NewOuter, REN_NonTransactional);

	return true;
}


void UJointEdGraphNode::ReconstructNode()
{
	UpdateNodeInstance();

	UpdateNodeClassData();

	PatchNodeInstanceFromClassDataIfNeeded();

	UpdatePins();

	RequestUpdateSlate();

	NodeConnectionListChanged();
	
	//if(!IsLocked()) NotifyGraphRequestUpdate();
}

void UJointEdGraphNode::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	AddContextMenuActionsFragments(Menu, "Fragment", Context);
}


void UJointEdGraphNode::CreateAddFragmentSubMenu(UToolMenu* Menu, UEdGraph* Graph) const
{
	TSharedRef<SJointGraphEditorActionMenu> Widget =
		SNew(SJointGraphEditorActionMenu)
		.GraphObj(Graph)
		.GraphNodes(TArray<UJointEdGraphNode*>({const_cast<UJointEdGraphNode*>(this)}))
		.AutoExpandActionMenu(true);

	FToolMenuSection& Section = Menu->FindOrAddSection("Fragment");
	Section.AddEntry(FToolMenuEntry::InitWidget("FragmentWidget", Widget, FText(), false));
}


void UJointEdGraphNode::AddContextMenuActionsFragments(UToolMenu* Menu, const FName SectionName,
                                                          UGraphNodeContextMenuContext* Context) const
{
	FToolMenuSection& Section = Menu->FindOrAddSection(SectionName);
	Section.AddSeparator("Fragment");
	Section.AddSubMenu(
		"AddFragment"
		, LOCTEXT("AddFragment", "Add Fragment...")
		, LOCTEXT("AddFragmentTooltip", "Adds new fragment as a subnode")
		, FNewToolMenuDelegate::CreateUObject(this, &UJointEdGraphNode::CreateAddFragmentSubMenu,
		                                      (UEdGraph*)Context->Graph));
}

EOrientation UJointEdGraphNode::GetSubNodeBoxOrientation()
{
	return SubNodeBoxOrientation;
}

bool UJointEdGraphNode::GetUseFixedNodeSize() const
{
	return bUseFixedNodeSize;
}

void UJointEdGraphNode::RecalculateNodeDepth()
{
	NodeDepth = ParentNode != nullptr && ParentNode->IsValidLowLevel() ? ParentNode->NodeDepth + 1 : 0;

	for (UJointEdGraphNode* SubNode : SubNodes)
	{
		if(!SubNode) continue;

		SubNode->RecalculateNodeDepth();
	}
}

const uint16& UJointEdGraphNode::GetNodeDepth() const
{
	return NodeDepth;
}

void UJointEdGraphNode::ResizeNode(const FVector2D& NewSize)
{
	//revert the action if the size of the new size is smaller than the min size, since it will be likely a click event's overhead.
	//if(NewSize.Size() <= JointGraphNodeResizableDefs::MinNodeSize.Size()) return;

	const FVector2D& Min = GetNodeMinimumSize();
	const FVector2D& Max = GetNodeMaximumSize();

	const FVector2D& NodeSize = FVector2D(
		FMath::Clamp<float>(NewSize.X, Min.X, Max.X),
		FMath::Clamp<float>(NewSize.Y, Min.Y, Max.Y)
	);

	NodeWidth = NodeSize.X;
	NodeHeight = NodeSize.Y;
}


FVector2D UJointEdGraphNode::GetNodeMaximumSize() const
{
	return JointGraphNodeResizableDefs::MaxNodeSize;
}

FVector2D UJointEdGraphNode::GetNodeMinimumSize() const
{
	return JointGraphNodeResizableDefs::MinNodeSize;
}


bool UJointEdGraphNode::CheckCanAddSubNode(UJointEdGraphNode* SubNode)
{
	FPinConnectionResponse ThisNodeResponse = CanAttachSubNodeOnThis(SubNode);
	FPinConnectionResponse SubNodeResponse = SubNode->CanAttachThisAtParentNode(this);

	if (!CanHaveSubNode())
	{
		FText NotificationText = FText::FromString("Sub node add action has been denied");
		FText NotificationSubText = FText::FromString(this->GetName() + ":\n\n" + "Can not have sub nodes.");

		FNotificationInfo NotificationInfo(NotificationText);
		NotificationInfo.SubText = NotificationSubText;
		NotificationInfo.Image = FJointEditorStyle::Get().GetBrush("JointUI.Image.JointManager");
		NotificationInfo.bFireAndForget = true;
		NotificationInfo.FadeInDuration = 0.3f;
		NotificationInfo.FadeOutDuration = 1.3f;
		NotificationInfo.ExpireDuration = 4.5f;

		FSlateNotificationManager::Get().AddNotification(NotificationInfo);


		return false;
	}


	if (ThisNodeResponse.Response == ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW)
	{
		FText NotificationText = FText::FromString("Sub node add action has been denied");
		FText NotificationSubText = FText::FromString(this->GetName() + ":\n\n" + ThisNodeResponse.Message.ToString());

		FNotificationInfo NotificationInfo(NotificationText);
		NotificationInfo.SubText = NotificationSubText;
		NotificationInfo.Image = FJointEditorStyle::Get().GetBrush("JointUI.Image.JointManager");
		NotificationInfo.bFireAndForget = true;
		NotificationInfo.FadeInDuration = 0.3f;
		NotificationInfo.FadeOutDuration = 1.3f;
		NotificationInfo.ExpireDuration = 4.5f;

		FSlateNotificationManager::Get().AddNotification(NotificationInfo);


		return false;
	}

	if (SubNodeResponse.Response == ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW)
	{
		//DisallowMessage
		FText NotificationText = FText::FromString("Sub node add action has been denied");
		FText NotificationSubText =
			FText::FromString(SubNode->GetName() + ":\n\n" + SubNodeResponse.Message.ToString());

		FNotificationInfo NotificationInfo(NotificationText);
		NotificationInfo.SubText = NotificationSubText;
		NotificationInfo.Image = FJointEditorStyle::Get().GetBrush("JointUI.Image.JointManager");
		NotificationInfo.bFireAndForget = true;
		NotificationInfo.FadeInDuration = 0.3f;
		NotificationInfo.FadeOutDuration = 1.3f;
		NotificationInfo.ExpireDuration = 4.5f;

		FSlateNotificationManager::Get().AddNotification(NotificationInfo);


		return false;
	}
	return true;
}

void UJointEdGraphNode::AddSubNode(UJointEdGraphNode* SubNode, const bool bIsUpdateLocked)
{
	if (!CheckCanAddSubNode(SubNode)) return;

	//MUST NOT HAVE any transaction to avoid the modify() being called multiple time.
	//This can result an unexpected action. (ex, calling AddSubNode() right after the RemoveSubNode() -> Can corrupt the caches badly.)
	//Make only the highest execution for the action (ex, SGraphNode, Toolkit) can modify the objects.

	// set outer to be the graph, so it doesn't go away
	SubNode->SetParentNodeTo(this);

	SubNodes.Add(SubNode);

	SubNode->UpdatePins();
	SubNode->AutowireNewNode(nullptr);
	
	if(!bIsUpdateLocked)
	{
		Update();
	}
}


int32 UJointEdGraphNode::FindSubNodeDropIndex(UJointEdGraphNode* SubNode) const
{
	const int32 InsertIndex = SubNodes.IndexOfByKey(SubNode) + 1;
	return InsertIndex;
}

void UJointEdGraphNode::SetParentNodeTo(UJointEdGraphNode* NewParentNode)
{
	if (NewParentNode && NewParentNode->IsValidLowLevel())
	{
		this->ParentNode = NewParentNode;

		if (GetCastedNodeInstance())
		{
			if (NewParentNode->GetCastedNodeInstance())
			{
				GetCastedNodeInstance()->ParentNode = NewParentNode->GetCastedNodeInstance();
			}
			else
			{
				GetCastedNodeInstance()->ParentNode = nullptr;
			}
		}
	}
	else
	{
		this->ParentNode = nullptr;

		if (GetCastedNodeInstance()) GetCastedNodeInstance()->ParentNode = nullptr;
	}

	RecalculateNodeDepth();
}

void UJointEdGraphNode::Update()
{
	UpdateNodeInstance();

	UpdateSubNodeChain();

	UpdatePins();

	NodeConnectionListChanged();

	RequestUpdateSlate();
}

void UJointEdGraphNode::RearrangeSubNodeAt(UJointEdGraphNode* SubNode, int32 DropIndex, const bool bIsUpdateLocked)
{
	//MUST NOT HAVE any transaction to avoid the modify() being called multiple time.
	//This can result an unexpected action. (ex, calling AddSubNode() right after the RemoveSubNode() -> Can corrupt the caches badly.)
	//Make only the highest execution for the action (ex, SGraphNode, Toolkit) can modify the objects.

	if (!SubNodes.IsValidIndex(DropIndex)) return;

	if (!SubNodes.Contains(SubNode)) return;

	const int SavedIndex = SubNodes.Find(SubNode);

	if (DropIndex == SavedIndex) return;

	SubNodes.RemoveAt(SavedIndex);

	if (DropIndex != INDEX_NONE)
	{
		SubNodes.Insert(SubNode, ((SavedIndex > DropIndex) ? DropIndex : DropIndex - 1));
	}
	else
	{
		SubNodes.Add(SubNode);
	}

	SubNode->SetParentNodeTo(this);

	if(!bIsUpdateLocked) Update();

}


void UJointEdGraphNode::RemoveSubNode(UJointEdGraphNode* SubNode, const bool bIsUpdateLocked)
{
	//MUST NOT HAVE any transaction to avoid the modify() being called multiple time.
	//This can result an unexpected action. (ex, calling AddSubNode() right after the RemoveSubNode() -> Can corrupt the caches badly.)
	//Make only the highest execution for the action (ex, SGraphNode, Toolkit) can modify the objects.

	SubNodes.RemoveSingle(SubNode);

	SubNode->SetParentNodeTo(nullptr);

	if(!bIsUpdateLocked) Update();
	
}

void UJointEdGraphNode::RemoveAllSubNodes(bool bIsUpdateLocked = true)
{
	SubNodes.Reset();

	if(!bIsUpdateLocked) Update();
}

void UJointEdGraphNode::SyncNodeInstanceSubNodeListFromGraphNode()
{
	UJointNodeBase* NodeBaseInstance = GetCastedNodeInstance();

	if (!NodeBaseInstance) return;

	//Refresh the sub node array from the graph node's sub nodes.
	NodeBaseInstance->SubNodes.Empty();

	for (UJointEdGraphNode* InSubNode : SubNodes)
	{
		if (InSubNode == nullptr) continue;

		//Something went wrong.
		if (InSubNode == this) continue;

		UJointNodeBase* SubNodeInstance = InSubNode->GetCastedNodeInstance();

		if (!SubNodeInstance) continue;

		NodeBaseInstance->SubNodes.Add(SubNodeInstance);

		InSubNode->SetParentNodeTo(this);

		//Propagate to the sub node.
		InSubNode->SyncNodeInstanceSubNodeListFromGraphNode();
	}
}

void UJointEdGraphNode::UpdateSubNodeChain()
{
	for (UJointEdGraphNode* SubNode : this->SubNodes)
	{
		if (SubNode == nullptr) continue;

		if (!SubNode->IsValidLowLevel()) continue;

		SubNode->SetParentNodeTo(this);

		SubNode->UpdateSubNodeChain();
	}
}


void UJointEdGraphNode::BindNodeInstancePropertyChangeEvents()
{
	UnbindNodeInstancePropertyChangeEvents();

	UJointNodeBase* NodeBaseInstance = GetCastedNodeInstance();

	if (!NodeBaseInstance) return;

	NodeBaseInstance->PropertyChangedNotifiers.AddUObject(this, &UJointEdGraphNode::OnNodeInstancePropertyChanged);
}

void UJointEdGraphNode::UnbindNodeInstancePropertyChangeEvents()
{
	UJointNodeBase* NodeBaseInstance = GetCastedNodeInstance();

	if (!NodeBaseInstance) return;

	NodeBaseInstance->PropertyChangedNotifiers.RemoveAll(this);
}

void UJointEdGraphNode::UpdateNodeInstance()
{
	AllocateNodeInstanceGuidIfNeeded();

	BindNodeInstance();
	
	UpdateNodeInstanceOuter();

	SyncNodeInstanceSubNodeListFromGraphNode();
}


void UJointEdGraphNode::OnNodeInstancePropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, const FString& PropertyName)
{
	UpdatePins();

	NotifyNodeInstancePropertyChangeToGraphNodeWidget(PropertyChangedEvent,PropertyName);

	NodeConnectionListChanged();
}

void UJointEdGraphNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	NotifyGraphNodePropertyChangeToGraphNodeWidget(PropertyChangedEvent);
	
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UJointEdGraphNode::OnDebugDataChanged(const FJointNodeDebugData* DebugData)
{
}

void UJointEdGraphNode::GrabSlateDetailLevelFromNodeInstance()
{
	if(UJointNodeBase* CastedNodeInstance = GetCastedNodeInstance()) SlateDetailLevel = CastedNodeInstance->DefaultEdSlateDetailLevel;
}


bool UJointEdGraphNode::IsSubNode() const
{
	return ParentNode == nullptr ? false : ParentNode->IsValidLowLevel();
}

void CollectAllSubNodesOf(const UJointEdGraphNode* Node, TArray<UJointEdGraphNode*>& Nodes)
{
	if (Node == nullptr) return;

	for (UJointEdGraphNode* SubNode : Node->SubNodes)
	{
		if (SubNode == nullptr) continue;

		Nodes.Add(SubNode);

		CollectAllSubNodesOf(SubNode, Nodes);
	}
}

TArray<UJointEdGraphNode*> UJointEdGraphNode::GetAllSubNodesInHierarchy() const
{
	TArray<UJointEdGraphNode*> Result;

	CollectAllSubNodesOf(this, Result);

	return Result;
}

bool UJointEdGraphNode::CanHaveBreakpoint() const
{
	return true;
}


void UJointEdGraphNode::ModifyGraphNodeSlate()
{
	return;

	//You can do something like this here. override this function to make your own slates for the node.

	// if (!GraphNodeSlate.IsValid()) return;
	//
	// const TSharedPtr<SJointGraphNodeBase> NodeSlate = StaticCastSharedPtr<SJointGraphNodeBase>(GraphNodeSlate);
	//
	// NodeSlate->CenterContentBox->AddSlot().AutoHeight()
	// [
	// 	SNew(STextBlock)
	// 	.Text(LOCTEXT("TestTextBlock_SingleSlate", "TestTextBlock_SingleSlate"))
	// ];
}

void UJointEdGraphNode::CompileNode(const TSharedPtr<class IMessageLogListing>& CompileResultMessage)
{
	if (!ErrorMsg.IsEmpty()) ErrorMsg.Empty();
	bHasCompilerMessage = false;

	OnCompileNode();

	if (CompileResultMessage.IsValid())
	{
		for (const TSharedPtr<FTokenizedMessage>& TokenizedMessage : CompileMessages)
		{
			CompileResultMessage->AddMessage(
				TokenizedMessage.ToSharedRef()
			);
		}
	}

	if (GetGraphNodeSlate().IsValid()) GetGraphNodeSlate().Pin()->UpdateErrorInfo();
}

void UJointEdGraphNode::AttachDeprecationCompilerMessage()
{
	const FString& DeprecationMessage = FGraphNodeClassHelper::GetDeprecationMessage(NodeInstance->GetClass());

	if (!DeprecationMessage.IsEmpty())
	{
		TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(EMessageSeverity::Error);
		TokenizedMessage->AddToken(
			FAssetNameToken::Create(GetJointManager() ? GetJointManager()->GetName() : "NONE"));
		TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(":")));
		TokenizedMessage->AddToken(FUObjectToken::Create(this));
		TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(DeprecationMessage)));
		TokenizedMessage.Get().SetMessageLink(FUObjectToken::Create(this));

		CompileMessages.Add(TokenizedMessage);
	}
}

void UJointEdGraphNode::AttachPropertyCompilerMessage()
{
	//Check for the node pointer structure error.

	if (!GetCastedNodeInstance()) return;


	for (TFieldIterator<FArrayProperty> It(NodeInstance->GetClass()); It; ++It)
	{
		FArrayProperty* ArrayProp = *It;

		if (!ArrayProp) continue;

		if(FStructProperty* StructureProp = CastField<FStructProperty>(ArrayProp->Inner))
		{
			if (StructureProp->Struct != FJointNodePointer::StaticStruct()) continue;

			FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(NodeInstance));
			
			for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
			{
				FJointNodePointer* Struct = StructureProp->ContainerPtrToValuePtr<FJointNodePointer>(ArrayHelper.GetRawPtr(Index));
				
				const TArray<TSharedPtr<FTokenizedMessage>>& Messages = FJointNodePointer::GetCompilerMessage(*Struct, GetCastedNodeInstance(), ArrayProp->GetName());

				for (TWeakPtr<FTokenizedMessage> Message : Messages)
				{
					CompileMessages.Add(Message.Pin());
				}
			}
			
		}
	}

	/*

	//Just for the future usage. by its design, we don't support hashing on this type.

	for (TFieldIterator<FSetProperty> It(NodeInstance->GetClass()); It; ++It)
	{
		FSetProperty* SetProp = *It;

		if (!SetProp) continue;

		if(FStructProperty* StructureProp = CastField<FStructProperty>(SetProp->ElementProp))
		{
			if (StructureProp->Struct != FJointNodePointer::StaticStruct()) continue;

			FScriptSetHelper SetHelper(SetProp, SetProp->ContainerPtrToValuePtr<void>(NodeInstance));
			
			for (int32 Index = 0; Index < SetHelper.Num(); ++Index)
			{
				FJointNodePointer* Struct = StructureProp->ContainerPtrToValuePtr<FJointNodePointer>(SetHelper.GetElementPtr(Index));
				
				const TArray<TSharedPtr<FTokenizedMessage>>& Messages = FJointNodePointer::GetCompilerMessage(*Struct, GetCastedNodeInstance(), SetProp->GetName());

				for (TWeakPtr<FTokenizedMessage> Message : Messages)
				{
					CompileMessages.Add(Message.Pin());
				}
			}
			
		}
	}

	for (TFieldIterator<FMapProperty> It(NodeInstance->GetClass()); It; ++It)
	{
		FMapProperty* MapProp = *It;

		if (!MapProp) continue;

		if(FStructProperty* StructureProp = CastField<FStructProperty>(MapProp))
		{
			if (StructureProp->Struct != FJointNodePointer::StaticStruct()) continue;

			FScriptMapHelper MapHelper(MapProp, MapProp->ContainerPtrToValuePtr<void>(NodeInstance));
			
			for (int32 Index = 0; Index < MapHelper.Num(); ++Index)
			{
				FJointNodePointer* Struct = StructureProp->ContainerPtrToValuePtr<FJointNodePointer>(MapHelper.GetKeyProperty());
				
				const TArray<TSharedPtr<FTokenizedMessage>>& Messages = FJointNodePointer::GetCompilerMessage(*Struct, GetCastedNodeInstance(), MapProp->GetName());

				for (TWeakPtr<FTokenizedMessage> Message : Messages)
				{
					CompileMessages.Add(Message.Pin());
				}
			}

			for (int32 Index = 0; Index < MapHelper.Num(); ++Index)
			{
				FJointNodePointer* Struct = StructureProp->ContainerPtrToValuePtr<FJointNodePointer>(MapHelper.GetValueProperty());
				
				const TArray<TSharedPtr<FTokenizedMessage>>& Messages = FJointNodePointer::GetCompilerMessage(*Struct, GetCastedNodeInstance(), MapProp->GetName());

				for (TWeakPtr<FTokenizedMessage> Message : Messages)
				{
					CompileMessages.Add(Message.Pin());
				}
			}
		}
		
	}
	*/

	for (TFieldIterator<FStructProperty> It(NodeInstance->GetClass()); It; ++It)
	{
		FStructProperty* Property = *It;

		if (Property == nullptr || Property->Struct != FJointNodePointer::StaticStruct()) continue;

		if (FJointNodePointer* Structure = Property->ContainerPtrToValuePtr<FJointNodePointer>(NodeInstance))
		{
			const TArray<TSharedPtr<FTokenizedMessage>>& Messages = FJointNodePointer::GetCompilerMessage(
				*Structure, GetCastedNodeInstance(), Property->GetName());

			for (TWeakPtr<FTokenizedMessage> Message : Messages)
			{
				CompileMessages.Add(Message.Pin());
			}
		}
	}
}

void UJointEdGraphNode::CompileAndAttachNodeInstanceCompilationMessages()
{
	TArray<FJointEdLogMessage> NodeInstanceCompilationMessages;

	if(!GetCastedNodeInstance()) return;
		
	GetCastedNodeInstance()->OnCompileNode(NodeInstanceCompilationMessages);

	for (const FJointEdLogMessage& NodeInstanceCompilationMessage : NodeInstanceCompilationMessages)
	{
		TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(FJointEdUtils::ResolveJointEdMessageSeverityToEMessageSeverity(NodeInstanceCompilationMessage.Severity));
		TokenizedMessage->AddToken(FAssetNameToken::Create(GetJointManager() ? GetJointManager()->GetName() : "NONE"));
		TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(":")));
		TokenizedMessage->AddToken(FUObjectToken::Create(this));
		TokenizedMessage->AddToken(FTextToken::Create(NodeInstanceCompilationMessage.Message));
		TokenizedMessage.Get().SetMessageLink(FUObjectToken::Create(this));

		CompileMessages.Add(TokenizedMessage);
	}
}

void UJointEdGraphNode::OnCompileNode()
{
	CompileMessages.Empty();

	if (NodeInstance)
	{
		AttachDeprecationCompilerMessage();

		AttachPropertyCompilerMessage();
		
		CompileAndAttachNodeInstanceCompilationMessages();
	}
	else
	{
		// Null instance. Do we have any meaningful class data?
		FString StoredClassName = ClassData.GetClassName();
		StoredClassName.RemoveFromEnd(TEXT("_C"));

		if (!StoredClassName.IsEmpty())
		{
			FText MissingClassMessage = FText::Format(
				LOCTEXT("MissingNodeInstanceClass", "'{0}' class missing. Referenced by {1}"),
				FText::FromString(StoredClassName),
				FText::FromString(this->GetPathName())
			);

			if (!MissingClassMessage.IsEmpty())
			{
				TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(EMessageSeverity::Error);
				TokenizedMessage->AddToken(
					FAssetNameToken::Create(GetJointManager() ? GetJointManager()->GetName() : "NONE"));
				TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(":")));
				TokenizedMessage->AddToken(FUObjectToken::Create(this));
				TokenizedMessage->AddToken(FTextToken::Create(MissingClassMessage));
				TokenizedMessage.Get().SetMessageLink(FUObjectToken::Create(this));

				CompileMessages.Add(TokenizedMessage);
			}
		}
	}
}

bool UJointEdGraphNode::HasCompileIssues() const
{
	return CompileMessages.Num() > 0;
}


void UJointEdGraphNode::NotifyClassDataUnknown()
{
	FGraphNodeClassHelper::AddUnknownClass(ClassData);
}

bool UJointEdGraphNode::CheckClassDataIsKnown()
{
	return FGraphNodeClassHelper::IsClassKnown(ClassData);
}

bool UJointEdGraphNode::PatchNodeInstanceFromClassDataIfNeeded()
{
	bool bPatched = false;

	if (NodeInstance == nullptr)
	{
		//If the node class data is unknown, revert the patch action and notify the we have invalid class data.
		if (!CheckClassDataIsKnown())
		{
			NotifyClassDataUnknown();

			return false;
		}
		//Try to patch up the data.

		//Check if we can and have to spawn a new node instance.
		// * NodeInstance can be already spawned by paste operation, so check that here. don't override it * 
		if (const UClass* NodeClass = ClassData.GetClass(true); NodeClass && NodeInstance == nullptr)
		{
			UEdGraph* MyGraph = GetGraph();

			if (UObject* GraphOwner = MyGraph ? MyGraph->GetOuter() : nullptr)
			{
				//Create new node instance.
				NodeInstance = NewObject<UObject>(GraphOwner, NodeClass);
				NodeInstance->SetFlags(RF_Transactional);

				NotifyGraphChanged();
			}
		}

		bPatched = (NodeInstance != nullptr);
	}


	return bPatched;
}

void UJointEdGraphNode::UpdateNodeClassData()
{
	if (!NodeInstance) return;

	UpdateNodeClassDataFrom(NodeInstance->GetClass(), ClassData);
}

void UJointEdGraphNode::UpdateNodeClassDataFrom(UClass* InstanceClass, FGraphNodeClassData& UpdatedData)
{
	if (!InstanceClass) return;

	if (const UBlueprint* BPOwner = Cast<UBlueprint>(InstanceClass->ClassGeneratedBy))
	{
		UpdatedData = FGraphNodeClassData(
			BPOwner->GetName(),
			BPOwner->GetOutermost()->GetName(),
			InstanceClass->GetName(),
			InstanceClass
		);
	}
	else
	{
		UpdatedData = FGraphNodeClassData(
			InstanceClass,
			FGraphNodeClassHelper::GetDeprecationMessage(InstanceClass)
		);
	}
}

void UJointEdGraphNode::AllocateNodeInstanceGuidIfNeeded() const
{
	if (UJointNodeBase* CastedNodeInstance = GetCastedNodeInstance())
	{
		if (CastedNodeInstance->NodeGuid == FGuid()) CastedNodeInstance->NodeGuid = FGuid::NewGuid();
	}
}

void UJointEdGraphNode::ReallocateNodeInstanceGuid() const
{
	if (UJointNodeBase* CastedNodeInstance = GetCastedNodeInstance())
	{
		CastedNodeInstance->NodeGuid = FGuid::NewGuid();
	}
}

FLinearColor UJointEdGraphNode::GetNodeBodyTintColor() const
{
	const UJointNodeBase* CastedNodeInstance = GetCastedNodeInstance();

	if(UJointEditorSettings* EdSettings = UJointEditorSettings::Get())
	{
		return CastedNodeInstance && CastedNodeInstance->bUseSpecifiedGraphNodeBodyColor
			? CastedNodeInstance->NodeBodyColor + GetNodeDepth() * EdSettings->NodeDepthAdditiveColor // NodeInstance specified Color
			: EdSettings->DefaultNodeColor + GetNodeDepth() * EdSettings->NodeDepthAdditiveColor; //Defa
	}

	return FLinearColor::Transparent;
	
}

void UJointEdGraphNode::ClearCachedParentGuid()
{
	CachedParentGuidForCopyPaste.Invalidate();
}

TSubclassOf<UJointNodeBase> UJointEdGraphNode::SupportedNodeClass()
{
	return UJointNodeBase::StaticClass();
}

void UJointEdGraphNode::SetGraphNodeSlate(const TSharedPtr<SJointGraphNodeBase>& InGraphNodeSlate)
{
	GraphNodeSlate = InGraphNodeSlate;
}

TWeakPtr<SJointGraphNodeBase> UJointEdGraphNode::GetGraphNodeSlate()
{
	return GraphNodeSlate;
}


#undef LOCTEXT_NAMESPACE
