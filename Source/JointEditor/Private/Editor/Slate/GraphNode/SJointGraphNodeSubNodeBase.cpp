//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "GraphNode/SJointGraphNodeSubNodeBase.h"

#include "Editor.h"
#include "JointAdvancedWidgets.h"
#include "JointEdGraph.h"
#include "Node/SubNode/JointEdGraphNode_Fragment.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SEditableTextBox.h"

#include "SCommentBubble.h"
#include "ScopedTransaction.h"

#include "JointEditorStyle.h"
#include "JointEditorToolkit.h"
#include "SGraphPanel.h"

#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Types/SlateAttributeMetaData.h"

#include "VoltAnimationManager.h"
#include "VoltDecl.h"
#include "GraphNode/JointGraphNodeSharedSlates.h"

#include "Misc/EngineVersionComparison.h"
#include "Slate/SRetainerWidget.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

#define LOCTEXT_NAMESPACE "SJointGraphNodeSubNodeBase"

void SJointGraphNodeSubNodeBase::Construct(const FArguments& InArgs, UEdGraphNode* InNode)
{
	this->GraphNode = InNode;
	this->SetCursor(EMouseCursor::Crosshairs);
	this->AssignSlateToGraphNode();
	this->InitializeSlate();
	this->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	SetVisibility(EVisibility::SelfHitTestInvisible);
	SetCanTick(false);
	SlatePrepass();
}

SJointGraphNodeSubNodeBase::~SJointGraphNodeSubNodeBase()
{
}

int32 SJointGraphNodeSubNodeBase::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                          const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                          int32 LayerId,
                                          const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (!IsDissolvedSubNode())
	{
		const class UObject* Brush1ResourceObj = NodeBodyBorderImage
			                                         ? NodeBodyBorderImage->GetResourceObject()
			                                         : nullptr;
		const class UObject* Brush2ResourceObj = NodeBackgroundInBorderImage
			                                         ? NodeBackgroundInBorderImage->GetResourceObject()
			                                         : nullptr;
		const class UObject* Brush3ResourceObj = NodeBackgroundOutBorderImage
			                                         ? NodeBackgroundOutBorderImage->GetResourceObject()
			                                         : nullptr;
		const class UObject* Brush4ResourceObj = GetIconicNodeSlateBrush()
			                                         ? GetIconicNodeSlateBrush()->GetResourceObject()
			                                         : nullptr;

		// Fallback if any brush resource pointer is non-null but not a valid UObject
		if ((Brush1ResourceObj != nullptr && !Brush1ResourceObj->IsValidLowLevelFast()) ||
			(Brush2ResourceObj != nullptr && !Brush2ResourceObj->IsValidLowLevelFast()) ||
			(Brush3ResourceObj != nullptr && !Brush3ResourceObj->IsValidLowLevelFast()) ||
			(Brush4ResourceObj != nullptr && !Brush4ResourceObj->IsValidLowLevelFast()))
		{
			// If the border image is not set or invalidated, we should not paint the node body.
			// TODO : Do we need to paint the fallback node body? Or even can we paint the fallback node body instead???
			// -> possibly, but probably will not worth. maybe just try to notify the editor toolkit to handle this.

			if (GetCastedGraphNode()
				&& GetCastedGraphNode()->GetCastedGraph()
				&& GetCastedGraphNode()->GetCastedGraph()->GetToolkit().IsValid())
				GetCastedGraphNode()->GetCastedGraph()->GetToolkit().Pin()->PopulateNeedReopeningToastMessage();

			return LayerId;
		}
	}

	int CurLayerId = SJointGraphNodeBase::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId,
	                                              InWidgetStyle, bParentEnabled);

	TArray<FOverlayWidgetInfo> OverlayWidgets = GetOverlayWidgets(false, GetDesiredSize());

	for (int32 WidgetIndex = 0; WidgetIndex < OverlayWidgets.Num(); ++WidgetIndex)
	{
		FOverlayWidgetInfo& OverlayInfo = OverlayWidgets[WidgetIndex];
		if (SWidget* Widget = OverlayInfo.Widget.Get())
		{
			FSlateAttributeMetaData::UpdateOnlyVisibilityAttributes(
				*Widget, FSlateAttributeMetaData::EInvalidationPermission::AllowInvalidationIfConstructed);
			if (Widget->GetVisibility() == EVisibility::Visible)
			{
				// call SlatePrepass as these widgets are not in the 'normal' child hierarchy
				Widget->SlatePrepass(AllottedGeometry.GetAccumulatedLayoutTransform().GetScale());


#if UE_VERSION_OLDER_THAN(5, 2, 0)

				const FGeometry WidgetGeometry = AllottedGeometry.MakeChild(
					OverlayInfo.OverlayOffset, Widget->GetDesiredSize());

#else
				const FGeometry WidgetGeometry = AllottedGeometry.MakeChild(
					Widget->GetDesiredSize(), FSlateLayoutTransform(1, OverlayInfo.OverlayOffset));
#endif

				CurLayerId = Widget->Paint(Args, WidgetGeometry, MyCullingRect, OutDrawElements, CurLayerId,
				                           InWidgetStyle, bParentEnabled);
			}
		}
	}


	return CurLayerId;
}


