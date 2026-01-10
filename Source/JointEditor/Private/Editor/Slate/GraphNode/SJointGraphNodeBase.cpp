//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "GraphNode/SJointGraphNodeBase.h"
#include "JointEdGraphNode.h"
#include "JointEdGraphNode_Fragment.h"

#include "Editor/Slate/GraphNode/JointGraphNodeSharedSlates.h"
#include "GraphEditorSettings.h"

#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Images/SImage.h"

#include "SCommentBubble.h"
#include "ScopedTransaction.h"

#include "JointEditorStyle.h"
#include "Editor.h"

#include "NodeFactory.h"
#include "SGraphPanel.h"
#include "Editor/GraphEditor/Private/DragNode.h"

#include "Framework/Application/SlateApplication.h"

#include "Runtime/Launch/Resources/Version.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"

#include "Logging/TokenizedMessage.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"


#include "Debug/JointDebugger.h"

#include "JointEditorSettings.h"
#include "JointEdUtils.h"

#include "Debug/JointGraphNodeSharedDebuggerSlates.h"
#include "GraphNode/SJointGraphNodeCompileResult.h"
#include "GraphNode/SJointGraphNodeSubNodeBase.h"
#include "GraphNode/SJointDetailsView.h"


#include "JointAdvancedWidgets.h"
#include "JointEditorLogChannels.h"
#include "SlateOptMacros.h"
#include "VoltAnimation.h"
#include "VoltAnimationManager.h"
#include "VoltDecl.h"
#include "EdGraph/EdGraph.h"

#include "Module/Volt_ASM_InterpBackgroundColor.h"
#include "Module/Volt_ASM_InterpChildSlotPadding.h"
#include "Module/Volt_ASM_InterpColor.h"
#include "Module/Volt_ASM_InterpWidgetTransform.h"
#include "Module/Volt_ASM_Sequence.h"
#include "Slate/SRetainerWidget.h"
#include "Widgets/SToolTip.h"

#include "Misc/EngineVersionComparison.h"

TSharedRef<FDragJointGraphNode> FDragJointGraphNode::New(const TSharedRef<SGraphPanel>& InGraphPanel,
                                                         const TSharedRef<SGraphNode>& InDraggedNode)
{
	TSharedRef<FDragJointGraphNode> Operation = MakeShareable(new FDragJointGraphNode);
	Operation->StartTime = FPlatformTime::Seconds();
	Operation->GraphPanel = InGraphPanel;
	Operation->DraggedNodes.Add(InDraggedNode);
	// adjust the decorator away from the current mouse location a small amount based on cursor size
	Operation->DecoratorAdjust = FSlateApplication::Get().GetCursorSize();
	Operation->Construct();
	return Operation;
}

TSharedRef<FDragJointGraphNode> FDragJointGraphNode::New(const TSharedRef<SGraphPanel>& InGraphPanel,
                                                         const TArray<TSharedRef<SGraphNode>>& InDraggedNodes)
{
	TSharedRef<FDragJointGraphNode> Operation = MakeShareable(new FDragJointGraphNode);
	Operation->StartTime = FPlatformTime::Seconds();
	Operation->GraphPanel = InGraphPanel;
	Operation->DraggedNodes = InDraggedNodes;
	Operation->DecoratorAdjust = FSlateApplication::Get().GetCursorSize();
	Operation->Construct();
	return Operation;
}

UJointEdGraphNode* FDragJointGraphNode::GetDropTargetNode() const
{
	return Cast<UJointEdGraphNode>(GetHoveredNode());
}

void FDragJointGraphNode::OnDrop(bool bDropWasHandled, const FPointerEvent& MouseEvent)
{
	NotifyDropAction();

	FDragNode::OnDrop(bDropWasHandled, MouseEvent);
}

bool FDragJointGraphNode::AffectedByPointerEvent(const FPointerEvent& PointerEvent)
{
	return true;
}

void FDragJointGraphNode::NotifyDropAction()
{
	for (TSharedRef<SGraphNode> DraggedNode : DraggedNodes)
	{
		StaticCastSharedRef<SJointGraphNodeBase>(DraggedNode)->OnDragEnded();
	}

	TArray<TWeakPtr<SWidget>> SavedDragOverNodes = DragOverNodes;

	for (TWeakPtr<SWidget> DragOverNode : SavedDragOverNodes)
	{
		if (!DragOverNode.IsValid()) continue;

		DragOverNode.Pin()->OnDragLeave(FDragDropEvent(FPointerEvent(), SharedThis(this)));
	}
}

#define LOCTEXT_NAMESPACE "SJointGraphNodeBase"

void SJointGraphNodeBase::Construct(const FArguments& InArgs, UEdGraphNode* InNode)
{
	this->GraphNode = InNode;
	this->SetCursor(EMouseCursor::Crosshairs);
	this->AssignSlateToGraphNode();
	this->InitializeSlate();
	this->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	SetCanTick(false);
}

SJointGraphNodeBase::~SJointGraphNodeBase()
{
}

void SJointGraphNodeBase::InitializeSlate()
{
	UpdateGraphNode();
}


TSharedRef<SWidget> SJointGraphNodeBase::PopulateSimpleDisplayForProperties()
{
	if (UJointEdGraphNode* InGraphNode = GetCastedGraphNode())
	{
		if (UJointNodeBase* InNodeInstance = InGraphNode->GetCastedNodeInstance())
		{
			if (!InGraphNode->GetEdNodeSetting().PropertyDataForSimpleDisplayOnGraphNode.IsEmpty())
			{
				//Don't populate again - really bad for the performance.

				if (!JointDetailsView)
				{
					
					SAssignNew(JointDetailsView, SJointDetailsView)
					.OwnerGraphNode(SharedThis(this))
					.Object(InNodeInstance)
					.EditorNodeObject(InGraphNode)
					.PropertyData(InGraphNode->GetEdNodeSetting().PropertyDataForSimpleDisplayOnGraphNode);
				}
				else
				{
					JointDetailsView->SetOwnerGraphNode(SharedThis(this));
					JointDetailsView->UpdatePropertyData(InGraphNode->GetEdNodeSetting().PropertyDataForSimpleDisplayOnGraphNode);
				}

				if (JointDetailsView)
				{
					return JointDetailsView.ToSharedRef();
				}

				 return SNullWidget::NullWidget;
			}
		}
	}

	return SNullWidget::NullWidget;
}

void SJointGraphNodeBase::ModifySlateFromGraphNode() const
{
	if (!this->GraphNode) return;

	if (UJointEdGraphNode* CastedGraphNode = Cast<UJointEdGraphNode>(this->GraphNode))
	{
		CastedGraphNode->ModifyGraphNodeSlate();
	}
}

void SJointGraphNodeBase::OnNodeInstancePropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent,
                                                        const FString& PropertyName)
{
	UpdateErrorInfo();

	UpdateNameBox();

	UpdateNodeTagBox();

	UpdateBuildTargetPreset();
}

void SJointGraphNodeBase::OnGraphNodePropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
	// if(PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UJointEdGraphNode, SubNodeBoxOrientation))
	// {
	// 	UpdateGraphNode();
	// }

	UpdateGraphNode();
}

void SJointGraphNodeBase::OnDebugDataChanged(const FJointNodeDebugData* Data)
{
	if (Data != nullptr)
	{
		AssignBreakpointOverlay();
	}
	else
	{
		RemoveBreakpointOverlay();
	}
}


void SJointGraphNodeBase::AssignSlateToGraphNode()
{
	if (!this->GraphNode) return;

	if (UJointEdGraphNode* CastedGraphNode = Cast<UJointEdGraphNode>(this->GraphNode))
	{
		CastedGraphNode->SetGraphNodeSlate(SharedThis(this));
	}
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SJointGraphNodeBase::UpdateNodeTagBox() const
{
	if (!NodeTagBox.IsValid() || !NodeTagContentBox.IsValid()) return;

	NodeTagContentBox->ClearChildren();

	if (GetCastedGraphNode() && GetCastedGraphNode()->GetCastedNodeInstance())
	{
		TArray<FGameplayTag> NodeInstanceTags;

		GetCastedGraphNode()->GetCastedNodeInstance()->NodeTags.GetGameplayTagArray(NodeInstanceTags);

		const int Length = NodeInstanceTags.Num();

		//Iterate backward
		for (int i = Length; i-- > 0;)
		{
			NodeTagContentBox->AddSlot()
			                 .AutoHeight()
			                 .Padding(FJointEditorStyle::Margin_Tiny)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("MainFrame.OpenIssueTracker"))
					.DesiredSizeOverride(FVector2D(8, 8))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(NodeInstanceTags[i].ToString()))
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h5")
				]
			];
		}

		NodeTagBox->SetVisibility(Length > 0 ? EVisibility::Visible : EVisibility::Collapsed);
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SJointGraphNodeBase::AssignBuildTargetPresetOverlay()
{
	BuildPresetOverlay = SNew(SJointBuildPreset)
		.ParentGraphNodeSlate(StaticCastSharedRef<SJointGraphNodeBase>(AsShared()))
		.ToolTipText(LOCTEXT("SJointBuildTargetPreset_ToolTip", "The Build Preset this node is using."));
}

void SJointGraphNodeBase::RemoveBuildPresetOverlay()
{
	BuildPresetOverlay.Reset();
}

void SJointGraphNodeBase::AssignBreakpointOverlay()
{
	BreakpointOverlay = SNew(SJointBreakpointIndicator)
		.GraphNodeSlate(StaticCastSharedRef<SJointGraphNodeBase>(AsShared()));
}

void SJointGraphNodeBase::RemoveBreakpointOverlay()
{
	BreakpointOverlay.Reset();
}

void SJointGraphNodeBase::AssignCompileResultOverlay()
{
	CompileResultOverlay = SNew(SJointGraphNodeCompileResult)
		.GraphNodeBase(StaticCastSharedRef<SJointGraphNodeBase>(AsShared()))
		.Visibility(EVisibility::SelfHitTestInvisible);
}

void SJointGraphNodeBase::RemoveCompileResultOverlay()
{
	CompileResultOverlay.Reset();
}

