//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "Node/JointEdGraphNode_Connector.h"

#include "JointEdGraphSchemaActions.h"
#include "JointManager.h"
#include "IMessageLogListing.h"
#include "JointAdvancedWidgets.h"
#include "JointEditorStyle.h"
#include "JointEdUtils.h"
#include "JointFunctionLibrary.h"
#include "GraphNode/SJointGraphNodeBase.h"

#include "EdGraph/EdGraph.h"
#include "Misc/UObjectToken.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "UJointEdGraphNode_Connector"


UJointEdGraphNode_Connector::UJointEdGraphNode_Connector()
{
	bIsNodeResizeable = false;

	bCanRenameNode = false;

	Direction = EEdGraphPinDirection::EGPD_Output;

	ConnectorName = LOCTEXT("ConnectorDefaultName", "New Connector");
	
}


bool UJointEdGraphNode_Connector::GetShouldHideNameBox() const
{
	return true;
}

bool UJointEdGraphNode_Connector::CanReplaceNodeClass()
{
	return false;
}

bool UJointEdGraphNode_Connector::CanReplaceEditorNodeClass()
{
	return false;
}

FText UJointEdGraphNode_Connector::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FText Title = LOCTEXT("ConnectorNodeTitle", "Connector - {0}, {1}"); // {0} - Graph Name, {1} - Direction

	const UJointEdGraphNode_Connector* OutputConnector = GetPairOutputConnector();

	FText TargetConnectorName = Direction == EEdGraphPinDirection::EGPD_Output ? ConnectorName : OutputConnector ? OutputConnector->ConnectorName : LOCTEXT("NoPairedOutputConnector", "No Paired Output Connector");
	
	return FText::Format(Title, TargetConnectorName, Direction == EEdGraphPinDirection::EGPD_Input ? LOCTEXT("Input", "Input") : LOCTEXT("Output", "Output"));
}

void UJointEdGraphNode_Connector::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UpdatePins();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UJointEdGraphNode_Connector::AllocateDefaultPins()
{
	ReallocatePins();

	RequestPopulationOfPinWidgets();
}

void UJointEdGraphNode_Connector::ReallocatePins()
{
	if (Direction == EEdGraphPinDirection::EGPD_Input)
	{
		TArray<FJointEdPinData> NewPinData;
		NewPinData.Add(FJointEdPinData("In", EEdGraphPinDirection::EGPD_Input));
		PinData = UJointFunctionLibrary::ImplementPins(PinData, NewPinData);
	}
	else
	{
		TArray<FJointEdPinData> NewPinData;
		NewPinData.Add(FJointEdPinData("Out", EEdGraphPinDirection::EGPD_Output));
		PinData = UJointFunctionLibrary::ImplementPins(PinData, NewPinData);
	}
}


FLinearColor UJointEdGraphNode_Connector::GetNodeTitleColor() const
{
	return FColor::Cyan;
}

FVector2D UJointEdGraphNode_Connector::GetNodeMinimumSize() const
{
	return FVector2D(20, 20);
}

bool UJointEdGraphNode_Connector::CanDuplicateNode() const
{
	return true; //Take care the occasion when there is another output node with the same Guid.
}

void UJointEdGraphNode_Connector::ReconstructNode()
{
	UpdateNodeInstance();

	UpdatePins();

	NodeConnectionListChanged();
}

void UJointEdGraphNode_Connector::PostPlacedNewNode()
{
	if (Direction == EEdGraphPinDirection::EGPD_Output) ConnectorGuid = FGuid::NewGuid();

	UpdatePins();

	Super::PostPlacedNewNode();
}

bool UJointEdGraphNode_Connector::CanHaveSubNode() const
{
	return false;
}

void UJointEdGraphNode_Connector::NotifyConnectionChangedToConnectedNodes()
{
	if (Direction == EEdGraphPinDirection::EGPD_Input)
	{
		for (UEdGraphPin* Pin : Pins)
		{
			if (Pin == nullptr) continue;

			for (const UEdGraphPin* LinkedTo : Pin->LinkedTo)
			{
				if (LinkedTo == nullptr) continue;

				if (LinkedTo->GetOwningNode() == nullptr) continue;

				if (LinkedTo->GetOwningNode() == GetPairOutputConnector()) continue;

				LinkedTo->GetOwningNode()->NodeConnectionListChanged();
			}
		}
	}
	else
	{
		TArray<UJointEdGraphNode_Connector*> Connectors = GetPairInputConnector();

		for (UEdGraphPin* Pin : Pins)
		{
			if (Pin == nullptr) continue;

			for (const UEdGraphPin* LinkedTo : Pin->LinkedTo)
			{
				if (LinkedTo == nullptr) continue;

				if (LinkedTo->GetOwningNode() == nullptr) continue;

				if (Connectors.Contains(LinkedTo->GetOwningNode())) continue;

				LinkedTo->GetOwningNode()->NodeConnectionListChanged();
			}
		}
	}
}

