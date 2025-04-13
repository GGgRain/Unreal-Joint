//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "Node/JointEdGraphNode_Connector.h"

#include "JointEdGraphSchemaActions.h"
#include "JointManager.h"
#include "IMessageLogListing.h"
#include "JointAdvancedWidgets.h"
#include "JointEditorStyle.h"
#include "JointFunctionLibrary.h"
#include "GraphNode/SJointGraphNodeBase.h"

#include "EdGraph/EdGraph.h"
#include "Misc/UObjectToken.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "UJointEdGraphNode_Connector"


UJointEdGraphNode_Connector::UJointEdGraphNode_Connector()
{
	bUseFixedNodeSize = false;

	bCanRenameNode = false;

	Direction = EEdGraphPinDirection::EGPD_Output;

	ConnectorName = LOCTEXT("ConnectorDefaultName", "New Connector");
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

FText UJointEdGraphNode_Connector::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::GetEmpty();
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

				if (LinkedTo->GetOwningNode() == GetCachedPairOutputConnector()) continue;

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

			for (const UEdGraphPin* LinkedTo : Pin->LinkedTo)
			{
				if (LinkedTo == nullptr) continue;

				if (LinkedTo->GetOwningNode() == nullptr) continue;

				UEdGraphNode* ConnectedNode = LinkedTo->GetOwningNode();

				if (!ConnectedNode) continue;

				UJointEdGraphNode* CastedGraphNode = Cast<UJointEdGraphNode>(ConnectedNode);

				if (!CastedGraphNode) continue;

				CastedGraphNode->AllocateReferringNodeInstancesOnConnection(ConnectedNodes);
			}
		}
	}

	if (Direction == EEdGraphPinDirection::EGPD_Input)
	{
		if (GetCachedPairOutputConnector()) GetCachedPairOutputConnector()->NotifyConnectionChangedToConnectedNodes();
	}
	else
	{
		for (UJointEdGraphNode_Connector* PairInputConnector : GetPairInputConnector())
		{
			if (PairInputConnector) PairInputConnector->NotifyConnectionChangedToConnectedNodes();
		}
	}
}

void UJointEdGraphNode_Connector::AllocateReferringNodeInstancesOnConnection(TArray<UJointNodeBase*>& Nodes)
{
	if (GetCachedPairOutputConnector()) Nodes = GetCachedPairOutputConnector()->ConnectedNodes;
}


void UJointEdGraphNode_Connector::UpdateNodeInstance()
{
}

void UJointEdGraphNode_Connector::UpdateNodeInstanceOuter() const
{
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

			CastedNode->CachedPairOutputConnector = this;

			CastedNode->UpdatePins();
		}
	}
}

UJointEdGraphNode_Connector* UJointEdGraphNode_Connector::GetCachedPairOutputConnector()
{
	if (!CachedPairOutputConnector.IsValid())
	{
		CachesPairOutputConnector();
	}
	else if (CachedPairOutputConnector->ConnectorGuid == this->ConnectorGuid && CachedPairOutputConnector->Direction !=
		EEdGraphPinDirection::EGPD_Output)
	{
		CachesPairOutputConnector();
	}

	if (CachedPairOutputConnector.IsValid())
	{
		if (CachedPairOutputConnector->GetGraph())
		{
			if (CachedPairOutputConnector->GetGraph()->Nodes.Contains(CachedPairOutputConnector))
				return
					CachedPairOutputConnector.Get();
		}
	}

	return nullptr;
}

void UJointEdGraphNode_Connector::CachesPairOutputConnector()
{
	CachedPairOutputConnector = nullptr;

	if (GetGraph() == nullptr) return;

	for (UEdGraphNode* EdGraphNode : GetGraph()->Nodes)
	{
		if (EdGraphNode == nullptr) continue;

		if (UJointEdGraphNode_Connector* CastedNode = Cast<UJointEdGraphNode_Connector>(EdGraphNode))
		{
			if (CastedNode->ConnectorGuid == this->ConnectorGuid && CastedNode->Direction ==
				EEdGraphPinDirection::EGPD_Output)
			{
				CachedPairOutputConnector = CastedNode;

				return;
			}
		}
	}
}

TArray<UJointEdGraphNode_Connector*> UJointEdGraphNode_Connector::GetPairInputConnector()
{
	TArray<UJointEdGraphNode_Connector*> Connectors;

	if (GetGraph() == nullptr) return Connectors;

	for (UEdGraphNode* EdGraphNode : GetGraph()->Nodes)
	{
		if (EdGraphNode == nullptr) continue;

		if (UJointEdGraphNode_Connector* CastedNode = Cast<UJointEdGraphNode_Connector>(EdGraphNode))
		{
			if (CastedNode->ConnectorGuid == this->ConnectorGuid && CastedNode->Direction ==
				EEdGraphPinDirection::EGPD_Input)
			{
				Connectors.Add(CastedNode);
			}
		}
	}

	return Connectors;
}

void UJointEdGraphNode_Connector::ModifyGraphNodeSlate()
{
	if (!GetGraphNodeSlate().IsValid()) return;

	const TSharedPtr<SJointGraphNodeBase> NodeSlate = GetGraphNodeSlate().Pin();

	const TAttribute<FText> NodeText_Attr = TAttribute<FText>::CreateLambda([this]
		{
			if (Direction == EEdGraphPinDirection::EGPD_Input)
			{
				if (GetCachedPairOutputConnector())
				{
					return GetCachedPairOutputConnector()->ConnectorName;
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
			.Padding(FJointEditorStyle::Margin_Border)
			[

				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(FJointEditorStyle::Margin_Border)
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
					.Padding(FJointEditorStyle::Margin_Border)
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
					.ContentMargin(FJointEditorStyle::Margin_Button)
					.OutlineMargin(1.5)
					.OnPressed_UObject(this, &UJointEdGraphNode_Connector::OnAddInputNodeButtonPressed)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(FJointEditorStyle::Margin_Border)
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
						.Padding(FJointEditorStyle::Margin_Border)
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
			.Padding(FJointEditorStyle::Margin_Border)
			[

				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(FJointEditorStyle::Margin_Border)
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
				.Padding(FJointEditorStyle::Margin_Border)
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
		if (GetCachedPairOutputConnector() == nullptr)
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
