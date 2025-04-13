//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoltAnimationTrack.h"
#include "SharedType/JointSharedTypes.h"
#include "Widgets/SCompoundWidget.h"

class SLevelOfDetailBranchNode;
class SJointOutlineBorder;
class UVoltAnimationManager;
class UJointEdGraphNode;
class SGraphPin;
class SGraphNode;
class SVerticalBox;
class SJointGraphNodeBase;

class JOINTEDITOR_API SJointMultiNodeIndex : public SCompoundWidget
{
public:
	/** Delegate event fired when the hover state of this widget changes */
	DECLARE_DELEGATE_OneParam(FOnHoverStateChanged, bool /* bHovered */);

	/** Delegate used to receive the color of the node, depending on hover state and state of other siblings */
	DECLARE_DELEGATE_RetVal_OneParam(FSlateColor, FOnGetIndexColor, bool /* bHovered */);

	SLATE_BEGIN_ARGS(SJointMultiNodeIndex)
		{
		}

		SLATE_ATTRIBUTE(FText, Text)
		SLATE_EVENT(FOnHoverStateChanged, OnHoverStateChanged)
		SLATE_EVENT(FOnGetIndexColor, OnGetIndexColor)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

	/** Get the color we use to display the rounded border */
	FSlateColor GetColor() const;

private:
	/** Delegate event fired when the hover state of this widget changes */
	FOnHoverStateChanged OnHoverStateChangedEvent;

	/** Delegate used to receive the color of the node, depending on hover state and state of other siblings */
	FOnGetIndexColor OnGetIndexColorEvent;
};


class JOINTEDITOR_API SJointGraphPinOwnerNodeBox : public SCompoundWidget
{
	
public:
	
	SLATE_BEGIN_ARGS(SJointGraphPinOwnerNodeBox)
	{
	}
	SLATE_END_ARGS()

public:
	
	void Construct(const FArguments& InArgs, UJointEdGraphNode* InTargetNode, TSharedPtr<SJointGraphNodeBase> InOwnerGraphNode);

	void PopulateSlate();
	
public:

	const FText GetName();

public:

	void AddPin(const TSharedRef<SGraphPin>& TargetPin);

public:

	TSharedPtr<SVerticalBox> GetPinBox() const;

private:

	TWeakPtr<SVerticalBox> PinBox;
	
public:

	UJointEdGraphNode* TargetNode;
	TWeakPtr<SJointGraphNodeBase> OwnerGraphNode;
};



class JOINTEDITOR_API SJointGraphNodeInsertPoint : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SJointGraphNodeInsertPoint)
	{
	}
	SLATE_ARGUMENT(int, InsertIndex)
	SLATE_ARGUMENT(TSharedPtr<SJointGraphNodeBase>, ParentGraphNodeSlate)
SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void PopulateSlate();

public:
	
	/**
	 * A reference to the widget that hold the slate layout.
	 */
	TSharedPtr<SBorder> SlateBorder;

public:

	/**
	 * Parent Joint graph node slate. This slate pointer will be used on the insert action execution.
	 */
	TWeakPtr<SJointGraphNodeBase> ParentGraphNodeSlate;
	
	/**
	 * Index of where the node will be inserted by this slate's action.
	 */
	int InsertIndex = INDEX_NONE;
	
public:

	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;

	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

public:

	void Highlight(const float& Delay);

public:

	FVoltAnimationTrack Track;
	
};


class JOINTEDITOR_API SJointBuildPreset : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SJointBuildPreset){}
	SLATE_ARGUMENT(TSharedPtr<SJointGraphNodeBase>, ParentGraphNodeSlate)
	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

	void PopulateSlate();

public:

	void Update();
	
public:

	class UJointBuildPreset* GetBuildTargetPreset();

public:

	TSharedPtr<SJointOutlineBorder> PresetBorder;

	TSharedPtr<STextBlock> PresetTextBlock;

public:

	/**
	 * Parent Joint graph node slate. This slate pointer will be used on the insert action execution.
	 */
	TWeakPtr<SJointGraphNodeBase> ParentGraphNodeSlate;

public:

	void OnHovered();

	void OnUnHovered();

};

/**
 * A slate that help editing the FJointNodePointer property in the graph.
 * JOINT 2.8 : not maintained anymore.
 */

