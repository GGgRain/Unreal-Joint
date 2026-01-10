//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "GraphNode/JointGraphNodeSharedSlates.h"

#include "BlueprintEditor.h"
#include "IDetailPropertyExtensionHandler.h"
#include "JointEdGraph.h"
#include "JointEditorStyle.h"
#include "SGraphPin.h"
#include "Node/JointNodeBase.h"

#include "Debug/JointDebugger.h"
#include "JointEdGraphNode.h"
#include "JointEditorToolkit.h"
#include "JointManager.h"

#include "ISinglePropertyView.h"
#include "IStructureDetailsView.h"
#include "JointAdvancedWidgets.h"
#include "JointEdGraphNodesCustomization.h"
#include "JointEditorNodePickingManager.h"
#include "JointEditorSettings.h"

#include "JointEdUtils.h"
#include "SGraphPanel.h"
#include "SLevelOfDetailBranchNode.h"
#include "VoltAnimationManager.h"
#include "VoltDecl.h"
#include "Async/Async.h"
#include "Framework/Commands/GenericCommands.h"

#include "GraphNode/SJointGraphNodeBase.h"
#include "GraphNode/SJointGraphNodeSubNodeBase.h"
#include "Module/Volt_ASM_Delay.h"
#include "Module/Volt_ASM_InterpRenderOpacity.h"
#include "Module/Volt_ASM_InterpWidgetTransform.h"
#include "Module/Volt_ASM_Sequence.h"
#include "Module/Volt_ASM_SetWidgetTransformPivot.h"
#include "Module/Volt_ASM_Simultaneous.h"

#include "Slate/SRetainerWidget.h"

#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SInvalidationPanel.h"
#include "Widgets/Images/SImage.h"
#include "HAL/PlatformApplicationMisc.h"

class UVolt_ASM_InterpWidgetTransform;

void SJointMultiNodeIndex::Construct(const FArguments& InArgs)
{
	SetCanTick(false);

	OnHoverStateChangedEvent = InArgs._OnHoverStateChanged;
	OnGetIndexColorEvent = InArgs._OnGetIndexColor;

	const FSlateBrush* IndexBrush = FJointEditorStyle::Get().GetBrush("JointUI.Border.Round");

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			// Add a dummy box here to make sure the widget doesnt get smaller than the brush
			SNew(SBox)
			.WidthOverride(IndexBrush->ImageSize.X)
			.HeightOverride(IndexBrush->ImageSize.Y)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(IndexBrush)
			.Padding(FJointEditorStyle::Margin_Normal)
			.BorderBackgroundColor(FLinearColor(1, 1, 1, 1))
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(InArgs._Text)
				.Font(FJointEditorStyle::GetUEEditorSlateStyleSet().GetFontStyle("BTEditor.Graph.BTNode.IndexText"))
				.ColorAndOpacity(FLinearColor(0, 0, 0, 1))
			]
		]
	];
}

void SJointMultiNodeIndex::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	OnHoverStateChangedEvent.ExecuteIfBound(true);
	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
}

void SJointMultiNodeIndex::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	OnHoverStateChangedEvent.ExecuteIfBound(false);
	SCompoundWidget::OnMouseLeave(MouseEvent);
}

FSlateColor SJointMultiNodeIndex::GetColor() const
{
	if (OnGetIndexColorEvent.IsBound()) { return OnGetIndexColorEvent.Execute(IsHovered()); }

	return FSlateColor::UseForeground();
}


#define LOCTEXT_NAMESPACE "SJointGraphPinOwnerNodeBox"

void SJointGraphPinOwnerNodeBox::Construct(const FArguments& InArgs, UJointEdGraphNode* InTargetNode,
                                           TSharedPtr<SJointGraphNodeBase> InOwnerGraphNode)
{
	this->TargetNode = InTargetNode;
	this->OwnerGraphNode = InOwnerGraphNode;

	SetCanTick(false);

	PopulateSlate();
}

void SJointGraphPinOwnerNodeBox::PopulateSlate()
{
	this->ChildSlot.DetachWidget();

	FLinearColor Color = this->TargetNode ? this->TargetNode->GetNodeTitleColor() * 0.2 : FLinearColor::Black;

	FLinearColor NonAlphaColor = this->TargetNode ? this->TargetNode->GetNodeTitleColor() * 0.05 : FLinearColor::Black;
	NonAlphaColor = NonAlphaColor.GetClamped(0.012, 1);
	NonAlphaColor.A = 1;

	this->ChildSlot[
		SNew(SVerticalBox)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SVerticalBox::Slot()
		.Padding(FJointEditorStyle::Margin_Normal)
		.AutoHeight()
		[
			SNew(STextBlock)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.Text(GetName())
			.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h4")
		]
		+ SVerticalBox::Slot()
		.Padding(FJointEditorStyle::Margin_Normal)
		.AutoHeight()
		[
			SAssignNew(PinBox, SVerticalBox)
			.Visibility(EVisibility::SelfHitTestInvisible)

			/*
			SNew(SJointOutlineBorder)
			.NormalColor(Color)
			.HoverColor(NonAlphaColor * 1.3)
			.OutlineNormalColor(Color)
			.OutlineHoverColor(NonAlphaColor * 6)
			.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.Visibility(EVisibility::Visible)
			.ContentMargin(FJointEditorStyle::Margin_Normal)
			[
				SAssignNew(PinBox, SVerticalBox)
				.Visibility(EVisibility::SelfHitTestInvisible)
			]*/
		]
	];
}

const FText SJointGraphPinOwnerNodeBox::GetName()
{
	if (TargetNode)
	{
		if (TargetNode->GetCastedNodeInstance())
		{
			return FText::FromString(TargetNode->GetCastedNodeInstance()->GetName());
		}
	}

	return FText::GetEmpty();
}


void SJointGraphPinOwnerNodeBox::AddPin(const TSharedRef<SGraphPin>& TargetPin)
{
	TargetPin->SetOwner(StaticCastSharedRef<SGraphNode>(this->OwnerGraphNode.Pin().ToSharedRef()));

	PinBox.Pin()->AddSlot()
	      .Padding(FJointEditorStyle::Margin_Tiny)
	      .AutoHeight()
	      .HAlign(HAlign_Left)
	      .VAlign(VAlign_Center)
	[
		TargetPin
	];
}

