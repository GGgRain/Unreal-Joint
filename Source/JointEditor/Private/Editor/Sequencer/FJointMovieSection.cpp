// Copyright Epic Games, Inc. All Rights Reserved.

#include "Editor/Sequencer/FJointMovieSection.h"

#include "Sections/MovieSceneCameraCutSection.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GameFramework/Actor.h"
#include "Editor.h"
#include "ISequencer.h"
#include "MovieScene.h"
#include "SequencerSectionPainter.h"
#include "ScopedTransaction.h"

#include "MovieSceneTimeHelpers.h"
#include "VoltDecl.h"
#include "Editor/Slate/JointAdvancedWidgets.h"
#include "Editor/Slate/GraphNode/JointGraphNodeSharedSlates.h"
#include "Fonts/FontMeasure.h"

#include "Misc/EngineVersionComparison.h"

#if UE_VERSION_OLDER_THAN(5, 3, 0)
#include "MovieSceneTools/Public/CommonMovieSceneTools.h"
#else
#include "AnimatedRange.h"
#include "TimeToPixel.h"
#endif


#include "Node/JointNodeBase.h"
#include "Sequencer/MovieSceneJointSectionTemplate.h"
#include "Sequencer/MovieSceneJointTrack.h"

#include "Misc/EngineVersionComparison.h"


// UVolt_ASM_InterpWidgetTransform 
#include "Module/Volt_ASM_InterpWidgetTransform.h"

#if UE_VERSION_OLDER_THAN(5, 4, 0)

#else
#include "MVVM/ViewModels/ViewDensity.h"
#endif


#define LOCTEXT_NAMESPACE "FJointMovieSection"


/* FJointMovieSection structors
 *****************************************************************************/

FJointMovieSection::FJointMovieSection(TSharedPtr<ISequencer> InSequencer, UMovieSceneSection& InSection)
	: Section(&InSection)
	  , SequencerPtr(InSequencer)
	  , TimeSpace(ETimeSpace::Global)
{
	AdditionalDrawEffect = ESlateDrawEffect::NoGamma;
	
	if (Section != nullptr)
	{
		if (UMovieSceneJointSection* JointSection = Cast<UMovieSceneJointSection>(Section))
		{
			JointSection->OnPropertyChanged.Unbind();
			JointSection->OnPropertyChanged.BindRaw(this, &FJointMovieSection::OnSectionObjectEdited);
		}
	}
}

FJointMovieSection::~FJointMovieSection()
{
}

/* ISequencerSection interface
 *****************************************************************************/

void FJointMovieSection::Tick(const FGeometry& AllottedGeometry, const FGeometry& ClippedGeometry, const double InCurrentTime, const float InDeltaTime)
{
}

/* FThumbnailSection interface
 *****************************************************************************/

FText FJointMovieSection::GetSectionTitle() const
{
	return FText::GetEmpty();
}

FText FJointMovieSection::GetSectionTitleText_Internal() const
{
	if (Section != nullptr)
	{
		if (UMovieSceneJointSection* JointSection = Cast<UMovieSceneJointSection>(Section))
		{
			const FJointNodePointer NodePointer = JointSection->GetJointNodePointer();

			return FText::FromString(NodePointer.Node ? NodePointer.Node->GetName() : TEXT("No Joint Node Specified"));
		}
	}

	return FText::GetEmpty();
}

#if UE_VERSION_OLDER_THAN(5, 4, 0)

float FJointMovieSection::GetSectionHeight() const
{
	return 50.f;
}

#else

float FJointMovieSection::GetSectionHeight(const UE::Sequencer::FViewDensityInfo& ViewDensity) const
{
	return 50;
}

#endif


FMargin FJointMovieSection::GetContentPadding() const
{
	return FMargin(6.f, 10.f);
}

UMovieSceneSection* FJointMovieSection::GetSectionObject()
{
	return Section.Get();
}

TSharedRef<SWidget> FJointMovieSection::GenerateSectionWidget()
{
	SAssignNew(SectionWidgetBox, SBox)
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	.Padding(FMargin(1, 4));

	UpdateSectionBox();

	return SectionWidgetBox.ToSharedRef();
}

void FJointMovieSection::GenerateSectionLayout(class ISectionLayoutBuilder& LayoutBuilder)
{
	ISequencerSection::GenerateSectionLayout(LayoutBuilder);
}

bool FJointMovieSection::IsNodeValid() const
{
	if (Section != nullptr)
	{
		if (UMovieSceneJointSection* JointSection = Cast<UMovieSceneJointSection>(Section))
		{
			const FJointNodePointer NodePointer = JointSection->GetJointNodePointer();
			return NodePointer.IsValid();
		}
	}

	return false;
}

