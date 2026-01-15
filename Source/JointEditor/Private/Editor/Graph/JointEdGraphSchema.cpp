//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointEdGraphSchema.h"

#include "EdGraphSchema_K2_Actions.h"
#include "JointEdGraph.h"
#include "Node/JointNodeBase.h"

#include "JointEdGraphNode.h"
#include "JointEdGraphNode_Fragment.h"
#include "JointEdGraphNode_Manager.h"
#include "JointEditorSettings.h"
#include "JointEdUtils.h"
#include "JointGraphConnectionDrawingPolicy.h"
#include "JointManager.h"

#include "GraphEditorActions.h"
#include "GraphEditorDragDropAction.h"
#include "JointEdGraphNode_Composite.h"
#include "JointEdGraphNode_Reroute.h"
#include "JointEditorCommands.h"
#include "JointEditorLogChannels.h"
#include "K2Node_CallFunction.h"
#include "K2Node_MakeStruct.h"
#include "ScopedTransaction.h"

#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "Engine/World.h"

#include "Framework/Commands/GenericCommands.h"
#include "Modules/ModuleManager.h"
#include "Node/JointFragment.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "JointEdGraphSchema"


class JOINTEDITOR_API FJointEdGraphDragDropAction : public FGraphSchemaActionDragDropAction
{
public:
	DRAG_DROP_OPERATOR_TYPE(FJointEdGraphDragDropAction, FGraphSchemaActionDragDropAction)

	virtual FReply DroppedOnPanel(const TSharedRef<class SWidget>& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph) override;
	virtual FReply DroppedOnNode(FVector2D ScreenPosition, FVector2D GraphPosition) override;
	virtual FReply DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition) override;
	virtual FReply DroppedOnAction(TSharedRef<FEdGraphSchemaAction> Action) override;
	virtual FReply DroppedOnCategory(FText Category) override;
	virtual void HoverTargetChanged() override;

protected:
	/** Constructor */
	FJointEdGraphDragDropAction();

	static TSharedRef<FJointEdGraphDragDropAction> New(TSharedPtr<FEdGraphSchemaAction> InAction, FName InFuncName, UJointManager* InJointManager, UJointEdGraph* InJointEdGraph);

	UJointManager* SourceJointManager;
	UJointEdGraph* SourceJointEdGraph;
	FName SourceFuncName;

	friend class UJointEdGraphSchema;
};


FJointEdGraphDragDropAction::FJointEdGraphDragDropAction()
	: FGraphSchemaActionDragDropAction()
	  , SourceJointManager(nullptr)
	  , SourceJointEdGraph(nullptr)
	  , SourceFuncName(NAME_None)
{
}

FReply FJointEdGraphDragDropAction::DroppedOnPanel(const TSharedRef<class SWidget>& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph)
{
	if (UJointEdGraph* TargetJointGraph = Cast<UJointEdGraph>(&Graph))
	{
		//if (UAnimBlueprint* TargetAnimBlueprint = Cast<UAnimBlueprint>(FBlueprintEditorUtils::FindBlueprintForGraph(TargetRigGraph)))
		//{
		//FGraphNodeCreator<UAnimGraphNode_LinkedAnimLayer> LinkedInputLayerNodeCreator(*TargetRigGraph);
		//UAnimGraphNode_LinkedAnimLayer* LinkedAnimLayerNode = LinkedInputLayerNodeCreator.CreateNode();	
		//const FName GraphName = TargetRigGraph->GetFName();
		//LinkedAnimLayerNode->SetupFromLayerId(SourceFuncName);
		//LinkedInputLayerNodeCreator.Finalize();
		//LinkedAnimLayerNode->NodePosX = GraphPosition.X;
		//LinkedAnimLayerNode->NodePosY = GraphPosition.Y;
		//} 
	}
	return FReply::Unhandled();
}

FReply FJointEdGraphDragDropAction::DroppedOnNode(FVector2D ScreenPosition, FVector2D GraphPosition)
{
	return FReply::Unhandled();
}

FReply FJointEdGraphDragDropAction::DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition)
{
	return FReply::Unhandled();
}

FReply FJointEdGraphDragDropAction::DroppedOnAction(TSharedRef<FEdGraphSchemaAction> Action)
{
	return FReply::Unhandled();
}

