//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "Node/JointEdGraphNode_Composite.h"

#include "EdGraphUtilities.h"
#include "JointAdvancedWidgets.h"
#include "JointEdGraph.h"
#include "JointEditorSettings.h"
#include "JointEditorStyle.h"
#include "JointEditorToolkit.h"
#include "JointEdUtils.h"
#include "JointFunctionLibrary.h"
#include "ScopedTransaction.h"
#include "SGraphPanel.h"
#include "SGraphPreviewer.h"
#include "GraphNode/SJointGraphNodeBase.h"

#include "EdGraph/EdGraph.h"
#include "EditorWidget/SJointGraphPanel.h"
#include "EditorWidget/SJointGraphPreviewer.h"
#include "GraphNode/JointGraphNodeSharedSlates.h"
#include "GraphNode/SJointRetainerWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"


#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"


#define LOCTEXT_NAMESPACE "UJointEdGraphNode_Composite"


UJointEdGraphNode_Composite::UJointEdGraphNode_Composite()
{
	bIsNodeResizeable = true;
	bCanRenameNode = true; // use custom rename logic for the bound graph - and it utilizes the rename interface from the base node.

	NodeColor = UJointEditorSettings::Get()->DefaultNodeColor;

	static ConstructorHelpers::FObjectFinder<UMaterialInstanceConstant> NodeBodyBrush(
		TEXT(
			"MaterialInstanceConstant'/Joint/Editor/Composite/MI_CompositeNodeBackground.MI_CompositeNodeBackground'"));
	if (NodeBodyBrush.Succeeded())
	{
		DefaultEdNodeSetting.bUseCustomOuterNodeBodyImageBrush = true;
		DefaultEdNodeSetting.OuterNodeBodyImageBrush.SetResourceObject(NodeBodyBrush.Object);
		DefaultEdNodeSetting.OuterNodeBodyImageBrush.ImageSize = FVector2D(12.0, 12.0);
		DefaultEdNodeSetting.OuterNodeBodyImageBrush.DrawAs = ESlateBrushDrawType::Box;
		DefaultEdNodeSetting.OuterNodeBodyImageBrush.Margin = FMargin(0.5);
	}

	DefaultEdNodeSetting.bUseCustomInnerNodeBodyImageBrush = true;
	DefaultEdNodeSetting.InnerNodeBodyImageBrush.DrawAs = ESlateBrushDrawType::NoDrawType; // NoDrawType to hide the outline.
	DefaultEdNodeSetting.InnerNodeBodyImageBrush.Margin = FMargin(0.5);

	DefaultEdNodeSetting.bUseCustomNodeShadowImageBrush = true;
	DefaultEdNodeSetting.NodeShadowImageBrush.DrawAs = ESlateBrushDrawType::NoDrawType; // NoDrawType to hide the outline.

	DefaultEdNodeSetting.bUseSpecifiedGraphNodeBodyColor = true;
	DefaultEdNodeSetting.bUseIconicColorForNodeBodyOnStow = false;
	DefaultEdNodeSetting.NodeBodyColor = FLinearColor(0.5, 0.5, 0.5, 0.5);
	DefaultEdNodeSetting.NodeIconicColor = FColor(29, 130, 126, 125);
	DefaultEdNodeSetting.DefaultEdSlateDetailLevel = EJointEdSlateDetailLevel::SlateDetailLevel_Maximum;
}

FText UJointEdGraphNode_Composite::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FText Title = LOCTEXT( "CompositeNodeTitle", "Sub Graph Composite Node - {0}");
	
	if (BoundGraph)
	{
		return FText::Format(Title, FText::FromName(BoundGraph->GetFName()));
	}

	return FText::Format(Title, FText::FromName(FName("ERROR! No Bound Graph")));
}


bool UJointEdGraphNode_Composite::GetShouldHideNameBox() const
{
	return true;
}