void SJointGraphNodeBase::GetNodeColorScheme(const bool bIsSelected, FLinearColor& NormalColor, FLinearColor& HoverColor,
                                             FLinearColor& OutlineNormalColor, FLinearColor& OutlineHoverColor) const
{
	FLinearColor Color = GetNodeBodyBackgroundColor();

	const FLinearColor HSV = Color.LinearRGBToHSV();

	const float Value = HSV.B;

	if (bIsSelected)
	{
		const FLinearColor OffsetColor = FLinearColor::White * (1 - Value) * 0.005;

		NormalColor = Color * 3 + OffsetColor;
		HoverColor = Color * 3 + OffsetColor;
		OutlineNormalColor = Color * 10 + OffsetColor;
		OutlineHoverColor = Color * 12 + OffsetColor * 15;
	}
	else
	{
		const FLinearColor OffsetColor = FLinearColor::White * (1 - Value) * 0.001;

		NormalColor = Color;
		HoverColor = Color;
		OutlineNormalColor = Color * 1.25 + OffsetColor;
		OutlineHoverColor = Color * 2.5 + OffsetColor * 15;
	}
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SJointGraphNodeBase::PopulateNodeSlates()
{
	/**
	 * SDS2: 
	 * New layout for the whole slate:
	 *
	 *  LeftNodeBox                  NodeBody                  RightNodeBox
	 *  ___________       ________________________________     ____________
	 * |           |    |         CenterWholeBox         |    |            |
	 * |           |    |   __________________________   |    |            | 
	 * |           |    |  |   _____________________  |  |    |            |
	 * |           |    |  |  |NameBox             |  |  |    |            |
	 * |Input Pin 1|    |  |  |____________________|  |  |    |Output Pin 1|
	 * |Input Pin 2|    |  |   _____________________  |  |    |Output Pin 2|
	 * |Input Pin 3|    |  |  |CenterContentBox    |  |  |    |Output Pin 3|
	 * |Input Pin 4|    |  |  |____________________|  |  |    |Output Pin 4|
	 * |...        |    |  |   _____________________  |  |    |...         |
	 * |           |    |  |  |SubNodeBox          |  |  |    |            |
	 * |           |    |  |  |____________________|  |  |    |            |
	 * |           |    |  |__________________________|  |    |            |
	 * |___________|    |________________________________|    |____________| 
	 */

	/**
	 * Don't touch LeftNodeBox, RightNodeBox if possible.
	 * Those are related with the pin actions and difficult to manage without deep understanding of its structure.
	 *
	 * Consider overriding CenterWholeBox if possible.
	 * SJointGraphNodeBase also provide a static functions that help you to populate the default slate set by each section.
	 * Use it on your slate customization code.
	 *
	 * Check out how the default provided Joint Editor Graph Node classes. (especially UJointEdFragment_Context.)
	 */

	UpdateBuildTargetPreset();
	UpdateBreakpoint();
	UpdateErrorInfo();

	SAssignNew(LeftNodeBox, SVerticalBox)
	.Visibility(EVisibility::SelfHitTestInvisible);

	SAssignNew(RightNodeBox, SVerticalBox)
	.Visibility(EVisibility::SelfHitTestInvisible);

	
	CreateNodeBody();

	FMargin ContentPadding = FMargin(0.f);
	
	if (UJointEdGraphNode* InGraphNode = GetCastedGraphNode())
	{
		if (InGraphNode->GetEdNodeSetting().bUseCustomContentNodePadding)
			ContentPadding = InGraphNode->GetEdNodeSetting().ContentNodePadding;
	}
	
	NodeBody->SetContent(
		SNew(SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			CreateNodeBackground()
			//.Image(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			//.ColorAndOpacity(GetNodeBodyBackgroundColor())
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(ContentPadding)
		[
			CreateCenterWholeBox()
		]
	);


	//Set the color value to the current value.
	PlayNodeBackgroundColorResetAnimationIfPossible(true);

	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);

#if UE_VERSION_OLDER_THAN(5, 6, 0)
	TAttribute<FVector2D> LeftBoxOffset_Attr = TAttribute<FVector2D>::CreateLambda([this]
	{
		const FVector2D& LeftBoxSize = LeftNodeBox->GetDesiredSize();
		const FVector2D& Size = GetDesiredSize();

		return FVector2D(-LeftBoxSize.X, (Size.Y * 0.5f) - (LeftBoxSize.Y * 0.5f));
	});

	TAttribute<FVector2D> LeftBoxSize_Attr = TAttribute<FVector2D>::CreateLambda([this]
	{
		return LeftNodeBox->GetDesiredSize();
	});

#else
	TAttribute<FVector2f> LeftBoxOffset_Attr = TAttribute<FVector2f>::CreateLambda([this]
	{
		const FVector2f& LeftBoxSize = LeftNodeBox->GetDesiredSize();
		const FVector2f& Size = GetDesiredSize();

		return FVector2f(-LeftBoxSize.X, (Size.Y * 0.5f) - (LeftBoxSize.Y * 0.5f));
	});

	TAttribute<FVector2f> LeftBoxSize_Attr = TAttribute<FVector2f>::CreateLambda([this]
	{
		return LeftNodeBox->GetDesiredSize();
	});

#endif


#if UE_VERSION_OLDER_THAN(5, 6, 0)
	TAttribute<FVector2D> RightBoxOffset_Attr = TAttribute<FVector2D>::CreateLambda([this]
	{
		const FVector2D& RightBoxSize = RightNodeBox->GetDesiredSize();
		const FVector2D& Size = GetDesiredSize();

		return FVector2D(Size.X, (Size.Y * 0.5) - (RightBoxSize.Y * 0.5));
	});

	TAttribute<FVector2D> RightBoxSize_Attr = TAttribute<FVector2D>::CreateLambda([this]
	{
		return RightNodeBox->GetDesiredSize();
	});

#else
	TAttribute<FVector2f> RightBoxOffset_Attr = TAttribute<FVector2f>::CreateLambda([this]
	{
		const FVector2f& RightBoxSize = RightNodeBox->GetDesiredSize();
		const FVector2f& Size = GetDesiredSize();

		return FVector2f(Size.X, (Size.Y * 0.5) - (RightBoxSize.Y * 0.5));
	});

	TAttribute<FVector2f> RightBoxSize_Attr = TAttribute<FVector2f>::CreateLambda([this]
	{
		return RightNodeBox->GetDesiredSize();
	});
#endif


	this->GetOrAddSlot(ENodeZone::Left)
	    .Padding(FMargin(0))
	    .HAlign(HAlign_Fill)
	    .VAlign(VAlign_Fill)
#if UE_VERSION_OLDER_THAN(5, 6, 0)
		.SlotOffset(LeftBoxOffset_Attr)
		.SlotSize(LeftBoxSize_Attr)
#else
		.SlotOffset2f(LeftBoxOffset_Attr)
			.SlotSize2f(LeftBoxSize_Attr)
#endif
		[
			LeftNodeBox.ToSharedRef()
		];

	this->GetOrAddSlot(ENodeZone::Right)
	    .Padding(FMargin(0))
	    .HAlign(HAlign_Fill)
	    .VAlign(VAlign_Fill)
#if UE_VERSION_OLDER_THAN(5, 6, 0)
		.SlotOffset(RightBoxOffset_Attr)
		.SlotSize(RightBoxSize_Attr)
#else
		.SlotOffset2f(RightBoxOffset_Attr)
			.SlotSize2f(RightBoxSize_Attr)
#endif
		[
			RightNodeBox.ToSharedRef()
		];

	this->GetOrAddSlot(ENodeZone::Center)
	    .Padding(FMargin(0))
	    .HAlign(HAlign_Fill)
	    .VAlign(VAlign_Fill)
	[
		NodeBody.ToSharedRef()
	];


	// Create comment bubble
	TSharedPtr<SCommentBubble> CommentBubble;
	const FSlateColor CommentColor = GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor;

	SAssignNew(CommentBubble, SCommentBubble)
	.GraphNode(GraphNode)
	.Text(this, &SGraphNode::GetNodeComment)
	.OnTextCommitted(this, &SGraphNode::OnCommentTextCommitted)
	.ColorAndOpacity(CommentColor)
	.AllowPinning(true)
	.EnableTitleBarBubble(true)
	.EnableBubbleCtrls(true)
	.GraphLOD(this, &SGraphNode::GetCurrentLOD)
	.IsGraphNodeHovered(this, &SGraphNode::IsHovered);


	TAttribute<FVector2D> CommentSlotOffset_Attr = TAttribute<FVector2D>::CreateLambda([this, CommentBubble]
	{
		return CommentBubble.Get()->GetOffset();
	});

	GetOrAddSlot(ENodeZone::TopCenter)
		.SlotOffset(CommentSlotOffset_Attr)
		.SlotSize(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetSize))
		.AllowScaling(TAttribute<bool>(CommentBubble.Get(), &SCommentBubble::IsScalingAllowed))
		.VAlign(VAlign_Top)
		[
			CommentBubble.ToSharedRef()
		];

	CreateAdvancedViewArrow(CenterContentBox);

	CreateBelowWidgetControls(CenterContentBox);

	InitializeVoltVariables();
	
	//Modify the graph node slates from the editor graph node instance.
	ModifySlateFromGraphNode();
}

TSharedRef<SBorder> SJointGraphNodeBase::CreateNodeBody(const bool bSphere)
{
	const FSlateBrush* InBorderImage = FJointEditorStyle::Get().GetBrush(
		bSphere ? "JointUI.Border.NodeShadowSphere" : "JointUI.Border.NodeShadow");

	const FMargin* InPadding = &FJointEditorStyle::Margin_Shadow;
	
	if (UJointEdGraphNode* InGraphNode = GetCastedGraphNode())
	{
		if (InGraphNode->GetEdNodeSetting().bUseCustomNodeShadowImageBrush)
			InBorderImage = &InGraphNode->GetEdNodeSetting().NodeShadowImageBrush;

		if (InGraphNode->GetEdNodeSetting().bUseCustomShadowNodePadding)
			InPadding = &InGraphNode->GetEdNodeSetting().ShadowNodePadding;
	}

	NodeBodyBorderImage = InBorderImage;

	// If NodeBody already exists, update its brush and padding and return it to avoid destroying/recreating the widget
	if (NodeBody.IsValid())
	{
		NodeBody->SetBorderImage(InBorderImage);
		// SetPadding is available on SBorder; update if necessary
		NodeBody->SetPadding(*InPadding);
		return NodeBody.ToSharedRef();
	}

	NodeBody = SNew(SBorder)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.OnMouseButtonUp(this, &SJointGraphNodeBase::OnMouseButtonUp)
		.OnMouseButtonDown(this, &SJointGraphNodeBase::OnMouseButtonDown)
		.RenderTransformPivot(FVector2D(0.5))
		.Cursor(this, &SJointGraphNodeBase::GetCursor)
		.BorderImage(InBorderImage)
		.BorderBackgroundColor(FJointEditorStyle::Color_Node_Shadow)
		.Padding(*InPadding);
	
	return NodeBody.ToSharedRef();
}

