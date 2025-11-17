//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "Node/JointEdGraphNode_Tunnel.h"

#include "JointEdGraph.h"
#include "JointEditorStyle.h"
#include "JointFunctionLibrary.h"
#include "K2Node_Tunnel.h"
#include "Components/VerticalBox.h"
#include "GraphNode/SJointGraphNodeBase.h"

#include "EdGraph/EdGraph.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "UJointEdGraphNode_Tunnel"


UJointEdGraphNode_Tunnel::UJointEdGraphNode_Tunnel()
{
	bIsNodeResizeable = false;

	bCanRenameNode = false;
}

void UJointEdGraphNode_Tunnel::LockSynchronizing()
{
	bIsSynchronizingPins = true;
}

void UJointEdGraphNode_Tunnel::UnlockSynchronizing()
{
	bIsSynchronizingPins = false;
}

void UJointEdGraphNode_Tunnel::SynchronizeTunnelNodePins()
{
	if (bIsSynchronizingPins) return;

	LockSynchronizing();
	
	TArray<FJointEdPinData> InputPinData;
	TArray<FJointEdPinData> OutputPinData;

	for (const FJointEdPinData& PinDataElem : PinData)
	{
		if (PinDataElem.Direction == EEdGraphPinDirection::EGPD_Output)
		{
			FJointEdPinData ModifiedPinDataElem = PinDataElem;
			ModifiedPinDataElem.Direction = EEdGraphPinDirection::EGPD_Input;
			OutputPinData.Add(ModifiedPinDataElem);
		}else
		{
			FJointEdPinData ModifiedPinDataElem = PinDataElem;
			ModifiedPinDataElem.Direction = EEdGraphPinDirection::EGPD_Output;
			InputPinData.Add(ModifiedPinDataElem);
		}
	}

	//a function to implement the pins to the input sink node and output source node.
	auto ReimplementPin = [](TObjectPtr<UJointEdGraphNode_Tunnel> CompositeNode, EEdGraphPinDirection SelfTunnelNodeDirection, TArray<FJointEdPinData>& InPinData)
	{
		//since this node is a tunnel of the composite node, we have to only affect one side of the composite node (input or output)

		//get the pins...
		TArray<FJointEdPinData>& TotalPins = CompositeNode->GetPinDataFromThis();
		
		//query the pins that are input/ output pins according to the direction of this tunnel node. - while removing them from the original total pins array of the output source node.
		TArray<FJointEdPinData> QueriedPins;
		
		for (int32 i = TotalPins.Num() - 1; i >= 0; i--)
		{
			if (TotalPins[i].Direction == SelfTunnelNodeDirection)
			{
				QueriedPins.Add(TotalPins[i]);
				TotalPins.RemoveAt(i);
			}
		}

		//then re-implement the pins with the new data.
		QueriedPins = UJointFunctionLibrary::ImplementPins(QueriedPins, InPinData);

		//finally add them back to the total pins array.
		TotalPins.Append(QueriedPins);

		//update the pins of the CompositeNode
		//We don't want to trigger ReallocatePins() of the CompositeNode - except that, call every shits on the UpdatePins() function manually.
		CompositeNode->RemoveUnnecessaryPins();
		CompositeNode->UpdatePinsFromPinData();
		CompositeNode->ImplementPinDataPins();
		CompositeNode->RequestPopulationOfPinWidgets();
	};
	
	if (InputSinkNode) ReimplementPin(InputSinkNode, EEdGraphPinDirection::EGPD_Output, InputPinData);
	if (OutputSourceNode) ReimplementPin(OutputSourceNode, EEdGraphPinDirection::EGPD_Input, OutputPinData);

	UnlockSynchronizing();
}


bool UJointEdGraphNode_Tunnel::CanReplaceNodeClass()
{
	return false;
}

bool UJointEdGraphNode_Tunnel::CanReplaceEditorNodeClass()
{
	return false;
}

bool UJointEdGraphNode_Tunnel::CanUserDeleteNode() const
{
	return false;
}


void UJointEdGraphNode_Tunnel::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UpdatePins();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UJointEdGraphNode_Tunnel::AllocateDefaultPins()
{
	ReallocatePins();

	RequestPopulationOfPinWidgets();
}

void UJointEdGraphNode_Tunnel::ReallocatePins()
{
	SynchronizeTunnelNodePins();
}


FLinearColor UJointEdGraphNode_Tunnel::GetNodeTitleColor() const
{
	return FColor::Cyan;
}

FText UJointEdGraphNode_Tunnel::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FText Title = LOCTEXT( "TunnelNodeTitle", "Sub Graph Tunnel Node - {0}, {1}"); // {0} - Graph Name, {1} - Direction

	return FText::Format(Title, GetGraph() ? FText::FromString(GetGraph()->GetName()) : LOCTEXT("NoGraph", "No Graph"), bCanHaveInputs ? LOCTEXT("Input", "Input") : LOCTEXT("Output", "Output"));
	
}

bool UJointEdGraphNode_Tunnel::GetShouldHideNameBox() const
{
	return true;
}


FVector2D UJointEdGraphNode_Tunnel::GetNodeMinimumSize() const
{
	return FVector2D(20, 20);
}

bool UJointEdGraphNode_Tunnel::CanDuplicateNode() const
{
	return false; //Take care the occasion when there is another output node with the same Guid.
}

void UJointEdGraphNode_Tunnel::ReconstructNode()
{
	UpdateNodeInstance();

	UpdatePins();

	NodeConnectionListChanged();
}

void UJointEdGraphNode_Tunnel::PostPlacedNewNode()
{
	UpdatePins();

	Super::PostPlacedNewNode();
}

