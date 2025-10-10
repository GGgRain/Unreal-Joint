//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Slate/EditorWidget/JointToolkitToastMessages.h"


#include "JointEditorStyle.h"
#include "VoltAnimation.h"
#include "VoltDecl.h"
#include "Components/HorizontalBox.h"

#include "Module/Volt_ASM_InterpRenderOpacity.h"
#include "Module/Volt_ASM_InterpWidgetTransform.h"

#include "Slate/WidgetTransform.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"


void SJointToolkitToastMessageHub::ReleaseVoltAnimationManager()
{
	VOLT_RELEASE_MANAGER(&ToastMessageAnimationManager);
}

void SJointToolkitToastMessageHub::ImplementVoltAnimationManager()
{
	VOLT_IMPLEMENT_MANAGER(&ToastMessageAnimationManager, SharedThis(this));
}

void SJointToolkitToastMessageHub::Construct(const FArguments& InArgs)
{
	SetCanTick(false);

	ImplementVoltAnimationManager();

	if (ToastMessageAnimationManager) ToastMessageAnimationManager->OnAnimationEnded_NonDynamic.AddSP(
		this, &SJointToolkitToastMessageHub::OnAnimationEnded);

	PopulateSlate();
}

void SJointToolkitToastMessageHub::PopulateSlate()
{
	this->ChildSlot.DetachWidget();

	this->ChildSlot
	    .HAlign(HAlign_Fill)
	    .VAlign(VAlign_Fill)
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor::Transparent)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(ToasterDisplayBox, SHorizontalBox)
		]
	];
}

FGuid SJointToolkitToastMessageHub::AddToasterMessage(const TSharedPtr<SJointToolkitToastMessage>& Widget)
{
	if (HasToasterMessage(Widget->MessageGuid)) return FGuid();
	
	if (ToasterDisplayBox.IsValid() && Widget.IsValid())
	{
		Widget->SetRenderOpacity(0);

		Widget->SetMessageHub(SharedThis(this));

		ToasterDisplayBox.Pin()->AddSlot()
		                 .HAlign(HAlign_Center)
		                 .VAlign(VAlign_Center)
		                 .Padding(FJointEditorStyle::Margin_Tiny)
		                 .AutoWidth()
		[
			Widget.ToSharedRef()
		];

		Messages.Add(Widget->MessageGuid, Widget);

		Widget->PlayAppearAnimation();

		return Widget->MessageGuid;
	}

	return FGuid();
}

void SJointToolkitToastMessageHub::RemoveToasterMessage(const FGuid& ToasterGuid, const bool bInstant)
{
	if (!HasToasterMessage(ToasterGuid)) return;

	if (ToasterDisplayBox.IsValid())
	{
		if (const TWeakPtr<SJointToolkitToastMessage>* FoundMessage = Messages.Find(ToasterGuid))
		{
			if (bInstant)
			{
				ToasterDisplayBox.Pin()->RemoveSlot(FoundMessage->Pin().ToSharedRef());

				Messages.Remove(ToasterGuid);
			}
			else
			{
				FoundMessage->Pin()->PlayRemoveAnimation();
			}
		}
	}
}

TWeakPtr<SJointToolkitToastMessage> SJointToolkitToastMessageHub::FindToasterMessage(const FGuid& ToasterGuid)
{
	if (TWeakPtr<SJointToolkitToastMessage>* FoundMessage = Messages.Find(ToasterGuid))
	{
		return *FoundMessage;
	}

	return nullptr;
}

const bool SJointToolkitToastMessageHub::HasToasterMessage(const FGuid& ToasterGuid)
{
	return Messages.Contains(ToasterGuid);
}

UVoltAnimationManager* SJointToolkitToastMessageHub::GetAnimationManager()
{
	return ToastMessageAnimationManager;
}

void SJointToolkitToastMessageHub::OnAnimationEnded(UVoltAnimationManager* VoltAnimationManager,
                                                    const FVoltAnimationTrack& VoltAnimationTrack,
                                                    const UVoltAnimation* VoltAnimation)
{
	if (VoltAnimationTrack.TargetSlateInterface)
	{
		if (TSharedPtr<SWidget> Slate = VoltAnimationTrack.TargetSlateInterface->GetTargetSlate().Pin(); Slate.
			IsValid())
		{
			TSharedPtr<SJointToolkitToastMessage> CastedSlate = StaticCastSharedPtr<SJointToolkitToastMessage>(Slate);

			if (CastedSlate->bMarkedKilled && CastedSlate->RemoveTrack == VoltAnimationTrack)
			{
				RemoveToasterMessage(CastedSlate->MessageGuid, true);
			}
		}
	}
}


