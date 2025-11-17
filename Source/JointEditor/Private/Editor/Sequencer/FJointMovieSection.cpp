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



#define LOCTEXT_NAMESPACE "FJointMovieSection"


/* FJointMovieSection structors
 *****************************************************************************/

FJointMovieSection::FJointMovieSection(TSharedPtr<ISequencer> InSequencer, UMovieSceneSection& InSection)
	: Section(&InSection)
	, SequencerPtr(InSequencer)
	, TimeSpace(ETimeSpace::Global)
{
	AdditionalDrawEffect = ESlateDrawEffect::NoGamma;
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
	if (Section != nullptr)
	{
		if (UMovieSceneJointSection* JointSection = Cast<UMovieSceneJointSection>(Section))
		{
			const FJointNodePointer NodePointer = JointSection->GetJointNodePointer();
		
			return FText::FromString(NodePointer.Node ? NodePointer.Node->GetName() : TEXT("No Joint Node Specified") );
		}
	}

	return FText::GetEmpty();
}

float FJointMovieSection::GetSectionHeight() const
{
	return 30.f;
}

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
	
	SAssignNew(SectionWidgetBox,SBox)
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	.Padding(FMargin(1,4));

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
	if (Section == nullptr) 
	{
		return InPainter.LayerId;
	}
	
	UMovieSceneJointSection* JointSection = Cast<UMovieSceneJointSection>(Section.Get());
	if (!JointSection)
	{
		return InPainter.LayerId;
	}

	//InPainter.SectionGeometry.Size = FVector2D(InPainter.SectionGeometry.Size.X + 30, GetSectionHeight());
	
	//float TextOffsetX = JointSection->GetRange().GetLowerBound().IsClosed() ? FMath::Max(0.f, InPainter.GetTimeConverter().FrameToPixel(JointSection->GetRange().GetLowerBoundValue())) : 0.f;
	FString EventString = GetSectionTitle().ToString();
	bool bIsEventValid = IsNodeValid();
	//PaintNodeName(InPainter, InPainter.LayerId + 1, EventString, TextOffsetX, bIsEventValid);
	
	return InPainter.LayerId;
}


