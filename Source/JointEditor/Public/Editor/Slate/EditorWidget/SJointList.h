//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"

class UTexture2D;

//////////////////////////////////////////////////////////////////////////
// SJointList


class JOINTEDITOR_API SJointList : public SCompoundWidget
{
	
public:

public:
	SLATE_BEGIN_ARGS(SJointList){}
		SLATE_EVENT(FOnAssetSelected, OnAssetSelected);
		SLATE_EVENT(FOnAssetDoubleClicked, OnAssetDoubleClicked);
		SLATE_EVENT(FOnAssetsActivated, OnAssetsActivated);
	SLATE_END_ARGS()
public:
	
	void Construct(const FArguments& InArgs);

protected:
	
	void RebuildWidget();
	
	bool CanShowColumnForAssetRegistryTag(FName AssetType, FName TagName) const;

protected:
	// Set of tags to prevent creating details view columns for (infrequently used)
	TSet<FName> AssetRegistryTagsToIgnore;

	TSharedPtr<class SAssetPicker> AssetPicker;

protected:
	
	FOnAssetSelected OnAssetSelected;

	FOnAssetDoubleClicked OnAssetDoubleClicked;

	FOnAssetsActivated OnAssetsActivated;
	
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;

public:

	TArray< FAssetData > GetCurrentSelection(); 

};