void SJointToolkitToastMessage::Construct(const FArguments& InArgs)
{
	SetRenderOpacity(0);
	SetRenderTransformPivot(FVector2D(0.5, 0.5));
	SetCanTick(true);

	MessageGuid = FGuid::NewGuid();

	Duration = InArgs._Duration;

	SizeIncreaseInterpolationSpeed = InArgs._SizeIncreaseInterpolationSpeed;
	SizeDecreaseInterpolationSpeed = InArgs._SizeDecreaseInterpolationSpeed;

	AppearAnimationDuration = InArgs._AppearAnimationDuration;
	RemoveAnimationDuration = InArgs._RemoveAnimationDuration;
	
	AppearAnimationExp = InArgs._AppearAnimationExp;
	RemoveAnimationExp = InArgs._RemoveAnimationExp;

	this->ChildSlot.DetachWidget();

	SlotWidget = InArgs._Content.Widget;

	this->ChildSlot
	    .HAlign(HAlign_Center)
	    .VAlign(VAlign_Center)
		[
			SAssignNew(SizeBox, SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.WidthOverride_Lambda([this]()
			{
				return SizeOverride.X;
			})
			.HeightOverride_Lambda([this]()
			{
				return SizeOverride.Y;
			})
			[
				SlotWidget.Pin().ToSharedRef()
			]
	];

	if (Duration > 0)
	{
		RegisterActiveTimer(
			Duration, FWidgetActiveTimerDelegate::CreateSP(this, &SJointToolkitToastMessage::OnExpired));
	}
}

void SJointToolkitToastMessage::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime,
	const float InDeltaTime)
{
	if (SlotWidget.IsValid())
	{
		const FVector2D& Size = SlotWidget.Pin()->GetDesiredSize();

		if (bMarkedKilled)
		{
			SizeOverride.X = FMath::FInterpTo<float>(SizeOverride.X, 0, InDeltaTime, SizeDecreaseInterpolationSpeed);
			SizeOverride.Y = FMath::FInterpTo<float>(SizeOverride.Y, 0, InDeltaTime, SizeDecreaseInterpolationSpeed);
		}else
		{
			SizeOverride.X = FMath::FInterpTo<float>(SizeOverride.X, Size.X, InDeltaTime, SizeIncreaseInterpolationSpeed);
			SizeOverride.Y = FMath::FInterpTo<float>(SizeOverride.Y, Size.Y, InDeltaTime, SizeIncreaseInterpolationSpeed);
		}
	}
	
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

void SJointToolkitToastMessage::SetMessageHub(const TSharedPtr<SJointToolkitToastMessageHub>& Hub)
{
	MessageHub = Hub;
}

void SJointToolkitToastMessage::PlayAppearAnimation()
{
	if (!MessageHub.IsValid()) return;

	bMarkedKilled = false;

	VOLT_STOP_ANIM(MessageHub.Pin()->GetAnimationManager(), AppearTrack);
	VOLT_STOP_ANIM(MessageHub.Pin()->GetAnimationManager(), RemoveTrack);

	const UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
		.InterpolationMode(EVoltInterpMode::AlphaBased)
		.AlphaBasedEasingFunction(EEasingFunc::EaseOut)
		.AlphaBasedDuration(AppearAnimationDuration)
		.AlphaBasedBlendExp(6)
		.bUseStartWidgetTransform(true)
		.StartWidgetTransform(FWidgetTransform(FVector2D(0, 200), FVector2D(1, 1), FVector2D(0, 0), 0))
		.TargetWidgetTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(1, 1), FVector2D(0, 0), 0)),
		VOLT_MAKE_MODULE(UVolt_ASM_InterpRenderOpacity)
		.InterpolationMode(EVoltInterpMode::AlphaBased)
		.AlphaBasedEasingFunction(EEasingFunc::EaseOut)
		.AlphaBasedDuration(AppearAnimationDuration)
		.AlphaBasedBlendExp(6)
		.bUseStartOpacity(true)
		.StartOpacity(0)
		.TargetOpacity(1)
	);

	AppearTrack = VOLT_PLAY_ANIM(MessageHub.Pin()->GetAnimationManager(), SharedThis(this), Anim);
}

void SJointToolkitToastMessage::PlayRemoveAnimation()
{
	if (!MessageHub.IsValid()) return;

	bMarkedKilled = true;

	VOLT_STOP_ANIM(MessageHub.Pin()->GetAnimationManager(), AppearTrack);

	VOLT_STOP_ANIM(MessageHub.Pin()->GetAnimationManager(), RemoveTrack);

	const UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
		.InterpolationMode(EVoltInterpMode::AlphaBased)
		.AlphaBasedEasingFunction(EEasingFunc::EaseIn)
		.AlphaBasedDuration(RemoveAnimationDuration)
		.AlphaBasedBlendExp(6)
		.TargetWidgetTransform(FWidgetTransform(FVector2D(0, 50), FVector2D(1, 1), FVector2D(0, 0), 0)),
		VOLT_MAKE_MODULE(UVolt_ASM_InterpRenderOpacity)
		.InterpolationMode(EVoltInterpMode::AlphaBased)
		.AlphaBasedEasingFunction(EEasingFunc::EaseIn)
		.AlphaBasedDuration(RemoveAnimationDuration)
		.AlphaBasedBlendExp(6)
		.TargetOpacity(0)
	);

	RemoveTrack = VOLT_PLAY_ANIM(MessageHub.Pin()->GetAnimationManager(), SharedThis(this), Anim);
}

EActiveTimerReturnType SJointToolkitToastMessage::OnExpired(double X, float Arg)
{
	PlayRemoveAnimation();

	return EActiveTimerReturnType::Stop;
}