void UJointEdGraphNode_Composite::SynchronizeTunnelNodePins()
{
	if (bIsSynchronizingPins) return;

	bIsSynchronizingPins = true;

	TArray<FJointEdPinData> InputPinData;
	TArray<FJointEdPinData> OutputPinData;

	for (const FJointEdPinData& PinDataElem : PinData)
	{
		if (PinDataElem.Direction == EEdGraphPinDirection::EGPD_Output)
		{
			FJointEdPinData ModifiedPinDataElem = PinDataElem;
			ModifiedPinDataElem.Direction = EEdGraphPinDirection::EGPD_Input;
			OutputPinData.Add(ModifiedPinDataElem);
		}
		else
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

	bIsSynchronizingPins = false;
}


void UJointEdGraphNode_Composite::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		UpdatePins();

		Super::PostEditChangeProperty(PropertyChangedEvent);
	}
}

FLinearColor UJointEdGraphNode_Composite::GetNodeTitleColor() const
{
	return NodeColor;
}


FLinearColor UJointEdGraphNode_Composite::GetNodeBodyTintColor() const
{
	return NodeColor;
}

FVector2D UJointEdGraphNode_Composite::GetNodeMinimumSize() const
{
	return FVector2D(20, 20);
}

void UJointEdGraphNode_Composite::AllocateDefaultPins()
{
	ReallocatePins();

	RequestPopulationOfPinWidgets();
}

void UJointEdGraphNode_Composite::PrepareForCopying()
{
	Super::PrepareForCopying();

	if (BoundGraph)
	{
		//We want to make it have a 'duplicated' version of the BoundGraph - so, we're going to set the outer of the BoundGraph to this node temporarily to make the duplication process handle the nested-duplication of the BoundGraph.
		
		BoundGraph->Rename(*BoundGraph->GetName(), this, REN_DontCreateRedirectors | REN_NonTransactional);

		if (UJointEdGraph* CastedBoundGraph = Cast<UJointEdGraph>(BoundGraph))
		{
			CastedBoundGraph->PrepareForCopy();
		}
	}
	
}

void UJointEdGraphNode_Composite::PostCopyNode()
{
	Super::PostCopyNode();

	if (BoundGraph)
	{
		BoundGraph->Rename(*BoundGraph->GetName(), this->GetGraph(), REN_DontCreateRedirectors | REN_NonTransactional);

		if (UJointEdGraph* CastedBoundGraph = Cast<UJointEdGraph>(BoundGraph))
		{
			CastedBoundGraph->PostCopy();
		}
	}
}

void UJointEdGraphNode_Composite::PostPasteNode()
{
	Super::PostPasteNode();

	TSet<UEdGraphNode*> PastedNodes;

	if (BoundGraph)
	{
		FString NewName = BoundGraph->GetName();
		
		if (FJointEdUtils::GetSafeNameForObject(NewName, this->GetGraph()))
		{
			BoundGraph->Rename(
				*NewName,
				this->GetGraph(),
				REN_DontCreateRedirectors | REN_NonTransactional
			);
		}

		GetCastedGraph()->SubGraphs.Add(BoundGraph);

		if (UJointEdGraph* CastedBoundGraph = Cast<UJointEdGraph>(BoundGraph))
		{
			TSet<TWeakObjectPtr<UJointEdGraphNode>> Nodes = CastedBoundGraph->GetCachedJointGraphNodes(true);

			// Fill the pasted nodes set
			for (TWeakObjectPtr<UJointEdGraphNode> EdGraphNode : Nodes)
			{
				if (!EdGraphNode.IsValid() || EdGraphNode.Get() == nullptr) continue;
				PastedNodes.Add(EdGraphNode.Get());
			}

			// Post process the pasted nodes to fix up.
			FEdGraphUtilities::PostProcessPastedNodes(PastedNodes);
			
			// Handle any fixup of the imported nodes
			FJointEdUtils::MarkNodesAsModifiedAndValidateName(PastedNodes);
		}
	}

	ModifyGraphNodeSlate();
}