TSharedPtr<SVerticalBox> SJointGraphPinOwnerNodeBox::GetPinBox() const
{
	return PinBox.Pin();
}


void SJointGraphNodeInsertPoint::Construct(const FArguments& InArgs)
{
	ParentGraphNodeSlate = InArgs._ParentGraphNodeSlate;
	InsertIndex = InArgs._InsertIndex;

	SetCanTick(false);

	PopulateSlate();
}

void SJointGraphNodeInsertPoint::PopulateSlate()
{
	this->ChildSlot.DetachWidget();

	this->ChildSlot
	[
		SAssignNew(SlateBorder, SBorder)
		.RenderTransformPivot(FVector2D(0.5, 0.5))
		.RenderOpacity(0)
		.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.Padding(FJointEditorStyle::Margin_Normal * 0.6)
	];
}

FReply SJointGraphNodeInsertPoint::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragJointGraphNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragJointGraphNode>();

	return SCompoundWidget::OnDragOver(MyGeometry, DragDropEvent);
}

void SJointGraphNodeInsertPoint::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragJointGraphNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragJointGraphNode>();

	if (DragConnectionOp.IsValid())
	{
		if (!DragConnectionOp->DragOverNodes.Contains(SharedThis(this)))
			DragConnectionOp->DragOverNodes.Add(
				SharedThis(this));

		VOLT_STOP_ANIM(Track);

		UVoltAnimation* ExpandAnimation = VOLT_MAKE_ANIMATION()
		(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpRenderOpacity)
			.InterpolationMode(EVoltInterpMode::AlphaBased)
			.AlphaBasedDuration(0.3)
			.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
			.AlphaBasedBlendExp(6)
			.TargetOpacity(1),
			VOLT_MAKE_MODULE(UVolt_ASM_Sequence)
			(
				VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
				.InterpolationMode(EVoltInterpMode::AlphaBased)
				.AlphaBasedDuration(0.05)
				.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
				.AlphaBasedBlendExp(6)
				.TargetWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(1.1, 1.1),
				                                        FVector2D::ZeroVector, 0)),
				VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
				.InterpolationMode(EVoltInterpMode::AlphaBased)
				.AlphaBasedDuration(0.25)
				.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
				.AlphaBasedBlendExp(6)
				.TargetWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(1.0, 1.0),
				                                        FVector2D::ZeroVector, 0))
			)
		);

		Track = VOLT_PLAY_ANIM(SlateBorder, ExpandAnimation);
	}
}

void SJointGraphNodeInsertPoint::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragJointGraphNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragJointGraphNode>();

	if (DragConnectionOp.IsValid())
	{
		//Store the drag overed slates to notify when the drag drop action is ended.
		if (DragConnectionOp->DragOverNodes.Contains(SharedThis(this)))
			DragConnectionOp->DragOverNodes.Remove(
				SharedThis(this));

		VOLT_STOP_ANIM(Track);

		UVoltAnimation* DisappearAnimation = VOLT_MAKE_ANIMATION()
		(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpRenderOpacity)
			.InterpolationMode(EVoltInterpMode::AlphaBased)
			.AlphaBasedDuration(0.4)
			.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
			.AlphaBasedBlendExp(6)
			.TargetOpacity(0),
			VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
			.InterpolationMode(EVoltInterpMode::AlphaBased)
			.AlphaBasedDuration(0.4)
			.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
			.AlphaBasedBlendExp(6)
			.TargetWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(0, 0), FVector2D::ZeroVector, 0))
		);

		Track = VOLT_PLAY_ANIM(SlateBorder, DisappearAnimation);
	}

	SCompoundWidget::OnDragLeave(DragDropEvent);
}

FReply SJointGraphNodeInsertPoint::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragJointGraphNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragJointGraphNode>();


	VOLT_STOP_ANIM(Track);

	const UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpRenderOpacity)
		.bUseStartOpacity(true)
		.TargetOpacity(1)
		.StartOpacity(0)
	);

	Track = VOLT_PLAY_ANIM(SlateBorder, Anim);


	if (DragConnectionOp.IsValid())
	{
		GEditor->BeginTransaction(NSLOCTEXT("JointEdTransaction", "TransactionTitle_DragDropNode",
		                                    "Drag & Drop a sub node"));

		TArray<UEdGraphNode*> ModifiedNodes;

		for (TSharedRef<SGraphNode> GraphNode : DragConnectionOp->GetNodes())
		{
			if (GraphNode->GetType() != "SJointGraphNodeSubNodeBase") continue;

			TSharedRef<SJointGraphNodeSubNodeBase> GettingDroppedNodeCastedWidget = StaticCastSharedRef<
				SJointGraphNodeSubNodeBase>(GraphNode);

			UJointEdGraphNode* GettingDroppedNode = GettingDroppedNodeCastedWidget->GetCastedGraphNode();
			UJointEdGraphNode* RemoveFrom = GettingDroppedNodeCastedWidget->GetCastedGraphNode()->ParentNode;
			UJointEdGraphNode* AddedTo = ParentGraphNodeSlate.Pin()->GetCastedGraphNode();

			//If any of the nodes are invalid, continue.
			if (!(GettingDroppedNode && RemoveFrom && AddedTo)) continue;

			//If the connection is not allowed, continue.
			if (AddedTo->CanAttachSubNodeOnThis(GettingDroppedNode).Response ==
				ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW && GettingDroppedNode->
				                                                           CanAttachThisAtParentNode(AddedTo).Response
				== ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW)
				continue;

			//If current GettingDroppedNode is not seen before, newly modify it on this transaction before begin editing.
			if (!ModifiedNodes.Contains(GettingDroppedNode))
			{
				GettingDroppedNode->Modify();
				ModifiedNodes.Add(GettingDroppedNode);
			}

			//If current RemoveFrom is not seen before, newly modify it on this transaction before begin editing.
			if (!ModifiedNodes.Contains(RemoveFrom))
			{
				RemoveFrom->Modify();
				ModifiedNodes.Add(RemoveFrom);
			}

			//If current AddedTo is not seen before, newly modify it on this transaction before begin editing.

			if (!ModifiedNodes.Contains(AddedTo))
			{
				AddedTo->Modify();
				ModifiedNodes.Add(AddedTo);
			}


			if (RemoveFrom != AddedTo)
			{
				//Remove Sub node from the previous node.

				RemoveFrom->RemoveSubNode(GettingDroppedNode, true);

				//Update Sub node slates list of the RemoveFrom node.

				if (RemoveFrom->GetGraphNodeSlate().IsValid())
				{
					const TSharedPtr<SJointGraphNodeBase> RemoveFromCastedWidget = RemoveFrom->GetGraphNodeSlate().Pin();
				
					RemoveFromCastedWidget->PopulateSubNodeSlates();
				}

				//Add Sub node to the AddedTo node.

				AddedTo->AddSubNode(GettingDroppedNode);

				//Update RemoveFrom.

				RemoveFrom->Update();
			}

			GettingDroppedNodeCastedWidget->PlayInsertAnimation();

			AddedTo->RearrangeSubNodeAt(GettingDroppedNode, InsertIndex);

			GettingDroppedNodeCastedWidget->PlayNodeBackgroundColorResetAnimationIfPossible();

			TArray<UJointEdGraphNode*> SubSubNodes = GettingDroppedNode->GetAllSubNodesInHierarchy();

			for (UJointEdGraphNode* SubSubNode : SubSubNodes)
			{
				const TSharedPtr<SJointGraphNodeBase> Widget = SubSubNode->GetGraphNodeSlate().Pin();

				if (Widget) Widget->PlayNodeBackgroundColorResetAnimationIfPossible();
			}
		}

		GEditor->EndTransaction();

		return FReply::Handled();
	}
	return SCompoundWidget::OnDrop(MyGeometry, DragDropEvent);
}