TSharedRef<SJointOutlineBorder> SJointGraphNodeBase::CreateNodeBackground(const bool bSphere)
{
	FLinearColor NormalColor;
	FLinearColor HoverColor;
	FLinearColor OutlineNormalColor;
	FLinearColor OutlineHoverColor;

	GetNodeColorScheme(false, NormalColor, HoverColor, OutlineNormalColor, OutlineHoverColor);

	const FSlateBrush* InnerBorderImage = FJointEditorStyle::Get().GetBrush(
		bSphere ? "JointUI.Border.Sphere" : "JointUI.Border.Round");
	const FSlateBrush* OuterBorderImage = FJointEditorStyle::Get().GetBrush(
		bSphere ? "JointUI.Border.Sphere" : "JointUI.Border.Round");

	if (UJointEdGraphNode* InGraphNode = GetCastedGraphNode())
	{
		if (InGraphNode->GetEdNodeSetting().bUseCustomInnerNodeBodyImageBrush)
			InnerBorderImage = &InGraphNode->GetEdNodeSetting().InnerNodeBodyImageBrush;

		if (InGraphNode->GetEdNodeSetting().bUseCustomOuterNodeBodyImageBrush)
			OuterBorderImage = &InGraphNode->GetEdNodeSetting().OuterNodeBodyImageBrush;
	}

	NodeBackgroundInBorderImage = InnerBorderImage;
	NodeBackgroundOutBorderImage = OuterBorderImage;

	return SAssignNew(NodeBackground, SJointOutlineBorder)
		.RenderTransformPivot(FVector2D(0.5))
			//.Visibility(EVisibility::SelfHitTestInvisible)
		.NormalColor(NormalColor)
		.HoverColor(HoverColor)
		.OutlineNormalColor(OutlineNormalColor)
		.OutlineHoverColor(OutlineHoverColor)
		.UnHoverAnimationSpeed(9)
		.HoverAnimationSpeed(9)
		.InnerBorderImage(InnerBorderImage)
		.OuterBorderImage(OuterBorderImage);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


TSharedPtr<SGraphNode> SJointGraphNodeBase::CreateSubNodeWidget(const TSharedPtr<SGraphPanel>& OwnerGraphPanel,
                                                                UJointEdGraphNode_Fragment* EdFragment)
{
	TSharedPtr<SGraphNode> NewNode = FNodeFactory::CreateNodeWidget(EdFragment);

	if (OwnerGraphPanel.IsValid())
	{
		NewNode->SetOwner(OwnerGraphPanel.ToSharedRef());
		OwnerGraphPanel->AttachGraphEvents(NewNode);
	}

	return NewNode;
}


void SJointGraphNodeBase::ClearSlates()
{
	//Reset slots
	this->RemoveSlot(ENodeZone::Left);
	this->RemoveSlot(ENodeZone::Center);
	this->RemoveSlot(ENodeZone::Right);
	this->RemoveSlot(ENodeZone::TopCenter);
	
	RightNodeBox.Reset();
	LeftNodeBox.Reset();
	CenterContentBox.Reset();

	NodeBody.Reset();
	NameBox.Reset();

	JointDetailsView.Reset();
	BuildPresetOverlay.Reset();
	SubNodes.Reset();
}

FVector2D SJointGraphNodeBase::ComputeDesiredSize(float) const
{
	if (const UJointEdGraphNode* CastedGraphNode = GetCastedGraphNode())
	{
		if (CastedGraphNode->GetUseFixedNodeSize()) return CastedGraphNode->GetSize();

		return SNodePanel::SNode::ComputeDesiredSize(1);
	}

	if (GraphNode)
	{
		return FVector2D(GraphNode->NodeWidth, GraphNode->NodeHeight);
	}

	return GetNodeMinimumSize();
}

FVector2D SJointGraphNodeBase::GetNodeMinimumSize() const
{
	if (const UJointEdGraphNode* CastedGraphNode = GetCastedGraphNode())
	{
		return CastedGraphNode->GetNodeMinimumSize();
	}
	return JointGraphNodeResizableDefs::MinNodeSize;
}


FVector2D SJointGraphNodeBase::GetNodeMaximumSize() const
{
	if (const UJointEdGraphNode* CastedGraphNode = GetCastedGraphNode())
	{
		return CastedGraphNode->GetNodeMaximumSize();
	}

	return JointGraphNodeResizableDefs::MaxNodeSize;
}

void SJointGraphNodeBase::UpdateGraphNode()
{
	//Clear old slates.
	ClearSlates();

	//Generate basic layout for the widget.
	PopulateNodeSlates();

	//Generate pins for this node.
	PopulatePinWidgets();

	//Generate sub node slates for this node.
	PopulateSubNodeSlates();

	UserSize = ComputeDesiredSize(1);

	//Update the node tag box to validate.
	UpdateNodeTagBox();

	//Update the name box.
	UpdateNameBox();

	//Update the debugger slate for the need.
	if (UJointDebugger::IsPIESimulating())
	{
		UpdateDebuggerAnimationByState();
	}
}

void SJointGraphNodeBase::UpdateOwnerOfPinWidgets()
{
	TSet<TSharedRef<SWidget>> AllPins;
	GetPins(AllPins);

	TSharedPtr<SGraphNode> SelfPtr = SharedThis(this);

	if (!SelfPtr.IsValid()) return;

	for (TSharedRef<SWidget> Pin : AllPins)
	{
		//cast to TSharedRef<SGraphPin>
		TSharedRef<SGraphPin> PinAsGraphPin = StaticCastSharedRef<SGraphPin>(Pin);

		PinAsGraphPin->SetOwner(SelfPtr.ToSharedRef());
	}
}

void SJointGraphNodeBase::SetOwner(const TSharedRef<SGraphPanel>& OwnerPanel)
{
	OwnerGraphPanelPtr.Reset();

	SGraphNode::SetOwner(OwnerPanel);

	for (TSharedPtr<SGraphNode> InGraphNode : SubNodes)
	{
		if (!InGraphNode->GetOwnerPanel().IsValid())
		{
			InGraphNode->SetOwner(OwnerPanel);
			OwnerPanel->AttachGraphEvents(InGraphNode);
		}
	}

	//UpdateOwnerOfPinWidgets();
}

FGraphSelectionManager* SJointGraphNodeBase::GetSelectionManager() const
{
	if (GetOwnerPanel().IsValid())
	{
		return &GetOwnerPanel()->SelectionManager;
	}

	return nullptr;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SJointGraphNodeBase::PopulateSubNodeSlates()
{
	ClearChildrenOnSubNodePanel();

	UJointEdGraphNode* CastedGraphNode = GetCastedGraphNode();

	if (CastedGraphNode == nullptr) return;

	//Remove invalid elements from the SubNodes array.
	TArray<TSharedPtr<SJointGraphNodeBase>> SubNodeSlates;

	for (UJointEdGraphNode* Node : CastedGraphNode->SubNodes)
	{
		if (Node == nullptr) continue;

		if (!Node->GetGraphNodeSlate().IsValid()) continue;

		SubNodeSlates.Add(Node->GetGraphNodeSlate().Pin());
	}

	SubNodes.RemoveAll([SubNodeSlates](TSharedPtr<SGraphNode> SubNodeSlate)
	{
		return !SubNodeSlates.Contains(SubNodeSlate);
	});

	InsertPoints.Empty();


	TSharedPtr<SGraphPanel> OwnerPanel = GetOwnerPanel();

	const int SubNodesNum = CastedGraphNode->SubNodes.Num();

	for (int32 i = 0; i < SubNodesNum; i++)
	{
		UJointEdGraphNode* SubNode = CastedGraphNode->SubNodes[i];

		if (SubNode == nullptr) continue;

		UJointEdGraphNode_Fragment* EdFragment = Cast<UJointEdGraphNode_Fragment>(SubNode);

		if (EdFragment == nullptr) continue;

		if (EdFragment->ShouldManuallyImplementSlate()) continue;


		TSharedPtr<SGraphNode> SubNodeSlateToAdd;

		//When the slate already has a valid slate, just grab that and use that. + Check if the owner is correct - otherwise, reassign it.
		if (EdFragment->CheckGraphNodeSlateReusableOn(OwnerPanel))
		{
			SubNodeSlateToAdd = EdFragment->GetGraphNodeSlate().Pin();
			//SubNodeSlateToAdd->SetOwner(OwnerPanel.ToSharedRef());
		}
		else //if not, make a new one.
		{
			SubNodeSlateToAdd = CreateSubNodeWidget(OwnerPanel, EdFragment);

			AssignSubNode(SubNodeSlateToAdd);
		}


		TSharedPtr<SJointGraphNodeInsertPoint> Point1;

		TSharedPtr<SJointGraphNodeInsertPoint> Point2;

		AddSlateOnSubNodePanel(
			SNew(SBox)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SOverlay)
				.Visibility(EVisibility::SelfHitTestInvisible)
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SubNodeSlateToAdd.ToSharedRef()
				]
				+ SOverlay::Slot()
				.HAlign(GetCastedGraphNode()->GetSubNodeBoxOrientation() == Orient_Horizontal
					        ? HAlign_Left
					        : HAlign_Fill)
				.VAlign(GetCastedGraphNode()->GetSubNodeBoxOrientation() == Orient_Horizontal
					        ? VAlign_Fill
					        : VAlign_Top)
				[
					SAssignNew(Point1, SJointGraphNodeInsertPoint)
					.InsertIndex(i)
					.ParentGraphNodeSlate(SharedThis(this))
				]
				+ SOverlay::Slot()
				.HAlign(GetCastedGraphNode()->GetSubNodeBoxOrientation() == Orient_Horizontal
					        ? HAlign_Right
					        : HAlign_Fill)
				.VAlign(GetCastedGraphNode()->GetSubNodeBoxOrientation() == Orient_Horizontal
					        ? VAlign_Fill
					        : VAlign_Bottom)
				[
					SAssignNew(Point2, SJointGraphNodeInsertPoint)
					.InsertIndex(i + 1)
					.ParentGraphNodeSlate(SharedThis(this))
				]
			]
		);

		InsertPoints.Add(Point1);
		InsertPoints.Add(Point2);
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


void SJointGraphNodeBase::AssignSubNode(const TSharedPtr<SGraphNode>& SubNodeWidget)
{
	SubNodes.Add(SubNodeWidget);
}


FReply SJointGraphNodeBase::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragJointGraphNode> DragNodeOp = DragDropEvent.GetOperationAs<FDragJointGraphNode>();
	if (DragNodeOp.IsValid())
	{
		if (!DragNodeOp->IsValidOperation()) { return FReply::Handled(); }

		GEditor->BeginTransaction(NSLOCTEXT("JointEdTransaction", "TransactionTitle_DragDropNode",
		                                    "Drag & Drop sub node"));

		bool bReorderOperation = true;

		//mark for the transaction

		const TArray<TSharedRef<SGraphNode>>& DraggedNodes = DragNodeOp->GetNodes();
		UJointEdGraphNode* DropTargetNode = DragNodeOp->GetDropTargetNode();
		//const int32 InsertIndex = MyNode->FindSubNodeDropIndex(DropTargetNode);

		DropTargetNode->GetGraph()->Modify();

		DropTargetNode->Modify();
		if (DropTargetNode->GetCastedNodeInstance()) DropTargetNode->GetCastedNodeInstance()->Modify();

		for (int32 Idx = 0; Idx < DraggedNodes.Num(); Idx++)
		{
			if (UJointEdGraphNode* DraggedNode = Cast<UJointEdGraphNode>(DraggedNodes[Idx]->GetNodeObj()))
			{
				DraggedNode->Modify();
				if (DraggedNode->GetCastedNodeInstance()) DraggedNode->GetCastedNodeInstance()->Modify();

				if (DraggedNode->ParentNode)
				{
					DraggedNode->ParentNode->Modify();
					if (DraggedNode->ParentNode->GetCastedNodeInstance())
						DraggedNode->ParentNode->
						             GetCastedNodeInstance()->Modify();
				}
			}
		}

		for (int32 Idx = 0; Idx < DraggedNodes.Num(); Idx++)
		{
			UJointEdGraphNode* DraggedNode = Cast<UJointEdGraphNode>(DraggedNodes[Idx]->GetNodeObj());
			if (DraggedNode && DraggedNode->ParentNode)
			{
				
				//Remove the dragged node from the original parent node.
				UJointEdGraphNode* RemoveFrom = DraggedNode->ParentNode;
				UJointEdGraphNode* AddTo = DropTargetNode;

				//Lock Update if remove node action is done in the same node.
				RemoveFrom->RemoveSubNode(DraggedNode, true);

				//attach the dragged node to the graph node that this node is referring.
				AddTo->AddSubNode(DraggedNode, true);


				RemoveFrom->Update();

				if (RemoveFrom != AddTo)
				{
					AddTo->Update();
				}

				if (GraphNode && GraphNode->GetGraph())
				{
					GraphNode->GetGraph()->NotifyGraphChanged();
				}
				
				if (DraggedNode->GetGraphNodeSlate().IsValid())
				{
					TSharedPtr<SJointGraphNodeBase> DraggedNodeSlate = DraggedNode->GetGraphNodeSlate().Pin();
				
					DraggedNodeSlate->PlayDropAnimation();
					DraggedNodeSlate->PlayNodeBackgroundColorResetAnimationIfPossible();	
				}

				TArray<UJointEdGraphNode*> SubSubNodes = DraggedNode->GetAllSubNodesInHierarchy();

				for (UJointEdGraphNode* SubSubNode : SubSubNodes)
				{
					TSharedPtr<SJointGraphNodeBase> SubSubNodeSlate = SubSubNode->GetGraphNodeSlate().Pin();
					
					if (SubSubNodeSlate)
					{
						SubSubNodeSlate->PlayNodeBackgroundColorResetAnimationIfPossible();
					}
					
				}
			}
		}

		GEditor->EndTransaction();

		VOLT_STOP_ALL_ANIM(NodeBody);

		const UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
		(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
			.StartWidgetTransform(FWidgetTransform(
				                                                 FVector2D::ZeroVector,
				                                                 FVector2D(0.9, 0.9),
				                                                 FVector2D::ZeroVector,
				                                                 0))
			.RateBasedInterpSpeed(10)
		);

		VOLT_PLAY_ANIM(NodeBody, Anim);

		return FReply::Handled();
	}


	return SGraphNode::OnDrop(MyGeometry, DragDropEvent);
}