FReply FJointEdGraphDragDropAction::DroppedOnCategory(FText Category)
{
	return FReply::Unhandled();
}

void FJointEdGraphDragDropAction::HoverTargetChanged()
{
	FGraphSchemaActionDragDropAction::HoverTargetChanged();
	bDropTargetValid = true;
}


TSharedRef<FJointEdGraphDragDropAction> FJointEdGraphDragDropAction::New(TSharedPtr<FEdGraphSchemaAction> InAction, FName InFuncName, UJointManager* InJointManager, UJointEdGraph* InJointEdGraph)
{
	TSharedRef<FJointEdGraphDragDropAction> Action = MakeShareable(new FJointEdGraphDragDropAction);
	Action->SourceAction = InAction;
	Action->SourceJointManager = InJointManager;
	Action->SourceJointEdGraph = InJointEdGraph;
	Action->SourceFuncName = InFuncName;
	Action->Construct();
	return Action;
}


UJointEdGraphSchema::UJointEdGraphSchema(const class FObjectInitializer&)
{
}

bool UJointEdGraphSchema::CanGraphBeDropped(TSharedPtr<FEdGraphSchemaAction> InAction) const
{
	if (!InAction.IsValid())
	{
		return false;
	}

	if (InAction->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Graph* FuncAction = (FEdGraphSchemaAction_K2Graph*)InAction.Get();
		if (UJointEdGraph* JointGraph = Cast<UJointEdGraph>((UEdGraph*)FuncAction->EdGraph))
		{
			return true;
		}
	}

	return false;
}

FReply UJointEdGraphSchema::BeginGraphDragAction(TSharedPtr<FEdGraphSchemaAction> InAction, const FPointerEvent& MouseEvent) const
{
	if (!InAction.IsValid())
	{
		return FReply::Unhandled();
	}

	if (InAction->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Graph* FuncAction = (FEdGraphSchemaAction_K2Graph*)InAction.Get();
		if (UJointEdGraph* JointEdGraph = Cast<UJointEdGraph>((UEdGraph*)FuncAction->EdGraph))
		{
			if (UJointManager* TargetManager = JointEdGraph->GetJointManager())
			{
				return FReply::Handled().BeginDragDrop(FJointEdGraphDragDropAction::New(InAction, FuncAction->FuncName, TargetManager, JointEdGraph));
			}
		}
	}

	return FReply::Unhandled();
}

void UJointEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	ImplementAddCommentAction(ContextMenuBuilder);
	ImplementAddConnectorAction(ContextMenuBuilder);
	ImplementAddNodeActions(ContextMenuBuilder);
}

void UJointEdGraphSchema::GetGraphNodeContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	ImplementAddFragmentActions(ContextMenuBuilder);
}


void UJointEdGraphSchema::ImplementAddNodeActions(FGraphContextMenuBuilder& ContextMenuBuilder)
{
	// Add action
	if (ContextMenuBuilder.CurrentGraph == nullptr) return;

	const FText CustomCategory = LOCTEXT("CustomCategory", "Custom");

	UEdGraph* Graph = const_cast<UEdGraph*>(ContextMenuBuilder.CurrentGraph);

	TArray<FJointGraphNodeClassData> NodeClasses;

	FJointEdUtils::GetNodeSubClasses(UJointNodeBase::StaticClass(), NodeClasses);

	for (FJointGraphNodeClassData& NodeClass : NodeClasses)
	{
		if (NodeClass.GetClass() == nullptr) continue;

		if (NodeClass.GetClass()->IsChildOf(UJointFragment::StaticClass())) continue;

		//It will be hidden when the class is deprecated.
		if (NodeClass.IsAbstract() || NodeClass.IsDeprecated() || NodeClass.GetClass()->HasAnyClassFlags(
			CLASS_Deprecated | CLASS_Hidden))
			continue;

		if (UJointEditorSettings::Get() && UJointEditorSettings::Get()->NodeClassesToHide.
		                                                                Contains(NodeClass.GetClass()))
			continue;

		//Todo : Gotta change it to use context menu if there is multiple options.
		const UClass* EdGraphNodeClass = FJointEdUtils::FindEdClassForNode(NodeClass);

		if (EdGraphNodeClass == nullptr) EdGraphNodeClass = UJointEdGraphNode::StaticClass();

		const FText NodeTypeName = FText::FromString(FName::NameToDisplayString(NodeClass.ToString(), false));
		const FText NodeCategory = (!NodeClass.GetCategory().IsEmpty()) ? NodeClass.GetCategory() : CustomCategory;
		const FText NodeTooltip = NodeClass.GetClass()->GetToolTipText();

		UJointEdGraphNode* OpNode = NewObject<UJointEdGraphNode>(Graph, EdGraphNodeClass);
		OpNode->NodeClassData = NodeClass;

		const TSharedPtr<FJointSchemaAction_NewNode> AddNodeAction = CreateNewNodeAction(
			NodeCategory, NodeTypeName, NodeTooltip);
		AddNodeAction->NodeTemplate = OpNode;

		ContextMenuBuilder.AddAction(AddNodeAction);
	}
}