void SJointGraphNodeInsertPoint::Highlight(const float& Delay = 0)
{
	VOLT_STOP_ANIM(Track);

	const UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_Sequence)
		(
			VOLT_MAKE_MODULE(UVolt_ASM_Delay)
			.Duration(Delay),
			VOLT_MAKE_MODULE(UVolt_ASM_Simultaneous)
			(
				VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
				.InterpolationMode(EVoltInterpMode::AlphaBased)
				.AlphaBasedDuration(0.2)
				.AlphaBasedBlendExp(6)
				.AlphaBasedEasingFunction(EEasingFunc::CircularInOut)
				.bUseStartWidgetTransform(true)
				.StartWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(0.6, 0.6),
				                                       FVector2D::ZeroVector, 0))
				.TargetWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(0.5, 0.5),
				                                        FVector2D::ZeroVector, 0)),
				VOLT_MAKE_MODULE(UVolt_ASM_Sequence)(
					VOLT_MAKE_MODULE(UVolt_ASM_InterpRenderOpacity)
					.InterpolationMode(EVoltInterpMode::AlphaBased)
					.AlphaBasedDuration(0.1)
					.AlphaBasedBlendExp(6)
					.AlphaBasedEasingFunction(EEasingFunc::CircularOut)
					.bUseStartOpacity(true)
					.StartOpacity(0)
					.TargetOpacity(0.5),
					VOLT_MAKE_MODULE(UVolt_ASM_InterpRenderOpacity)
					.InterpolationMode(EVoltInterpMode::AlphaBased)
					.AlphaBasedDuration(0.9)
					.AlphaBasedBlendExp(6)
					.AlphaBasedEasingFunction(EEasingFunc::CircularIn)
					.bUseStartOpacity(true)
					.StartOpacity(0.5)
					.TargetOpacity(0)
				)
			)
		)
	);

	Track = VOLT_PLAY_ANIM(SlateBorder, Anim);
}

void SJointBuildPreset::Construct(const FArguments& InArgs)
{
	ParentGraphNodeSlate = InArgs._ParentGraphNodeSlate;

	PopulateSlate();
}

