// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ISequencerSection.h"
#include "VoltAnimationTrack.h"
#include "Layout/Margin.h"

#include "Misc/EngineVersionComparison.h"

enum class EJointMovieSectionType : uint8;
class SJointNodePointerSlate;
class SBox;
class STextBlock;

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
	
	/**
	 * we keep GetSectionTitle clear to not render text over the section on the SSequencerSection and instead provide our own widget.
	 * So we use this instead.
	 */
	virtual FText GetSectionTitleText_Internal() const;
	
#if UE_VERSION_OLDER_THAN(5, 4, 0)
	virtual float GetSectionHeight() const override;
#else
	virtual float GetSectionHeight(const UE::Sequencer::FViewDensityInfo& ViewDensity) const override;
#endif
	
	virtual int32 OnPaintSection(FSequencerSectionPainter& InPainter) const override;
	virtual FMargin GetContentPadding() const override;
	virtual UMovieSceneSection* GetSectionObject() override;
	virtual TSharedRef<SWidget> GenerateSectionWidget() override;
	virtual void GenerateSectionLayout( class ISectionLayoutBuilder& LayoutBuilder ) override;

public:

	bool IsNodeValid() const;

public:
	
	void UpdateSectionBox();

	void OnPreNodePointerChanged();
	void OnPostNodePointerChanged();

public:

	void GetColorScheme(EJointMovieSectionType SectionType, FLinearColor& OutNormalColor, FLinearColor& OutHoverColor, FLinearColor& OutOutlineNormalColor, FLinearColor& OutOutlineHoverColor) const;
	
public:

	/** Get the range that is currently visible in the section's time space */
	TRange<double> GetVisibleRange() const;

	/** Get the total range that thumbnails are to be generated for in the section's time space */
	TRange<double> GetTotalRange() const;
	
public:

	void OnSectionObjectEdited();

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
	TSharedPtr<STextBlock> SectionNameTextBlock;
	TSharedPtr<SJointNodePointerSlate> SectionJointNodePointerSlate;

public:
	
	void OnJointNodePointerSlateHovered();
	void OnJointNodePointerSlateUnhovered();
	
	FVoltAnimationTrack HoverAnimationHandle;

	
};