void UJointEdGraphNode_Connector::NodeConnectionListChanged()
{
	if (Direction == EEdGraphPinDirection::EGPD_Output)
	{
		ConnectedNodes.Empty();

		for (UEdGraphPin* Pin : Pins)
		{
			if (Pin == nullptr) continue;

			for (UEdGraphPin* LinkedTo : Pin->LinkedTo)
			{
				if (UJointEdGraphNode* LinkedToGraphNode = CastPinOwnerToJointEdGraphNode(LinkedTo))
				{
					LinkedToGraphNode->AllocateReferringNodeInstancesOnConnection(ConnectedNodes, LinkedTo);
				}
			}
		}
	}

	CachedPairOutputConnector = GetPairOutputConnector();

	if (Direction == EEdGraphPinDirection::EGPD_Input)
	{
		if (CachedPairOutputConnector) CachedPairOutputConnector->NotifyConnectionChangedToConnectedNodes();
	}
	else
	{
		for (UJointEdGraphNode_Connector* PairInputConnector : GetPairInputConnector())
		{
			if (PairInputConnector) PairInputConnector->NotifyConnectionChangedToConnectedNodes();
		}
	}
}

void UJointEdGraphNode_Connector::AllocateReferringNodeInstancesOnConnection(TArray<TObjectPtr<UJointNodeBase>>& Nodes, UEdGraphPin* SourcePin)
{
	CachedPairOutputConnector = GetPairOutputConnector();
	
	if (CachedPairOutputConnector) Nodes = CachedPairOutputConnector->ConnectedNodes;
}


void UJointEdGraphNode_Connector::UpdateNodeInstance()
{
}

void UJointEdGraphNode_Connector::UpdateNodeInstanceOuterToJointManager() const
{
	//bubble up (because we don't have own node instance)
}

void UJointEdGraphNode_Connector::DestroyNode()
{
	Super::DestroyNode();
}

void UJointEdGraphNode_Connector::PostPasteNode()
{
	if (Direction == EEdGraphPinDirection::EGPD_Output)
	{
		ConnectorGuid = FGuid::NewGuid(); //Reset Guid.
		ConnectorName = FText::FromString(ConnectorName.ToString() + "_New");
	}

	Super::PostPasteNode();
}

void UJointEdGraphNode_Connector::OnAddInputNodeButtonPressed()
{
	const FText Category = LOCTEXT("ConnectorCategory", "Connector");
	const FText MenuDesc = LOCTEXT("ConnectorMenuDesc", "Add Connector.....");
	const FText ToolTip = LOCTEXT("ConnectorToolTip",
	                              "Add a connector node on the graph that helps you editing and organizing the other nodes.");

	const TSharedPtr<FJointSchemaAction_AddConnector> AddConnectorAction = MakeShared<
		FJointSchemaAction_AddConnector>(Category, MenuDesc, ToolTip);

	UEdGraphNode* OutNode = AddConnectorAction->PerformAction(GetGraph(), nullptr,
	                                                          FVector2D(NodePosX - 200 + FMath::RandRange(-30, 30),
	                                                                    NodePosY + FMath::RandRange(-30, 30)),
	                                                          true);

	if (OutNode)
	{
		if (UJointEdGraphNode_Connector* CastedNode = Cast<UJointEdGraphNode_Connector>(OutNode))
		{
			CastedNode->Direction = EEdGraphPinDirection::EGPD_Input;

			CastedNode->ConnectorGuid = this->ConnectorGuid;
			
			CastedNode->UpdatePins();
		}
	}
}


UJointEdGraphNode_Connector* UJointEdGraphNode_Connector::GetPairOutputConnector() const
{
	TArray<UJointEdGraphNode_Connector*> Connectors;

	TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(GetJointManager());

	for (UJointEdGraph* Graph : Graphs)
	{
		for (UEdGraphNode* EdGraphNode : Graph->Nodes)
		{
			if (EdGraphNode == nullptr) continue;

			if (UJointEdGraphNode_Connector* CastedNode = Cast<UJointEdGraphNode_Connector>(EdGraphNode))
			{
				if (CastedNode->ConnectorGuid == this->ConnectorGuid && CastedNode->Direction == EEdGraphPinDirection::EGPD_Output)
				{
					return CastedNode;
				}
			}
		}
	}

	return nullptr;
}

