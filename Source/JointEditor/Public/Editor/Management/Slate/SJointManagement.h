//Copyright 2022~2024 DevGrain. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FTabManager;
class SWidget;
class FJointManagementTabHandler;
class FMenuBuilder;

class JOINTEDITOR_API SJointManagement : public SCompoundWidget
{
	
public:

	SLATE_BEGIN_ARGS(SJointManagement) {}
		
	SLATE_ARGUMENT(TSharedPtr<class FTabManager>, TabManager)
	SLATE_ARGUMENT(TSharedPtr<class FJointManagementTabHandler>, JointManagementTabHandler)

	SLATE_END_ARGS();
	
public:

	void Construct(const FArguments& InArgs);

public:

	void FillWindowMenu(FMenuBuilder& MenuBuilder);
	
	TSharedRef<SWidget> MakeToolbar();

public:

	/**
	 * Tab manager for the actions. One SJointManagement takes one FTabManager instance, and it will own that.
	 */
	TSharedPtr<FTabManager> TabManager;

public:

	TWeakPtr<FJointManagementTabHandler> JointManagementTabHandler;

	
};