void SJointBuildPreset::PopulateSlate()
{
	if (UJointBuildPreset* Preset = GetBuildTargetPreset())
	{
		this->ChildSlot.DetachWidget();

		FLinearColor NormalColor = Preset->PresetColor;
		FLinearColor HoverColor = Preset->PresetColor * 1.5;
		HoverColor.A = 0.1;
		FLinearColor OutlineNormalColor = Preset->PresetColor * 1.5;
		FLinearColor OutlineHoverColor = Preset->PresetColor * 1.5;
		OutlineHoverColor.A = 0.1;

		this->ChildSlot
		[
			SAssignNew(PresetBorder, SJointOutlineBorder)
			.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.NormalColor(NormalColor)
			.HoverColor(HoverColor)
			.OutlineNormalColor(OutlineNormalColor)
			.OutlineHoverColor(OutlineHoverColor)
			.OnHovered(this, &SJointBuildPreset::OnHovered)
			.OnUnhovered(this, &SJointBuildPreset::OnUnHovered)
			.ContentPadding(FJointEditorStyle::Margin_Normal)
			[
				SAssignNew(PresetTextBlock, STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h1")
				.Text(Preset->PresetInitial)
			]
		];
	}
}

void SJointBuildPreset::Update()
{
	if (UJointBuildPreset* Preset = GetBuildTargetPreset())
	{
		if (PresetBorder.IsValid())
		{
			FLinearColor NormalColor = Preset->PresetColor;
			FLinearColor HoverColor = Preset->PresetColor * 1.5;
			HoverColor.A = 0.1;
			FLinearColor OutlineNormalColor = Preset->PresetColor * 1.5;
			FLinearColor OutlineHoverColor = Preset->PresetColor * 1.5;
			OutlineHoverColor.A = 0.1;

			PresetBorder->NormalColor = NormalColor;
			PresetBorder->HoverColor = HoverColor;
			PresetBorder->OutlineNormalColor = OutlineNormalColor;
			PresetBorder->OutlineHoverColor = OutlineHoverColor;

			PresetBorder->PlayUnHoverAnimation();
		}
		if (PresetTextBlock.IsValid())
		{
			PresetTextBlock->SetText(Preset->PresetInitial);
		}
	}
}

UJointBuildPreset* SJointBuildPreset::GetBuildTargetPreset()
{
	if (ParentGraphNodeSlate.Pin().IsValid())
	{
		if (UJointEdGraphNode* EdGraphNode = ParentGraphNodeSlate.Pin()->GetCastedGraphNode())
		{
			if (UJointNodeBase* NodeInstance = EdGraphNode->GetCastedNodeInstance())
			{
				TSoftObjectPtr<UJointBuildPreset> PresetSoftPtr = NodeInstance->GetBuildPreset();

				if (!PresetSoftPtr.IsNull())
				{
					if (UJointBuildPreset* Preset = PresetSoftPtr.LoadSynchronous())
					{
						return Preset;
					}
				}
			}
		}
	}

	return nullptr;
}

void SJointBuildPreset::OnHovered()
{
	if (PresetTextBlock) PresetTextBlock->SetRenderOpacity(0.1);
}


void SJointBuildPreset::OnUnHovered()
{
	if (PresetTextBlock) PresetTextBlock->SetRenderOpacity(1);
}


void SJointNodePointerSlate::Construct(const FArguments& InArgs)
{
	SetRenderTransformPivot(FVector2D(0.5, 0.5));

	StructureOwnerEdNode = InArgs._StructureOwnerEdNode;
	PointerToTargetStructure = InArgs._PointerToStructure;
	TargetJointManager = InArgs._PickingTargetJointManager;

	bShouldShowDisplayName = InArgs._bShouldShowDisplayName;
	bShouldShowNodeName = InArgs._bShouldShowNodeName;
	
	OnNodePointerPerformedDele = InArgs._OnNodePickingPerformed;
	OnPreNodeChangedDele = InArgs._OnPreNodeChanged;
	OnPostNodeChangedDele = InArgs._OnPostNodeChanged;
	OnHoveredDele = InArgs._OnHovered;
	OnUnhoveredDele = InArgs._OnUnhovered;
	
	
	SetCanTick(false);

	SJointOutlineBorder::FArguments BorderArgs = InArgs._BorderArgs;

	BorderArgs
		.OnHovered(this, &SJointNodePointerSlate::OnHovered)
		.OnUnhovered(this, &SJointNodePointerSlate::OnUnhovered);

	BorderArgs[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		[
			SAssignNew(BackgroundBox, SVerticalBox)
			.RenderOpacity(1)
			.Visibility(EVisibility::SelfHitTestInvisible)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(InArgs._ContentMargin)
			[
				SAssignNew(DisplayNameBlock, STextBlock)
				.Text(InArgs._DisplayName)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
				.Visibility(bShouldShowDisplayName ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(InArgs._ContentMargin)
			[
				SAssignNew(RawNameBlock, STextBlock)
				.Text(GetRawName())
				.ColorAndOpacity(FJointEditorStyle::Color_SolidHover)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h5")
				.Visibility(bShouldShowNodeName ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(FeatureButtonsSlate, SJointNodePointerSlateFeatureButtons)
			.OnClearButtonPressed(this, &SJointNodePointerSlate::OnClearButtonPressed)
			.OnGotoButtonPressed(this, &SJointNodePointerSlate::OnGoButtonPressed)
			.OnPickupButtonPressed(this, &SJointNodePointerSlate::OnPickupButtonPressed)
			.OnCopyButtonPressed(this, &SJointNodePointerSlate::OnCopyButtonPressed)
			.OnPasteButtonPressed(this, &SJointNodePointerSlate::OnPasteButtonPressed)
		]
	];

	this->ChildSlot.DetachWidget();

	TSharedRef<SJointOutlineBorder> BorderWidget = SNew(SJointOutlineBorder).Me() = BorderArgs;

	this->ChildSlot
	    .Padding(InArgs._ContentMargin)
	[
		BorderWidget
	];	
	
}


const FText SJointNodePointerSlate::GetRawName()
{
	if (PointerToTargetStructure == nullptr) return LOCTEXT("InvalidStructure","Invalid Structure Provided!!");

	const TSoftObjectPtr<UJointNodeBase> SavedCurrentNodeInstance = PointerToTargetStructure->Node;

	if (SavedCurrentNodeInstance == nullptr) return LOCTEXT("NoNodeInstanceText", "Not Specified");

	return FText::FromString(SavedCurrentNodeInstance->GetName());
}

void SJointNodePointerSlate::StartHighlightingNodeOnGraph()
{
	if (!PointerToTargetStructure || !PointerToTargetStructure->Node) return;

	const TSoftObjectPtr<UJointNodeBase> NodeInstance = PointerToTargetStructure->Node;

	UJointManager* JointManager = GetTargetJointManager();

	if (JointManager == nullptr) return;

	IAssetEditorInstance* EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(
		JointManager, true);

	if (EditorInstance == nullptr) return;


	FJointEditorToolkit* Toolkit = static_cast<FJointEditorToolkit*>(EditorInstance);

	if (Toolkit->GetJointManager() && Toolkit->GetJointManager()->JointGraph)
	{
		if (UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(Toolkit->GetJointManager()->JointGraph))
		{
			TSet<TWeakObjectPtr<UJointEdGraphNode>> GraphNodes = CastedGraph->GetCachedJointGraphNodes();

			for (TWeakObjectPtr<UJointEdGraphNode> Node : GraphNodes)
			{
				if (!Node.IsValid()) continue;

				if (Node->GetCastedNodeInstance() == nullptr) continue;

				if (Node->GetCastedNodeInstance() != NodeInstance) continue;
				
				Toolkit->StartHighlightingNode(Node.Get(), false);

				break;
			}
		}
	}
}

void SJointNodePointerSlate::StopHighlightingNodeOnGraph()
{
	if (!PointerToTargetStructure || !PointerToTargetStructure->Node) return;

	const TSoftObjectPtr<UJointNodeBase> NodeInstance = PointerToTargetStructure->Node;

	UJointManager* JointManager = GetTargetJointManager();

	if (JointManager == nullptr) return;

	IAssetEditorInstance* EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(
		JointManager, true);

	if (EditorInstance == nullptr) return;


	FJointEditorToolkit* Toolkit = static_cast<FJointEditorToolkit*>(EditorInstance);

	if (Toolkit->GetJointManager() && Toolkit->GetJointManager()->JointGraph)
	{
		if (UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(Toolkit->GetJointManager()->JointGraph))
		{
			TSet<TWeakObjectPtr<UJointEdGraphNode>> GraphNodes = CastedGraph->GetCachedJointGraphNodes();

			for (TWeakObjectPtr<UJointEdGraphNode> Node : GraphNodes)
			{
				if (!Node.IsValid()) continue;

				if (Node->GetCastedNodeInstance() == nullptr) continue;

				if (Node->GetCastedNodeInstance() != NodeInstance) continue;
					
				Toolkit->StopHighlightingNode(Node.Get());
					
				break;
			}
		}
	}
}

void SJointNodePointerSlate::OnHovered()
{
	//Overlay Show
	BackgroundBox->SetRenderOpacity(0.5);

	FeatureButtonsSlate->UpdateVisualOnHovered();
	OnHoveredDele.ExecuteIfBound();

	StartHighlightingNodeOnGraph();
	
}

void SJointNodePointerSlate::OnUnhovered()
{
	BackgroundBox->SetRenderOpacity(1);

	FeatureButtonsSlate->UpdateVisualOnUnhovered();
	OnUnhoveredDele.ExecuteIfBound();

	StopHighlightingNodeOnGraph();
}


FReply SJointNodePointerSlate::OnPickupButtonPressed()
{
	if (PointerToTargetStructure == nullptr || TargetJointManager == nullptr) return FReply::Handled();
	
	FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(TargetJointManager);

	if (!Toolkit || !Toolkit->GetNodePickingManager().IsValid()) return FReply::Handled();
	
	if (!Toolkit->GetNodePickingManager()->IsInNodePicking())
	{
		Request = Toolkit->GetNodePickingManager()->StartNodePicking(
			StructureOwnerEdNode ? StructureOwnerEdNode->GetCastedNodeInstance() : nullptr,
			PointerToTargetStructure
		);

		//Attach lambda
		Request.Pin()->OnNodePickingPerformed = FOnNodePickingPerformed::CreateLambda(
			[this](UJointNodeBase* PickedNode)
			{
				GEditor->BeginTransaction(NSLOCTEXT("JointEdTransaction", "TransactionTitle_NodePointerChanged",
				                                    "Change Node Pointer"));
				
				OnPreNodeChangedDele.ExecuteIfBound();
				
				if (OnNodePointerPerformedDele.IsBound())
				{
					OnNodePointerPerformedDele.Execute(PickedNode);
				}
				
				OnPostNodeChangedDele.ExecuteIfBound();
				
				GEditor->EndTransaction();
			}
		);
	}
	else
	{
		Toolkit->GetNodePickingManager()->EndNodePicking();
	}

	return FReply::Handled();
}

UJointManager* SJointNodePointerSlate::GetTargetJointManager() const
{
	UJointManager* JointManager = nullptr;

	JointManager = TargetJointManager;

	if (JointManager == nullptr && StructureOwnerEdNode)
	{
		JointManager = StructureOwnerEdNode->GetJointManager();
	}

	return JointManager;
}

FReply SJointNodePointerSlate::OnGoButtonPressed()
{
	if (!PointerToTargetStructure || !PointerToTargetStructure->Node) return FReply::Handled();

	const TSoftObjectPtr<UJointNodeBase> NodeInstance = PointerToTargetStructure->Node;

	UJointManager* JointManager = GetTargetJointManager();

	FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(JointManager);

	if (!Toolkit) return FReply::Handled();

	if (Toolkit->GetJointManager() && Toolkit->GetJointManager()->JointGraph)
	{
		if (UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(Toolkit->GetJointManager()->JointGraph))
		{
			TSet<TWeakObjectPtr<UJointEdGraphNode>> GraphNodes = CastedGraph->GetCachedJointGraphNodes();

			for (TWeakObjectPtr<UJointEdGraphNode> Node : GraphNodes)
			{
				if (!Node.IsValid()) continue;

				if (Node->GetCastedNodeInstance() == nullptr) continue;

				if (Node->GetCastedNodeInstance() == NodeInstance)
				{
					Toolkit->JumpToHyperlink(Node.Get());
					
					// if(!Toolkit->GetNodePickingManager().IsValid()) break;
					//
					// if (!Toolkit->GetNodePickingManager()->IsInNodePicking() || Toolkit->GetNodePickingManager()->GetActiveRequest() != Request)
					// {
					// 	Toolkit->StartHighlightingNode(Node.Get(), true);
					// }

					break;
				}
			}
		}
	}


	return FReply::Handled();
}

FReply SJointNodePointerSlate::OnCopyButtonPressed()
{
	FString Value;

	if (PointerToTargetStructure) Value = PointerToTargetStructure->Node.ToString();

	FPlatformApplicationMisc::ClipboardCopy(*Value);

	if (FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(TargetJointManager))
	{
		Toolkit->PopulateNodePickerCopyToastMessage();
	}

	return FReply::Handled();
}

FReply SJointNodePointerSlate::OnPasteButtonPressed()
{
	StopHighlightingNodeOnGraph();
	
	FString Value;

	FPlatformApplicationMisc::ClipboardPaste(Value);
	
	GEditor->BeginTransaction(FGenericCommands::Get().Paste->GetDescription());
	
	if (StructureOwnerEdNode)
	{
		StructureOwnerEdNode->Modify();

		if (StructureOwnerEdNode->GetCastedNodeInstance()) StructureOwnerEdNode->GetCastedNodeInstance()->Modify();
	}

	OnPreNodeChangedDele.ExecuteIfBound();
	
	if (Value.IsEmpty() || Value.Equals(TEXT("None"), ESearchCase::CaseSensitive) || !
		FPackageName::IsShortPackageName(Value))
	{
		if (PointerToTargetStructure)
		{
			TSoftObjectPtr<UJointNodeBase> Node;
			Node = FSoftObjectPath(Value);
			
			// check if the node is originated from the same Joint Manager
			UJointManager* InTargetJointManager = GetTargetJointManager();
			if (InTargetJointManager && Node.Get())
			{
				if (Node.Get()->GetJointManager() == InTargetJointManager)
				{
					if (PointerToTargetStructure) PointerToTargetStructure->Node = FSoftObjectPath(Value);
					if (PointerToTargetStructure->Node.Get()) PointerToTargetStructure->EditorNode = PointerToTargetStructure->Node.Get()->EdGraphNode.Get();
				}
			}
		}
	}

	if (FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(TargetJointManager))
	{
		Toolkit->PopulateNodePickerPastedToastMessage();
	}

	if (StructureOwnerEdNode) StructureOwnerEdNode->ReconstructNode();

	OnPostNodeChangedDele.ExecuteIfBound();

	GEditor->EndTransaction();
	
	StartHighlightingNodeOnGraph();

	return FReply::Handled();
}

FReply SJointNodePointerSlate::OnClearButtonPressed()
{
	StopHighlightingNodeOnGraph();
	
	GEditor->BeginTransaction(NSLOCTEXT("JointEdTransaction", "TransactionTitle_NodePointerReset","Reset Node Pointer to default"));
	
	OnPreNodeChangedDele.ExecuteIfBound();
	
	if (StructureOwnerEdNode)
	{
		StructureOwnerEdNode->Modify();

		if (StructureOwnerEdNode->GetCastedNodeInstance()) StructureOwnerEdNode->GetCastedNodeInstance()->Modify();
	}

	if (PointerToTargetStructure)
	{
		PointerToTargetStructure->Node.Reset();
		PointerToTargetStructure->EditorNode.Reset();	
	}

	if (StructureOwnerEdNode) StructureOwnerEdNode->ReconstructNode();

	OnPostNodeChangedDele.ExecuteIfBound();

	GEditor->EndTransaction();

	StartHighlightingNodeOnGraph();

	return FReply::Handled();
}


void SJointNodePointerSlateFeatureButtons::Construct(const FArguments& InArgs)
{
	SetRenderTransformPivot(FVector2D(0.5, 0.5));

	SetCanTick(false);

	this->ChildSlot.DetachWidget();

	this->ChildSlot
	    .Padding(FJointEditorStyle::Margin_Normal)
	[
		SAssignNew(ButtonHorizontalBox, SHorizontalBox)
		.Visibility(EVisibility::Visible)
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SJointOutlineButton)
			.NormalColor(FLinearColor::Transparent)
			.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
			.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OutlineNormalColor(FLinearColor::Transparent)
			.ContentPadding(FJointEditorStyle::Margin_Tiny)
			.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
			.ToolTipText(LOCTEXT("PickupButtonTooltip", "Pick up a new node for the property."))
			.OnClicked(InArgs._OnPickupButtonPressed)
			.RenderOpacity(BUTTON_INITIAL_OPACITY)
			.RenderTransform(BUTTON_INITIAL_TRANSFORM)
			[
				SNew(SImage)
				.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.EyeDropper"))
				.DesiredSizeOverride(FVector2D(16, 16))
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SJointOutlineButton)
			.NormalColor(FLinearColor::Transparent)
			.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
			.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OutlineNormalColor(FLinearColor::Transparent)
			.ContentPadding(FJointEditorStyle::Margin_Tiny)
			.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
			.ToolTipText(LOCTEXT("GoButtonTooltip", "Go to the node selected"))
			.OnClicked(InArgs._OnGotoButtonPressed)
			.RenderOpacity(BUTTON_INITIAL_OPACITY)
			.RenderTransform(BUTTON_INITIAL_TRANSFORM)
			[
				SNew(SImage)
				.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.ArrowRight"))
				.DesiredSizeOverride(FVector2D(16, 16))
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SJointOutlineButton)
			.NormalColor(FLinearColor::Transparent)
			.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
			.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OutlineNormalColor(FLinearColor::Transparent)
			.ContentPadding(FJointEditorStyle::Margin_Tiny)
			.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
			.ToolTipText(LOCTEXT("CopyButtonTooltip", "Copy the structure to the clipboard"))
			.OnClicked(InArgs._OnCopyButtonPressed)
			.RenderOpacity(BUTTON_INITIAL_OPACITY)
			.RenderTransform(BUTTON_INITIAL_TRANSFORM)
			[
				SNew(SImage)
				.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("GenericCommands.Copy"))
				.DesiredSizeOverride(FVector2D(16, 16))
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SJointOutlineButton)
			.NormalColor(FLinearColor::Transparent)
			.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
			.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OutlineNormalColor(FLinearColor::Transparent)
			.ContentPadding(FJointEditorStyle::Margin_Tiny)
			.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
			.ToolTipText(LOCTEXT("PasteTooltip", "Paste the structure from the clipboard"))
			.OnClicked(InArgs._OnPasteButtonPressed)
			.RenderOpacity(BUTTON_INITIAL_OPACITY)
			.RenderTransform(BUTTON_INITIAL_TRANSFORM)
			[
				SNew(SImage)
				.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("GenericCommands.Paste"))
				.DesiredSizeOverride(FVector2D(16, 16))
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SJointOutlineButton)
			.NormalColor(FLinearColor::Transparent)
			.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
			.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OutlineNormalColor(FLinearColor::Transparent)
			.ContentPadding(FJointEditorStyle::Margin_Tiny)
			.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
			.ToolTipText(LOCTEXT("ResetTooltip", "Reset the structure"))
			.OnClicked(InArgs._OnClearButtonPressed)
			.RenderOpacity(BUTTON_INITIAL_OPACITY)
			.RenderTransform(BUTTON_INITIAL_TRANSFORM)
			[
				SNew(SImage)
				.DesiredSizeOverride(FVector2D(16, 16))
				.ColorAndOpacity(FLinearColor(1, 0.5, 0.3))
				.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Unlink"))
			]
		]
	];

	// Initialize the button animations ( TODO: Fix this bs )
	UpdateVisualOnUnhovered();
}

void SJointNodePointerSlateFeatureButtons::UpdateVisualOnHovered()
{
	// Clean up old anim tracks
	for (const FVoltAnimationTrack& ButtonAnimTrack : ButtonAnimTracks)
	{
		VOLT_STOP_ANIM(ButtonAnimTrack);
	}

	ButtonAnimTracks.Empty();

	// Play animation
	for (int i = 0; i < ButtonHorizontalBox->GetChildren()->Num(); i++)
	{
		TSharedPtr<SWidget> ChildWidget = ButtonHorizontalBox->GetChildren()->GetChildAt(i);

		UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
		(
			VOLT_MAKE_MODULE(UVolt_ASM_Sequence)
			(
				VOLT_MAKE_MODULE(UVolt_ASM_Delay)
				.Duration(i * BUTTON_ANIM_DELAY_PER_ICON),
				VOLT_MAKE_MODULE(UVolt_ASM_Simultaneous)
				(
					VOLT_MAKE_MODULE(UVolt_ASM_InterpRenderOpacity)
					.InterpolationMode(EVoltInterpMode::AlphaBased)
					.AlphaBasedDuration(0.3)
					.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
					.AlphaBasedBlendExp(6)
					.TargetOpacity(1),
					VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
					.InterpolationMode(EVoltInterpMode::AlphaBased)
					.AlphaBasedDuration(0.25)
					.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
					.AlphaBasedBlendExp(6)
					.TargetWidgetTransform(FWidgetTransform(
						                                                 FVector2D::ZeroVector,
						                                                 FVector2D(1.0, 1.0),
						                                                 FVector2D::ZeroVector,
						                                                 0))
				)
			)
		);

		ButtonAnimTracks.Add(VOLT_PLAY_ANIM(ChildWidget, Anim));
	}
}

void SJointNodePointerSlateFeatureButtons::UpdateVisualOnUnhovered()
{
	// Clean up old anim tracks
	for (const FVoltAnimationTrack& ButtonAnimTrack : ButtonAnimTracks)
	{
		VOLT_STOP_ANIM(ButtonAnimTrack);
	}

	ButtonAnimTracks.Empty();

	// Play animation
	for (int i = 0; i < ButtonHorizontalBox->GetChildren()->Num(); i++)
	{
		TSharedPtr<SWidget> ChildWidget = ButtonHorizontalBox->GetChildren()->GetChildAt(i);

		UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
		(
			VOLT_MAKE_MODULE(UVolt_ASM_Sequence)
			(
				VOLT_MAKE_MODULE(UVolt_ASM_Delay)
				.Duration(i * BUTTON_ANIM_DELAY_PER_ICON),
				VOLT_MAKE_MODULE(UVolt_ASM_Simultaneous)
				(
					VOLT_MAKE_MODULE(UVolt_ASM_InterpRenderOpacity)
					.InterpolationMode(EVoltInterpMode::AlphaBased)
					.AlphaBasedDuration(0.3)
					.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
					.AlphaBasedBlendExp(6)
					.TargetOpacity(0),
					VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
					.InterpolationMode(EVoltInterpMode::AlphaBased)
					.AlphaBasedDuration(0.25)
					.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
					.AlphaBasedBlendExp(6)
					.TargetWidgetTransform(FWidgetTransform(
						                                                 FVector2D(0, 10),
						                                                 FVector2D(1.0, 1.0),
						                                                 FVector2D::ZeroVector,
						                                                 0))
				)
			)
		);

		ButtonAnimTracks.Add(VOLT_PLAY_ANIM(ChildWidget, Anim));
	}
}

SLATE_IMPLEMENT_WIDGET(SJointSlateDrawer)
void SJointSlateDrawer::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
}