FReply SJointGraphNodeBase::OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	if (!GetOwnerPanel().IsValid()) return FReply::Handled();

	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !(GEditor->bIsSimulatingInEditor || GEditor->PlayWorld))
	{
		if (MouseDownScreenPosition != JointGraphNodeDragDropOperationDefs::NullDragPosition
			// Check whether we have a valid drag drop action here.
			&& FVector2D::Distance(MouseDownScreenPosition, MouseEvent.GetScreenSpacePosition()) > DragStartDistance)
		{
			//if we are holding mouse over a subnode
			UJointEdGraphNode* TestNode = Cast<UJointEdGraphNode>(GraphNode);
			if (TestNode && TestNode->IsSubNode())
			{
				const TSharedRef<SGraphPanel>& Panel = GetOwnerPanel().ToSharedRef();
				const TSharedRef<SGraphNode>& Node = SharedThis(this);


				TSharedRef<FDragJointGraphNode> DragOperation_GraphNode = FDragJointGraphNode::New(Panel, Node);

				OnDragStarted();

				return FReply::Handled().BeginDragDrop(DragOperation_GraphNode);
			}
		}
	}

	if (bUserIsDragging)
	{
		FVector2D GraphSpaceCoordinates = NodeCoordToGraphCoord(MouseEvent.GetScreenSpacePosition());
		FVector2D OldGraphSpaceCoordinates = NodeCoordToGraphCoord(MouseEvent.GetLastScreenSpacePosition());
		TSharedPtr<SWindow> OwnerWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
		FVector2D Delta = (GraphSpaceCoordinates - OldGraphSpaceCoordinates) / (OwnerWindow.IsValid()
			                                                                        ? OwnerWindow->GetDPIScaleFactor()
			                                                                        : 1.0f);


		//Clamp delta value based on resizing direction
		if (MouseZone == CRWZ_LeftBorder || MouseZone == CRWZ_RightBorder)
		{
			Delta.Y = 0.0f;
		}
		else if (MouseZone == CRWZ_TopBorder || MouseZone == CRWZ_BottomBorder)
		{
			Delta.X = 0.0f;
		}

		//Resize node delta value
		FVector2D DeltaNodeSize = Delta;

		//Modify node size delta value based on resizing direction
		if ((MouseZone == CRWZ_LeftBorder) || (MouseZone == CRWZ_TopBorder) || (MouseZone == CRWZ_TopLeftBorder))
		{
			DeltaNodeSize = -DeltaNodeSize;
		}
		else if (MouseZone == CRWZ_TopRightBorder)
		{
			DeltaNodeSize.Y = -DeltaNodeSize.Y;
		}
		else if (MouseZone == CRWZ_BottomLeftBorder)
		{
			DeltaNodeSize.X = -DeltaNodeSize.X;
		}
		// Apply delta unfiltered to DragSize
		DragSize.X += DeltaNodeSize.X;
		DragSize.Y += DeltaNodeSize.Y;
		// apply snap
		const float SnapSize = UJointEditorSettings::GetJointGridSnapSize();

		FVector2D SnappedSize;
		SnappedSize.X = SnapSize * FMath::RoundToFloat(DragSize.X / SnapSize);
		SnappedSize.Y = SnapSize * FMath::RoundToFloat(DragSize.Y / SnapSize);

		// Enforce min/max sizing
		const FVector2D MinSize = GetNodeMinimumSize();
		SnappedSize.X = FMath::Max(SnappedSize.X, MinSize.X);
		SnappedSize.Y = FMath::Max(SnappedSize.Y, MinSize.Y);

		const FVector2D MaxSize = GetNodeMaximumSize();
		SnappedSize.X = FMath::Min(SnappedSize.X, MaxSize.X);
		SnappedSize.Y = FMath::Min(SnappedSize.Y, MaxSize.Y);

#if UE_VERSION_OLDER_THAN(5, 6, 0)
		FVector2D DeltaNodePos(0, 0);
#else
		FVector2f DeltaNodePos(0, 0);
#endif

		if (UserSize != SnappedSize)
		{
			//Modify node position (resizing top and left sides)
			if (MouseZone != CRWZ_BottomBorder && MouseZone != CRWZ_RightBorder && MouseZone != CRWZ_BottomRightBorder)
			{
				//Delta value to move graph node position
				DeltaNodePos = UserSize - SnappedSize;

				//Clamp position delta based on resizing direction
				if (MouseZone == CRWZ_BottomLeftBorder)
				{
					DeltaNodePos.Y = 0.0f;
				}
				else if (MouseZone == CRWZ_TopRightBorder)
				{
					DeltaNodePos.X = 0.0f;
				}
			}
			UserSize = SnappedSize;
			GraphNode->ResizeNode(UserSize);
			DeltaNodePos = GetCorrectedNodePosition() - GetPosition();
		}

		if (!ResizeTransactionPtr.IsValid() && UserSize != StoredUserSize)
		{
			// Start resize transaction.  The transaction is started here so all MoveTo actions are captured while empty
			//	transactions are not created
			ResizeTransactionPtr = MakeShareable(
				new FScopedTransaction(NSLOCTEXT("JointEdTransaction", "TransactionTitle_ResizeNodeAction",
				                                 "Resize node")));
		}

		SGraphNode::FNodeSet NodeFilter;

#if UE_VERSION_OLDER_THAN(5, 6, 0)
		SGraphNode::MoveTo(GetPosition() + DeltaNodePos, NodeFilter);
#else
		SGraphNode::MoveTo(GetPosition2f() + DeltaNodePos, NodeFilter);
#endif
	}
	else
	{
		const FVector2D LocalMouseCoordinates = SenderGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		MouseZone = FindMouseZone(LocalMouseCoordinates);
	}
	return SGraphNode::OnMouseMove(SenderGeometry, MouseEvent);
}


void SJointGraphNodeBase::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SGraphNodeResizable::OnMouseEnter(MyGeometry, MouseEvent);
}

void SJointGraphNodeBase::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	ResetDragMousePos();

	SGraphNodeResizable::OnMouseLeave(MouseEvent);
}

FReply SJointGraphNodeBase::OnMouseButtonUp(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	ResetDragMousePos();

	return SGraphNodeResizable::OnMouseButtonUp(SenderGeometry, MouseEvent);
}


FReply SJointGraphNodeBase::OnMouseButtonDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	CacheMouseDownPos(MouseEvent);

	return SGraphNodeResizable::OnMouseButtonDown(SenderGeometry, MouseEvent);
}

void SJointGraphNodeBase::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	if (TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation(); Operation.IsValid())
	{
		if (Operation->IsOfType<FDragJointGraphNode>())
		{
			TSharedPtr<FDragJointGraphNode> DragConnectionOp = StaticCastSharedPtr<FDragJointGraphNode>(Operation);

			// Inform the Drag and Drop operation that we are hovering over this node.
			//TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, DragDropEvent);
			//DragConnectionOp->SetHoveredNode(SubNode.IsValid() ? SubNode : SharedThis(this));
			DragConnectionOp->SetHoveredNode(SharedThis(this));
			

			UJointEdGraphNode* TestNode = Cast<UJointEdGraphNode>(GraphNode);

			if (!DragConnectionOp->DragOverNodes.Contains(SharedThis(this)))
				DragConnectionOp->DragOverNodes.Add(
					SharedThis(this));


			VOLT_STOP_ANIM(NodeBodyTransformTrack);

			UVoltAnimation* ExpandAnimation = VOLT_MAKE_ANIMATION()
			(
				VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
				.InterpolationMode(EVoltInterpMode::AlphaBased)
				.AlphaBasedDuration(0.5)
				.AlphaBasedBlendExp(6)
				.AlphaBasedEasingFunction(EEasingFunc::CircularOut)
				.TargetWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(1.002, 1.002),
				                                        FVector2D::ZeroVector, 0))
			);

			NodeBodyTransformTrack = VOLT_PLAY_ANIM(NodeBody, ExpandAnimation);

			SharedThis(this)->PlayInsertPointHighlightAnimation();
		}
		else if (Operation->IsOfType<FGraphEditorDragDropAction>())
		{
			// Is someone dragging a connection?

			// Inform the Drag and Drop operation that we are hovering over this pin.
			TSharedPtr<FGraphEditorDragDropAction> DragConnectionOp = StaticCastSharedPtr<FGraphEditorDragDropAction>(Operation);
			if (!IsSubNodeWidget())
			{
				DragConnectionOp->SetHoveredNode(SharedThis(this));
			}
		}else
		{
			SGraphNode::OnDragEnter(MyGeometry, DragDropEvent);
		}
	}
	//Internal execution blows up the Joint graph action.
	//SGraphNode::OnDragEnter(MyGeometry, DragDropEvent);
}

void SJointGraphNodeBase::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	if (TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation(); Operation.IsValid())
	{
		if (DragDropEvent.GetOperationAs<FDragJointGraphNode>())
		{
			TSharedPtr<FDragJointGraphNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragJointGraphNode>();

			if (DragConnectionOp.IsValid() && DragConnectionOp->DragOverNodes.Contains(SharedThis(this))) DragConnectionOp->DragOverNodes.Remove(SharedThis(this));

			VOLT_STOP_ANIM(NodeBodyTransformTrack);

			const UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
			(
				VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
				.InterpolationMode(EVoltInterpMode::AlphaBased)
				.AlphaBasedDuration(0.5)
				.AlphaBasedBlendExp(6)
				.AlphaBasedEasingFunction(EEasingFunc::CircularIn)
				.TargetWidgetTransform(FWidgetTransform(
					                                                 FVector2D::ZeroVector,
					                                                 FVector2D(1, 1),
					                                                 FVector2D::ZeroVector,
					                                                 0))
			);

			NodeBodyTransformTrack = VOLT_PLAY_ANIM(NodeBody, Anim);
		}
		else if (DragDropEvent.GetOperationAs<FGraphEditorDragDropAction>())
		{
			TSharedPtr<FGraphEditorDragDropAction> DragConnectionOp = DragDropEvent.GetOperationAs<FGraphEditorDragDropAction>();

			if (!IsSubNodeWidget())
			{
				DragConnectionOp->SetHoveredNode(nullptr);
			}
		}
	}
}

