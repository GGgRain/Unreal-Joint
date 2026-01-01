//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "GraphNode/SJointGraphPin.h"

#include "JointEdGraphNode.h"
#include "Rendering/DrawElements.h"
#include "JointAdvancedWidgets.h"

#include "JointEditorStyle.h"
#include "VoltDecl.h"
#include "Components/HorizontalBox.h"
#include "Module/Volt_ASM_InterpRenderOpacity.h"
#include "Node/JointNodeBase.h"
#include "Widgets/Input/SButton.h"

void SJointGraphPinBase::PopulateSlate()
{

	TSharedPtr<SHorizontalBox> FullWidget;
	
	FLinearColor Color = (GetPinColor().GetSpecifiedColor() * 0.25).GetClamped(0.03,1);
	Color.A = 1;

	FMargin Margin = FJointEditorStyle::Margin_Normal * 1.2;
	FMargin HorizontalOnlyMargin = Margin;
	HorizontalOnlyMargin.Bottom = 0;
	HorizontalOnlyMargin.Top = 0;
	
	if(GetDirection() == EGPD_Input)
	{
		FullWidget =
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(HorizontalOnlyMargin)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				GetLabelWidget(NAME_DefaultPinLabelStyle)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SJointOutlineBorder)
				.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.ContentPadding(Margin)
				.NormalColor(Color)
				.HoverColor(Color * 2)
				.OutlineNormalColor(Color)
				.OutlineHoverColor(Color * 10)
				.bUseOutline(true)
				.OnHovered(this, &SJointGraphPinBase::OnHovered)
				.OnUnhovered(this, &SJointGraphPinBase::OnUnHovered)
			];
	}else
	{
		FullWidget =
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SJointOutlineBorder)
				.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.ContentPadding(Margin)
				.NormalColor(Color)
				.HoverColor(Color * 2)
				.OutlineNormalColor(Color)
				.OutlineHoverColor(Color * 10)
				.bUseOutline(true)
				.OnHovered(this, &SJointGraphPinBase::OnHovered)
				.OnUnhovered(this, &SJointGraphPinBase::OnUnHovered)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(HorizontalOnlyMargin)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				GetLabelWidget(NAME_DefaultPinLabelStyle)
			];
	}

	FullPinHorizontalRowWidget = FullWidget;
	
	// Set up a hover for pins that is tinted the color of the pin.
	SBorder::Construct(SBorder::FArguments()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		.BorderBackgroundColor(FLinearColor::Transparent)
		.Padding(0)
		.OnMouseButtonDown(this, &SJointGraphPinBase::OnPinNameMouseDown)
		[
			FullWidget.ToSharedRef()
		]
	);
}

void SJointGraphPinBase::OnHovered()
{
	VOLT_STOP_ANIM(TextBlockAnimationTrack);

	const UVoltAnimation* Animation = VOLT_MAKE_ANIMATION()
		(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpRenderOpacity)
			.InterpolationMode(EVoltInterpMode::RateBased)
			.RateBasedInterpSpeed(10)
			.bUseStartOpacity(true)
			.StartOpacity(TextBlock->GetRenderOpacity())
			.TargetOpacity(1)
		);
	
	TextBlockAnimationTrack = VOLT_PLAY_ANIM(TextBlock, Animation);
}

void SJointGraphPinBase::OnUnHovered()
{
	
	VOLT_STOP_ANIM(TextBlockAnimationTrack);

	const UVoltAnimation* Animation = VOLT_MAKE_ANIMATION()
		(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpRenderOpacity)
			.InterpolationMode(EVoltInterpMode::RateBased)
			.RateBasedInterpSpeed(10)
			.bUseStartOpacity(true)
			.StartOpacity(TextBlock->GetRenderOpacity())
			.TargetOpacity(ShouldAlwaysDisplayNameText() ? 0.6 : 0)
		);
	
	TextBlockAnimationTrack = VOLT_PLAY_ANIM(TextBlock, Animation);
}