void UJointEdGraphSchema::ImplementAddFragmentActions(FGraphContextMenuBuilder& ContextMenuBuilder)
{
	if (ContextMenuBuilder.CurrentGraph == nullptr) return;

	UEdGraph* Graph = const_cast<UEdGraph*>(ContextMenuBuilder.CurrentGraph);

	TArray<FJointGraphNodeClassData> FragmentClasses;
	FJointEdUtils::GetNodeSubClasses(UJointFragment::StaticClass(), FragmentClasses);

	for (FJointGraphNodeClassData& NodeClass : FragmentClasses)
	{
		//Make sure we are going to display this action or not.
		if (NodeClass.GetClass() == nullptr) continue;

		//It will be hidden when the class is deprecated.
		if (NodeClass.IsAbstract() || NodeClass.IsDeprecated() || NodeClass.GetClass()->HasAnyClassFlags(
			CLASS_Deprecated | CLASS_Hidden))
			continue;

		if (UJointEditorSettings::Get() && UJointEditorSettings::Get()->NodeClassesToHide.
		                                                                Contains(NodeClass.GetClass()))
			continue;

		//Grab the Ed node class
		TSubclassOf<UJointEdGraphNode> TargetFragmentEdClass = FJointEdUtils::FindEdClassForNode(NodeClass);

		if (TargetFragmentEdClass == nullptr) TargetFragmentEdClass = UJointEdGraphNode_Fragment::StaticClass();


		const FText NodeTypeName = FText::FromString(FName::NameToDisplayString(NodeClass.ToString(), false));
		const FText NodeCategory = !NodeClass.GetCategory().IsEmpty() ? NodeClass.GetCategory() : FText::GetEmpty();
		const FText NodeTooltip = NodeClass.GetClass()->GetToolTipText();

		UJointEdGraphNode* OpNode = NewObject<UJointEdGraphNode>(Graph, TargetFragmentEdClass);
		OpNode->NodeClassData = NodeClass;

		const TSharedPtr<FJointSchemaAction_NewSubNode> AddSubnodeAction = CreateNewSubNodeAction(
			NodeCategory, NodeTypeName, NodeTooltip);
		AddSubnodeAction->NodesToAttachTo = ContextMenuBuilder.SelectedObjects;
		AddSubnodeAction->NodeTemplate = OpNode;

		ContextMenuBuilder.AddAction(AddSubnodeAction);
	}
}

void UJointEdGraphSchema::ImplementAddCommentAction(FGraphContextMenuBuilder& ContextMenuBuilder)
{
	const FText Category = LOCTEXT("CommentCategory", "Comment");
	const FText MenuDesc = LOCTEXT("CommentMenuDesc", "Add Comment.....");
	const FText ToolTip = LOCTEXT("CommentToolTip",
	                              "Add a comment on the graph that helps you editing and organizing the other nodes.");

	const TSharedPtr<FJointSchemaAction_AddComment> AddCommentAction = MakeShared<FJointSchemaAction_AddComment>(
		Category, MenuDesc, ToolTip);

	ContextMenuBuilder.AddAction(AddCommentAction);
}