SJointSlateDrawer::FSlot& SJointSlateDrawer::GetSlot(int32 SlotIndex)
{
	check(this->IsValidSlotIndex(SlotIndex));
	FSlotBase& BaseSlot = static_cast<FSlotBase&>(Children[SlotIndex]);
	return static_cast<SJointSlateDrawer::FSlot&>(BaseSlot);
}

const SJointSlateDrawer::FSlot& SJointSlateDrawer::GetSlot(int32 SlotIndex) const
{
	check(this->IsValidSlotIndex(SlotIndex));
	const FSlotBase& BaseSlot = static_cast<const FSlotBase&>(Children[SlotIndex]);
	return static_cast<const SJointSlateDrawer::FSlot&>(BaseSlot);
}


void SJointSlateDrawer::Construct(const FArguments& InArgs)
{
	SetRenderTransformPivot(FVector2D(0.5, 0.5));
	SetCanTick(false);

	Children.Reserve(InArgs._Slots.Num());

	for (const FSlot::FSlotArguments& Arg : InArgs._Slots)
	{
		// Because we want to override the AutoWidth, the base class doesn't exactly have the same parent.
		//We are casting from parent to child to a different parent to prevent a reinterpret_cast
		const FSlotBase::FSlotArguments& ChilSlotArgument = static_cast<const FSlotBase::FSlotArguments&>(Arg);
		const SBoxPanel::FSlot::FSlotArguments& BoxSlotArgument = static_cast<const SBoxPanel::FSlot::FSlotArguments&>(ChilSlotArgument);
		// Because InArgs is const&, we need to do some hacking here. That would need to changed in the future.
		//The Slot has a unique_ptr, it cannot be copied. Anyway, previously, the Children.Add(), was wrong if we added the same slot twice.
		//Because of that, it doesn't matter if we steal the slot from the FArguments.
		Children.AddSlot(MoveTemp(const_cast<SBoxPanel::FSlot::FSlotArguments&>(BoxSlotArgument)));
	}
	
	// Initialize the button animations ( TODO: Fix this bs )
	UpdateVisualOnUnhovered();
}