bool UJointEdGraphNode_Composite::CanDuplicateNode() const
{
	return true; //Take care the occasion when there is another output node with the same Guid.
}

void UJointEdGraphNode_Composite::ReconstructNode()
{
	UpdateNodeInstance();

	UpdatePins();

	NodeConnectionListChanged();
}

void UJointEdGraphNode_Composite::PostPlacedNewNode()
{
	UpdatePins();

	// Create a new graph
	BoundGraph = UJointEdGraph::CreateNewJointGraph(GetGraph(), GetJointManager(), FName("SubGraph"));
	BoundGraph->bAllowDeletion = false;

	check(BoundGraph);

	// Create the entry/exit nodes inside the new graph
	{
		FGraphNodeCreator<UJointEdGraphNode_Tunnel> EntryNodeCreator(*BoundGraph);
		UJointEdGraphNode_Tunnel* EntryNode = EntryNodeCreator.CreateNode();
		EntryNode->bCanHaveOutputs = true;
		EntryNode->bCanHaveInputs = false;
		EntryNode->OutputSourceNode = this;
		EntryNodeCreator.Finalize();

		InputSinkNode = EntryNode;
	}
	{
		FGraphNodeCreator<UJointEdGraphNode_Tunnel> ExitNodeCreator(*BoundGraph);
		UJointEdGraphNode_Tunnel* ExitNode = ExitNodeCreator.CreateNode();
		ExitNode->bCanHaveOutputs = false;
		ExitNode->bCanHaveInputs = true;
		ExitNode->InputSinkNode = this;
		ExitNodeCreator.Finalize();

		OutputSourceNode = ExitNode;
	}

	// Add the new graph as a child of our parent graph
	GetGraph()->SubGraphs.Add(BoundGraph);

	Super::PostPlacedNewNode();
}

bool UJointEdGraphNode_Composite::CanHaveSubNode() const
{
	return false;
}


bool UJointEdGraphNode_Composite::CanReplaceNodeClass()
{
	return false;
}

bool UJointEdGraphNode_Composite::CanReplaceEditorNodeClass()
{
	return false;
}

void UJointEdGraphNode_Composite::NodeConnectionListChanged()
{
	if (bIsUpdatingNodeConnection) return;

	bIsUpdatingNodeConnection = true;

	if (InputSinkNode) InputSinkNode->NodeConnectionListChanged();
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

void UJointEdGraphNode_Composite::AllocateReferringNodeInstancesOnConnection(TArray<TObjectPtr<UJointNodeBase>>& Nodes, UEdGraphPin* SourcePin)
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

	if (Owner == this)
	{
		// 1
		if (SourcePin->Direction == EEdGraphPinDirection::EGPD_Input)
		{
			AllocateConnectionFromSelf(InputSinkNode, SourcePin, Nodes);
		}
		else
		{
			AllocateConnectionFromSelf(OutputSourceNode, SourcePin, Nodes);
		}
	}
	else if (Owner == InputSinkNode)
	{
		//2 - 1
		AllocateConnection(EEdGraphPinDirection::EGPD_Input, SourcePin, Nodes);
	}
	else if (Owner == OutputSourceNode)
	{
		//2 - 2
		AllocateConnection(EEdGraphPinDirection::EGPD_Output, SourcePin, Nodes);
	}
}


void UJointEdGraphNode_Composite::UpdateNodeInstance()
{
}

bool UJointEdGraphNode_Composite::CanUserDeleteNode() const
{
	return true;
}

void UJointEdGraphNode_Composite::DestroyNode()
{
	// This is where the toolkit actually starts to destroy and invalidate the graph. 
	if (BoundGraph)
	{
		FJointEdUtils::RemoveGraph(Cast<UJointEdGraph>(BoundGraph));

		BoundGraph = nullptr;
	}

	Super::DestroyNode();
}

