//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointManager.h"
#include "SearchTree/Builder/JointTreeBuilder.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class FJointEditorToolkit;
struct FJointPropertyTreeFilterArgs;
struct FJointPropertyTreeBuilderArgs;
class IJointTreeItem;
class UTexture2D;
class SCheckBox;



/** Enum which tells us what type of bones we should be showing */
enum class EJointManagerViewerMode : int32
{
	Search,
	Replace
};

class JOINTEDITOR_API SJointManagerViewer : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SJointManagerViewer)
	{
	}
	SLATE_ARGUMENT(TWeakPtr<FJointEditorToolkit>, ToolKitPtr)
	SLATE_ARGUMENT(TArray<UJointManager*>, JointManagers)
	SLATE_ARGUMENT(FJointPropertyTreeBuilderArgs, BuilderArgs)
	SLATE_END_ARGS()
	
public:
	
	void Construct(const FArguments& InArgs);

	void RebuildWidget();

	void RequestTreeRebuild();

public:

	void ReplaceNextSrc();

	void ReplaceAllSrc();

	EJointManagerViewerMode GetMode() const;
	
	void SetMode(EJointManagerViewerMode Mode);

	void SetTargetManager(TArray<UJointManager*> NewManagers);

public:

	TSharedRef<SWidget> CreateModeSelectionSection();

public:

	//Callbacks
	void OnNodeVisibilityCheckBoxCheckStateChanged(ECheckBoxState CheckBoxState);

	void OnFilterTextChanged(const FText& Text);
	void OnFilterTextCommitted(const FText& Text, ETextCommit::Type Arg);

	void OnFilterReplaceFromTextChanged(const FText& Text);
	void OnFilterReplaceToTextChanged(const FText& Text);
	
	void OnSearchModeButtonPressed();
	void OnReplaceModeButtonPressed();
	
	void OnReplaceNextButtonPressed();
	void OnReplaceAllButtonPressed();

public:
	
	FText SearchFilterText;

	FText ReplaceFromText;

	FText ReplaceToText;

private:
	
	TWeakPtr<FJointEditorToolkit> ToolKitPtr;

	TArray<UJointManager*> JointManagers;

	FJointPropertyTreeBuilderArgs BuilderArgs;


	EJointManagerViewerMode Mode = EJointManagerViewerMode::Search;


public:

	TSharedPtr<SSearchBox> ReplaceFromSearchBox;
	TSharedPtr<SSearchBox> ReplaceToSearchBox;
	TSharedPtr<SSearchBox> SearchSearchBox;

	
public:

	TSharedPtr<class SJointTree> Tree;

	TSharedPtr<SCheckBox> NodeVisibilityCheckbox;
	
};