FReply SJointGraphNodeBase::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	
	// Is someone dragging a node?
	TSharedPtr<FDragJointGraphNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragJointGraphNode>();
	if (DragConnectionOp.IsValid())
	{
		// Inform the Drag and Drop operation that we are hovering over this node.
		//TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, DragDropEvent);
		DragConnectionOp->SetHoveredNode(SharedThis(this));

		return FReply::Handled();
	}
	
	
	return FReply::Unhandled();
	//return SGraphNode::OnDragOver(MyGeometry, DragDropEvent);
}

void SJointGraphNodeBase::OnDragStarted()
{
	VOLT_STOP_ALL_ANIM(SharedThis(this));

	UVoltAnimation* Animation = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
		.TargetWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(0.95, 0.95), FVector2D::ZeroVector, 0))
	);

	VOLT_PLAY_ANIM(SharedThis(this), Animation);
}

void SJointGraphNodeBase::OnDragEnded()
{
	VOLT_STOP_ALL_ANIM(SharedThis(this));

	UVoltAnimation* Animation = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
		.TargetWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(1, 1), FVector2D::ZeroVector, 0))
	);

	VOLT_PLAY_ANIM(SharedThis(this), Animation);
}

void SJointGraphNodeBase::ResetDragMousePos()
{
	MouseDownScreenPosition = JointGraphNodeDragDropOperationDefs::NullDragPosition;
}

void SJointGraphNodeBase::CacheMouseDownPos(const FPointerEvent& MouseEvent)
{
	MouseDownScreenPosition = MouseEvent.GetScreenSpacePosition();
}

void SJointGraphNodeBase::OnRenameTextCommited(const FText& InText, ETextCommit::Type CommitInfo)
{
	OnTextCommitted.ExecuteIfBound(InText, CommitInfo, GraphNode);

	UpdateErrorInfo();

	if (ErrorReporting.IsValid())
	{
		ErrorReporting->SetError(ErrorMsg);
	}
	
	if (InlineEditableText)
	{
		InlineEditableText->SetText(GetGraphNodeName());
	}
}

/*
FNavigationReply SJointGraphNodeBase::OnNavigation(const FGeometry& MyGeometry,
	const FNavigationEvent& InNavigationEvent)
{
	EUINavigation Navigation = InNavigationEvent.GetNavigationType();
	
	//It can have 2 actions.

	//1. When player pressed Up or Down: It will move up or down on the level side.
	//2. When player pressed Left or Right: It will iterate to left or right to the nodes on the same parent.
	
	if(Navigation == EUINavigation::Up)
	{
		//1. If it has any children widget, focus on the first / last children according to the navigation type.

		TSharedPtr<SWidget> ParentWidget = GetParentWidget();

		if(ParentWidget && ParentWidget->GetTypeAsString() == "SJointGraphNodeBase" && ParentWidget->GetTypeAsString() == "SJointGraphNodeSubNodeBase")
		{
			TSharedPtr<SJointGraphNodeBase> GraphNodeBase = StaticCastSharedPtr<SJointGraphNodeBase>(ParentWidget);
			
			if (FGraphSelectionManager* SelectionManager = GetSelectionManager())
			{
				SelectionManager->SelectedNodes.Add(GraphNodeBase->GetCastedGraphNode());
			}
		}
		
	}if(Navigation == EUINavigation::Down){
		
	}else
	{
		//2. if it has parent widget, focus on the next children of this.
		if(GetParentWidget())
		{
		
			if(FChildren* InChildren = GetParentWidget()->GetChildren())
			{
				const int& SlotCount = InChildren->Num();
		
				for( int i = 0; i < SlotCount; ++i )
				{
					TSharedRef<SWidget> ChildrenWidget = InChildren->GetChildAt(i);

					//if(ChildrenWidget)
				}
			}

			//3. if it's the last children, focus the parent node again.
		
		}	
	}
	
	
	return SGraphNodeResizable::OnNavigation(MyGeometry, InNavigationEvent);
}
*/

void SJointGraphNodeBase::UpdateErrorInfo()
{
	//We don't use graph node's original properties for the error reporting. Reset it thus.
	ErrorColor = FLinearColor(0, 0, 0);
	ErrorMsg.Empty();

	//Update CompileResultOverlay with the compile result.
	//TODO: Change it to populate the overlay only when it's needed.
	if (const UJointEdGraphNode* InGraphNode = GetCastedGraphNode())
	{
		const TArray<TSharedPtr<class FTokenizedMessage>>& Messages = InGraphNode->CompileMessages;

		if (Messages.Num() > 0)
		{
			if (!CompileResultOverlay) AssignCompileResultOverlay();

			CompileResultOverlay->UpdateWidgetWithCompileResult(InGraphNode->CompileMessages);
		}
		else
		{
			RemoveCompileResultOverlay();
		}
	}
}

void SJointGraphNodeBase::UpdateBreakpoint()
{
	if (UJointEdGraphNode* InGraphNode = GetCastedGraphNode())
	{
		if (const FJointNodeDebugData* DebugData = UJointDebugger::GetDebugDataForInstance(InGraphNode); DebugData != nullptr)
		{
			if (DebugData->bHasBreakpoint)
			{
				if (!BreakpointOverlay.IsValid())
				{
					AssignBreakpointOverlay();
				}

				return;
			}
		}
	}

	RemoveBreakpointOverlay();
}

void SJointGraphNodeBase::UpdateBuildTargetPreset()
{
	if (UJointEdGraphNode* InGraphNode = GetCastedGraphNode())
	{
		if (UJointNodeBase* NodeInstance = InGraphNode->GetCastedNodeInstance())
		{
			if (!NodeInstance->GetBuildPreset().IsNull())
			{
				if (!BuildPresetOverlay.IsValid())
				{
					AssignBuildTargetPresetOverlay();
				}

				BuildPresetOverlay->Update();

				return;
			}
		}
	}

	RemoveBuildPresetOverlay();
}

void SJointGraphNodeBase::UpdateNameBox()
{
	// populate the dissolve indicator if needed.
	if (NameBox)
	{
		//iterate over the slots of NameBox and remove the dissolve indicator if exists.
		if (DissolveIndicator)
		{
			NameBox->RemoveSlot(DissolveIndicator.ToSharedRef());
		}

		DissolveIndicator = nullptr;

		// Check whether the node has dissolved subnodes.
		DissolvedSubnodeCounts = 0;

		if (UJointEdGraphNode* InGraphNode = GetCastedGraphNode())
		{
			for (UJointEdGraphNode* SubNode : InGraphNode->SubNodes)
			{
				UJointEdGraphNode_Fragment* SubNodeAsFragment = Cast<UJointEdGraphNode_Fragment>(SubNode);

				if (!SubNodeAsFragment) continue;

				if (SubNodeAsFragment->IsDissolvedSubNode()) DissolvedSubnodeCounts++;
			}
		}

		if (DissolvedSubnodeCounts > 0)
		{
			NameBox->AddSlot()
			       .AutoWidth()
			       .HAlign(HAlign_Left)
			       .VAlign(VAlign_Center)
			       .Padding(FJointEditorStyle::Margin_Normal)
			[
				CreateDissolvedSubNodeIndication()
			];
		}
	}
}

UJointEdGraphNode* SJointGraphNodeBase::GetCastedGraphNode() const
{
	if (!GraphNode) return nullptr;

	return Cast<UJointEdGraphNode>(GraphNode);
}

FText SJointGraphNodeBase::GetIndexTooltipText()
{
	FText Result = FText::GetEmpty();

	if (!this->GraphNode) { return Result; }

	if (!Cast<UJointEdGraphNode>(this->GraphNode)) { return Result; }

	return Cast<UJointEdGraphNode>(this->GraphNode)->GetPriorityIndicatorTooltipText();
}

EVisibility SJointGraphNodeBase::GetIndexVisibility()
{
	EVisibility Result = EVisibility::Collapsed;

	if (!this->GraphNode) { return Result; }

	if (!Cast<UJointEdGraphNode>(this->GraphNode)) { return Result; }

	return Cast<UJointEdGraphNode>(this->GraphNode)->GetPriorityIndicatorVisibility();
}

FText SJointGraphNodeBase::GetIndexText()
{
	FText Result = FText::GetEmpty();

	if (!this->GraphNode) { return Result; }

	if (!Cast<UJointEdGraphNode>(this->GraphNode)) { return Result; }

	return Cast<UJointEdGraphNode>(this->GraphNode)->GetPriorityIndicatorText();
}

TSharedRef<SGraphNode> SJointGraphNodeBase::GetNodeUnderMouse(const FGeometry& MyGeometry,
                                                              const FPointerEvent& MouseEvent)
{
	TSharedPtr<SGraphNode> ResultNode;

	// We just need to find the one WidgetToFind among our descendants.
	TSet<TSharedRef<SWidget>> SubWidgetsSet;
	for (int32 i = 0; i < SubNodes.Num(); i++) { SubWidgetsSet.Add(SubNodes[i].ToSharedRef()); }

	TMap<TSharedRef<SWidget>, FArrangedWidget> Result;
	FindChildGeometries(MyGeometry, SubWidgetsSet, Result);

	if (Result.Num() > 0)
	{
		FArrangedChildren ArrangedChildren(EVisibility::Visible);
		Result.GenerateValueArray(ArrangedChildren.GetInternalArray());

		const int32 HoveredIndex = SWidget::FindChildUnderMouse(ArrangedChildren, MouseEvent);
		if (HoveredIndex != INDEX_NONE)
		{
			ResultNode = StaticCastSharedRef<SGraphNode>(ArrangedChildren[HoveredIndex].Widget);
		}
	}

	return ResultNode.IsValid() ? ResultNode.ToSharedRef() : SGraphNode::GetNodeUnderMouse(MyGeometry, MouseEvent);
}

FChildren* SJointGraphNodeBase::GetChildren()
{
	//if(SubNodeBox) return SubNodeBox->GetChildren();

	return SGraphNodeResizable::GetChildren();
}

void SJointGraphNodeBase::OnGraphSelectionChanged(const TSet<UObject*>& NewSelection)
{
	PlaySelectionAnimation();
}

void SJointGraphNodeBase::PlayHighlightAnimation(bool bBlinkForOnce, float SpeedMultiplier)
{
	if (!NodeBackground || !NodeBackground->InnerBorder) return;

	VOLT_STOP_ANIM(NodeBackground->InnerBorderBackgroundColorTrack);

	const UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_Sequence)
		.bShouldLoop(!bBlinkForOnce)
		//.bShouldLoop(false)
		(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
			.RateBasedInterpSpeed(25 * SpeedMultiplier)
			.TargetColor(GetNodeBodyBackgroundColor() + FLinearColor(0.3, 0.3, 0.3, 0.3)),
			VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
			.RateBasedInterpSpeed(16 * SpeedMultiplier)
			.TargetColor(GetNodeBodyBackgroundColor())
		)
	);

	NodeBackground->InnerBorderBackgroundColorTrack = VOLT_PLAY_ANIM(NodeBackground->InnerBorder, Anim);
}

void SJointGraphNodeBase::StopHighlightAnimation()
{
	if (!NodeBackground || !NodeBackground->InnerBorder) return;

	VOLT_STOP_ANIM(NodeBackground->InnerBorderBackgroundColorTrack);

	const UVoltAnimation* ColorAnimation = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
		.TargetColor(GetNodeBodyBackgroundColor())
	);

	NodeBackground->InnerBorderBackgroundColorTrack = VOLT_PLAY_ANIM(NodeBackground->InnerBorder, ColorAnimation);
}