void UJointEdGraphSchema::ImplementAddConnectorAction(FGraphContextMenuBuilder& ContextMenuBuilder)
{
	const FText Category = LOCTEXT("ConnectorCategory", "Connector");
	const FText MenuDesc = LOCTEXT("ConnectorMenuDesc", "Add Connector.....");
	const FText ToolTip = LOCTEXT("ConnectorToolTip",
	                              "Add a connector node on the graph that helps you editing and organizing the other nodes.");

	const TSharedPtr<FJointSchemaAction_AddConnector> AddConnectorAction = MakeShared<
		FJointSchemaAction_AddConnector>(Category, MenuDesc, ToolTip);

	ContextMenuBuilder.AddAction(AddConnectorAction);
}

TSharedPtr<FJointSchemaAction_NewNode> UJointEdGraphSchema::CreateNewNodeAction(
	const FText& Category, const FText& MenuDesc, const FText& Tooltip)
{
	TSharedPtr<FJointSchemaAction_NewNode> NewAction = MakeShared<FJointSchemaAction_NewNode>(
		Category, MenuDesc, Tooltip, 0);

	return NewAction;
}

TSharedPtr<FJointSchemaAction_NewSubNode> UJointEdGraphSchema::CreateNewSubNodeAction(
	const FText& Category, const FText& MenuDesc, const FText& Tooltip)
{
	TSharedPtr<FJointSchemaAction_NewSubNode> NewAction = MakeShared<FJointSchemaAction_NewSubNode>(
		Category, MenuDesc, Tooltip, 0);

	return NewAction;
}

bool UJointEdGraphSchema::PruneGatewayNode(UJointEdGraphNode_Composite* InNode, UEdGraphNode* InEntryNode, UEdGraphNode* InResultNode, FKismetCompilerContext* CompilerContext, TSet<UEdGraphNode*>* OutExpandedNodes) const
{
	// This function is responsible for collapsing (removing) the gateway node and moving all of its connections to the entry or result node as appropriate.
	// It does not handle the actual moving of nodes into the subgraph, just the connections - that is handled elsewhere. (see FJointEditorToolkit::ExpandNode)
	bool bSuccessful = true;

	// We iterate the array in reverse so we can both remove the subpins safely after we've read them and
	// so we have split nested structs we combine them back together in the right order
	for (int32 BoundaryPinIndex = InNode->Pins.Num() - 1; BoundaryPinIndex >= 0; --BoundaryPinIndex)
	{
		UEdGraphPin* const BoundaryPin = InNode->Pins[BoundaryPinIndex];

		// For each pin in the gateway node, find the associated pin in the entry or result node.
		UEdGraphNode* const GatewayNode = (BoundaryPin->Direction == EGPD_Input) ? InEntryNode : InResultNode;
		UEdGraphPin* GatewayPin = nullptr;
		if (GatewayNode)
		{
			for (int32 PinIdx = GatewayNode->Pins.Num() - 1; PinIdx >= 0; --PinIdx)
			{
				UEdGraphPin* const Pin = GatewayNode->Pins[PinIdx];

				if ((Pin->PinName == BoundaryPin->PinName) && (Pin->Direction != BoundaryPin->Direction))
				{
					GatewayPin = Pin;
					break;
				}
			}
		}

		if (GatewayPin)
		{
			CombineTwoPinNetsAndRemoveOldPins(BoundaryPin, GatewayPin);
		}
		else
		{
			if (BoundaryPin->LinkedTo.Num() > 0 && BoundaryPin->ParentPin == nullptr)
			{
				UE_LOG(
					LogJointEditor,
					Warning,
					TEXT("%s"),
					*FText::Format(
						NSLOCTEXT("K2Node", "PinOnBoundryNode_WarningFmt", "Warning: Pin '{0}' on boundary node '{1}' could not be found in the composite node '{2}'"),
						FText::FromString(BoundaryPin->PinName.ToString()),
						GatewayNode ? FText::FromString(GatewayNode->GetName()) : NSLOCTEXT("K2Node", "PinOnBoundryNode_WarningNoNode", "(null)"),
						FText::FromString(GetName())
					).ToString()
				);
			}
			else
			{
				// Associated pin was not found but there were no links on this side either, so no harm no foul
			}
		}
	}

	return bSuccessful;
}

