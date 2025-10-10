#pragma once

#include "CoreMinimal.h"
#include "Editor/Style/JointEditorStyle.h"
#include "VoltAnimationManager.h"
#include "Slate/WidgetTransform.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SCheckBox.h"


class SButton;

class JOINTEDITOR_API SJointOutlineBorder : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SJointOutlineBorder) :
			_NormalColor(FLinearColor(0.01, 0.01, 0.01)),
			_HoverColor(FLinearColor(0.02, 0.02, 0.02)),
			_OutlineNormalColor(FLinearColor(0.01, 0.01, 0.01)),
			_OutlineHoverColor(FLinearColor(0.1, 0.1, 0.1)),
			_NormalTransform(FWidgetTransform(FVector2D(0, 0), FVector2D(1, 1), FVector2D(0, 0), 0)),
			_HoverTransform(FWidgetTransform(FVector2D(0, 0), FVector2D(1, 1), FVector2D(0, 0), 0)),
			_ContentPadding(FMargin(10)),
			_OutlinePadding(FMargin(1)),
			_HoverAnimationSpeed(20),
			_UnHoverAnimationSpeed(20),
			_bUseOutline(true),
			_UnhoveredCheckingInterval(0.02),
			_OnMouseButtonDown(),
			_OnMouseButtonUp(),
			_OnMouseMove(),
			_OnMouseDoubleClick()
		{
		}

		SLATE_ARGUMENT(EHorizontalAlignment, HAlign)
		SLATE_ARGUMENT(EVerticalAlignment, VAlign)

		SLATE_DEFAULT_SLOT(FArguments, Content)

		SLATE_ATTRIBUTE(const FSlateBrush*, OuterBorderImage)
		SLATE_ATTRIBUTE(const FSlateBrush*, InnerBorderImage)

		SLATE_ARGUMENT(FLinearColor, NormalColor)
		SLATE_ARGUMENT(FLinearColor, HoverColor)

		SLATE_ARGUMENT(FLinearColor, OutlineNormalColor)
		SLATE_ARGUMENT(FLinearColor, OutlineHoverColor)

		SLATE_ARGUMENT(FWidgetTransform, NormalTransform)
		SLATE_ARGUMENT(FWidgetTransform, HoverTransform)

		SLATE_ARGUMENT(FMargin, ContentPadding)
		SLATE_ARGUMENT(FMargin, OutlinePadding)

		SLATE_ARGUMENT(float, HoverAnimationSpeed)
		SLATE_ARGUMENT(float, UnHoverAnimationSpeed)

		SLATE_ARGUMENT(bool, bUseOutline)

		/**
		 * The interval we are going to use for the slate for the collision detection.
		 * It's because the slate architecture doesn't support CCD - like mouse event bubbling.
		 */
		SLATE_ARGUMENT(float, UnhoveredCheckingInterval)

		SLATE_EVENT(FSimpleDelegate, OnHovered)
		SLATE_EVENT(FSimpleDelegate, OnUnhovered)

		SLATE_EVENT(FPointerEventHandler, OnMouseButtonDown)
		SLATE_EVENT(FPointerEventHandler, OnMouseButtonUp)
		SLATE_EVENT(FPointerEventHandler, OnMouseMove)
		SLATE_EVENT(FPointerEventHandler, OnMouseDoubleClick)

	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);

public:
	float HoverAnimationSpeed;
	float UnHoverAnimationSpeed;

public:
	FLinearColor NormalColor;
	FLinearColor HoverColor;
	FLinearColor OutlineNormalColor;
	FLinearColor OutlineHoverColor;

public:
	FWidgetTransform NormalTransform;
	FWidgetTransform HoverTransform;

public:
	TSharedPtr<SWidget> Content;

	TSharedPtr<SWidget> OuterBorder;

	TSharedPtr<SWidget> InnerBorder;

public:
	/** The delegate to execute when the button is hovered */
	FSimpleDelegate OnHovered;

	/** The delegate to execute when the button exit the hovered state */
	FSimpleDelegate OnUnhovered;