void UJointEdGraphNode_Composite::ModifyGraphNodeSlate()
{
	
	if (!BoundGraph) return;
	
	const TSharedPtr<SJointGraphNodeBase> NodeSlate = GetGraphNodeSlate().Pin();
	if (!NodeSlate) return;

	if (!NodeSlate->CenterWholeBox.IsValid()) return;
	if (!NodeSlate->CenterContentBox.IsValid()) return;

	//make an attribute for const FSlateBrush* , lock icon.

	TAttribute<const FSlateBrush*> LockIconBrush = TAttribute<const FSlateBrush*>::Create(
		TAttribute<const FSlateBrush*>::FGetter::CreateLambda([this]() -> const FSlateBrush*
		{
			if (bLockPreviewer)
			{
				return FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Lock");
			}
			else
			{
				return FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Unlock");
			}
		})
	);

	TAttribute<const FSlateBrush*> PreviewerIconBrush = TAttribute<const FSlateBrush*>::Create(
		TAttribute<const FSlateBrush*>::FGetter::CreateLambda([this]() -> const FSlateBrush*
		{
			if (bShowPreviewer)
			{
				return FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.FolderOpen");
			}
			else
			{
				return FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.FolderClosed");
			}
		})
	);

	TAttribute<bool> FitPreviewerButtonEnabled = TAttribute<bool>::Create(
		TAttribute<bool>::FGetter::CreateLambda([this]() -> bool
		{
			return bShowPreviewer && !bLockPreviewer;
		})
	);


	if (bShowPreviewer)
	{
		auto& Slot = NodeSlate->CenterWholeBox->GetSlot(1);
		Slot.SetAutoHeight();
		Slot.SetVerticalAlignment(VAlign_Top);
		Slot.SetHorizontalAlignment(HAlign_Fill);

		NodeSlate->SetToolTip(nullptr);

		NodeSlate->CenterContentBox->AddSlot()
		         .HAlign(HAlign_Center)
		         .VAlign(VAlign_Center)
		         .AutoHeight()
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(FJointEditorStyle::Margin_Normal)
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SImage)
						.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(TEXT("GraphEditor.SubGraph_16x")))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SAssignNew(BoundGraphNameEditableTextBlock, SInlineEditableTextBlock)
						.Text(FText::FromString(BoundGraph->GetName()))
						.Style(FJointEditorStyle::Get(), "JointUI.InlineEditableTextBlock.Regular.h4")
						.OnVerifyTextChanged_UObject(this, &UJointEdGraphNode_Composite::OnVerifyNameTextChanged)
						.OnTextCommitted_UObject(this, &UJointEdGraphNode_Composite::OnRenameTextCommited)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.Padding(FJointEditorStyle::Margin_Normal.Left, 0, FJointEditorStyle::Margin_Normal.Right, FJointEditorStyle::Margin_Normal.Bottom)
				[
					SNew(SBox)
					.WidthOverride_Lambda([this]() { return GetSize().X - 25; })
					.HeightOverride_Lambda([this]() { return GetSize().Y - 48; })
					.Clipping(EWidgetClipping::ClipToBounds)
					[
						SAssignNew(JointGraphPreviewer, SJointGraphPreviewer, BoundGraph)
						.Visibility(bLockPreviewer ? EVisibility::HitTestInvisible : EVisibility::Visible)
						.Clipping(EWidgetClipping::Inherit)
					]
				]
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Top)
			[
				SNew(SButton)
				.OnHovered_UObject(this, &UJointEdGraphNode_Composite::OnPreviewerHovered)
				.OnUnhovered_UObject(this, &UJointEdGraphNode_Composite::OnPreviewerUnhovered)
				.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Empty")
				.ContentPadding(FMargin(0))
				[
					SAssignNew(ButtonDrawer, SJointSlateDrawer)
					.Visibility(EVisibility::Visible)
					+ SJointSlateDrawer::Slot()
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(SJointOutlineButton)
						.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
						.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.ContentPadding(FJointEditorStyle::Margin_Normal)
						.OutlinePadding(1.5)
						.NormalColor(FLinearColor::Transparent)
						.OutlineNormalColor(FLinearColor::Transparent)
						.OnClicked_UObject(this, &UJointEdGraphNode_Composite::OnGraphClicked)
						.ToolTipText(LOCTEXT("GoToSubGraph_Tooltip", "Go to the sub graph"))
						[
							SNew(SBox)
							.WidthOverride(16)
							.HeightOverride(16)
							[
								SNew(SImage)
								.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.ArrowRight"))
							]
						]
					]
					+ SJointSlateDrawer::Slot()
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(SJointOutlineButton)
						.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
						.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.ContentPadding(FJointEditorStyle::Margin_Normal)
						.OutlinePadding(1.5)
						.NormalColor(FLinearColor::Transparent)
						.OutlineNormalColor(FLinearColor::Transparent)
						.OnClicked_UObject(this, &UJointEdGraphNode_Composite::OnPreviewerOpenCloseClicked)
						.ToolTipText(LOCTEXT("PreviewerButton_Tooltip", "Open & Close the previewer"))
						[
							SNew(SBox)
							.WidthOverride(16)
							.HeightOverride(16)
							[
								SNew(SImage)
								.Image(PreviewerIconBrush)
							]
						]
					]
					+ SJointSlateDrawer::Slot()
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(SJointOutlineButton)
						.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
						.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.ContentPadding(FJointEditorStyle::Margin_Normal)
						.OutlinePadding(1.5)
						.NormalColor(FLinearColor::Transparent)
						.OutlineNormalColor(FLinearColor::Transparent)
						.OnClicked_UObject(this, &UJointEdGraphNode_Composite::OnLockPreviewerClicked)
						.ToolTipText(LOCTEXT("LockGraphButton_Tooltip", "Lock & Unlock the previewer"))
						[
							SNew(SBox)
							.WidthOverride(16)
							.HeightOverride(16)
							[
								SNew(SImage)
								.Image(LockIconBrush)
							]
						]
					]
					+ SJointSlateDrawer::Slot()
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(SJointOutlineButton)
						.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
						.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.ContentPadding(FJointEditorStyle::Margin_Normal)
						.OutlinePadding(1.5)
						.NormalColor(FLinearColor::Transparent)
						.OutlineNormalColor(FLinearColor::Transparent)
						.OnClicked_UObject(this, &UJointEdGraphNode_Composite::OnFitPreviewerClicked)
						.ToolTipText(LOCTEXT("FitGraphButton_Tooltip", "Fit the previewer to the center of the graph."))
						.IsEnabled(FitPreviewerButtonEnabled)
						[
							SNew(SBox)
							.WidthOverride(16)
							.HeightOverride(16)
							[
								SNew(SImage)
								.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Fullscreen"))
							]
						]
					]
				]
			]
		];
	}
	else
	{
		TSharedPtr<SToolTip> FinalToolTip = NULL;
		SAssignNew(FinalToolTip, SToolTip)
		[
			SNew(SBox)
			.WidthOverride(UJointEditorSettings::Get()->CompositeNodeTooltipSize.X)
			.HeightOverride(UJointEditorSettings::Get()->CompositeNodeTooltipSize.Y)
			[
				SAssignNew(JointGraphPreviewer, SJointGraphPreviewer, BoundGraph)
				.Visibility(EVisibility::Visible)
				.Clipping(EWidgetClipping::Inherit)
			]
		];

		NodeSlate->SetToolTip(FinalToolTip);

		auto& Slot = NodeSlate->CenterWholeBox->GetSlot(1);
		Slot.SetFillHeight(1);
		Slot.SetVerticalAlignment(VAlign_Fill);
		Slot.SetHorizontalAlignment(HAlign_Fill);

		NodeSlate->CenterContentBox->AddSlot()
		         .HAlign(HAlign_Center)
		         .VAlign(VAlign_Center)
		         .FillHeight(1)
		[
			SNew(SButton)
			.OnHovered_UObject(this, &UJointEdGraphNode_Composite::OnPreviewerHovered)
			.OnUnhovered_UObject(this, &UJointEdGraphNode_Composite::OnPreviewerUnhovered)
			.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Empty")
			.ContentPadding(FMargin(0))
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(FJointEditorStyle::Margin_Normal)
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(24)
						.HeightOverride(24)
						[
							SNew(SImage)
							.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(TEXT("GraphEditor.SubGraph_16x")))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SAssignNew(BoundGraphNameEditableTextBlock, SInlineEditableTextBlock)
						.Text(FText::FromString(BoundGraph->GetName()))
						.Style(FJointEditorStyle::Get(), "JointUI.InlineEditableTextBlock.Regular.h1")
						.OnVerifyTextChanged_UObject(this, &UJointEdGraphNode_Composite::OnVerifyNameTextChanged)
						.OnTextCommitted_UObject(this, &UJointEdGraphNode_Composite::OnRenameTextCommited)
					]
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SAssignNew(ButtonDrawer, SJointSlateDrawer)
					.Visibility(EVisibility::Visible)
					+ SJointSlateDrawer::Slot()
					.Padding(FJointEditorStyle::Margin_Normal.Left, FJointEditorStyle::Margin_Normal.Top * 15, FJointEditorStyle::Margin_Normal.Right, 0)
					[
						SNew(SJointOutlineButton)
						.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
						.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.ContentPadding(FJointEditorStyle::Margin_Normal)
						.OutlinePadding(1.5)
						.NormalColor(FLinearColor::Transparent)
						.OutlineNormalColor(FLinearColor::Transparent)
						.OnClicked_UObject(this, &UJointEdGraphNode_Composite::OnGraphClicked)
						.ToolTipText(LOCTEXT("GoToSubGraph_Tooltip", "Go to the sub graph"))
						[
							SNew(SBox)
							.WidthOverride(24)
							.HeightOverride(24)
							[
								SNew(SImage)
								.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.ArrowRight"))
							]
						]
					]
					+ SJointSlateDrawer::Slot()
					.Padding(FJointEditorStyle::Margin_Normal.Left, FJointEditorStyle::Margin_Normal.Top * 15, FJointEditorStyle::Margin_Normal.Right, 0)
					[
						SNew(SJointOutlineButton)
						.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
						.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.ContentPadding(FJointEditorStyle::Margin_Normal)
						.OutlinePadding(1.5)
						.NormalColor(FLinearColor::Transparent)
						.OutlineNormalColor(FLinearColor::Transparent)
						.OnClicked_UObject(this, &UJointEdGraphNode_Composite::OnPreviewerOpenCloseClicked)
						.ToolTipText(LOCTEXT("PreviewerButton_Tooltip", "Open & Close the previewer"))
						[
							SNew(SBox)
							.WidthOverride(24)
							.HeightOverride(24)
							[
								SNew(SImage)
								.Image(PreviewerIconBrush)
							]
						]
					]
				]
			]
		];
	}
}