void SJointSlateDrawer::UpdateVisualOnHovered()
{
	// Clean up old anim tracks
	for (const FVoltAnimationTrack& ButtonAnimTrack : ChildrenAnimTracks)
	{
		VOLT_STOP_ANIM(ButtonAnimTrack);
	}

	ChildrenAnimTracks.Empty();

	// Play animation
	for (int i = 0; i < Children.Num(); i++)
	{
		TSharedPtr<SWidget> ChildWidget = Children.GetChildAt(i);

		UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
		(
			VOLT_MAKE_MODULE(UVolt_ASM_Sequence)
			(
				VOLT_MAKE_MODULE(UVolt_ASM_Delay)
				.Duration(i * BUTTON_ANIM_DELAY_PER_ICON),
				VOLT_MAKE_MODULE(UVolt_ASM_Simultaneous)
				(
					VOLT_MAKE_MODULE(UVolt_ASM_InterpRenderOpacity)
					.InterpolationMode(EVoltInterpMode::AlphaBased)
					.AlphaBasedDuration(0.3)
					.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
					.AlphaBasedBlendExp(6)
					.TargetOpacity(1),
					VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
					.InterpolationMode(EVoltInterpMode::AlphaBased)
					.AlphaBasedDuration(0.25)
					.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
					.AlphaBasedBlendExp(6)
					.TargetWidgetTransform(FWidgetTransform(
					FVector2D::ZeroVector,
					FVector2D(1.0, 1.0),
					FVector2D::ZeroVector,
					0))
				)
			)
		);

		ChildrenAnimTracks.Add(VOLT_PLAY_ANIM(ChildWidget, Anim));
	}
}