class JOINTEDITOR_API SJointNodePointerSlate : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SJointNodePointerSlate)
	{
	}
	SLATE_ARGUMENT(UJointEdGraphNode*, GraphContextObject)
	SLATE_ATTRIBUTE(FText, DisplayName)
	SLATE_ARGUMENT(FJointNodePointer*, PointerToStructure)
	SLATE_ARGUMENT(bool, bShouldShowDisplayName)
	SLATE_ARGUMENT(bool, bShouldShowNodeName)

	SLATE_END_ARGS()
	
public:

	void Construct(const FArguments& InArgs);
	
public:

	bool bShouldShowDisplayName = true;

	bool bShouldShowNodeName = true;

public:

	TSharedPtr<STextBlock> DisplayNameBlock;

	TSharedPtr<STextBlock> RawNameBlock;

public:

	TSharedPtr<SVerticalBox> BackgroundBox;

	TSharedPtr<SHorizontalBox> ButtonHorizontalBox;

private:

	const FText GetRawName();

private:
	
	FJointNodePointer* PointerToTargetStructure = nullptr;

	UJointEdGraphNode* OwnerJointEdGraphNode = nullptr;

private:

	TWeakPtr<class FJointEditorNodePickingManagerRequest> Request = nullptr;

public:

	void OnHovered();

	void OnUnhovered();

public:
	
	FReply OnGoButtonPressed();
	
	FReply OnPickupButtonPressed();

	FReply OnCopyButtonPressed();

	FReply OnPasteButtonPressed();

	FReply OnClearButtonPressed();

};


class JOINTEDITOR_API SJointNodeDescription : public SCompoundWidget
{
	
public:

	SLATE_BEGIN_ARGS(SJointNodeDescription)
	{
	}
	SLATE_ARGUMENT(TSubclassOf<UJointNodeBase>, ClassToDescribe)
	SLATE_END_ARGS()
	
public:

	void Construct(const FArguments& InArgs);

	void PopulateSlate();

public:

	FReply OnOpenEditorButtonPressed();

public:
	
	TSubclassOf<UJointNodeBase> ClassToDescribe;
};


class JOINTEDITOR_API SJointDetailView : public SCompoundWidget
{

public:

	SLATE_BEGIN_ARGS(SJointDetailView) :
		_OwnerGraphNode(nullptr),
		_Object(nullptr),
		_EditorNodeObject(nullptr)
	{} 
	SLATE_ARGUMENT(TSharedPtr<SJointGraphNodeBase>, OwnerGraphNode)
	SLATE_ARGUMENT(UJointNodeBase*, Object)
	SLATE_ARGUMENT(UJointEdGraphNode*, EditorNodeObject)
	SLATE_ARGUMENT(TArray<FJointGraphNodePropertyData>, PropertyData)

	SLATE_END_ARGS()
	
public:

	void Construct(const FArguments& InArgs);

	~SJointDetailView();

public:

	void PopulateSlate();

public:
	
	bool GetIsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	
	bool GetIsRowVisible(FName InRowName, FName InParentName) const;

public:

	bool UseLowDetailedRendering() const;
	
	void CachePropertiesToShow();

	TSharedRef<SWidget> GetLODContent(bool bArg);

public:

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

public:
	
	UJointNodeBase* Object;
	UJointEdGraphNode* EdNode;
	TArray<FJointGraphNodePropertyData> PropertyData;

public:

	void UpdatePropertyData(const TArray<FJointGraphNodePropertyData>& PropertyData);
	
	void SetOwnerGraphNode(TSharedPtr<SJointGraphNodeBase> InOwnerGraphNode);

	void RequestUpdate();

public:

	void ClearContentFromRetainer();

	void SetContentToRetainer(const TSharedPtr<SWidget>& Content);


private:
	
	TArray<FName> PropertiesToShow;

public:

	TSharedPtr<SRetainerWidget> JointDetailViewRetainerWidget;
	
	TSharedPtr<SInvalidationPanel> JointDetailViewInvalidationPanel;

	
	TSharedPtr<IDetailsView> DetailViewWidget;

public:
	
	TWeakPtr<SJointGraphNodeBase> OwnerGraphNode;

private:

	//This is a huge crap. probably it's the worst code I've ever wrote, and sadly, this is the best I've got so far.
	//Don't blame me, it's a fault of epic games.

	bool bNeedsUpdate = false;

	int TickRequestedCount = 2;

	bool bTickExpired = false;
	
};