int32 FJointMovieSection::OnPaintSection(FSequencerSectionPainter& InPainter) const
{
	int32 LayerId = 0; //InPainter.PaintSectionBackground();
	
	if (Section == nullptr)
	{
		return InPainter.LayerId;
	}

	UMovieSceneJointSection* JointSection = Cast<UMovieSceneJointSection>(Section.Get());
	if (!JointSection)
	{
		return InPainter.LayerId;
	}
	
	return InPainter.LayerId + 1;
}


void FJointMovieSection::UpdateSectionBox()
{
	if (SectionWidgetBox && Section != nullptr)
	{
		if (UMovieSceneJointSection* JointSection = Cast<UMovieSceneJointSection>(Section))
		{
			FLinearColor NormalColor;
			FLinearColor HoverColor;
			FLinearColor OutlineNormalColor;
			FLinearColor OutlineHoverColor;

			GetColorScheme(JointSection->SectionType, NormalColor, HoverColor, OutlineNormalColor, OutlineHoverColor);

			if (JointSection->SectionType == EJointMovieSectionType::ActiveForRange)
			{
				SectionWidgetBox.Get()->SetHAlign(HAlign_Fill);
				SectionWidgetBox.Get()->SetContent(
					SNew(SOverlay)
					+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(SJointOutlineBorder)
						.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.NormalColor(NormalColor)
						.HoverColor(HoverColor)
						.OutlineNormalColor(OutlineNormalColor)
						.OutlineHoverColor(OutlineHoverColor)
						.ContentPadding(FMargin(0))
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SAssignNew(SectionNameTextBlock, STextBlock)
							.Text(this, &FJointMovieSection::GetSectionTitleText_Internal)
							.Justification(ETextJustify::Center)
							.ColorAndOpacity(FLinearColor::White)
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
					
							SAssignNew(SectionJointNodePointerSlate, SJointNodePointerSlate)
							.ContentMargin(FMargin(0))
							.PointerToStructure(&JointSection->NodePointer)
							.PickingTargetJointManager(JointSection->GetTypedOuterJointTrack()->GetJointManager())
							.bShouldShowDisplayName(false)
							.bShouldShowNodeName(false)
							.OnPreNodeChanged(this, &FJointMovieSection::OnPreNodePointerChanged) // simply just repopulate the box.
							.OnHovered(this, &FJointMovieSection::OnJointNodePointerSlateHovered)
							.OnUnhovered(this, &FJointMovieSection::OnJointNodePointerSlateUnhovered)
							.BorderArgs(SJointOutlineBorder::FArguments()
										.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Empty"))
										.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Empty"))
										.ContentPadding(FMargin(0))
										.HAlign(HAlign_Fill)
										.VAlign(VAlign_Center)
							)
					]);
				
			}
			else if (JointSection->SectionType == EJointMovieSectionType::BeginPlay || JointSection->SectionType == EJointMovieSectionType::EndPlay || JointSection->SectionType == EJointMovieSectionType::MarkAsPending)
			{
				SectionWidgetBox.Get()->SetHAlign(HAlign_Left);
				SectionWidgetBox.Get()->SetContent(
					SNew(SOverlay)
					+ SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Fill)
					[
						SNew(SJointOutlineBorder)
						.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Solid"))
						.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Solid"))
						.NormalColor(NormalColor)
						.HoverColor(HoverColor)
						.OutlineNormalColor(OutlineNormalColor)
						.OutlineHoverColor(OutlineHoverColor)
						.ContentPadding(FMargin(1))
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Normal)
					[
						SAssignNew(SectionNameTextBlock, STextBlock)
							.Text(this, &FJointMovieSection::GetSectionTitleText_Internal)
							.Justification(ETextJustify::Center)
							.ColorAndOpacity(FLinearColor::White)
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SAssignNew(SectionJointNodePointerSlate, SJointNodePointerSlate)
							.ContentMargin(FMargin(0))
							.PickingTargetJointManager(JointSection->GetTypedOuterJointTrack()->GetJointManager())
							.PointerToStructure(&JointSection->NodePointer)
							.bShouldShowDisplayName(false)
							.bShouldShowNodeName(false)
							.OnPreNodeChanged(this, &FJointMovieSection::OnPreNodePointerChanged) // simply just repopulate the box.
							.OnPostNodeChanged(this, &FJointMovieSection::OnPostNodePointerChanged)
							.OnHovered(this, &FJointMovieSection::OnJointNodePointerSlateHovered)
							.OnUnhovered(this, &FJointMovieSection::OnJointNodePointerSlateUnhovered)
							.BorderArgs(SJointOutlineBorder::FArguments()
							            .OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Empty"))
							            .InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Empty"))
							            .ContentPadding(FMargin(0))
							            .HAlign(HAlign_Fill)
							            .VAlign(VAlign_Center)
							)
					]);
			}
		}
	}
}

void FJointMovieSection::OnPreNodePointerChanged()
{
	if (Section.IsValid())
	{
		Section->Modify();
	}
}

void FJointMovieSection::OnPostNodePointerChanged()
{
	UpdateSectionBox();
}