TSharedRef<SWidget> SJointGraphNodeSubNodeBase::CreateNodeContentArea()
{
	//It's still here but never be populated.
	//SAssignNew(LeftNodeBox, SVerticalBox);
	//SAssignNew(RightNodeBox, SVerticalBox);

	// NODE CONTENT AREA
	return SNew(SHorizontalBox)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			// RIGHT
			SAssignNew(CenterContentBox, SVerticalBox)
		];
}


TSharedRef<SWidget> SJointGraphNodeSubNodeBase::CreateSubNodePanelSection()
{
	if (SubNodePanel.IsValid())
	{
		if (UJointEdGraphNode* CastedGraphNode = GetCastedGraphNode(); LastSeenSubNodeOrientation != CastedGraphNode->
			GetSubNodeBoxOrientation())
		{
			PopulateSubNodePanel();
		}
	}
	else
	{
		PopulateSubNodePanel();
	}

	return SubNodePanel.ToSharedRef();
}

void SJointGraphNodeSubNodeBase::PopulateNodeSlates()
{
	//ClearSlates();

	TSharedPtr<STextBlock> DescriptionText;

	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode).Visibility(EVisibility::SelfHitTestInvisible);


	/**
	 * SDS2: 
	 * New layout for the whole slate:
	 *
	 *             NodeBody              
	 *  ________________________________
	 * |         CenterWholeBox         |
	 * |   __________________________   |
	 * |  |   _____________________  |  |
	 * |  |  |NameBox             |  |  |
	 * |  |  |____________________|  |  |
	 * |  |   _____________________  |  |
	 * |  |  |CenterContentBox    |  |  |
	 * |  |  |____________________|  |  |
	 * |  |   _____________________  |  |
	 * |  |  |SubNodeBox          |  |  |
	 * |  |  |____________________|  |  |
	 * |  |__________________________|  |
	 * |________________________________|
	 */

	/**
	 * It still have LeftNodeBox, RightNodeBox on its structure but those are hidden, and please don't touch LeftNodeBox, RightNodeBox if possible.
	 * Those are related with the pin actions and difficult to manage without deep understanding of its structure.
	 *
	 * Consider overriding CenterWholeBox if possible.
	 * SJointGraphNodeBase also provide a static functions that help you to populate the default slate set by each section.
	 * Use it on your slate customization code.
	 *
	 * Check out how the default provided Joint Editor Graph Node classes. (especially UJointEdFragment_Context.)
	 */

	UpdateBreakpoint();
	UpdateErrorInfo();
	UpdateBuildTargetPreset();

	FMargin ContentPadding = FMargin(0.f);
	
	if (UJointEdGraphNode* InGraphNode = GetCastedGraphNode())
	{
		if (InGraphNode->GetEdNodeSetting().bUseCustomContentNodePadding)
			ContentPadding = InGraphNode->GetEdNodeSetting().ContentNodePadding;
	}
	

	if (!IsDissolvedSubNode())
	{
		switch (GetSlateDetailLevel())
		{
		case EJointEdSlateDetailLevel::SlateDetailLevel_Stow:

			CreateNodeBody(true);

			NodeBody->SetContent(
				SNew(SOverlay)
				.Visibility(EVisibility::SelfHitTestInvisible)
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SBox)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.Visibility(EVisibility::SelfHitTestInvisible)
					.MinDesiredWidth(JointGraphNodeResizableDefs::MinFragmentSize.X)
					.MinDesiredHeight(JointGraphNodeResizableDefs::MinFragmentSize.Y)
					[
						/*SAssignNew(NodeBackground, SImage)
						.Visibility(EVisibility::SelfHitTestInvisible)
						.RenderTransformPivot(FVector2D(0.5))
						.Image(FJointEditorStyle::Get().GetBrush("JointUI.Border.Sphere"))
						.ColorAndOpacity(GetNodeBodyBackgroundColor())*/
						CreateNodeBackground(true)
					]
				]
				+ SOverlay::Slot()
				.Padding(ContentPadding)
				[
					SAssignNew(CenterWholeBox, SVerticalBox)
					.Visibility(EVisibility::SelfHitTestInvisible)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						CreateNameBox()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Top)
					[
						CreateSubNodePanelSection()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Top)
					[
						CreateNodeTagBox()
					]
				]
			);
			break;
		case EJointEdSlateDetailLevel::SlateDetailLevel_Minimal_Content:

			CreateNodeBody();

			NodeBody->SetContent(
				SNew(SOverlay)
				.Visibility(EVisibility::SelfHitTestInvisible)
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SBox)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.Visibility(EVisibility::SelfHitTestInvisible)
					.MinDesiredWidth(JointGraphNodeResizableDefs::MinFragmentSize.X)
					.MinDesiredHeight(JointGraphNodeResizableDefs::MinFragmentSize.Y)
					[
						/*SAssignNew(NodeBackground, SImage)
						.Visibility(EVisibility::SelfHitTestInvisible)
						.RenderTransformPivot(FVector2D(0.5))
						.Image(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.ColorAndOpacity(GetNodeBodyBackgroundColor())*/
						CreateNodeBackground()
					]
				]
				+ SOverlay::Slot()
				.Padding(ContentPadding)
				[
					SAssignNew(CenterWholeBox, SVerticalBox)
					.Visibility(EVisibility::SelfHitTestInvisible)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						CreateNameBox()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Top)
					[
						SNew(SHorizontalBox)
						.Visibility(EVisibility::SelfHitTestInvisible)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							CreateNodeContentArea()
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
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
					]
				]
			);

			break;
		case EJointEdSlateDetailLevel::SlateDetailLevel_Maximum:

			CreateNodeBody();

			NodeBody->SetContent(
				SNew(SOverlay)
				.Visibility(EVisibility::SelfHitTestInvisible)
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SBox)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.Visibility(EVisibility::SelfHitTestInvisible)
					.MinDesiredWidth(JointGraphNodeResizableDefs::MinFragmentSize.X)
					.MinDesiredHeight(JointGraphNodeResizableDefs::MinFragmentSize.Y)
					[
						/*SAssignNew(NodeBackground, SImage)
						.Visibility(EVisibility::SelfHitTestInvisible)
						.RenderTransformPivot(FVector2D(0.5))
						.Image(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.ColorAndOpacity(GetNodeBodyBackgroundColor())*/
						CreateNodeBackground()
					]
				]
				+ SOverlay::Slot()
				.Padding(ContentPadding)
				[
					SAssignNew(CenterWholeBox, SVerticalBox)
					.Visibility(EVisibility::SelfHitTestInvisible)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						CreateNameBox()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Top)
					[
						SNew(SHorizontalBox)
						.Visibility(EVisibility::SelfHitTestInvisible)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							CreateNodeContentArea()
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
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
					]
				]

			);

			break;
		}

		//Set the color value to the current value.
		PlayNodeBackgroundColorResetAnimationIfPossible(true);

		this->ContentScale.Bind(this, &SGraphNode::GetContentScale);

		this->GetOrAddSlot(ENodeZone::Center)
		    .HAlign(HAlign_Fill)
		    .VAlign(VAlign_Fill)
		[
			NodeBody.ToSharedRef()
		];


		InitializeVoltVariables();
	}
	else
	{

		switch (GetSlateDetailLevel())
		{
		case EJointEdSlateDetailLevel::SlateDetailLevel_Stow:

			SAssignNew(CenterWholeBox, SVerticalBox)
			.Visibility(EVisibility::SelfHitTestInvisible)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::SelfHitTestInvisible)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					CreateNodeContentArea()
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					CreateSubNodePanelSection()
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			[
				CreateNodeTagBox()
			];
			break;
			
		case EJointEdSlateDetailLevel::SlateDetailLevel_Minimal_Content:

			SAssignNew(CenterWholeBox, SVerticalBox)
			.Visibility(EVisibility::SelfHitTestInvisible)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::SelfHitTestInvisible)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					CreateNodeContentArea()
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
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

			break;
		case EJointEdSlateDetailLevel::SlateDetailLevel_Maximum:

			SAssignNew(CenterWholeBox, SVerticalBox)
			.Visibility(EVisibility::SelfHitTestInvisible)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::SelfHitTestInvisible)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					CreateNodeContentArea()
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
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

			break;
		}

		//Set the color value to the current value.
		PlayNodeBackgroundColorResetAnimationIfPossible(true);

		this->ContentScale.Bind(this, &SGraphNode::GetContentScale);

		this->GetOrAddSlot(ENodeZone::Center)
		    .HAlign(HAlign_Fill)
		    .VAlign(VAlign_Fill)
		[
			CenterWholeBox.ToSharedRef()
		];


		InitializeVoltVariables();
	}
	
	//Modify the graph node slates from the editor graph node instance.
	ModifySlateFromGraphNode();
}