bool UJointEdGraphNode_Composite::CanHaveBreakpoint() const
{
	return false;
}

void UJointEdGraphNode_Composite::GetNodeContextMenuActions(UToolMenu* Menu,
                                                            UGraphNodeContextMenuContext* Context) const
{
	return;
}

bool UJointEdGraphNode_Composite::UseLowDetailedRendering() const
{
	const TSharedPtr<SJointGraphNodeBase> NodeSlate = GetGraphNodeSlate().Pin();

	if (NodeSlate)
	{
		if (const TSharedPtr<SGraphPanel>& MyOwnerPanel = NodeSlate->GetOwnerPanel())
		{
			if (MyOwnerPanel.IsValid())
			{
				return MyOwnerPanel->GetCurrentLOD() < EGraphRenderingLOD::DefaultDetail;
			}
		}
	}

	return false;
}

bool UJointEdGraphNode_Composite::UseCaptureDetailedRendering() const
{
	const TSharedPtr<SJointGraphNodeBase> NodeSlate = GetGraphNodeSlate().Pin();

	if (NodeSlate)
	{
		if (const TSharedPtr<SGraphPanel>& MyOwnerPanel = NodeSlate->GetOwnerPanel())
		{
			if (MyOwnerPanel.IsValid())
			{
				return MyOwnerPanel->GetCurrentLOD() >= EGraphRenderingLOD::DefaultDetail;
			}
		}
	}

	return false;
}