void FJointMovieSection::PaintNodeName(FSequencerSectionPainter& Painter, int32 LayerId, const FString& InEventString, float PixelPos, bool bIsEventValid) const
{
	static const int32   FontSize      = 10;
	static const float   BoxOffsetPx   = 10.f;
	static const TCHAR*  WarningString = TEXT("\xf071");

	const FSlateFontInfo FontAwesomeFont = FJointEditorStyle::GetUEEditorSlateStyleSet().GetFontStyle("FontAwesome.8");
	const FSlateFontInfo SmallLayoutFont = FCoreStyle::GetDefaultFontStyle("Bold", 8);
	const FLinearColor   DrawColor       = FLinearColor::White;

	TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	// Set up the warning size. Static since it won't ever change
	static FVector2D WarningSize    = FontMeasureService->Measure(WarningString, FontAwesomeFont);
	const  FMargin   WarningPadding = (bIsEventValid || InEventString.Len() == 0) ? FMargin(0.f) : FMargin(0.f, 0.f, 4.f, 0.f);
	const  FMargin   BoxPadding     = FMargin(2.0f, 4.0f);

	const FVector2D  TextSize       = FontMeasureService->Measure(InEventString, SmallLayoutFont);
	const FVector2D  IconSize       = bIsEventValid ? FVector2D::ZeroVector : WarningSize;
	const FVector2D  PaddedIconSize = IconSize + WarningPadding.GetDesiredSize();
	const FVector2D  BoxSize        = FVector2D(TextSize.X + PaddedIconSize.X, FMath::Max(TextSize.Y, PaddedIconSize.Y )) + BoxPadding.GetDesiredSize();

	// Flip the text position if getting near the end of the view range
	bool  bDrawLeft    = (Painter.SectionGeometry.Size.X - PixelPos) < (BoxSize.X + 18.f) - BoxOffsetPx;
	float BoxPositionX = bDrawLeft ? PixelPos - BoxSize.X - BoxOffsetPx : PixelPos + BoxOffsetPx;
	if (BoxPositionX < 0.f)
	{
		BoxPositionX = 0.f;
	}

	FVector2D BoxOffset  = FVector2D(BoxPositionX,                    Painter.SectionGeometry.Size.Y*.5f - BoxSize.Y*.5f);
	FVector2D IconOffset = FVector2D(BoxPadding.Left,                 BoxSize.Y*.5f - IconSize.Y*.5f);
	FVector2D TextOffset = FVector2D(IconOffset.X + PaddedIconSize.X, BoxSize.Y*.5f - TextSize.Y*.5f);

	/*
	// Draw the background box
	FSlateDrawElement::MakeBox(
		Painter.DrawElements,
		LayerId + 4,
		Painter.SectionGeometry.ToPaintGeometry(BoxOffset, BoxSize),
		FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		FLinearColor::Black.CopyWithNewOpacity(0.5f)
	);
	*/

	if (!bIsEventValid)
	{
		// Draw a warning icon for unbound repeaters
		FSlateDrawElement::MakeText(
			Painter.DrawElements,
			LayerId + 5,
			Painter.SectionGeometry.ToPaintGeometry(BoxOffset + IconOffset, IconSize),
			WarningString,
			FontAwesomeFont,
			ESlateDrawEffect::None,
			FJointEditorStyle::GetUEEditorSlateStyleSet().GetWidgetStyle<FTextBlockStyle>("Log.Warning").ColorAndOpacity.GetSpecifiedColor()
		);
	}

	FSlateDrawElement::MakeText(
		Painter.DrawElements,
		LayerId + 5,
		Painter.SectionGeometry.ToPaintGeometry(BoxOffset + TextOffset, TextSize),
		InEventString,
		SmallLayoutFont,
		ESlateDrawEffect::None,
		bIsEventValid ? DrawColor : FLinearColor(1.f, 0.5f, 0.f)
	);
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
			
			GetColorScheme(NormalColor, HoverColor, OutlineNormalColor, OutlineHoverColor);
			
			if (JointSection->SectionType == EJointMovieSectionType::Range)
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
					.VAlign(VAlign_Fill)
					[
						SNew(SJointNodePointerSlate)
						.ContentMargin(FMargin(0))
						.PointerToStructure(&JointSection->NodePointer)
						.PickingTargetJointManager(JointSection->GetTypedOuterJointTrack()->GetJointManager())
						.bShouldShowDisplayName(false)
						.bShouldShowNodeName(false)
						.OnNodeChanged(this, &FJointMovieSection::OnNodePointerChanged)  // simply just repopulate the box.
						.BorderArgs(SJointOutlineBorder::FArguments()
							.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Empty"))
							.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Empty"))
							.ContentPadding(FMargin(0))
							.HAlign(HAlign_Fill)
							.VAlign(VAlign_Center)
						)
					]);
			}else
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
					.VAlign(VAlign_Fill)
					[
						SNew(SJointNodePointerSlate)
						.ContentMargin(FMargin(0))
						.PickingTargetJointManager(JointSection->GetTypedOuterJointTrack()->GetJointManager())
						.PointerToStructure(&JointSection->NodePointer)
						.bShouldShowDisplayName(false)
						.bShouldShowNodeName(false)
						.OnNodeChanged(this, &FJointMovieSection::OnNodePointerChanged) // simply just repopulate the box.
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

void FJointMovieSection::OnNodePointerChanged()
{
	UpdateSectionBox();
}

void FJointMovieSection::GetColorScheme(FLinearColor& OutNormalColor, FLinearColor& OutHoverColor, FLinearColor& OutOutlineNormalColor, FLinearColor& OutOutlineHoverColor) const
{
	if (IsNodeValid())
	{
		OutNormalColor = FLinearColor(0.10, 0.40, 0.30);
		OutHoverColor = FLinearColor(0.20, 0.80, 0.70);
		OutOutlineNormalColor = FLinearColor(0.10, 0.60, 0.50);
		OutOutlineHoverColor = FLinearColor(0.80, 0.90, 0.90);	
	}else
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
	TRange<FFrameNumber> SectionRange   = Section->GetRange();
	FFrameRate           TickResolution = Section->GetTypedOuter<UMovieScene>()->GetTickResolution();

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


/* FJointMovieSection callbacks
 *****************************************************************************/


#undef LOCTEXT_NAMESPACE