void SJointGraphPinBase::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
	GraphPinObj = InPin;

	SetCanTick(false);

	this->SetCursor(EMouseCursor::Default);

	CachePinData();
	
	PopulateSlate();
	
}


TSharedRef<SWidget> SJointGraphPinBase::GetLabelWidget(const FName& InLabelStyle)
{
	
	return SAssignNew(TextBlock, STextBlock)
		.RenderOpacity(ShouldAlwaysDisplayNameText() ? 0.6 : 0)
		.Text(GraphPinObj && GraphPinObj->GetOwningNode() && GraphPinObj->GetOwningNode()->GetSchema() ? GetPinLabel() : FText::GetEmpty())
		.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h3")
		.Visibility(bShowLabel ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
		.ColorAndOpacity(this, &SJointGraphPinBase::GetPinTextColor);
}

void SJointGraphPinBase::CachePinData()
{
	CachedPinData = GetPinDataForCurrentPin();
}

FJointEdPinData* SJointGraphPinBase::GetCachedPinData() const
{
	return CachedPinData;
}

FJointEdPinData* SJointGraphPinBase::GetPinDataForCurrentPin() const
{
	if(UJointEdGraphNode* InGraphNode = GetPinParentEdNode())
	{
		if (FJointEdPinData* PinData = InGraphNode->GetPinDataForPinFromHierarchy(this->GetPinObj())) return PinData;
	}

	return nullptr;
}

UJointEdGraphNode* SJointGraphPinBase::GetPinRealParentEdNode() const
{
	if( UJointEdGraphNode* InGraphNode = GetPinParentEdNode()) {
		
		if(UEdGraphPin* Pin = InGraphNode->FindOriginalSubNodePin(GetPinObj()))
		{
			if(UEdGraphNode* InOwnerGraphNode = Pin->GetOwningNodeUnchecked())
			{
				if(UJointEdGraphNode* InCastedGraphNode = Cast<UJointEdGraphNode>(InOwnerGraphNode))
				{
					return InCastedGraphNode;
				}
			}
		}
	}

	return nullptr;
}

FSlateColor SJointGraphPinBase::GetPinColor() const
{
	if (!bShouldUseParentNodeColorAsOwnColor) return SGraphPin::GetPinColor();

	if( UJointEdGraphNode* InGraphNode = GetPinParentEdNode()) {
		
		if(UEdGraphPin* Pin = InGraphNode->FindOriginalSubNodePin(GetPinObj()))
		{
			if(UEdGraphNode* InOwnerGraphNode = Pin->GetOwningNodeUnchecked())
			{
				if(UJointEdGraphNode* InCastedGraphNode = Cast<UJointEdGraphNode>(InOwnerGraphNode))
				{
					return InCastedGraphNode->GetNodeTitleColor();
				}
			}
		}
		
		return InGraphNode->GetNodeTitleColor();
	}
	
	return SGraphPin::GetPinColor();
}

UJointEdGraphNode* SJointGraphPinBase::GetPinParentEdNode() const
{
	return GraphPinObj && GraphPinObj->GetOwningNode() ? Cast<UJointEdGraphNode>(GraphPinObj->GetOwningNode()) : nullptr;
}

UJointNodeBase* SJointGraphPinBase::GetPinParentNode() const
{
	const UJointEdGraphNode* EdNode = GetPinParentEdNode();
	
	return EdNode && EdNode->NodeInstance ? Cast<UJointNodeBase>(EdNode->NodeInstance) : nullptr;
}

bool SJointGraphPinBase::ShouldAlwaysDisplayNameText()
{
	if(FJointEdPinData* PinData = GetCachedPinData())
	{
		return PinData->Settings.bAlwaysDisplayNameText;
	}

	return false;
}

const FSlateBrush* SJointGraphPinBase::GetPinIcon() const
{
	return FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.ChevronRight");
}