void UJointEdGraphNode_Composite::OnPreviewerHovered()
{
	if (ButtonDrawer)
	{
		ButtonDrawer->UpdateVisualOnHovered();
	}
}


void UJointEdGraphNode_Composite::OnPreviewerUnhovered()
{
	if (ButtonDrawer)
	{
		ButtonDrawer->UpdateVisualOnUnhovered();
	}
}

FReply UJointEdGraphNode_Composite::OnGraphClicked()
{
	if (OptionalToolkit.IsValid())
	{
		OptionalToolkit.Pin()->JumpToHyperlink(BoundGraph);
	}

	return FReply::Handled();
}

FReply UJointEdGraphNode_Composite::OnPreviewerOpenCloseClicked()
{
	bShowPreviewer = !bShowPreviewer;

	RequestUpdateSlate();

	return FReply::Handled();
}

FReply UJointEdGraphNode_Composite::OnLockPreviewerClicked()
{
	bLockPreviewer = !bLockPreviewer;

	RequestUpdateSlate();

	return FReply::Handled();
}

FReply UJointEdGraphNode_Composite::OnFitPreviewerClicked()
{
	if (JointGraphPreviewer.Pin() && JointGraphPreviewer.Pin()->GetGraphPanel().IsValid())
	{
		JointGraphPreviewer.Pin()->GetGraphPanel().Pin()->ZoomToFit(false);
	}

	return FReply::Handled();
}