FVector2D SJointGraphNodeSubNodeBase::GetNodeMinimumSize() const
{
	return JointGraphNodeResizableDefs::MinFragmentSize;
}

FVector2D SJointGraphNodeSubNodeBase::GetNodeMaximumSize() const
{
	return JointGraphNodeResizableDefs::MaxFragmentSize;
}


void SJointGraphNodeSubNodeBase::PopulateSubNodePanel()
{
	if (UJointEdGraphNode* CastedGraphNode = GetCastedGraphNode())
	{
		LastSeenSubNodeOrientation = CastedGraphNode->GetSubNodeBoxOrientation();

		switch (LastSeenSubNodeOrientation)
		{
		case Orient_Horizontal:

			SAssignNew(SubNodePanel, SHorizontalBox)
			.Visibility(EVisibility::SelfHitTestInvisible);

			break;
		case Orient_Vertical:

			SAssignNew(SubNodePanel, SVerticalBox)
			.Visibility(EVisibility::SelfHitTestInvisible);

			break;
		}
	}
}

void SJointGraphNodeSubNodeBase::ClearChildrenOnSubNodePanel()
{
	if (UJointEdGraphNode* CastedGraphNode = GetCastedGraphNode())
	{
		switch (CastedGraphNode->GetSubNodeBoxOrientation())
		{
		case Orient_Horizontal:

			if (const TSharedPtr<SHorizontalBox> CastedSubNodePanel = INLINE_GetCastedSubNodePanel<SHorizontalBox>())
			{
				CastedSubNodePanel->ClearChildren();
			}

			break;
		case Orient_Vertical:

			if (const TSharedPtr<SVerticalBox> CastedSubNodePanel = INLINE_GetCastedSubNodePanel<SVerticalBox>())
			{
				CastedSubNodePanel->ClearChildren();
			}

			break;
		}
	}
}

