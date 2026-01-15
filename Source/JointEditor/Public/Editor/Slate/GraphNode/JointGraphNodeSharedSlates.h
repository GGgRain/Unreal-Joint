//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointManager.h"
#include "Editor/Slate/JointAdvancedWidgets.h"
#include "PropertyEditorDelegates.h"
#include "VoltAnimationTrack.h"
#include "SharedType/JointSharedTypes.h"
#include "Editor/Toolkit/JointEditorNodePickingManager.h"
#include "Widgets/SCompoundWidget.h"

class SJointNodePointerSlateFeatureButtons;
class UVoltAnimationManager;
class UJointEdGraphNode;

class SLevelOfDetailBranchNode;
class SJointOutlineBorder;
class SJointGraphNodeBase;

class SGraphPin;
class SGraphNode;
class SVerticalBox;
class SHorizontalBox;
class STextBlock;
class SBorder;

class IDetailsView;

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
	
	TSharedPtr<SBox> PresetBox;

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

	SLATE_BEGIN_ARGS(SJointNodePointerSlate) :
		_StructureOwnerEdNode(nullptr),
		_PointerToStructure(nullptr),
		_bShouldShowDisplayName(true),
		_bShouldShowNodeName(true),
		_BorderArgs(SJointOutlineBorder::FArguments()
			.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OutlineNormalColor(FLinearColor(0.04, 0.04, 0.04))
			.OutlineHoverColor(FLinearColor(0.4, 0.4, 0.5))
			.ContentPadding(FJointEditorStyle::Margin_Normal)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
		),
		_ContentMargin(FJointEditorStyle::Margin_Small)
	{}
	//Optional, if not set, it will not notify it.
	SLATE_ARGUMENT(UJointEdGraphNode*, StructureOwnerEdNode)
	SLATE_ARGUMENT(FJointNodePointer*, PointerToStructure)
		
	SLATE_ATTRIBUTE(FText, DisplayName)
	SLATE_ARGUMENT(bool, bShouldShowDisplayName)
	SLATE_ARGUMENT(bool, bShouldShowNodeName)
	SLATE_ARGUMENT(SJointOutlineBorder::FArguments, BorderArgs)
	SLATE_ARGUMENT(FMargin, ContentMargin)
	// The joint manager that is used for the pick and go operation.
	SLATE_ARGUMENT(UJointManager*, PickingTargetJointManager)

	SLATE_EVENT(FSimpleDelegate, OnHovered)
	SLATE_EVENT(FSimpleDelegate, OnUnhovered)
		
	SLATE_EVENT(FOnNodePickingPerformed, OnNodePickingPerformed)
	SLATE_EVENT(FSimpleDelegate, OnPreNodeChanged)
	SLATE_EVENT(FSimpleDelegate, OnPostNodeChanged)

	//Delagates
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
	
	TSharedPtr<SJointNodePointerSlateFeatureButtons> FeatureButtonsSlate;

private:

	const FText GetRawName();

private:
	
	FJointNodePointer* PointerToTargetStructure = nullptr;
	UJointEdGraphNode* StructureOwnerEdNode = nullptr;
	
	UJointManager* TargetJointManager = nullptr;

private:

	TWeakPtr<class FJointEditorNodePickingManagerRequest> Request = nullptr;

public:

	FOnNodePickingPerformed OnNodePointerPerformedDele;
	
	FSimpleDelegate OnPreNodeChangedDele;
	FSimpleDelegate OnPostNodeChangedDele;
	
	FSimpleDelegate OnHoveredDele;
	FSimpleDelegate OnUnhoveredDele;

public:

	UJointManager* GetTargetJointManager() const;

public:

	void OnHovered();

	void OnUnhovered();

public:

	FReply OnPickupButtonPressed();

	FReply OnGoButtonPressed();
	
	FReply OnCopyButtonPressed();

	FReply OnPasteButtonPressed();

	FReply OnClearButtonPressed();
	
public:
	
	void StartHighlightingNodeOnGraph();

	void StopHighlightingNodeOnGraph();
	
