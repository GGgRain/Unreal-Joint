#include "JointAdvancedWidgets.h"

#include "VoltDecl.h"
#include "Module/Volt_ASM_Delay.h"
#include "Module/Volt_ASM_InterpBackgroundColor.h"
#include "Module/Volt_ASM_InterpBoxProperties.h"
#include "Module/Volt_ASM_InterpColor.h"
#include "Module/Volt_ASM_InterpForegroundColor.h"
#include "Module/Volt_ASM_InterpRenderOpacity.h"
#include "Module/Volt_ASM_InterpWidgetTransform.h"
#include "Module/Volt_ASM_Sequence.h"
#include "Module/Volt_ASM_SetWidgetTransformPivot.h"
#include "Module/Volt_ASM_Simultaneous.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "JointAdvancedWidgets"

void SJointOutlineBorder::Construct(const FArguments& InArgs)
{
	Content = InArgs._Content.Widget;

	NormalColor = InArgs._NormalColor;
	HoverColor = InArgs._HoverColor;

	OutlineNormalColor = InArgs._OutlineNormalColor;
	OutlineHoverColor = InArgs._OutlineHoverColor;

	NormalTransform = InArgs._NormalTransform;
	HoverTransform = InArgs._HoverTransform;

	OnHovered = InArgs._OnHovered;
	OnUnhovered = InArgs._OnUnhovered;


	UnhoveredCheckingInterval = InArgs._UnhoveredCheckingInterval;
	HoverAnimationSpeed = InArgs._HoverAnimationSpeed;
	UnHoverAnimationSpeed = InArgs._UnHoverAnimationSpeed;

	if (InArgs._OnMouseButtonDown.IsBound())
	{
		SetOnMouseButtonDown(InArgs._OnMouseButtonDown);
	}

	if (InArgs._OnMouseButtonUp.IsBound())
	{
		SetOnMouseButtonUp(InArgs._OnMouseButtonUp);
	}

	if (InArgs._OnMouseMove.IsBound())
	{
		SetOnMouseMove(InArgs._OnMouseMove);
	}

	if (InArgs._OnMouseDoubleClick.IsBound())
	{
		SetOnMouseDoubleClick(InArgs._OnMouseDoubleClick);
	}

	if (InArgs._bUseOutline)
	{
		ChildSlot[
			SAssignNew(OuterBorder, SBorder)
			.RenderTransformPivot(FVector2D(0.5, 0.5))
			.BorderBackgroundColor(OutlineNormalColor)
			.BorderImage(InArgs._OuterBorderImage)
			.Padding(InArgs._OutlinePadding)
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			.Visibility(EVisibility::SelfHitTestInvisible)
			[
				SAssignNew(InnerBorder, SBorder)
				.BorderBackgroundColor(NormalColor)
				.BorderImage(InArgs._InnerBorderImage)
				.Padding(InArgs._ContentPadding)
				.VAlign(InArgs._VAlign)
				.HAlign(InArgs._HAlign)
				.Visibility(EVisibility::SelfHitTestInvisible)
				[
					InArgs
					._Content
					.Widget
				]
			]
		];
	}
	else
	{
		ChildSlot[
			SAssignNew(InnerBorder, SBorder)
			.BorderBackgroundColor(NormalColor)
			.BorderImage(InArgs._InnerBorderImage)
			.Padding(InArgs._ContentPadding)
			.VAlign(InArgs._VAlign)
			.HAlign(InArgs._HAlign)
			.Visibility(EVisibility::SelfHitTestInvisible)
			[
				InArgs
				._Content
				.Widget
			]
		];
	}

	PlayUnHoverAnimation();
}

void SJointOutlineBorder::StopAllAnimation()
{
	VOLT_STOP_ANIM(InnerBorderBackgroundColorTrack);

	VOLT_STOP_ANIM(OuterBorderBackgroundColorTrack);

	VOLT_STOP_ANIM(OuterBorderTransformTrack);
}

