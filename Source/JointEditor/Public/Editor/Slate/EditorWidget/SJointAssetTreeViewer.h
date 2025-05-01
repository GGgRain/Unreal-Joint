//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"

class UJointManager;
class SJointManagerViewer;

//////////////////////////////////////////////////////////////////////////
// SJointList

class JOINTEDITOR_API SJointAssetTreeViewer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SJointAssetTreeViewer){}
	SLATE_END_ARGS()
public:
	
	~SJointAssetTreeViewer();
	
	void GatherJointManagers();

	void Construct(const FArguments& InArgs);

	void RebuildWidget();

	void RequestTreeRebuild();



protected:
	
	void PropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);
	
protected:

	TSharedPtr<SJointManagerViewer> ManagerTree;
	
protected:

	TArray<TWeakObjectPtr<UJointManager>> Managers;

};