void SJointGraphNodeBase::PlayNodeBackgroundColorResetAnimationIfPossible(bool bInstant)
{
	if (!NodeBackground) return;

	VOLT_STOP_ANIM(NodeBackground->InnerBorderBackgroundColorTrack);

	const UVoltAnimation* ColorAnimation = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpColor)
		.InterpolationMode(EVoltInterpMode::AlphaBased)
		.AlphaBasedDuration(bInstant ? 0.05 : 0.4)
		.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
		.AlphaBasedBlendExp(6)
		.TargetColor(GetNodeBodyBackgroundColor())
	);

	NodeBackground->InnerBorderBackgroundColorTrack = VOLT_PLAY_ANIM(NodeBackground->InnerBorder, ColorAnimation);
}

void SJointGraphNodeBase::PlaySelectionAnimation()
{
	if (!NodeBackground.IsValid()) return;
	
	if (NodeBackground)
	{
		//Update animation.
		if (CheckWhetherNodeSelected())
		{
			FLinearColor NormalColor;
			FLinearColor HoverColor;
			FLinearColor OutlineNormalColor;
			FLinearColor OutlineHoverColor;

			GetNodeColorScheme(true, NormalColor, HoverColor, OutlineNormalColor, OutlineHoverColor);

			NodeBackground->NormalColor = NormalColor;
			NodeBackground->HoverColor = HoverColor;
			NodeBackground->OutlineNormalColor = OutlineNormalColor;
			NodeBackground->OutlineHoverColor = OutlineHoverColor;

			NodeBackground->PlayUnHoverAnimation();
		}
		else
		{
			FLinearColor NormalColor;
			FLinearColor HoverColor;
			FLinearColor OutlineNormalColor;
			FLinearColor OutlineHoverColor;

			GetNodeColorScheme(false, NormalColor, HoverColor, OutlineNormalColor, OutlineHoverColor);

			NodeBackground->NormalColor = NormalColor;
			NodeBackground->HoverColor = HoverColor;
			NodeBackground->OutlineNormalColor = OutlineNormalColor;
			NodeBackground->OutlineHoverColor = OutlineHoverColor;

			NodeBackground->PlayUnHoverAnimation();
		}
	}
}

void SJointGraphNodeBase::PlayDebuggerAnimation(bool bIsPausedFrom, bool bIsBeginPlay, bool bIsPending, bool bIsEndPlay)
{
	if (bIsPausedFrom)
	{
		PlayNodeBodyColorAnimation(
			UJointEditorSettings::Get()->DebuggerPausedNodeColor,
			UJointEditorSettings::Get()->DebuggerPausedNodeColor * 0.2f,
			0.8f,
			true);
	}
	else if (bIsBeginPlay)
	{
		PlayNodeBodyScaleAnimation(1.035f, 0.22f);
		SetNodeBodyToDebuggerExecutedImage();
		PlayNodeBodyColorAnimation(
			UJointEditorSettings::Get()->DebuggerPlayingNodeColor,
			UJointEditorSettings::Get()->DebuggerPlayingNodeColor * 0.90f,
			0.4f,
			true);
	}
	else if (bIsPending)
	{
		SetNodeBodyToDebuggerExecutedImage();
		PlayNodeBodyColorAnimation(
			UJointEditorSettings::Get()->DebuggerPendingNodeColor,
			UJointEditorSettings::Get()->DebuggerPendingNodeColor * 0.90f,
			0.4f,
			true);
	}
	else if (bIsEndPlay)
	{
		SetNodeBodyToDebuggerExecutedImage();
		PlayNodeBodyColorAnimation(
		UJointEditorSettings::Get()->DebuggerEndedNodeColor,
		UJointEditorSettings::Get()->DebuggerEndedNodeColor,
			0.4f,
			false);
	}
}

void SJointGraphNodeBase::UpdateDebuggerAnimationByState()
{
	if (GetCastedGraphNode() && GetCastedGraphNode()->GetCastedNodeInstance())
	{
		if (const UJointEditorSettings* EditorSettings = UJointEditorSettings::Get())
		{
			UJointNodeBase* NodeInstance = GetCastedGraphNode()->GetCastedNodeInstance();

			if (NodeInstance->IsNodeBegunPlay() && !NodeInstance->IsNodePending() && !NodeInstance->IsNodeEndedPlay())
			{
				PlayDebuggerAnimation(false, true, false, false);
			}
			else if (NodeInstance->IsNodeBegunPlay() && NodeInstance->IsNodePending() && !NodeInstance->
				IsNodeEndedPlay())
			{
				PlayDebuggerAnimation(false, false, true, false);
			}
			else if (NodeInstance->IsNodeBegunPlay() && NodeInstance->IsNodePending() && NodeInstance->
				IsNodeEndedPlay())
			{
				PlayDebuggerAnimation(false, false, false, true);
			}
		}
	}
}

void SJointGraphNodeBase::SetNodeBodyToDebuggerExecutedImage()
{
	if (!NodeBody.IsValid()) return;
	
	if (NodeBody.IsValid())
	{
		const FSlateBrush* InBorderImage = FJointEditorStyle::Get().GetBrush(
			GetSlateDetailLevel() == EJointEdSlateDetailLevel::SlateDetailLevel_Stow 
				? "JointUI.Border.Sphere" 
				: "JointUI.Border.Round"
		);
	
		NodeBody->SetBorderImage(InBorderImage);
	}
}

void SJointGraphNodeBase::PlayNodeBodyScaleAnimation(float Scale, float Duration)
{
	if (!NodeBody.IsValid()) return;
	
	if (NodeBody.IsValid())
	{
		VOLT_STOP_ANIM(NodeBodyColorTrack);

		if (NodeBody)
		{
			UVoltAnimation* Animation = nullptr;

			Animation = VOLT_MAKE_ANIMATION()
				(
					VOLT_MAKE_MODULE(UVolt_ASM_Sequence)
					.bShouldLoop(false)
					(
						VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
						.InterpolationMode(EVoltInterpMode::AlphaBased)
						.AlphaBasedDuration(Duration * 0.5)
						.AlphaBasedEasingFunction(EEasingFunc::SinusoidalOut)
						.TargetWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(Scale, Scale), FVector2D::ZeroVector, 0)),
						VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
						.InterpolationMode(EVoltInterpMode::AlphaBased)
						.AlphaBasedDuration(Duration * 0.5)
						.AlphaBasedEasingFunction(EEasingFunc::SinusoidalOut)
						.TargetWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(1, 1), FVector2D::ZeroVector, 0))
					)
				);

			NodeBodyTransformTrack = VOLT_PLAY_ANIM(NodeBody, Animation);
		}
	}
}

void SJointGraphNodeBase::PlayNodeBodyColorAnimation(const FLinearColor Color, const FLinearColor BlinkTargetColor, float Duration, const bool bBlink = false)
{
	if (!NodeBody.IsValid()) return;

	if (NodeBody.IsValid())
	{
		VOLT_STOP_ANIM(NodeBodyColorTrack);

		UVoltAnimation* ColorAnimation = nullptr;

		if (bBlink)
		{
			ColorAnimation = VOLT_MAKE_ANIMATION()
			(
				VOLT_MAKE_MODULE(UVolt_ASM_Sequence)
				.bShouldLoop(true)
				(
					VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
					.InterpolationMode(EVoltInterpMode::AlphaBased)
					.AlphaBasedDuration(Duration * 0.5)
					.AlphaBasedEasingFunction(EEasingFunc::SinusoidalOut)
					.TargetColor(Color),
					VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
					.InterpolationMode(EVoltInterpMode::AlphaBased)
					.AlphaBasedDuration(Duration * 0.5)
					.AlphaBasedEasingFunction(EEasingFunc::SinusoidalOut)
					.TargetColor(BlinkTargetColor)
				)
			);
		}
		else
		{
			ColorAnimation = VOLT_MAKE_ANIMATION()
			(
				VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
				.InterpolationMode(EVoltInterpMode::AlphaBased)
				.AlphaBasedDuration(Duration)
				.AlphaBasedEasingFunction(EEasingFunc::SinusoidalOut)
				.TargetColor(BlinkTargetColor)
			);
		}

		NodeBodyColorTrack = VOLT_PLAY_ANIM(NodeBody, ColorAnimation);
	}
}

void SJointGraphNodeBase::ResetNodeBodyColorAnimation()
{
	if (!NodeBackground.IsValid()) return;

	if (NodeBackground.IsValid())
	{
		VOLT_STOP_ANIM(NodeBodyColorTrack);

		UVoltAnimation* ColorAnimation = VOLT_MAKE_ANIMATION()
		(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
			.InterpolationMode(EVoltInterpMode::AlphaBased)
			.AlphaBasedDuration(0.3)
			.AlphaBasedEasingFunction(EEasingFunc::SinusoidalOut)
			.TargetColor(FJointEditorStyle::Color_Node_Shadow)
		);

		NodeBodyColorTrack = VOLT_PLAY_ANIM(NodeBody, ColorAnimation);
	}
}

void SJointGraphNodeBase::PlayDropAnimation()
{
	if (!NodeBody.IsValid()) return;

	if (NodeBody.IsValid())
	{
		PlayHighlightAnimation(true);

		VOLT_STOP_ANIM(NodeBodyTransformTrack);

		const UVoltAnimation* ExpandAnimation = VOLT_MAKE_ANIMATION()
		(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
			.bUseStartWidgetTransform(true)
			.RateBasedInterpSpeed(8)
			.StartWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(1.1, 0.4), FVector2D::ZeroVector, 0))
			.TargetWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(1, 1), FVector2D::ZeroVector, 0))
		);

		NodeBodyTransformTrack = VOLT_PLAY_ANIM(NodeBody, ExpandAnimation);
	}
}

void SJointGraphNodeBase::PlayInsertAnimation()
{
	if (!NodeBody.IsValid()) return;
	
	if (NodeBody.IsValid())
	{
		PlayHighlightAnimation(true);

		VOLT_STOP_ANIM(NodeBodyTransformTrack);

		const UVoltAnimation* ExpandAnimation = VOLT_MAKE_ANIMATION()
		(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
			.bUseStartWidgetTransform(true)
			.RateBasedInterpSpeed(8)
			.StartWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(1.1, 0.4), FVector2D::ZeroVector,
												   (FMath::FRand() - 0.5f) * 20))
			.TargetWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(1, 1), FVector2D::ZeroVector, 0))
		);

		NodeBodyTransformTrack = VOLT_PLAY_ANIM(NodeBody, ExpandAnimation);
	}
}


void SJointGraphNodeBase::PlayInsertPointHighlightAnimation()
{
	const float DelayTotalTime = 0.5;

	const int InsertPointsNum = InsertPoints.Num();

	float AccumlatedInterval = 0;

	for (TSharedPtr<SJointGraphNodeInsertPoint> JointGraphNodeInsertPoint : InsertPoints)
	{
		if (!JointGraphNodeInsertPoint.IsValid()) continue;

		JointGraphNodeInsertPoint->Highlight(AccumlatedInterval);

		AccumlatedInterval += DelayTotalTime / InsertPointsNum;
	}
}