void SJointSlateDrawer::UpdateVisualOnUnhovered()
{
	// Clean up old anim tracks
	for (const FVoltAnimationTrack& ButtonAnimTrack : ChildrenAnimTracks)
	{
		VOLT_STOP_ANIM(ButtonAnimTrack);
	}

	ChildrenAnimTracks.Empty();

	// Play animation
	for (int i = 0; i < Children.Num(); i++)
	{
		TSharedPtr<SWidget> ChildWidget = Children.GetChildAt(i);

		UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
		(
			VOLT_MAKE_MODULE(UVolt_ASM_Sequence)
			(
				VOLT_MAKE_MODULE(UVolt_ASM_Delay)
				.Duration(i * BUTTON_ANIM_DELAY_PER_ICON),
				VOLT_MAKE_MODULE(UVolt_ASM_Simultaneous)
				(
					VOLT_MAKE_MODULE(UVolt_ASM_InterpRenderOpacity)
					.InterpolationMode(EVoltInterpMode::AlphaBased)
					.AlphaBasedDuration(0.3)
					.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
					.AlphaBasedBlendExp(6)
					.TargetOpacity(0),
					VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
					.InterpolationMode(EVoltInterpMode::AlphaBased)
					.AlphaBasedDuration(0.25)
					.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
					.AlphaBasedBlendExp(6)
					.TargetWidgetTransform(FWidgetTransform(
					FVector2D(0, 10),
					FVector2D(1.0, 1.0),
					FVector2D::ZeroVector,
					0))
				)
			)
		);

		ChildrenAnimTracks.Add(VOLT_PLAY_ANIM(ChildWidget, Anim));
	}
}

