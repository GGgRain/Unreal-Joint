//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointEdGraphSchema.h"

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
#include "ScopedTransaction.h"

#include "ToolMenu.h"
#include "ToolMenuSection.h"

#include "Framework/Commands/GenericCommands.h"
#include "Modules/ModuleManager.h"
#include "Node/JointFragment.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "JointEdGraphSchema"

UJointEdGraphSchema::UJointEdGraphSchema(const class FObjectInitializer&)
{
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

    TArray<FGraphNodeClassData> NodeClasses;

    FJointEdUtils::GetNodeSubClasses(UJointNodeBase::StaticClass(), NodeClasses);

    for (FGraphNodeClassData& NodeClass : NodeClasses)
    {
        if (NodeClass.GetClass() == nullptr) continue;

        if (NodeClass.GetClass()->IsChildOf(UJointFragment::StaticClass())) continue;

        //It will be hidden when the class is deprecated.
        if (NodeClass.IsAbstract() || NodeClass.IsDeprecated() || NodeClass.GetClass()->HasAnyClassFlags(
            CLASS_Deprecated | CLASS_Hidden))
            continue;

        if (UJointEditorSettings::Get() && UJointEditorSettings::Get()->NodeClassesToHide.
            Contains(NodeClass.GetClass())) continue;

        //Todo : Gotta change it to use context menu if there is multiple options.
        const UClass* EdGraphNodeClass = FJointEdUtils::FindEdClassForNode(NodeClass);

        if (EdGraphNodeClass == nullptr) EdGraphNodeClass = UJointEdGraphNode::StaticClass();

        const FText NodeTypeName = FText::FromString(FName::NameToDisplayString(NodeClass.ToString(), false));
        const FText NodeCategory = (!NodeClass.GetCategory().IsEmpty()) ? NodeClass.GetCategory() : CustomCategory;
        const FText NodeTooltip = NodeClass.GetClass()->GetToolTipText();

        UJointEdGraphNode* OpNode = NewObject<UJointEdGraphNode>(Graph, EdGraphNodeClass);
        OpNode->ClassData = NodeClass;

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

    TArray<FGraphNodeClassData> FragmentClasses;
    FJointEdUtils::GetNodeSubClasses(UJointFragment::StaticClass(), FragmentClasses);

    for (FGraphNodeClassData& NodeClass : FragmentClasses)
    {
        //Make sure we are going to display this action or not.
        if (NodeClass.GetClass() == nullptr) continue;

        //It will be hidden when the class is deprecated.
        if (NodeClass.IsAbstract() || NodeClass.IsDeprecated() || NodeClass.GetClass()->HasAnyClassFlags(
            CLASS_Deprecated | CLASS_Hidden))
            continue;

        if (UJointEditorSettings::Get() && UJointEditorSettings::Get()->NodeClassesToHide.
            Contains(NodeClass.GetClass())) continue;

        //Grab the Ed node class
        TSubclassOf<UJointEdGraphNode> TargetFragmentEdClass = FJointEdUtils::FindEdClassForNode(NodeClass);

        if (TargetFragmentEdClass == nullptr) TargetFragmentEdClass = UJointEdGraphNode_Fragment::StaticClass();


        const FText NodeTypeName = FText::FromString(FName::NameToDisplayString(NodeClass.ToString(), false));
        const FText NodeCategory = !NodeClass.GetCategory().IsEmpty() ? NodeClass.GetCategory() : FText::GetEmpty();
        const FText NodeTooltip = NodeClass.GetClass()->GetToolTipText();

        UJointEdGraphNode* OpNode = NewObject<UJointEdGraphNode>(Graph, TargetFragmentEdClass);
        OpNode->ClassData = NodeClass;

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


FLinearColor UJointEdGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
    return FLinearColor::White;
}

bool UJointEdGraphSchema::ShouldAlwaysPurgeOnModification() const
{
    return Super::ShouldAlwaysPurgeOnModification();
}

void UJointEdGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
    Super::BreakSinglePinLink(SourcePin, TargetPin);
}

void UJointEdGraphSchema::SetNodePosition(UEdGraphNode* Node, const FVector2D& Position) const
{
    Super::SetNodePosition(Node, Position);
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

    FToolMenuSection& NodeActionSection = Menu->AddSection("NodeActionsMenu",
                                                           LOCTEXT("NodeActionsMenuHeader", "Node Actions"));

    if (!Context->bIsDebugging)
    {
        // Node contextual actions
        NodeActionSection.AddMenuEntry(FGenericCommands::Get().Delete);
        NodeActionSection.AddMenuEntry(FGenericCommands::Get().Cut);
        NodeActionSection.AddMenuEntry(FGenericCommands::Get().Copy);
        NodeActionSection.AddMenuEntry(FGenericCommands::Get().Duplicate);

        NodeActionSection.AddMenuEntry(FGraphEditorCommands::Get().ReconstructNodes);

        if (const UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(Context->Node); CastedNode && !CastedNode->
            IsSubNode()) NodeActionSection.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);

        if (Context->Node->bCanRenameNode) NodeActionSection.AddMenuEntry(FGenericCommands::Get().Rename);
    }

    FToolMenuSection& DebugActionSection = Menu->AddSection("DebugActionsMenu",
                                                            LOCTEXT("DebugActionsMenuHeader", "Debug Actions"));

    if (!Context->bIsDebugging)
    {
        DebugActionSection.AddMenuEntry(FGraphEditorCommands::Get().AddBreakpoint);
        DebugActionSection.AddMenuEntry(FGraphEditorCommands::Get().RemoveBreakpoint);
        DebugActionSection.AddMenuEntry(FGraphEditorCommands::Get().EnableBreakpoint);
        DebugActionSection.AddMenuEntry(FGraphEditorCommands::Get().DisableBreakpoint);
        DebugActionSection.AddMenuEntry(FGraphEditorCommands::Get().ToggleBreakpoint);
    }

    Super::GetContextMenuActions(Menu, Context);
}