void FJointMovieSection::GetColorScheme(EJointMovieSectionType SectionType, FLinearColor& OutNormalColor, FLinearColor& OutHoverColor, FLinearColor& OutOutlineNormalColor, FLinearColor& OutOutlineHoverColor) const
{
	
	if (IsNodeValid())
	{
		if (SectionType == EJointMovieSectionType::BeginPlay || SectionType == EJointMovieSectionType::ActiveForRange)
		{
			OutNormalColor = FLinearColor(0.10, 0.40, 0.30);
			OutHoverColor = FLinearColor(0.20, 0.80, 0.70);
			OutOutlineNormalColor = FLinearColor(0.10, 0.60, 0.50);
			OutOutlineHoverColor = FLinearColor(0.80, 0.90, 0.90);
			return;
		}
		else if (SectionType == EJointMovieSectionType::EndPlay)
		{
			OutNormalColor = FLinearColor(0.35, 0.35, 0.35);
			OutHoverColor = FLinearColor(0.50, 0.50, 0.50);    
			OutOutlineNormalColor = FLinearColor(0.65, 0.65, 0.65);
			OutOutlineHoverColor = FLinearColor(0.90, 0.90, 0.90);
			return;
		}
		else if (SectionType == EJointMovieSectionType::MarkAsPending)
		{
			OutNormalColor = FLinearColor(0.60, 0.60, 0.30);
			OutHoverColor = FLinearColor(0.80, 0.80, 0.40);
			OutOutlineNormalColor = FLinearColor(0.80, 0.80, 0.20);
			OutOutlineHoverColor = FLinearColor(0.90, 0.90, 0.90);
			return;
		}
	}
	else
	{
		OutNormalColor = FLinearColor(0.80, 0.10, 0.10);
		OutHoverColor = FLinearColor(0.80, 0.20, 0.20);
		OutOutlineNormalColor = FLinearColor(0.90, 0.10, 0.10);
		OutOutlineHoverColor = FLinearColor(0.90, 0.90, 0.90);
	}
}


TRange<double> FJointMovieSection::GetVisibleRange() const
{
	const FFrameRate TickResolution = Section->GetTypedOuter<UMovieScene>()->GetTickResolution();
	TRange<double> GlobalVisibleRange = SequencerPtr.Pin()->GetViewRange();
	TRange<double> SectionRange = Section->GetRange() / TickResolution;

	if (TimeSpace == ETimeSpace::Global)
	{
		return GlobalVisibleRange;
	}

	TRange<double> Intersection = TRange<double>::Intersection(GlobalVisibleRange, SectionRange);
	return TRange<double>(
		Intersection.GetLowerBoundValue() - SectionRange.GetLowerBoundValue(),
		Intersection.GetUpperBoundValue() - SectionRange.GetLowerBoundValue()
	);
}

TRange<double> FJointMovieSection::GetTotalRange() const
{
	TRange<FFrameNumber> SectionRange = Section->GetRange();
	FFrameRate TickResolution = Section->GetTypedOuter<UMovieScene>()->GetTickResolution();

	if (TimeSpace == ETimeSpace::Global)
	{
		return SectionRange / TickResolution;
	}
	else
	{
		const bool bHasDiscreteSize = SectionRange.GetLowerBound().IsClosed() && SectionRange.GetUpperBound().IsClosed();
		TRangeBound<double> UpperBound = bHasDiscreteSize
			                                 ? TRangeBound<double>::Exclusive(FFrameNumber(UE::MovieScene::DiscreteSize(SectionRange)) / TickResolution)
			                                 : TRangeBound<double>::Open();

		return TRange<double>(0, UpperBound);
	}
}

void FJointMovieSection::OnSectionObjectEdited()
{
	UpdateSectionBox();
}

void FJointMovieSection::OnJointNodePointerSlateHovered()
{
	// Play anim for the text
	
	VOLT_STOP_ANIM(HoverAnimationHandle);
	
	const UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
			.TargetWidgetTransform(
				FWidgetTransform(
			FVector2D(0,-20),
			FVector2D(1,1),
			FVector2D::ZeroVector,
			0)
			)
			.RateBasedInterpSpeed(7)
	);
	
	HoverAnimationHandle = VOLT_PLAY_ANIM(SectionNameTextBlock, Anim);
}

void FJointMovieSection::OnJointNodePointerSlateUnhovered()
{
	// Play anim for the text
	
	VOLT_STOP_ANIM(HoverAnimationHandle);
	
	const UVoltAnimation* Anim = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_InterpWidgetTransform)
			.TargetWidgetTransform(
				FWidgetTransform(
			FVector2D(0,0),
			FVector2D(1,1),
			FVector2D::ZeroVector,
			0)
			)
			.RateBasedInterpSpeed(7)
	);
	
	HoverAnimationHandle = VOLT_PLAY_ANIM(SectionNameTextBlock, Anim);
}


/* FJointMovieSection callbacks
 *****************************************************************************/


#undef LOCTEXT_NAMESPACE