public:
	
	void BlinkSelf();
	
public:
	
	TSharedPtr<SJointOutlineBorder> BorderWidget = nullptr;
	
public:
	
	FVoltAnimationTrack BlinkAnimTrack;
	
};


/**
 * A slate for the button overlay of the SJointNodePointerSlate.
 */

class JOINTEDITOR_API SJointNodePointerSlateFeatureButtons : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SJointNodePointerSlateFeatureButtons) {}
		SLATE_EVENT(FOnClicked, OnPickupButtonPressed)
		SLATE_EVENT(FOnClicked, OnGotoButtonPressed)
		SLATE_EVENT(FOnClicked, OnCopyButtonPressed)
		SLATE_EVENT(FOnClicked, OnPasteButtonPressed)
		SLATE_EVENT(FOnClicked, OnClearButtonPressed)
	SLATE_END_ARGS()
	
public:

	void Construct(const FArguments& InArgs);

public:

	void UpdateVisualOnHovered();
	
	void UpdateVisualOnUnhovered();

public:

	TArray<FVoltAnimationTrack> ButtonAnimTracks;

	TSharedPtr<SHorizontalBox> ButtonHorizontalBox;

public:

	const float BUTTON_INITIAL_OPACITY = 0.f;
	const float BUTTON_ANIM_DELAY_PER_ICON = 0.04f;
	const FSlateRenderTransform BUTTON_INITIAL_TRANSFORM = FSlateRenderTransform(FVector2D(0.f, 50.f));
};


/**
 * A template slate with drawer animations. 
 */

class JOINTEDITOR_API SJointSlateDrawer : public SBoxPanel
{
public:
	SLATE_DECLARE_WIDGET(SJointSlateDrawer, SBoxPanel)
public:

	class FSlot : public SBoxPanel::TSlot<FSlot>
	{
	public:
		SLATE_SLOT_BEGIN_ARGS(FSlot, SBoxPanel::TSlot<FSlot>)
		/** The widget's DesiredSize will be used as the space required. */
		SLATE_SLOT_END_ARGS()

		void Construct(const FChildren& SlotOwner, FSlotArguments&& InArgs)
		{
			SBoxPanel::TSlot<FSlot>::Construct(SlotOwner, MoveTemp(InArgs));
		}
	};
	
	static FSlot::FSlotArguments Slot()
	{
		return FSlot::FSlotArguments(MakeUnique<FSlot>());
	}

	using FScopedWidgetSlotArguments = SBoxPanel::FScopedWidgetSlotArguments<SJointSlateDrawer::FSlot>;
	FScopedWidgetSlotArguments AddSlot()
	{
		return InsertSlot(INDEX_NONE);
	}

	FScopedWidgetSlotArguments InsertSlot(int32 Index = INDEX_NONE)
	{
		return FScopedWidgetSlotArguments(MakeUnique<FSlot>(), this->Children, Index);
	}

	FSlot& GetSlot(int32 SlotIndex);
	const FSlot& GetSlot(int32 SlotIndex) const;
	
public:

	SLATE_BEGIN_ARGS(SJointSlateDrawer) {}
		SLATE_SLOT_ARGUMENT(SJointSlateDrawer::FSlot, Slots)
	SLATE_END_ARGS()
	

	FORCENOINLINE SJointSlateDrawer()
	: SBoxPanel( Orient_Horizontal )
	{
		SetCanTick(false);
		bCanSupportFocus = false;
	}
	
	void Construct(const FArguments& InArgs);

public:

	void UpdateVisualOnHovered();
	
	void UpdateVisualOnUnhovered();

public:

	TArray<FVoltAnimationTrack> ChildrenAnimTracks;

public:

	const float BUTTON_INITIAL_OPACITY = 0.f;
	const float BUTTON_ANIM_DELAY_PER_ICON = 0.04f;
	const FSlateRenderTransform BUTTON_INITIAL_TRANSFORM = FSlateRenderTransform(FVector2D(0.f, 50.f));
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