void UJointEdGraphSchema::CombineTwoPinNetsAndRemoveOldPins(UEdGraphPin* InPinA, UEdGraphPin* InPinB) const
{
	check(InPinA != NULL);
	check(InPinB != NULL);
	ensure(InPinA->Direction != InPinB->Direction);

	if ((InPinA->LinkedTo.Num() == 0) && (InPinA->Direction == EGPD_Input))
	{
		// Push the literal value of A to InPinB's connections
		for (int32 IndexB = 0; IndexB < InPinB->LinkedTo.Num(); ++IndexB)
		{
			UEdGraphPin* FarB = InPinB->LinkedTo[IndexB];
			// TODO: Michael N. says this if check should be unnecessary once the underlying issue is fixed.
			// (Probably should use a check() instead once it's removed though.  See additional cases below.
			if (FarB != nullptr)
			{
				FarB->DefaultValue = InPinA->DefaultValue;
				FarB->DefaultObject = InPinA->DefaultObject;
				FarB->DefaultTextValue = InPinA->DefaultTextValue;
			}
		}
	}
	else if ((InPinB->LinkedTo.Num() == 0) && (InPinB->Direction == EGPD_Input))
	{
		// Push the literal value of B to InPinA's connections
		for (int32 IndexA = 0; IndexA < InPinA->LinkedTo.Num(); ++IndexA)
		{
			UEdGraphPin* FarA = InPinA->LinkedTo[IndexA];
			// TODO: Michael N. says this if check should be unnecessary once the underlying issue is fixed.
			// (Probably should use a check() instead once it's removed though.  See additional cases above and below.
			if (FarA != nullptr)
			{
				FarA->DefaultValue = InPinB->DefaultValue;
				FarA->DefaultObject = InPinB->DefaultObject;
				FarA->DefaultTextValue = InPinB->DefaultTextValue;
			}
		}
	}
	else
	{
		// Make direct connections between the things that connect to A or B, removing A and B from the picture
		for (int32 IndexA = 0; IndexA < InPinA->LinkedTo.Num(); ++IndexA)
		{
			UEdGraphPin* FarA = InPinA->LinkedTo[IndexA];
			// TODO: Michael N. says this if check should be unnecessary once the underlying issue is fixed.
			// (Probably should use a check() instead once it's removed though.  See additional cases above.
			if (FarA != nullptr)
			{
				for (int32 IndexB = 0; IndexB < InPinB->LinkedTo.Num(); ++IndexB)
				{
					UEdGraphPin* FarB = InPinB->LinkedTo[IndexB];

					if (FarB != nullptr)
					{
						FarA->Modify();
						FarB->Modify();
						TryCreateConnection(FarA, FarB);
					}
				}
			}
		}
	}

	InPinA->BreakAllPinLinks();
	InPinB->BreakAllPinLinks();
}

FLinearColor UJointEdGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FLinearColor::White;
}

UEdGraphPin* UJointEdGraphSchema::DropPinOnNode(UEdGraphNode* InTargetNode, const FName& InSourcePinName, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection) const
{
	UEdGraphPin* ResultPin = nullptr;

	if (UJointEdGraphNode_Composite* Composite = Cast<UJointEdGraphNode_Composite>(InTargetNode))
	{
		FJointEdPinData* NewPinData = nullptr;
		Composite->Modify();

		FJointEdPinDataSetting Setting;
		Setting.bAlwaysDisplayNameText = true;
		
		if (InSourcePinDirection == EGPD_Output)
		{
			NewPinData = new FJointEdPinData(Composite->CreateUniquePinName("In"), EEdGraphPinDirection::EGPD_Input, Setting);
		}
		else if (InSourcePinDirection == EGPD_Input)
		{
			NewPinData = new FJointEdPinData(Composite->CreateUniquePinName("Out"), EEdGraphPinDirection::EGPD_Output, Setting);
		}

		Composite->GetPinDataFromThis().Add(MoveTemp(*NewPinData));
		Composite->UpdatePins();
		
		delete NewPinData;
	}else if (UJointEdGraphNode_Tunnel* Tunnel = Cast<UJointEdGraphNode_Tunnel>(InTargetNode))
	{
		FJointEdPinData* NewPinData = nullptr;
		Tunnel->Modify();

		FJointEdPinDataSetting Setting;
		Setting.bAlwaysDisplayNameText = true;
		
		if (InSourcePinDirection == EGPD_Output)
		{
			NewPinData = new FJointEdPinData(Tunnel->CreateUniquePinName("Out"), EEdGraphPinDirection::EGPD_Input, Setting);
		}
		else if (InSourcePinDirection == EGPD_Input)
		{
			NewPinData = new FJointEdPinData(Tunnel->CreateUniquePinName("In"), EEdGraphPinDirection::EGPD_Output, Setting);
		}

		Tunnel->GetPinDataFromThis().Add(MoveTemp(*NewPinData));
		Tunnel->UpdatePins();

		delete NewPinData;
	}
	
	return ResultPin;
}