void SJointOutlineBorder::PlayHoverAnimation()
{
	if (InnerBorder)
	{
		VOLT_STOP_ANIM(InnerBorderBackgroundColorTrack);

		UVoltAnimation* ContentAnim = VOLT_MAKE_ANIMATION()(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
			.TargetColor(HoverColor)
			.RateBasedInterpSpeed(HoverAnimationSpeed)
		);

		InnerBorderBackgroundColorTrack = VOLT_PLAY_ANIM(InnerBorder->AsShared(), ContentAnim);
	}

	if (OuterBorder)
	{
		VOLT_STOP_ANIM(OuterBorderBackgroundColorTrack);

		VOLT_STOP_ANIM(OuterBorderTransformTrack);

		UVoltAnimation* OutlineAnim = VOLT_MAKE_ANIMATION()(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
			.TargetColor(OutlineHoverColor)
			.RateBasedInterpSpeed(HoverAnimationSpeed)
		);

		UVoltAnimation* OutlineTransformAnim = VOLT_MAKE_ANIMATION()(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
			.TargetWidgetTransform(HoverTransform)
			.RateBasedInterpSpeed(HoverAnimationSpeed)
		);

		OuterBorderBackgroundColorTrack = VOLT_PLAY_ANIM(OuterBorder->AsShared(), OutlineAnim);

		OuterBorderTransformTrack = VOLT_PLAY_ANIM(OuterBorder->AsShared(), OutlineTransformAnim);
	}
}

void SJointOutlineBorder::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	PlayHoverAnimation();

	RegisterActiveTimer(UnhoveredCheckingInterval,
	                    FWidgetActiveTimerDelegate::CreateSP(this, &SJointOutlineBorder::CheckUnhovered));

	OnHovered.ExecuteIfBound();

	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
}

void SJointOutlineBorder::PlayUnHoverAnimation()
{
	if (InnerBorder)
	{
		VOLT_STOP_ANIM(InnerBorderBackgroundColorTrack);

		UVoltAnimation* ContentAnim = VOLT_MAKE_ANIMATION()(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
			.TargetColor(NormalColor)
			.RateBasedInterpSpeed(UnHoverAnimationSpeed)
		);

		InnerBorderBackgroundColorTrack = VOLT_PLAY_ANIM(InnerBorder->AsShared(), ContentAnim);
	}

	if (OuterBorder)
	{
		VOLT_STOP_ANIM(OuterBorderBackgroundColorTrack);

		VOLT_STOP_ANIM(OuterBorderTransformTrack);

		UVoltAnimation* OutlineAnim = VOLT_MAKE_ANIMATION()(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
			.TargetColor(OutlineNormalColor)
			.RateBasedInterpSpeed(UnHoverAnimationSpeed)
		);

		UVoltAnimation* OutlineTransformAnim = VOLT_MAKE_ANIMATION()(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
			.TargetWidgetTransform(NormalTransform)
			.RateBasedInterpSpeed(UnHoverAnimationSpeed)
		);

		OuterBorderBackgroundColorTrack = VOLT_PLAY_ANIM(OuterBorder->AsShared(), OutlineAnim);

		OuterBorderTransformTrack = VOLT_PLAY_ANIM(OuterBorder->AsShared(), OutlineTransformAnim);
	}
}

EActiveTimerReturnType SJointOutlineBorder::CheckUnhovered(double InCurrentTime, float InDeltaTime)
{
	//If it's still hovered, Continue.
	if (IsHovered())
	{
		return EActiveTimerReturnType::Continue;
	}

	PlayUnHoverAnimation();

	OnUnhovered.ExecuteIfBound();

	return EActiveTimerReturnType::Stop;
}


