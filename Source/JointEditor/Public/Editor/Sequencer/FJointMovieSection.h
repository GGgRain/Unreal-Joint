// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ISequencerSection.h"
#include "Layout/Margin.h"
#include "Node/JointNodeBase.h"

class SBox;
class AActor;
class FMenuBuilder;
class FSequencerSectionPainter;
class FTrackEditorThumbnailPool;

/**
 * An Interface class for UMovieSceneJointSection - for the editor purposes only.
 */
class JOINTEDITOR_API FJointMovieSection
	: public ISequencerSection
	, public TSharedFromThis<FJointMovieSection>
{
public:

	/** Create and initialize a new instance. */
	FJointMovieSection(TSharedPtr<ISequencer> InSequencer, UMovieSceneSection& InSection);

	/** Virtual destructor. */
	virtual ~FJointMovieSection();

public:

	// ISequencerSection interface
	virtual void Tick(const FGeometry& AllottedGeometry, const FGeometry& ClippedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FText GetSectionTitle() const override;
	virtual float GetSectionHeight() const override;
	virtual int32 OnPaintSection(FSequencerSectionPainter& InPainter) const override;
	virtual FMargin GetContentPadding() const override;
	virtual UMovieSceneSection* GetSectionObject() override;
	virtual TSharedRef<SWidget> GenerateSectionWidget() override;
	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) override;

public:

	bool IsNodeValid() const;

public:
	
	void PaintNodeName(FSequencerSectionPainter& Painter, int32 LayerId, const FString& InEventString, float PixelPos, bool bIsEventValid) const;

	void UpdateSectionBox();

	void OnNodePointerChanged();

public:

	void GetColorScheme(FLinearColor& OutNormalColor, FLinearColor& OutHoverColor, FLinearColor& OutOutlineNormalColor, FLinearColor& OutOutlineHoverColor) const;
	
public:

	/** Get the range that is currently visible in the section's time space */
	TRange<double> GetVisibleRange() const;

	/** Get the total range that thumbnails are to be generated for in the section's time space */
	TRange<double> GetTotalRange() const;

public:

	/** The section we are visualizing. */
	TWeakObjectPtr<UMovieSceneSection> Section;

	/** The parent sequencer we are a part of. */
	TWeakPtr<ISequencer> SequencerPtr;

	ESlateDrawEffect AdditionalDrawEffect;

	enum class ETimeSpace
	{
		Global,
		Local,
	};

	/** Enumeration value specifyin in which time-space to generate thumbnails */
	ETimeSpace TimeSpace;

public:

	TSharedPtr<SBox> SectionWidgetBox;
};