bool UJointEdGraphSchema::SupportsDropPinOnNode(UEdGraphNode* InTargetNode, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection, FText& OutErrorMessage) const
{
	bool bIsSupported = false;
	if (UJointEdGraphNode_Composite* CompositeNode = Cast<UJointEdGraphNode_Composite>(InTargetNode))
	{
		bIsSupported = true;
		OutErrorMessage = LOCTEXT("AddPinToNode", "Add Pin to Node");
	}else if (UJointEdGraphNode_Tunnel* TunnelNode = Cast<UJointEdGraphNode_Tunnel>(InTargetNode))
	{
		bIsSupported = true;
		
		if (TunnelNode->bCanHaveOutputs) // for the Input
		{
			OutErrorMessage = LOCTEXT("AddPinToNode", "Add Pin to the input of the graph..");
		}
		
		if (TunnelNode->bCanHaveInputs) // for the output
		{
			OutErrorMessage = LOCTEXT("AddPinToNode", "Add Pin to the output of the graph..");
		}
	}
	return bIsSupported;
}

bool UJointEdGraphSchema::ShouldAlwaysPurgeOnModification() const
{
	return Super::ShouldAlwaysPurgeOnModification();
}

void UJointEdGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	Super::BreakSinglePinLink(SourcePin, TargetPin);
}

bool UJointEdGraphSchema::FadeNodeWhenDraggingOffPin(const UEdGraphNode* Node, const UEdGraphPin* Pin) const
{
	return false;
}

void UJointEdGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	Super::BreakNodeLinks(TargetNode);
}

void UJointEdGraphSchema::ReconstructNode(UEdGraphNode& TargetNode, bool bIsBatchRequest) const
{
	Super::ReconstructNode(TargetNode, bIsBatchRequest);
}

TSharedPtr<FEdGraphSchemaAction> UJointEdGraphSchema::GetCreateCommentAction() const
{
	return nullptr;
}

int32 UJointEdGraphSchema::GetNodeSelectionCount(const UEdGraph* Graph) const
{
	if (Graph)
	{
		TSharedPtr<SGraphEditor> GraphEditorPtr = SGraphEditor::FindGraphEditorForGraph(Graph);
		if (GraphEditorPtr.IsValid())
		{
			return GraphEditorPtr->GetNumberOfSelectedNodes();
		}
	}

	return 0;
}

void UJointEdGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
	Super::BreakPinLinks(TargetPin, bSendsNodeNotifcation);
}

void UJointEdGraphSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	check(Context && Context->Graph);

	//If the context is for a node.
	if (!Context->Node) return;

	if (!Context->bIsDebugging)
	{
		FToolMenuSection& NodeActionSection = Menu->AddSection("NodeActionsMenu",LOCTEXT("NodeActionsMenuHeader", "Node Actions"));
		// Node contextual actions
		NodeActionSection.AddMenuEntry(FGenericCommands::Get().Delete);
		NodeActionSection.AddMenuEntry(FGenericCommands::Get().Cut);
		NodeActionSection.AddMenuEntry(FGenericCommands::Get().Copy);
		NodeActionSection.AddMenuEntry(FGenericCommands::Get().Duplicate);
		NodeActionSection.AddMenuEntry(FGraphEditorCommands::Get().ReconstructNodes);
		NodeActionSection.AddMenuEntry(FGraphEditorCommands::Get().CollapseNodes);
		NodeActionSection.AddMenuEntry(FGraphEditorCommands::Get().ExpandNodes);
		if (const UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(Context->Node); CastedNode && !CastedNode->IsSubNode()) NodeActionSection.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
		if (Context->Node->bCanRenameNode) NodeActionSection.AddMenuEntry(FGenericCommands::Get().Rename);
	}

	FToolMenuSection& DebugActionSection = Menu->AddSection("DebugActionsMenu",LOCTEXT("DebugActionsMenuHeader", "Debug Actions"));
	DebugActionSection.AddMenuEntry(FGraphEditorCommands::Get().AddBreakpoint);
	DebugActionSection.AddMenuEntry(FGraphEditorCommands::Get().RemoveBreakpoint);
	DebugActionSection.AddMenuEntry(FGraphEditorCommands::Get().EnableBreakpoint);
	DebugActionSection.AddMenuEntry(FGraphEditorCommands::Get().DisableBreakpoint);
	DebugActionSection.AddMenuEntry(FGraphEditorCommands::Get().ToggleBreakpoint);


	if (!Context->bIsDebugging)
	{
		FToolMenuSection& DissolveSolidifyActionsSession = Menu->AddSection("DissolveSolidifyActionsMenu",LOCTEXT("DebugActionsMenuDissolveSolidifyHeader", "Dissolve & Solidify Actions"));
		DissolveSolidifyActionsSession.AddMenuEntry(FJointEditorCommands::Get().DissolveSubNodesIntoParentNode);
		DissolveSolidifyActionsSession.AddMenuEntry(FJointEditorCommands::Get().DissolveExactSubNodeIntoParentNode);
		DissolveSolidifyActionsSession.AddMenuEntry(FJointEditorCommands::Get().DissolveOnlySubNodesIntoParentNode);
		DissolveSolidifyActionsSession.AddMenuEntry(FJointEditorCommands::Get().SolidifySubNodesFromParentNode);
	}

	Super::GetContextMenuActions(Menu, Context);
}

FName UJointEdGraphSchema::GetParentContextMenuName() const
{
	//By default, there is no parent context menu.
	return NAME_None;
}

void UJointEdGraphSchema::GetGraphDisplayInformation(const UEdGraph& Graph, FGraphDisplayInfo& DisplayInfo) const
{
	if (const UJointEdGraph* CastedGraph = Cast<const UJointEdGraph>(&Graph))
	{
		DisplayInfo.PlainName = FText::FromString( Graph.GetName() );
		DisplayInfo.DisplayName = DisplayInfo.PlainName;
		DisplayInfo.Tooltip = CastedGraph->IsRootGraph() ? LOCTEXT("JointRootGraphTooltip", "The root graph of the Joint Manager") : LOCTEXT("JointSubGraphTooltip", "A sub graph, contained within a composite node.");
	}
}

void UJointEdGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	//check if we have the root graph.

	UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(&Graph);

	if (CastedGraph && CastedGraph->GetRootGraph() && CastedGraph->GetRootGraph() == CastedGraph)
	{
		FGraphNodeCreator<UJointEdGraphNode_Manager> NodeCreator(Graph);

		UJointEdGraphNode_Manager* ManagerNode = NodeCreator.CreateNode();

		NodeCreator.Finalize();

		SetNodeMetaData(ManagerNode, FNodeMetadata::DefaultGraphNode);

		ManagerNode->NodeInstance = CastedGraph->GetJointManager();
	}
}

const FPinConnectionResponse UJointEdGraphSchema::CanCreateConnection(
	const UEdGraphPin* A, const UEdGraphPin* B) const
{
	return A == nullptr || A->IsPendingKill() || B == nullptr || B->IsPendingKill() ? FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW,INVTEXT("Unknown Reason"))
		: A == B ? FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW,INVTEXT("Cannot make a connection with itself"))
		: !A->GetOwningNode() || !B->GetOwningNode() ? FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW,INVTEXT("Cannot make a connection with a pin that doesn't belong to a valid node"))
		: A->GetOwningNode() && B->GetOwningNode() && !A->GetOwningNode()->GetGraph() || !B->GetOwningNode()->GetGraph() ? FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW,INVTEXT("Cannot make a connection with a node that doesn't belong to a valid graph"))
		: A->GetOwningNode() && B->GetOwningNode() && A->GetOwningNode()->GetGraph() != B->GetOwningNode()->GetGraph() ? FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW,INVTEXT("Cannot make a connection between nodes in different graphs"))
		: A->Direction == EEdGraphPinDirection::EGPD_Output && B->Direction == EEdGraphPinDirection::EGPD_Output ? FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW,INVTEXT("Cannot make a connection between output nodes"))
		: A->Direction == EEdGraphPinDirection::EGPD_Input && B->Direction == EEdGraphPinDirection::EGPD_Input ? FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW,INVTEXT("Cannot make a connection between input nodes"))
		: A->Direction == EEdGraphPinDirection::EGPD_Output && B->Direction == EEdGraphPinDirection::EGPD_Input ? FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_BREAK_OTHERS_A,INVTEXT("Make a connection between those node"))
		: A->Direction == EEdGraphPinDirection::EGPD_Input && B->Direction == EEdGraphPinDirection::EGPD_Output ? FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_BREAK_OTHERS_B,INVTEXT("Make a connection between those node"))
		: FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW,INVTEXT("Unknown reason"));
}