void SJointOutlineButton::Construct(const FArguments& InArgs)
{
	Content = InArgs._Content.Widget;

	NormalColor = InArgs._NormalColor;
	HoverColor = InArgs._HoverColor;
	PressColor = InArgs._PressColor;

	OutlineNormalColor = InArgs._OutlineNormalColor;
	OutlineHoverColor = InArgs._OutlineHoverColor;
	OutlinePressColor = InArgs._OutlinePressColor;

	OnClicked = InArgs._OnClicked;
	OnPressed = InArgs._OnPressed;
	OnReleased = InArgs._OnReleased;
	OnHovered = InArgs._OnHovered;
	OnUnhovered = InArgs._OnUnhovered;

	UnhoveredCheckingInterval = InArgs._UnhoveredCheckingInterval;

	ChildSlot[
		SAssignNew(Border, SBorder)
		.BorderBackgroundColor(OutlineNormalColor)
		.BorderImage(InArgs._OutlineBorderImage)
		.Padding(InArgs._OutlinePadding)
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			SAssignNew(Button, SButton)
			.ButtonStyle(InArgs._ButtonStyle)
			.ButtonColorAndOpacity(NormalColor)
			.ContentPadding(InArgs._ContentPadding)
			.OnClicked(OnClicked)
			.OnReleased(OnReleased)
			.OnPressed(this, &SJointOutlineButton::EventOnPressed)
			.OnHovered(this, &SJointOutlineButton::EventOnHovered)
			.VAlign(InArgs._VAlign)
			.HAlign(InArgs._HAlign)
			[
				Content.ToSharedRef()
			]
		]
	];
}

void SJointOutlineButton::PlayPressedAnim()
{
	VOLT_STOP_ANIM(TransformTrack);
	VOLT_STOP_ANIM(ColorTrack);
	VOLT_STOP_ANIM(OutlineColorTrack);

	UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_SetWidgetTransformPivot)
		.TargetWidgetTransformPivot(FVector2D(0.5, 0.5)),
		VOLT_MAKE_MODULE(UVolt_ASM_Sequence)
		.bShouldLoop(false)
		(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
			.TargetWidgetTransform(FWidgetTransform(FVector2D(0, 0), FVector2D(0.95, 0.95), FVector2D(0, 0), 0))
			.RateBasedInterpSpeed(45),
			VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
			.TargetWidgetTransform(FWidgetTransform(FVector2D(0, 0), FVector2D(1, 1), FVector2D(0, 0), 0))
			.RateBasedInterpSpeed(20)
		)
	);

	UVoltAnimation* ColorAnim = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_Sequence)
		.bShouldLoop(false)
		(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
			.TargetColor(PressColor)
			.RateBasedInterpSpeed(45),
			VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
			.TargetColor(HoverColor)
			.RateBasedInterpSpeed(20)
		)
	);

	UVoltAnimation* BorderColorAnim = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_Sequence)
		.bShouldLoop(false)
		(
			VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
			.TargetColor(OutlinePressColor)
			.RateBasedInterpSpeed(45),
			VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
			.TargetColor(OutlineHoverColor)
			.RateBasedInterpSpeed(20)
		)
	);

	TransformTrack = VOLT_PLAY_ANIM(Button, Anim);

	ColorTrack = VOLT_PLAY_ANIM(Button, ColorAnim);

	OutlineColorTrack = VOLT_PLAY_ANIM(Border, BorderColorAnim);
}

void SJointOutlineButton::EventOnPressed()
{
	PlayPressedAnim();

	OnPressed.ExecuteIfBound();
}

void SJointOutlineButton::PlayHoveredAnimation()
{
	VOLT_STOP_ANIM(ColorTrack);
	VOLT_STOP_ANIM(OutlineColorTrack);

	UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
		.TargetColor(HoverColor)
		.RateBasedInterpSpeed(20)
	);

	UVoltAnimation* BorderColorAnim = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
		.TargetColor(OutlineHoverColor)
		.RateBasedInterpSpeed(25)
	);

	ColorTrack = VOLT_PLAY_ANIM(Button, Anim);
	OutlineColorTrack = VOLT_PLAY_ANIM(Border, BorderColorAnim);
}

void SJointOutlineButton::EventOnHovered()
{
	PlayHoveredAnimation();

	OnHovered.ExecuteIfBound();

	RegisterActiveTimer(UnhoveredCheckingInterval,
	                    FWidgetActiveTimerDelegate::CreateSP(this, &SJointOutlineButton::CheckUnhovered));
}

void SJointOutlineButton::EventOnUnHovered()
{
	PlayUnhoveredAnimation();

	OnUnhovered.ExecuteIfBound();
}