void UJointEdGraphNode_Composite::OnRenameTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{
	if (BoundGraph == nullptr) return;

	FString Name = InText.ToString();

	if (FJointEdUtils::GetSafeNameForObjectRenaming(Name, BoundGraph, BoundGraph->GetOuter()))
	{
		FScopedTransaction Transaction(LOCTEXT("RenameGraph", "Rename Graph"));

		BoundGraph->Modify();
		BoundGraph->GetOuter()->Modify();
		BoundGraph->GetPackage()->Modify();

		BoundGraph->Rename(*Name, BoundGraph->GetOuter(), REN_NonTransactional);

		if (BoundGraphNameEditableTextBlock)
		{
			BoundGraphNameEditableTextBlock->SetText(FText::FromString(BoundGraph->GetName()));
		}

		if (OptionalToolkit.IsValid())
		{
			OptionalToolkit.Pin()->RefreshJointEditorOutliner();
		}
	}
}


bool UJointEdGraphNode_Composite::OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage)
{
	bool bValid(true);

	bValid = FJointEdUtils::IsNameSafeForObjectRenaming(InText.ToString(), BoundGraph, BoundGraph->GetOuter(), OutErrorMessage);

	if (OutErrorMessage.IsEmpty())
	{
		OutErrorMessage = FText::FromString(TEXT("Error"));
	}

	return bValid;
}

void UJointEdGraphNode_Composite::RequestStartRenaming()
{
	BoundGraphNameEditableTextBlock->EnterEditingMode();
}


#undef LOCTEXT_NAMESPACE
