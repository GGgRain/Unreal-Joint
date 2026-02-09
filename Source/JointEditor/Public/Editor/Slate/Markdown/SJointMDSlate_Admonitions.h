//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

//type enum
enum class EJointMDAdmonitionStyleType : uint8
{
	Vertical_Verbose,
	Vertical_NoHeaderText,
	Vertical_NoIcon,
	Horizontal_Verbose,
	Horizontal_NoHeaderText,
	Horizontal_NoIcon,
	OnlyContent
};


//type enum
enum class EJointMDAdmonitionType : uint8
{
	Info,
	Note,
	Warning,
	Caution,
	Error,
	Important
};

class JOINTEDITOR_API SJointMDSlate_Admonitions : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SJointMDSlate_Admonitions)
		:
		_AdmonitionType(EJointMDAdmonitionType::Info),
		_AdmonitionStyleType(EJointMDAdmonitionStyleType::Vertical_Verbose),
		_CustomHeaderText(FText::GetEmpty()),
		_DescriptionText(INVTEXT("No description provided. (BLAME if needed)")),
		_bUseDescriptionText(false),
		_bAutoWrapDescriptionText(false)
		{}
		SLATE_ARGUMENT(EJointMDAdmonitionType, AdmonitionType)
		SLATE_ARGUMENT(EJointMDAdmonitionStyleType, AdmonitionStyleType)
		SLATE_ARGUMENT(FText, CustomHeaderText)
		SLATE_ARGUMENT(FText, DescriptionText)
		SLATE_ARGUMENT(bool, bUseDescriptionText) // if true, it uses DescriptionText as content, ignoring the Content slot
		SLATE_ARGUMENT(bool, bAutoWrapDescriptionText)
		SLATE_DEFAULT_SLOT( FArguments, Content )
	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

public:
	
	EJointMDAdmonitionType AdmonitionType;
	
};

