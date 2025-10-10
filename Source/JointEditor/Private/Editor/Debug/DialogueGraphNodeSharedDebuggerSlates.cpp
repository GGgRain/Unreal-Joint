//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "Debug/JointGraphNodeSharedDebuggerSlates.h"

#include "JointEditorStyle.h"
#include "Debug/JointNodeDebugData.h"
#include "Debug/JointDebugger.h"
#include "GraphNode/SJointGraphNodeBase.h"

#include "VoltAnimationManager.h"
#include "VoltDecl.h"
#include "Module/Volt_ASM_InterpWidgetTransform.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "SJointBreakpointIndicator"

void SJointBreakpointIndicator::Construct(const FArguments& InArgs)
{
	ParentGraphNodeSlate = InArgs._GraphNodeSlate;

	PopulateSlate();
}

void SJointBreakpointIndicator::PopulateSlate()
{
	this->ChildSlot.DetachWidget();

	UpdateDebugData();

	this->ChildSlot
		.Padding(FJointEditorStyle::Margin_Normal)
		[
			SNew(SBox)
			.WidthOverride(22)
			.HeightOverride(22)
			[
				SAssignNew(BreakpointImage, SImage)
				.RenderTransformPivot(FVector2D(0.5, 0.5))
				.Image(GetBreakpointImageBrush())
				.ColorAndOpacity(GetBreakpointColor())
				.ToolTipText(GetBreakpointTooltipText())
			]
		];
}

void SJointBreakpointIndicator::UpdateVisual()
{
	UpdateDebugData();

	if(BreakpointImage.IsValid())
	{
		BreakpointImage->SetColorAndOpacity(GetBreakpointColor());
		BreakpointImage->SetImage(GetBreakpointImageBrush());
		BreakpointImage->SetToolTipText(GetBreakpointTooltipText());
	}
}

const FSlateColor SJointBreakpointIndicator::GetBreakpointColor()
{
	if (DebugData == nullptr) return FLinearColor::Transparent;

	if (DebugData->bHasBreakpoint)
	{
		if (UJointDebugger::Get()->IsDebuggerEnabled())
		{
			return DebugData->bIsBreakpointEnabled ? FLinearColor(1, 0.4, 0.2) : FLinearColor(0.5, 0.2, 0.1);
		}
		else
		{
			return DebugData->bIsBreakpointEnabled
					   ? FLinearColor(0.01, 0.05, 1, 0.95)
					   : FLinearColor(0.01, 0.05, 1, 0.95);
		}
	}

	return FLinearColor::Transparent;
}

const FSlateBrush* SJointBreakpointIndicator::GetBreakpointImageBrush()
{
	const FSlateBrush* OutBrush = nullptr;
	
	if (DebugData != nullptr)
	{
		if (DebugData->bHasBreakpoint)
		{
			if (DebugData->bIsBreakpointEnabled)
			{
				OutBrush = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(
					"Kismet.DebuggerOverlay.Breakpoint.EnabledAndValid");
			}
			else
			{
				OutBrush = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(
					"Kismet.DebuggerOverlay.Breakpoint.Disabled");
			}
		}
	}
	return OutBrush;
}

const FText SJointBreakpointIndicator::GetBreakpointTooltipText()
{
	if (DebugData != nullptr)
	{
		if (UJointDebugger::Get()->IsDebuggerEnabled())
		{
			if (DebugData->bHasBreakpoint)
			{
				return DebugData->bIsBreakpointEnabled
						   ? LOCTEXT("BreakpointEnabled",
									 "This node has a breakpoint. the debugger will stop halt the playback when met this node.")
						   : LOCTEXT("BreakpointDisabled",
									 "This node has a breakpoint but disabled. the debugger will not halt the playback on this node.");
			}
		}
		else
		{
			if (DebugData->bHasBreakpoint)
			{
				return DebugData->bIsBreakpointEnabled
						   ? LOCTEXT("BreakpointDisabled_DebuggerDisabled",
									 "This node has a breakpoint, but the debugger is diabled. will not affect the exection.")
						   : LOCTEXT("BreakpointDisabled_DebuggerDisabled",
									 "This node has a breakpoint but disabled, and the debugger is diabled. will not affect the exection.");
			}
		}
	}

	return LOCTEXT("Breakpoint", "This indicates that this node has a breakpoint.");
}

void SJointBreakpointIndicator::UpdateDebugData()
{
	if (DebugData == nullptr)
	{
		//Try to collect it.
		if (this->ParentGraphNodeSlate.IsValid())
		{
			UJointEdGraphNode* Node =this->ParentGraphNodeSlate.Pin()->GetCastedGraphNode();
			
			DebugData = UJointDebugger::GetDebugDataForInstance(Node);
		}
	}
	
}

void SJointBreakpointIndicator::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);

	const UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
		.InterpolationMode(EVoltInterpMode::AlphaBased)
		.AlphaBasedEasingFunction(EEasingFunc::EaseOut)
		.AlphaBasedDuration(0.2)
		.AlphaBasedBlendExp(6)
		.TargetWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(1.25, 1.25), FVector2D::ZeroVector, 0))
	);

	VOLT_STOP_ALL_ANIM(BreakpointImage);

	VOLT_PLAY_ANIM(BreakpointImage, Anim);
}

void SJointBreakpointIndicator::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseLeave(MouseEvent);

	const UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
		.InterpolationMode(EVoltInterpMode::AlphaBased)
		.AlphaBasedEasingFunction(EEasingFunc::EaseIn)
		.AlphaBasedDuration(0.35)
		.AlphaBasedBlendExp(6)
		.TargetWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(1.0, 1.0), FVector2D::ZeroVector, 0))
	);

	VOLT_STOP_ALL_ANIM(BreakpointImage);

	VOLT_PLAY_ANIM(BreakpointImage, Anim);
}


#undef LOCTEXT_NAMESPACE