void SJointGraphNodeSubNodeBase::AddSlateOnSubNodePanel(const TSharedRef<SWidget>& Slate)
{
	if (UJointEdGraphNode* CastedGraphNode = GetCastedGraphNode())
	{
		switch (CastedGraphNode->GetSubNodeBoxOrientation())
		{
		case Orient_Horizontal:

			if (const TSharedPtr<SHorizontalBox> CastedSubNodePanel = INLINE_GetCastedSubNodePanel<SHorizontalBox>())
			{
				CastedSubNodePanel->AddSlot()
				                  .AutoWidth()
				                  .VAlign(VAlign_Center)
				                  .HAlign(HAlign_Center)
				                  .Padding(FJointEditorStyle::Margin_SubNode)
				[
					Slate
				];
			}

			break;
		case Orient_Vertical:

			if (const TSharedPtr<SVerticalBox> CastedSubNodePanel = INLINE_GetCastedSubNodePanel<SVerticalBox>())
			{
				CastedSubNodePanel->AddSlot()
				                  .AutoHeight()
				                  .VAlign(VAlign_Center)
				                  .HAlign(HAlign_Center)
				                  .Padding(FJointEditorStyle::Margin_SubNode)
				[
					Slate
				];
			}

			break;
		}
	}
}

bool SJointGraphNodeSubNodeBase::IsSubNodeWidget() const
{
	return true;
}