void SJointOutlineButton::PlayUnhoveredAnimation()
{
	VOLT_STOP_ANIM(ColorTrack);
	VOLT_STOP_ANIM(OutlineColorTrack);

	UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
		.TargetColor(NormalColor)
		.RateBasedInterpSpeed(20)
	);

	UVoltAnimation* BorderColorAnim = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
		.TargetColor(OutlineNormalColor)
		.RateBasedInterpSpeed(25)
	);

	ColorTrack = VOLT_PLAY_ANIM(Button, Anim);
	OutlineColorTrack = VOLT_PLAY_ANIM(Border, BorderColorAnim);
}

EActiveTimerReturnType SJointOutlineButton::CheckUnhovered(double InCurrentTime, float InDeltaTime)
{
	if (IsHovered()) return EActiveTimerReturnType::Continue;

	EventOnUnHovered();

	return EActiveTimerReturnType::Stop;
}


void SJointOutlineToggleButton::Construct(const FArguments& InArgs)
{
	OnColor = InArgs._OnColor;
	OffColor = InArgs._OffColor;

	OnPressed = InArgs._OnPressed;
	OnHovered = InArgs._OnHovered;
	OnUnhovered = InArgs._OnUnhovered;

	ButtonLength = InArgs._ButtonLength;
	ToggleMovementPercentage = InArgs._ToggleMovementPercentage;

	IsChecked = InArgs._IsChecked;
	OnCheckStateChanged = InArgs._OnCheckStateChanged;


	ChildSlot[
		SAssignNew(Button, SJointOutlineButton)
		.OutlineBorderImage(InArgs._OutlineBorderImage)
		.ButtonStyle(InArgs._ButtonStyle)
		.NormalColor(OffColor)
		.HoverColor(OffColor)
		.OnPressed(this, &SJointOutlineToggleButton::EventOnPressed)
		.OnHovered(this, &SJointOutlineToggleButton::EventOnHovered)
		.ContentPadding(FMargin(ButtonLength, 2))
		[
			SAssignNew(Toggle, SBorder)
			.BorderImage(InArgs._ToggleImage)
			.Padding(FMargin(InArgs._ToggleSize))
		]
	];

	LastState = IsChecked.Get(ECheckBoxState::Undetermined);

	PlayToggleAnimation(LastState, true);
}

void SJointOutlineToggleButton::PlayToggleAnimation(const ECheckBoxState& CheckBoxState, const bool& bInstant = false)
{
	VOLT_STOP_ALL_ANIM(Toggle->AsShared());


	UVoltAnimation* Transform = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
		.TargetWidgetTransform(FWidgetTransform(
			FVector2D(CheckBoxState == ECheckBoxState::Checked
				          ? ButtonLength * ToggleMovementPercentage
				          : ButtonLength * -ToggleMovementPercentage, 0),
			FVector2D(1, 1), FVector2D(0, 0), 0))
		.RateBasedInterpSpeed(15)
	);

	Button->HoverColor = CheckBoxState == ECheckBoxState::Checked ? OnColor : OffColor;
	Button->NormalColor = CheckBoxState == ECheckBoxState::Checked ? OnColor : OffColor;
	Button->PressColor = CheckBoxState == ECheckBoxState::Checked ? OnColor : OffColor;

	if (bInstant)
	{
		Button->PlayUnhoveredAnimation();
	}
	else
	{
		Button->PlayPressedAnim();
	}

	VOLT_PLAY_ANIM(Toggle, Transform);
}

void SJointOutlineToggleButton::EventOnPressed()
{
	OnCheckStateChanged.Execute(LastState);

	const ECheckBoxState& State = IsChecked.Get(ECheckBoxState::Undetermined);

	if (State != LastState)
	{
		LastState = State;

		PlayToggleAnimation(LastState);
	}
}

void SJointOutlineToggleButton::EventOnHovered()
{
	OnHovered.ExecuteIfBound();
}

void SJointOutlineToggleButton::EventOnUnHovered()
{
	OnUnhovered.ExecuteIfBound();
}

EActiveTimerReturnType SJointOutlineToggleButton::CheckUnhovered(double InCurrentTime, float InDeltaTime)
{
	if (IsHovered()) return EActiveTimerReturnType::Continue;

	EventOnUnHovered();

	return EActiveTimerReturnType::Stop;
}

#undef LOCTEXT_NAMESPACE