void SJointGraphNodeBase::InitializeVoltVariables()
{
	if (!NodeBackground) return;

	FLinearColor NormalColor;
	FLinearColor HoverColor;
	FLinearColor OutlineNormalColor;
	FLinearColor OutlineHoverColor;

	GetNodeColorScheme(false, NormalColor, HoverColor, OutlineNormalColor, OutlineHoverColor);

	if (NodeBackground->InnerBorder)
	{
		const UVoltAnimation* ContentAnim = VOLT_MAKE_ANIMATION()(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
			.InterpolationMode(EVoltInterpMode::AlphaBased)
			.AlphaBasedDuration(KINDA_SMALL_NUMBER)
			.TargetColor(NormalColor)
		);

		VOLT_PLAY_ANIM(NodeBackground->InnerBorder, ContentAnim);
	}

	if (NodeBackground->OuterBorder)
	{
		const UVoltAnimation* ContentAnim = VOLT_MAKE_ANIMATION()(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
			.InterpolationMode(EVoltInterpMode::AlphaBased)
			.AlphaBasedDuration(KINDA_SMALL_NUMBER)
			.TargetColor(OutlineNormalColor)
		);

		VOLT_PLAY_ANIM(NodeBackground->OuterBorder, ContentAnim);
	}
}

const FSlateBrush* SJointGraphNodeBase::GetShadowBrush(bool bSelected) const
{
	return FStyleDefaults::GetNoBrush();
}


const EJointEdSlateDetailLevel::Type SJointGraphNodeBase::GetSlateDetailLevel() const
{
	if (UJointEdGraphNode* CastedGraphNode = GetCastedGraphNode())
	{
		return CastedGraphNode->SlateDetailLevel.GetValue();
	}

	return EJointEdSlateDetailLevel::SlateDetailLevel_Maximum;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<SWidget> SJointGraphNodeBase::CreateCenterContentBox()
{
	return SAssignNew(CenterContentBox, SVerticalBox)
		.Visibility(EVisibility::SelfHitTestInvisible);
}

void SJointGraphNodeBase::PopulatePinWidgets()
{
	if (LeftNodeBox.IsValid() && RightNodeBox.IsValid())
	{
		InputPins.Empty();
		OutputPins.Empty();

		LeftNodeBox->ClearChildren();
		RightNodeBox->ClearChildren();

		CreatePinWidgets();
		CreateSubNodePinWidgets();
	}
}

void SJointGraphNodeBase::CreateSubNodePinWidgets()
{
	UJointEdGraphNode* Node = GetCastedGraphNode();

	if (!Node) return;

	//Revert the action if the graph node is sub node. This action is only allowed on the base node type.
	if (Node->IsSubNode()) return;

	//Revert if the pin boxes are not available for some rare occasion.
	if (!(LeftNodeBox.IsValid() && RightNodeBox.IsValid())) return;

	TArray<UEdGraphPin*> SavedPins = Node->Pins;

	TMap<UEdGraphNode*, TSharedPtr<SJointGraphPinOwnerNodeBox>> LeftPinNodeBoxes;
	TMap<UEdGraphNode*, TSharedPtr<SJointGraphPinOwnerNodeBox>> RightPinNodeBoxes;


	for (UEdGraphPin* Pin : SavedPins)
	{
		if (!Pin) continue;
		if (!Pin->GetOwningNode()) continue;
		if (Node->CheckPinIsOriginatedFromThis(Pin)) continue; // revert if this pin is implemented in the node itself.

		UEdGraphPin* OriginalPin = Node->FindOriginalSubNodePin(Pin);

		if (!OriginalPin) continue; // revert


		UJointEdGraphNode* SubNode = nullptr;

		if (OriginalPin->GetOwningNode())
		{
			if (Cast<UJointEdGraphNode>(OriginalPin->GetOwningNode()))
			{
				SubNode = Cast<UJointEdGraphNode>(OriginalPin->GetOwningNode());
			}
		}

		if (SubNode == nullptr) continue;

		TSharedPtr<SJointGraphPinOwnerNodeBox> PinBoxToAdd;


		switch (Pin->Direction)
		{
		case EGPD_Input:

			if (!LeftPinNodeBoxes.Contains(SubNode))
			{
				PinBoxToAdd = SNew(SJointGraphPinOwnerNodeBox, SubNode,
				                   StaticCastSharedRef<SJointGraphNodeBase>(AsShared()));

				LeftPinNodeBoxes.Add(SubNode, PinBoxToAdd);

				LeftNodeBox->AddSlot()
				           .HAlign(HAlign_Fill)
				           .VAlign(VAlign_Fill)
				           .AutoHeight()
				[
					PinBoxToAdd.ToSharedRef()
				];
			}
			else
			{
				PinBoxToAdd = *LeftPinNodeBoxes.Find(SubNode);
			}

			break;

		case EGPD_Output:

			if (!RightPinNodeBoxes.Contains(SubNode))
			{
				PinBoxToAdd = SNew(SJointGraphPinOwnerNodeBox, SubNode,
				                   StaticCastSharedRef<SJointGraphNodeBase>(AsShared()));

				RightPinNodeBoxes.Add(SubNode, PinBoxToAdd);

				RightNodeBox->AddSlot()
				            .HAlign(HAlign_Fill)
				            .VAlign(VAlign_Fill)
				            .AutoHeight()
				[
					PinBoxToAdd.ToSharedRef()
				];
			}
			else
			{
				PinBoxToAdd = *RightPinNodeBoxes.Find(SubNode);
			}

			break;

		default:
			break;
		}

		TSharedPtr<SGraphPin> PinWidget = CreatePinWidget(Pin);

		PinBoxToAdd->AddPin(PinWidget.ToSharedRef());

		switch (Pin->Direction)
		{
		case EGPD_Input:

			InputPins.Add(PinWidget.ToSharedRef());

			break;

		case EGPD_Output:

			OutputPins.Add(PinWidget.ToSharedRef());

			break;

		default:
			break;
		}
	}
}

void SJointGraphNodeBase::CreatePinWidgets()
{
	UJointEdGraphNode* CastedGraphNode = GetCastedGraphNode();

	if (!CastedGraphNode) return;

	// Create Pin widgets for each of the pins.
	for (int32 PinIndex = 0; PinIndex < GraphNode->Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* CurPin = GraphNode->Pins[PinIndex];

		if (!ensureMsgf(CurPin->GetOuter() == GraphNode
		                , TEXT(
			                "Graph node ('%s' - %s) has an invalid %s pin: '%s'; (with a bad %s outer: '%s'); skiping creation of a widget for this pin."
		                )
		                , *GraphNode->GetNodeTitle(ENodeTitleType::ListView).ToString()
		                , *GraphNode->GetPathName()
		                , (CurPin->Direction == EEdGraphPinDirection::EGPD_Input) ? TEXT("input") : TEXT("output")
		                , CurPin->PinFriendlyName.IsEmpty() ? *CurPin->PinName.ToString() : *CurPin->PinFriendlyName.
		                ToString()
		                , CurPin->GetOuter() ? *CurPin->GetOuter()->GetClass()->GetName() : TEXT("UNKNOWN")
		                , CurPin->GetOuter() ? *CurPin->GetOuter()->GetPathName() : TEXT("NULL")))
		{
			continue;
		}

		if (!CastedGraphNode->CheckPinIsOriginatedFromThis(CurPin))
		{
			//It means that this pin is a pin for the sub node. skip the creation and handle in CreateSubNodePinWidgets().
			continue;
		}

		CreateStandardPinWidget(CurPin);
	}
}

void SJointGraphNodeBase::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();

	if (const bool bAdvancedParameter = (PinObj != nullptr) && PinObj->bAdvancedView && false /** We don't use it */)
	{
		PinToAdd->SetVisibility(TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced));
	}

	if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
	{
		LeftNodeBox->AddSlot()
		           .AutoHeight()
		           .HAlign(HAlign_Right)
		           .VAlign(VAlign_Center)
		           .Padding(FJointEditorStyle::Margin_Tiny)
		[
			PinToAdd
		];
		InputPins.Add(PinToAdd);
	}
	else // Direction == EEdGraphPinDirection::EGPD_Output
	{
		RightNodeBox->AddSlot()
		            .AutoHeight()
		            .HAlign(HAlign_Left)
		            .VAlign(VAlign_Center)
		            .Padding(FJointEditorStyle::Margin_Tiny)
		[
			PinToAdd
		];
		OutputPins.Add(PinToAdd);
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


FText SJointGraphNodeBase::GetGraphNodeName() const
{
	return this->GraphNode ? this->GraphNode->GetNodeTitle(ENodeTitleType::FullTitle) : FText::GetEmpty();
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<SWidget> SJointGraphNodeBase::CreateCenterWholeBox()
{
	EVisibility InVisibility = GetCastedGraphNode()
		                           ? GetCastedGraphNode()->CanHaveSubNode()
			                             ? EVisibility::SelfHitTestInvisible
			                             : EVisibility::Collapsed
		                           : EVisibility::Collapsed;

	SAssignNew(CenterWholeBox, SVerticalBox)
	.Visibility(EVisibility::SelfHitTestInvisible)
	+ SVerticalBox::Slot()
	.AutoHeight()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Top)
	[
		CreateNameBox()
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Top)
	[
		CreateCenterContentBox()
	]
	+ SVerticalBox::Slot()
	.FillHeight(1)
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		//Vertical
		SNew(SScrollBox)
		.Visibility(InVisibility)
		.AllowOverscroll(EAllowOverscroll::Yes)
		.AnimateWheelScrolling(true)
		+ SScrollBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			CreateSubNodePanelSection()
		]
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Top)
	[
		PopulateSimpleDisplayForProperties()
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Top)
	[
		CreateNodeTagBox()
	];


	return CenterWholeBox.ToSharedRef();
}