FName UJointEdGraphSchema::GetContextMenuName() const
{
    return GetContextMenuName(GetClass());
}

FName UJointEdGraphSchema::GetContextMenuName(UClass* InClass) const
{
#if WITH_EDITOR
    return FName(*(FString(TEXT("GraphEditor.GraphContextMenu.")) + InClass->GetName()));
#else
    return NAME_None;
#endif
}

FName UJointEdGraphSchema::GetParentContextMenuName() const
{
#if WITH_EDITOR
    if (GetClass() != UEdGraphSchema::StaticClass())
    {
        if (UClass* SuperClass = GetClass()->GetSuperClass())
        {
            return GetContextMenuName(SuperClass);
        }
    }
#endif
    return NAME_None;
}

void UJointEdGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
    FGraphNodeCreator<UJointEdGraphNode_Manager> NodeCreator(Graph);

    UJointEdGraphNode_Manager* ManagerNode = NodeCreator.CreateNode();

    NodeCreator.Finalize();

    SetNodeMetaData(ManagerNode, FNodeMetadata::DefaultGraphNode);

    if (const UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(&Graph))
        ManagerNode->NodeInstance = CastedGraph->
            GetJointManager();
}

const FPinConnectionResponse UJointEdGraphSchema::CanCreateConnection(
    const UEdGraphPin* A, const UEdGraphPin* B) const
{
    return A == nullptr || A->IsPendingKill() || B == nullptr || B->IsPendingKill()
               ? FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW,
                                        INVTEXT("Unknown Reason"))
               : A == B
               ? FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW,
                                        INVTEXT("Cannot make a connection with itself"))
               : A->Direction == EEdGraphPinDirection::EGPD_Output && B->Direction == EEdGraphPinDirection::EGPD_Output
               ? FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW,
                                        INVTEXT("Cannot make a connection between output nodes"))
               : A->Direction == EEdGraphPinDirection::EGPD_Input && B->Direction == EEdGraphPinDirection::EGPD_Input
               ? FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW,
                                        INVTEXT("Cannot make a connection between input nodes"))
               : A->Direction == EEdGraphPinDirection::EGPD_Output && B->Direction == EEdGraphPinDirection::EGPD_Input
               ? FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_BREAK_OTHERS_A,
                                        INVTEXT("Make a connection between those node"))
               : A->Direction == EEdGraphPinDirection::EGPD_Input && B->Direction == EEdGraphPinDirection::EGPD_Output
               ? FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_BREAK_OTHERS_B,
                                        INVTEXT("Make a connection between those node"))
               : FPinConnectionResponse(ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW,
                                        INVTEXT("Unknown reason"));
}


const FPinConnectionResponse UJointEdGraphSchema::CanMergeNodes(const UEdGraphNode* NodeA,
                                                                const UEdGraphNode* NodeB) const
{
    if (GetWorld() && GetWorld()->IsGameWorld()) return FPinConnectionResponse(
        CONNECT_RESPONSE_DISALLOW, TEXT("Can not edit on PIE mode."));

    if (NodeA == nullptr || !NodeA->IsValidLowLevel() || NodeB == nullptr || !NodeB->IsValidLowLevel()) return
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

    const FPinConnectionResponse& AResponse = CastedANode->CanAttachThisAtParentNode(CastedBNode);

    const FPinConnectionResponse& BResponse = CastedBNode->CanAttachSubNodeOnThis(CastedANode);

    //Disallow it if it has been denied on the graph node side.
    if (AResponse.Response == ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW || BResponse.Response ==
        ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW)
    {
        FString NewResponseText;

        if (AResponse.Response == ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW)
            NewResponseText += AResponse.
                Message.ToString();
        if (BResponse.Response == ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW)
            NewResponseText += BResponse.
                Message.ToString();

        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, FText::FromString(NewResponseText));
    }


    return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT("Attach a subnode on this node"));
}

FConnectionDrawingPolicy* UJointEdGraphSchema::CreateConnectionDrawingPolicy(
    int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect,
    class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
    return new FJointGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect,
                                                  InDrawElements, InGraphObj);
}


#undef LOCTEXT_NAMESPACE