TArray<UJointEdGraphNode_Connector*> UJointEdGraphNode_Connector::GetPairInputConnector()
{
	TArray<UJointEdGraphNode_Connector*> Connectors;
	
	TArray<UJointEdGraph*> Graphs = UJointEdGraph::GetAllGraphsFrom(GetJointManager());

	for (UJointEdGraph* Graph : Graphs)
	{
		for (UEdGraphNode* EdGraphNode : Graph->Nodes)
		{
			if (EdGraphNode == nullptr) continue;

			if (UJointEdGraphNode_Connector* CastedNode = Cast<UJointEdGraphNode_Connector>(EdGraphNode))
			{
				if (CastedNode->ConnectorGuid == this->ConnectorGuid && CastedNode->Direction == EEdGraphPinDirection::EGPD_Input)
				{
					Connectors.Add(CastedNode);
				}
			}
		}
	}

	return Connectors;
}

void UJointEdGraphNode_Connector::ModifyGraphNodeSlate()
{
	if (!GetGraphNodeSlate().IsValid()) return;

	CachedPairOutputConnector = GetPairOutputConnector();

	const TSharedPtr<SJointGraphNodeBase> NodeSlate = GetGraphNodeSlate().Pin();

	const TAttribute<FText> NodeText_Attr = TAttribute<FText>::CreateLambda([this]
		{
			if (Direction == EEdGraphPinDirection::EGPD_Input)
			{
				if (CachedPairOutputConnector)
				{
					return CachedPairOutputConnector->ConnectorName;
				}

				return FText::FromString(
					LOCTEXT("NoConnectorName", "No pair output connector!\nConnector ID: ").ToString() + ConnectorGuid.
					ToString());
			}

			return ConnectorName;
		});

	if (Direction == EEdGraphPinDirection::EGPD_Output)
	{
		NodeSlate->CenterContentBox->AddSlot()
			.Padding(FJointEditorStyle::Margin_Normal)
			[

				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
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
								"GraphEditor.MakeStruct_16x"))
						]
					]
					+ SHorizontalBox::Slot()
					.Padding(FJointEditorStyle::Margin_Normal)
					.FillWidth(1)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(NodeText_Attr)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SJointOutlineButton)
					.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
					.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.ContentPadding(FJointEditorStyle::Margin_Normal)
					.OutlinePadding(1.5)
					.OnPressed_UObject(this, &UJointEdGraphNode_Connector::OnAddInputNodeButtonPressed)
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
									"LevelEditor.OpenPlaceActors"))
							]
						]
						+ SHorizontalBox::Slot()
						.Padding(FJointEditorStyle::Margin_Normal)
						.FillWidth(1)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("NewInputConnectorButtonText", "Add New Input..."))
						]
					]
				]

			];
	}
	else
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
							"GraphEditor.MakeStruct_16x"))
					]
				]
				+ SHorizontalBox::Slot()
				.Padding(FJointEditorStyle::Margin_Normal)
				.FillWidth(1)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(NodeText_Attr)
				]

			];
	}
}

void UJointEdGraphNode_Connector::OnCompileNode()
{
	Super::OnCompileNode();
	
	if (Direction == EEdGraphPinDirection::EGPD_Input)
	{
		if (GetPairOutputConnector() == nullptr)
		{
			TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(EMessageSeverity::Info);
			TokenizedMessage->AddToken(FAssetNameToken::Create(GetJointManager() ? GetJointManager()->GetName() : "NONE"));
			TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(":")));
			TokenizedMessage->AddToken(FUObjectToken::Create(this));
			TokenizedMessage->AddToken(FTextToken::Create(LOCTEXT("Compile_NoOutputConnector","No paired output connector has been detected. This connector will not work as intended.")) );
			TokenizedMessage.Get().SetMessageLink(FUObjectToken::Create(this));
			
			CompileMessages.Add(TokenizedMessage);
		}
	}

}

bool UJointEdGraphNode_Connector::CanHaveBreakpoint() const
{
	return false;
}

void UJointEdGraphNode_Connector::GetNodeContextMenuActions(UToolMenu* Menu,
                                                               UGraphNodeContextMenuContext* Context) const
{
	return;
}

#undef LOCTEXT_NAMESPACE