FReply SJointGraphNodeSubNodeBase::OnMouseButtonUp(const FGeometry& SenderGeometry,
                                                   const FPointerEvent& MouseEvent)
{
	MouseDownScreenPosition = JointGraphNodeDragDropOperationDefs::NullDragPosition;

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bUserIsDragging)
	{
		bUserIsDragging = false;

		// Resize the node	
		UserSize.X = FMath::RoundToFloat(UserSize.X);
		UserSize.Y = FMath::RoundToFloat(UserSize.Y);

		GetNodeObj()->ResizeNode(UserSize);

		// End resize transaction
		ResizeTransactionPtr.Reset();

		return FReply::Handled().ReleaseMouseCapture();
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (UJointEdGraphNode* CastedNode = GetCastedGraphNode())
		{
			TArray<UEdGraphPin*> DraggedPin;
			FVector2D Pos = FVector2D(MouseEvent.GetScreenSpacePosition().X, MouseEvent.GetScreenSpacePosition().Y);
			if (GetOwnerPanel().IsValid())
				GetOwnerPanel()->
					SummonContextMenu(Pos, Pos, CastedNode, nullptr, DraggedPin);

			return FReply::Handled();
		}
	}

	return FReply::Handled();
}

FReply SJointGraphNodeSubNodeBase::OnMouseButtonDown(const FGeometry& SenderGeometry,
                                                     const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		MouseDownScreenPosition = MouseEvent.GetScreenSpacePosition();

		UJointEdGraphNode* TestNode = Cast<UJointEdGraphNode>(GraphNode);

		if (TestNode && TestNode->IsSubNode())
		{
			TSharedPtr<SGraphPanel> Ptr = GetOwnerPanel();
			
			if (Ptr.IsValid())
			{
				Ptr->SelectionManager.ClickedOnNode(GraphNode, MouseEvent);
			}
		}
	}
	return FReply::Handled();
}


FReply SJointGraphNodeSubNodeBase::OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
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

				OnDragStarted();

				TSharedRef<FDragJointGraphNode> DragOperation_GraphNode = FDragJointGraphNode::New(Panel, Node);
				return FReply::Handled().BeginDragDrop(DragOperation_GraphNode);
			}
		}
	}

	return FReply::Handled();
}


FLinearColor SJointGraphNodeSubNodeBase::GetNodeBodyBackgroundColor() const
{
	if (GetSlateDetailLevel() == EJointEdSlateDetailLevel::SlateDetailLevel_Stow)
	{
		if (UJointEdGraphNode* InGraphNode = GetCastedGraphNode())
		{
			if (InGraphNode->GetEdNodeSetting().bUseIconicColorForNodeBodyOnStow)
			{
				return InGraphNode->GetNodeTitleColor();
			}
			else
			{
				return InGraphNode->GetNodeBodyTintColor();
			}
		}
	}

	return SJointGraphNodeBase::GetNodeBodyBackgroundColor();
}

const bool SJointGraphNodeSubNodeBase::IsDissolvedSubNode() const
{
	if (const UJointEdGraphNode_Fragment* CastedSelfGraphNode = Cast<UJointEdGraphNode_Fragment>(GraphNode)) return CastedSelfGraphNode->IsDissolvedSubNode();

	return false;
}


#undef LOCTEXT_NAMESPACE