public:
	FVoltAnimationTrack InnerBorderBackgroundColorTrack;

	FVoltAnimationTrack OuterBorderTransformTrack;

	FVoltAnimationTrack OuterBorderBackgroundColorTrack;

public:
	float UnhoveredCheckingInterval;

public:
	void StopAllAnimation();

	void PlayHoverAnimation();

	void PlayUnHoverAnimation();

public:
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

public:
	EActiveTimerReturnType CheckUnhovered(double InCurrentTime, float InDeltaTime);
};


class JOINTEDITOR_API SJointOutlineButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SJointOutlineButton) :
			_OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round")),
			_ButtonStyle(&FJointEditorStyle::Get().GetWidgetStyle<FButtonStyle>("JointUI.Button.Round.White")),

			_NormalColor(FLinearColor(0.03, 0.03, 0.03)),
			_HoverColor(FLinearColor(0.06, 0.06, 0.1)),
			_PressColor(FLinearColor(0.10, 0.10, 0.20)),

			_OutlineNormalColor(FLinearColor(0.03, 0.03, 0.03)),
			_OutlineHoverColor(FLinearColor(0.6, 0.6, 0.8)),
			_OutlinePressColor(FLinearColor(0.9, 0.9, 0.9)),

			_ContentPadding(FMargin(12)),
			_OutlinePadding(FMargin(1)),

			_UnhoveredCheckingInterval(0.02)

		{
		}

		SLATE_DEFAULT_SLOT(FArguments, Content)

		SLATE_ARGUMENT(EHorizontalAlignment, HAlign)
		SLATE_ARGUMENT(EVerticalAlignment, VAlign)

		SLATE_ATTRIBUTE(const FSlateBrush*, OutlineBorderImage)
		SLATE_STYLE_ARGUMENT(FButtonStyle, ButtonStyle)

		SLATE_ARGUMENT(FLinearColor, NormalColor)
		SLATE_ARGUMENT(FLinearColor, HoverColor)
		SLATE_ARGUMENT(FLinearColor, PressColor)

		SLATE_ARGUMENT(FLinearColor, OutlineNormalColor)
		SLATE_ARGUMENT(FLinearColor, OutlineHoverColor)
		SLATE_ARGUMENT(FLinearColor, OutlinePressColor)

		SLATE_ARGUMENT(FMargin, ContentPadding)
		SLATE_ARGUMENT(FMargin, OutlinePadding)

		/**
		 * The interval we are going to use for the slate for the collision detection.
		 * It's because the slate architecture doesn't support CCD - like mouse event bubbling.
		 */
		SLATE_ARGUMENT(float, UnhoveredCheckingInterval)


		/** Called when the button is clicked */
		SLATE_EVENT(FOnClicked, OnClicked)
		/** Called when the button is pressed */
		SLATE_EVENT(FSimpleDelegate, OnPressed)
		/** Called when the button is released */
		SLATE_EVENT(FSimpleDelegate, OnReleased)
		SLATE_EVENT(FSimpleDelegate, OnHovered)
		SLATE_EVENT(FSimpleDelegate, OnUnhovered)

	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);

public:
	FLinearColor NormalColor;
	FLinearColor HoverColor;
	FLinearColor PressColor;

	FLinearColor OutlineNormalColor;
	FLinearColor OutlineHoverColor;
	FLinearColor OutlinePressColor;

public:
	TSharedPtr<SButton> Button;
	TSharedPtr<SBorder> Border;
	TSharedPtr<SWidget> Content;

public:
	FOnClicked OnClicked;

	/** The delegate to execute when the button is pressed */
	FSimpleDelegate OnPressed;

	/** The delegate to execute when the button is released */
	FSimpleDelegate OnReleased;

	/** The delegate to execute when the button is hovered */
	FSimpleDelegate OnHovered;

	/** The delegate to execute when the button exit the hovered state */
	FSimpleDelegate OnUnhovered;