bool UJointEdGraphNode_Tunnel::CanHaveSubNode() const
{
	return false;
}


void UJointEdGraphNode_Tunnel::NodeConnectionListChanged()
{
	if (bIsUpdatingNodeConnection) return;

	bIsUpdatingNodeConnection = true;
	
	if (InputSinkNode)	InputSinkNode->NodeConnectionListChanged();
	if (OutputSourceNode) OutputSourceNode->NodeConnectionListChanged();

	//Iterate the pins and notify the connected nodes to update their connection state.
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin == nullptr) continue;

		for (UEdGraphPin* LinkedTo : Pin->LinkedTo)
		{
			if (UJointEdGraphNode* LinkedPinOwner = CastPinOwnerToJointEdGraphNode(LinkedTo))
			{
				LinkedPinOwner->NodeConnectionListChanged();
			}
		}
	}

	bIsUpdatingNodeConnection = false;

}

void UJointEdGraphNode_Tunnel::AllocateReferringNodeInstancesOnConnection(TArray<TObjectPtr<UJointNodeBase>>& Nodes, UEdGraphPin* SourcePin)
{
	if (SourcePin == nullptr) return;

	// 1. if we have SourcePin originated from this node, we will refer it to the InputSinkNode or OutputSourceNode.
	// 2. if we have SourcePin originated from InputSinkNode / OutputSourceNode, we will feed Nodes var with this node's connection state.
	
	UEdGraphNode* Owner = SourcePin->GetOwningNode();


	//a function to implement the pins to the input sink node and output source node.

	auto AllocateConnectionFromSelf = [this](TObjectPtr<UJointEdGraphNode_Tunnel> InSourceNode, UEdGraphPin* InSourcePin, TArray<TObjectPtr<UJointNodeBase>>& InNodes)
	{
		if (InSourceNode == nullptr) return;
		
		if (UEdGraphPin* FoundPin = InSourceNode->FindPin(InSourcePin->PinName))
		{
			InSourceNode->AllocateReferringNodeInstancesOnConnection(InNodes, InSourcePin);
		}
	};
	
	auto AllocateConnection = [this](EEdGraphPinDirection SelfTunnelNodeDirection, UEdGraphPin* InSourcePin, TArray<TObjectPtr<UJointNodeBase>>& InNodes)
	{
		InNodes.Empty();
		
		if (UEdGraphPin* FoundPin = FindPin(InSourcePin->PinName, SelfTunnelNodeDirection))
		{
			for (UEdGraphPin* LinkedTo : FoundPin->LinkedTo)
			{
				if (UJointEdGraphNode* LinkedPinOwner = CastPinOwnerToJointEdGraphNode(LinkedTo))
				{
					LinkedPinOwner->AllocateReferringNodeInstancesOnConnection(InNodes, LinkedTo);
				}
			}
		}
	};
	
	if (Owner == this )
	{
		// 1
		if (SourcePin->Direction == EEdGraphPinDirection::EGPD_Input)
		{
			AllocateConnectionFromSelf(InputSinkNode, SourcePin, Nodes);
		}else
		{
			AllocateConnectionFromSelf(OutputSourceNode, SourcePin, Nodes);
		}	
	}else if (Owner == InputSinkNode)
	{
		//2 - 1
		AllocateConnection(EEdGraphPinDirection::EGPD_Input, SourcePin, Nodes);
	}else if (Owner == OutputSourceNode)
	{
		//2 - 2
		AllocateConnection(EEdGraphPinDirection::EGPD_Output, SourcePin, Nodes);
	}
}


void UJointEdGraphNode_Tunnel::UpdateNodeInstance()
{
}

void UJointEdGraphNode_Tunnel::DestroyNode()
{
	Super::DestroyNode();
}

void UJointEdGraphNode_Tunnel::ModifyGraphNodeSlate()
{
	if (!GetGraphNodeSlate().IsValid()) return;

	const TSharedPtr<SJointGraphNodeBase> NodeSlate = GetGraphNodeSlate().Pin();

	if (bCanHaveInputs)
	{
		NodeSlate->CenterContentBox->AddSlot()
				 .Padding(FJointEditorStyle::Margin_Normal)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(FJointEditorStyle::Margin_Normal)
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(16)
				.HeightOverride(16)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(
						"Icons.SortDown"))
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(FJointEditorStyle::Margin_Normal)
			.FillWidth(1)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NewOutputConnectorButtonText", "Out"))
			]
		];
	}else if (bCanHaveOutputs)
	{
		NodeSlate->CenterContentBox->AddSlot()
				 .Padding(FJointEditorStyle::Margin_Normal)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(FJointEditorStyle::Margin_Normal)
			.FillWidth(1)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NewInputConnectorButtonText", "In"))
			]
			+ SHorizontalBox::Slot()
						.Padding(FJointEditorStyle::Margin_Normal)
						.AutoWidth()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SBox)
							.WidthOverride(16)
							.HeightOverride(16)
							[
								SNew(SImage)
								.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(
									"Icons.SortUp"))
							]
						]
		];
	}
}

bool UJointEdGraphNode_Tunnel::CanHaveBreakpoint() const
{
	return false;
}

void UJointEdGraphNode_Tunnel::GetNodeContextMenuActions(UToolMenu* Menu,
                                                         UGraphNodeContextMenuContext* Context) const
{
	return;
}

UJointEdGraphNode_Tunnel* UJointEdGraphNode_Tunnel::GetInputSink() const
{
	return InputSinkNode;
}

UJointEdGraphNode_Tunnel* UJointEdGraphNode_Tunnel::GetOutputSource() const
{
	return OutputSourceNode;
}

#undef LOCTEXT_NAMESPACE