void SJointNodeDescription::Construct(const FArguments& InArgs)
{
	this->ClassToDescribe = InArgs._ClassToDescribe;

	SetCanTick(false);
	SetVisibility(EVisibility::SelfHitTestInvisible);
	PopulateSlate();
}

void SJointNodeDescription::PopulateSlate()
{
	if (ClassToDescribe == nullptr) return;

	this->ChildSlot.DetachWidget();

	this->ChildSlot
	    .Padding(FJointEditorStyle::Margin_Normal)
	[
		SNew(SJointOutlineBorder)
		.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.NormalColor(FJointEditorStyle::Color_Normal)
		.OutlineNormalColor(FJointEditorStyle::Color_Normal)
		.ContentPadding(FJointEditorStyle::Margin_Normal)
		.Visibility(EVisibility::Visible)
		[
			SNew(SVerticalBox)
			.Visibility(EVisibility::SelfHitTestInvisible)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::SelfHitTestInvisible)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SBorder)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.Visibility(EVisibility::SelfHitTestInvisible)
					.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.NodeShadow"))
					.BorderBackgroundColor(FJointEditorStyle::Color_Node_Shadow)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SNew(SBorder)
						.Visibility(EVisibility::SelfHitTestInvisible)
						.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.BorderBackgroundColor(UJointEditorSettings::Get()->DefaultNodeColor)
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(STextBlock)
							.Visibility(EVisibility::SelfHitTestInvisible)
							.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h2")
							.Text(FJointEdUtils::GetFriendlyNameFromClass(ClassToDescribe))
						]
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(STextBlock)
					.Visibility(EVisibility::SelfHitTestInvisible)
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h4")
					.ColorAndOpacity(FLinearColor(1, 1, 1, 0.7))
					.Justification(ETextJustify::Center)
					.Text(FText::FromString(ClassToDescribe->GetDesc()))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SJointOutlineButton)
					.NormalColor(FLinearColor::Transparent)
					.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
					.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.OutlineNormalColor(FLinearColor::Transparent)
					.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
					.ContentPadding(FJointEditorStyle::Margin_Normal)
					.IsEnabled(ClassToDescribe && ClassToDescribe->ClassGeneratedBy)
					.ToolTipText(ClassToDescribe && ClassToDescribe->ClassGeneratedBy
						             ? LOCTEXT("OpenEditorTooltip",
						                       "Open Blueprint Editor for the node class asset.")
						             : LOCTEXT("CantOpenEditorOnNative",
						                       "Cannot open an editor for a native node class."))
					.OnClicked(this, &SJointNodeDescription::OnOpenEditorButtonPressed)
					[
						SNew(SImage)
						.Visibility(EVisibility::SelfHitTestInvisible)
						.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.OpenInExternalEditor"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(STextBlock)
				.Visibility(EVisibility::SelfHitTestInvisible)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Italic.h5")
				.ColorAndOpacity(FLinearColor(0.7, 0.7, 0.7, 1))
				.Text(FText::FromString(ClassToDescribe->GetPathName()))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(STextBlock)
				.Visibility(EVisibility::SelfHitTestInvisible)
				.Text(ClassToDescribe->GetToolTipText())
				.AutoWrapText(true)
			]
		]
	];
}

FReply SJointNodeDescription::OnOpenEditorButtonPressed()
{
	if (ClassToDescribe && ClassToDescribe->ClassGeneratedBy)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(ClassToDescribe->ClassGeneratedBy))
		{
			GEditor->EditObject(Blueprint);
		}
	}


	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