public:
	float UnhoveredCheckingInterval;

public:
	FVoltAnimationTrack OutlineColorTrack;
	FVoltAnimationTrack ColorTrack;
	FVoltAnimationTrack ContentColorTrack;
	FVoltAnimationTrack TransformTrack;

public:
	void PlayHoveredAnimation();
	void PlayUnhoveredAnimation();
	void PlayPressedAnim();

public:
	void EventOnPressed();
	void EventOnHovered();
	void EventOnUnHovered();

public:
	EActiveTimerReturnType CheckUnhovered(double InCurrentTime, float InDeltaTime);
};


class JOINTEDITOR_API SJointOutlineToggleButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SJointOutlineToggleButton) :
			_ToggleImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Sphere")),
			_OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Sphere")),
			_ButtonStyle(&FJointEditorStyle::Get().GetWidgetStyle<FButtonStyle>("JointUI.Button.Sphere.White")),
			_OnColor(FLinearColor(0.1, 0.4, 0.2)),
			_OffColor(FLinearColor(0.05, 0.05, 0.05)),
			_ButtonLength(16),
			_ToggleSize(12),
			_ToggleMovementPercentage(0.8),
			_UnhoveredCheckingInterval(0.02)
		{
		}

		SLATE_ARGUMENT(UVoltAnimationManager*, AnimationManager)
		SLATE_ATTRIBUTE(const FSlateBrush*, ToggleImage)
		SLATE_ATTRIBUTE(const FSlateBrush*, OutlineBorderImage)
		SLATE_STYLE_ARGUMENT(FButtonStyle, ButtonStyle)
		SLATE_ARGUMENT(FLinearColor, OnColor)
		SLATE_ARGUMENT(FLinearColor, OffColor)
		SLATE_ARGUMENT(float, ButtonLength)
		SLATE_ARGUMENT(float, ToggleSize)
		SLATE_ARGUMENT(float, ToggleMovementPercentage)

		/**
		 * The interval we are going to use for the slate for the collision detection.
		 * It's because the slate architecture doesn't support CCD - like mouse event bubbling.
		 */
		SLATE_ARGUMENT(float, UnhoveredCheckingInterval)
		
		SLATE_ATTRIBUTE(ECheckBoxState, IsChecked)
		SLATE_EVENT(FOnCheckStateChanged, OnCheckStateChanged)
		SLATE_EVENT(FSimpleDelegate, OnPressed)
		SLATE_EVENT(FSimpleDelegate, OnHovered)
		SLATE_EVENT(FSimpleDelegate, OnUnhovered)
	SLATE_END_ARGS();

public:
	
	void Construct(const FArguments& InArgs);

public:
	
	TSharedPtr<SJointOutlineButton> Button;

	TSharedPtr<SWidget> Toggle;

public:

	TAttribute<ECheckBoxState> IsChecked;

public:
	
	/** The delegate to execute when the button is checked state has changed */
	FOnCheckStateChanged OnCheckStateChanged;
	
	/** The delegate to execute when the button is pressed */
	FSimpleDelegate OnPressed;

	/** The delegate to execute when the button is hovered */
	FSimpleDelegate OnHovered;

	/** The delegate to execute when the button exit the hovered state */
	FSimpleDelegate OnUnhovered;

public:
	float UnhoveredCheckingInterval;

public:
	
	FLinearColor OnColor;
	FLinearColor OffColor;

public:
	
	ECheckBoxState LastState = ECheckBoxState::Undetermined;

	float ButtonLength = 0;

	float ToggleMovementPercentage = 0.8;

public:
	
	void PlayToggleAnimation(const ECheckBoxState& bToggled, const bool& bInstant);

public:
	void EventOnPressed();
	void EventOnHovered();
	void EventOnUnHovered();

public:
	EActiveTimerReturnType CheckUnhovered(double InCurrentTime, float InDeltaTime);
};