const FPinConnectionResponse UJointEdGraphSchema::CanMergeNodes(const UEdGraphNode* NodeA,
                                                                const UEdGraphNode* NodeB) const
{
	if (GetWorld() && GetWorld()->IsGameWorld())
		return FPinConnectionResponse(
			CONNECT_RESPONSE_DISALLOW, TEXT("Can not edit on PIE mode."));

	if (NodeA == nullptr || !NodeA->IsValidLowLevel() || NodeB == nullptr || !NodeB->IsValidLowLevel())
		return
			FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Unknown Reason"));

	if (NodeA == NodeB) return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Both are the same node"));


	const UJointEdGraphNode* CastedANode = Cast<UJointEdGraphNode>(NodeA);
	const UJointEdGraphNode* CastedBNode = Cast<UJointEdGraphNode>(NodeB);

	if (CastedANode == CastedBNode)
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW,
		                              TEXT("Both node are same."));

	TArray<UJointEdGraphNode*> SubNodes = CastedANode->GetAllSubNodesInHierarchy();

	if (SubNodes.Contains(CastedBNode))
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW,
		                              TEXT(
			                              "Can not attach the parent node to its own child sub node."));

	if (!(CastedANode && CastedBNode))
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW,
		                              TEXT(
			                              "Both node must be overrided from UJointEdGraphNode. Revert the action."));

	if (!CastedBNode->CanHaveSubNode())
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW,
		                              TEXT("This node can not have any sub node."));

	FPinConnectionResponse Response;
	CastedBNode->CheckCanAddSubNode(CastedANode,Response, false);
	
	//Disallow it if it has been denied on the graph node side.
	if (Response.Response == ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW)
	{
		FString NewResponseText;

		if (Response.Response == ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW)
			NewResponseText += Response.Message.ToString();
		
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, FText::FromString(NewResponseText));
	}


	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT("Attach a subnode on this node"));
}

FConnectionDrawingPolicy* UJointEdGraphSchema::CreateConnectionDrawingPolicy(
	int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect,
	class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
	return new FJointGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

void UJointEdGraphSchema::OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2D& GraphPosition) const
{
	const FScopedTransaction Transaction(LOCTEXT("CreateRerouteNodeOnWire", "Create Reroute Node"));

	//@TODO: This constant is duplicated from inside of SGraphNodeKnot
	const FVector2D NodeSpacerSize(42.0f, 24.0f);
	const FVector2D KnotTopLeft = GraphPosition - (NodeSpacerSize * 0.5f);

	// Create a new knot
	UEdGraph* ParentGraph = PinA->GetOwningNode()->GetGraph();
	UJointEdGraphNode_Reroute* NewReroute = FJointSchemaAction_NewNode::SpawnNode<UJointEdGraphNode_Reroute>(ParentGraph, nullptr, KnotTopLeft);
	NewReroute->UpdatePins();

	// Move the connections across (only notifying the knot, as the other two didn't really change)
	PinA->BreakLinkTo(PinB);
	PinA->MakeLinkTo((PinA->Direction == EGPD_Output) ? NewReroute->GetPinAt(0) : NewReroute->GetPinAt(1));
	PinB->MakeLinkTo((PinB->Direction == EGPD_Output) ? NewReroute->GetPinAt(0) : NewReroute->GetPinAt(1));
	//NewReroute->PropagatePinType();
}


#undef LOCTEXT_NAMESPACE