TSharedRef<SWidget> SJointGraphNodeBase::CreateNameBox()
{
	if (!this ) return SNullWidget::NullWidget;

	UJointEdGraphNode* Node = GetCastedGraphNode();
	
	//Don't display if we don't need to display it
	if (!Node) return SNullWidget::NullWidget;
	//if (this->GraphNode->GetNodeTitle(ENodeTitleType::FullTitle).IsEmpty()) return SNullWidget::NullWidget;

	EVisibility NodeHintTextVisibility = this->GetSlateDetailLevel() !=
	                                     EJointEdSlateDetailLevel::SlateDetailLevel_Maximum &&
	                                     GetWhetherToDisplayIconicNodeText()
		                                     ? EVisibility::SelfHitTestInvisible
		                                     : EVisibility::Collapsed;

	EVisibility NodeTitleVisibility = this->GetSlateDetailLevel() ==
	                                  EJointEdSlateDetailLevel::SlateDetailLevel_Maximum
		                                  ? EVisibility::Visible
		                                  : this->InlineEditableText && this->InlineEditableText->IsInEditMode()
		                                  ? EVisibility::Visible
		                                  : EVisibility::Collapsed;
	
	const TSharedPtr<SToolTip> ToolTip = SNew(SToolTip)
		[
			SNew(SVerticalBox)
			.Visibility( Node->GetShouldHideNameBox()
							? EVisibility::Collapsed
							: EVisibility::SelfHitTestInvisible)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(STextBlock)
				.Visibility(EVisibility::SelfHitTestInvisible)
				.Text(CheckRenameNodeInstance()
					      ? LOCTEXT("NodeRenameBox", "Press F2 to rename this node.")
					      : LOCTEXT("NodeRenameBox_Revert", "Can not rename this node."))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			[
				SNew(SJointNodeDescription)
				.Visibility(EVisibility::SelfHitTestInvisible)
				.ClassToDescribe(GetCastedGraphNode() && GetCastedGraphNode()->GetCastedNodeInstance()
					                 ? GetCastedGraphNode()->GetCastedNodeInstance()->GetClass()
					                 : nullptr)
			]
		];


	if (const FSlateBrush* IconBrush = GetIconicNodeSlateBrush(); IconBrush != nullptr && !IconBrush->ImageSize.
	                                                                                                  IsNearlyZero() && IconBrush->DrawAs != ESlateBrushDrawType::NoDrawType)
	{
		SAssignNew(NameBox, SHorizontalBox)
		.Visibility(Node->GetShouldHideNameBox()
			? EVisibility::Collapsed
			: EVisibility::SelfHitTestInvisible)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(FJointEditorStyle::Margin_Large)
		[
			SNew(SImage)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.Image(GetIconicNodeSlateBrush())
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0, FJointEditorStyle::Margin_Large.Bottom, FJointEditorStyle::Margin_Large.Bottom,
		                 FJointEditorStyle::Margin_Large.Bottom))
		[
			SAssignNew(NodeTitleHintTextBlock, STextBlock)
			.Visibility(NodeHintTextVisibility)
			.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h4")
			.ColorAndOpacity(FLinearColor(1, 1, 1, 0.5))
			.Text(GetIconicNodeText())
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(FJointEditorStyle::Margin_Large)
		[
			SAssignNew(InlineEditableText, SInlineEditableTextBlock)
			.Visibility(NodeTitleVisibility)
			.Style(FJointEditorStyle::Get(), "JointUI.InlineEditableTextBlock.NodeTitleInlineEditableText")
			.Text(GetGraphNodeName())
			.ToolTip(ToolTip)
			.OnVerifyTextChanged(this, &SJointGraphNodeBase::VerifyRenameNameOnTextChanged)
			.OnTextCommitted(this, &SJointGraphNodeBase::OnNameTextCommited)
			.IsSelected(this, &SJointGraphNodeBase::IsSelectedExclusively)
		];
	}
	else
	{
		SAssignNew(NameBox, SHorizontalBox)
		.Visibility(Node->GetShouldHideNameBox()
			? EVisibility::Collapsed
			: EVisibility::SelfHitTestInvisible)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(FJointEditorStyle::Margin_Large)
		[
			SNew(STextBlock)
			.Visibility(NodeHintTextVisibility)
			.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h4")
			.ColorAndOpacity(FLinearColor(1, 1, 1, 0.5))
			.Text(GetIconicNodeText())
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(FJointEditorStyle::Margin_Large)
		[
			SAssignNew(InlineEditableText, SInlineEditableTextBlock)
			.Visibility(NodeTitleVisibility)
			.Style(FJointEditorStyle::Get(), "JointUI.InlineEditableTextBlock.NodeTitleInlineEditableText")
			.Text(GetGraphNodeName())
			.ToolTip(ToolTip)
			.OnVerifyTextChanged(this, &SJointGraphNodeBase::VerifyRenameNameOnTextChanged)
			.OnTextCommitted(this, &SJointGraphNodeBase::OnRenameTextCommited)
			.IsSelected(this, &SJointGraphNodeBase::IsSelectedExclusively)
		];
	}

	return NameBox.ToSharedRef();
}


TSharedRef<SWidget> SJointGraphNodeBase::CreateDissolvedSubNodeIndication()
{
	SAssignNew(DissolveIndicator, SHorizontalBox)
	.Visibility(EVisibility::Visible)
	.ToolTipText(
		FText::Format(
			LOCTEXT("DissolveIndicatorToolTip", "This node has {0} dissolved sub-nodes - you can solidify them again by selecting this node and pressing Ctrl + Shift + S"),
			FText::FromString(FString::FromInt(GetDissolvedSubnodeCounts()))
		))
	+ SHorizontalBox::Slot()
	.AutoWidth()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	.Padding(FMargin(0))
	[
		SNew(SImage)
		.Visibility(EVisibility::SelfHitTestInvisible)
		.DesiredSizeOverride(FVector2D(12, 12))
		.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("ShowFlagsMenu.Decals"))
	]
	+ SHorizontalBox::Slot()
	.AutoWidth()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	.Padding(FMargin(2, 0, 0, 0))
	[
		SNew(STextBlock)
		.Visibility(EVisibility::SelfHitTestInvisible)
		.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h5")
		.Text(FText::FromString(FString::FromInt(GetDissolvedSubnodeCounts())))
	];

	return DissolveIndicator.ToSharedRef();
}

TSharedRef<SWidget> SJointGraphNodeBase::CreateSubNodePanelSection()
{
	if (!SubNodePanel.IsValid()) PopulateSubNodePanel();

	return SubNodePanel.ToSharedRef();
}


TSharedRef<SWidget> SJointGraphNodeBase::CreateNodeTagBox()
{
	return SAssignNew(NodeTagBox, SBorder)
		.Visibility(EVisibility::Collapsed)
		.ToolTipText(LOCTEXT("TagBoxToolTip", "The tags this node has."))
		.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
		.Padding(FJointEditorStyle::Margin_Normal)
		[
			SAssignNew(NodeTagContentBox, SVerticalBox)
		];
}

void SJointGraphNodeBase::PopulateSubNodePanel()
{
	const TAttribute<float> WrapRowSize_Attr = TAttribute<float>::CreateLambda(
		[this]
		{
			if (UJointEdGraphNode* CastedGraphNode = GetCastedGraphNode(); SubNodePanel && CastedGraphNode)
			{
				return CastedGraphNode->GetUseFixedNodeSize()
					       ? CastedGraphNode->GetSubNodeBoxOrientation() == EOrientation::Orient_Horizontal
						         //Hardcoded margin for the box. There is no way to get the desiredsize of the fragmentcontentbox by itself.
						         ? static_cast<float>(GetDesiredSize().X + 10)
						         : static_cast<float>(GetDesiredSize().Y + 10)
					       : CastedGraphNode->GetSubNodeBoxOrientation() == EOrientation::Orient_Horizontal
					       //Max-out the size.
					       ? static_cast<float>(JointGraphNodeResizableDefs::MaxFragmentSize.X)
					       : static_cast<float>(JointGraphNodeResizableDefs::MaxFragmentSize.Y);
			}

			return 100.f;
		});

	SAssignNew(SubNodePanel, SWrapBox)
	.Visibility(EVisibility::SelfHitTestInvisible)
	.Orientation(GetCastedGraphNode()->GetSubNodeBoxOrientation())
	.PreferredSize(WrapRowSize_Attr);
}

void SJointGraphNodeBase::ClearChildrenOnSubNodePanel()
{
	if (const TSharedPtr<SWrapBox> CastedSubNodePanel = INLINE_GetCastedSubNodePanel<SWrapBox>())
	{
		CastedSubNodePanel->ClearChildren();
	}
}

void SJointGraphNodeBase::AddSlateOnSubNodePanel(const TSharedRef<SWidget>& Slate)
{
	if (const TSharedPtr<SWrapBox> CastedSubNodePanel = INLINE_GetCastedSubNodePanel<SWrapBox>())
	{
		CastedSubNodePanel->AddSlot()
		                  .VAlign(VAlign_Fill)
		                  .HAlign(HAlign_Fill)
		                  .Padding(FJointEditorStyle::Margin_SubNode)
		[
			Slate
		];
	}
}

bool SJointGraphNodeBase::IsSubNodeWidget() const
{
	return false;
}

FLinearColor SJointGraphNodeBase::GetNodeBodyBackgroundColor() const
{
	if (UJointEdGraphNode* CastedGraphNode = GetCastedGraphNode()) return CastedGraphNode->GetNodeBodyTintColor();

	return FLinearColor::Transparent;
}


bool SJointGraphNodeBase::VerifyRenameNameOnTextChanged(const FText& InText, FText& OutErrorMessage)
{
	bool bValid(true);

	if (GetEditableNodeTitle() != InText.ToString())
	{
		if (UJointEdGraphNode* EdNode = Cast<UJointEdGraphNode>(GraphNode))
		{
			bValid = FJointEdUtils::IsNameSafeForObjectRenaming(InText.ToString(), EdNode, EdNode->GetOuter(), OutErrorMessage);
		}
	}

	if (OutErrorMessage.IsEmpty())
	{
		OutErrorMessage = FText::FromString(TEXT("Error"));
	}

	return bValid;
}

bool SJointGraphNodeBase::CheckWhetherNodeSelected() const
{
	if (const FGraphSelectionManager* SelectionManager = GetSelectionManager())
	{
		return SelectionManager->SelectedNodes.Contains(this->GetCastedGraphNode());
	}

	return false;
}

bool SJointGraphNodeBase::CheckRenameNodeInstance() const
{
	if (UJointEdGraphNode* InGraphNode = GetCastedGraphNode())
	{
		if (UJointNodeBase* InGraphNodeInstance = InGraphNode->GetCastedNodeInstance())
		{
			return InGraphNode->GetCanRenameNode();
		}
	}
	
	return false;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


TArray<FOverlayWidgetInfo> SJointGraphNodeBase::GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const
{
	TArray<FOverlayWidgetInfo> Widgets;

	FVector2D OverlayOverallOffset = FVector2D(0, 8);

	FVector2D OverlayOffset = FVector2D(0, 0);

	AttachWidgetOnOverlayInfo(BuildPresetOverlay, Widgets, OverlayOverallOffset, OverlayOffset);
	AttachWidgetOnOverlayInfo(BreakpointOverlay, Widgets, OverlayOverallOffset, OverlayOffset);
	AttachWidgetOnOverlayInfo(CompileResultOverlay, Widgets, OverlayOverallOffset, OverlayOffset);

	return Widgets;
}

void SJointGraphNodeBase::AttachWidgetOnOverlayInfo(const TSharedPtr<SWidget>& WidgetToAdd,
                                                    TArray<FOverlayWidgetInfo>& Widgets,
                                                    FVector2D& OverlayOverallOffset, FVector2D& OverlayOffset)
{
	if (WidgetToAdd.IsValid())
	{
		const FVector2D WidgetSize = WidgetToAdd->GetDesiredSize();

		FOverlayWidgetInfo Overlay(WidgetToAdd);

		Overlay.OverlayOffset = OverlayOverallOffset + OverlayOffset + FVector2D(
			0,
			-WidgetSize.Y / 2);

		if (WidgetToAdd->GetVisibility() != EVisibility::Collapsed && WidgetToAdd->GetVisibility() !=
			EVisibility::Hidden)
		{
			OverlayOffset.X += WidgetSize.X;
		}

		Widgets.Add(Overlay);
	}
}

const FSlateBrush* SJointGraphNodeBase::GetIconicNodeSlateBrush() const
{
	if (UJointEdGraphNode* InGraphNode = GetCastedGraphNode())
	{
		return &InGraphNode->GetEdNodeSetting().IconicNodeImageBrush;
	}

	return nullptr;
}


const FText SJointGraphNodeBase::GetIconicNodeText() const
{
	if (const UJointEdGraphNode* CastedGraphNode = GetCastedGraphNode())
	{
		return FJointEdUtils::GetFriendlyNameOfNode(CastedGraphNode);
	}

	return FText::GetEmpty();
}

const bool SJointGraphNodeBase::GetWhetherToDisplayIconicNodeText() const
{
	if (UJointEdGraphNode* InGraphNode = GetCastedGraphNode())
	{
		return InGraphNode->GetEdNodeSetting().bAllowDisplayClassFriendlyNameText;
	}

	return false;
}

const uint16 SJointGraphNodeBase::GetDissolvedSubnodeCounts() const
{
	return DissolvedSubnodeCounts;
}
#undef LOCTEXT_NAMESPACE
