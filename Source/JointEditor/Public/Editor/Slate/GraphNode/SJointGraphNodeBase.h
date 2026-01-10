//Copyright 2022~2024 DevGrain. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"
#include "SGraphNodeResizable.h"
#include "VoltAnimationTrack.h"

#include "Editor/Debug/JointNodeDebugData.h"

#include "Editor/GraphEditor/Private/DragNode.h"
#include "SharedType/JointSharedTypes.h"

class SJointOutlineBorder;
class SJointBuildPreset;
class SJointDetailsView;
class SJointGraphNodeInsertPoint;
class UJointEdGraphNode_Fragment;
class UVoltAnimationManager;
class UJointEdGraphNode;


class SWidget;
class SJointGraphNodeCompileResult;
class SImage;
class SBorder;
class SHorizontalBox;
class SVerticalBox;
class SWrapBox;
class SCheckBox;
class SGraphNode;


namespace JointGraphNodeResizableDefs
{
	/** Minimum size for node */
	static const FVector2D MinNodeSize( 100.f,100.f);

	/** Maximum size for node */
	static const FVector2D MaxNodeSize( 5000.0f, 5000.0f );

	/** Minimum size for node */
	static const FVector2D MinFragmentSize( 4.f,4.f);

	/** Maximum size for node */
	static const FVector2D MaxFragmentSize( 5000.0f, 5000.0f );
}

namespace JointGraphNodeDragDropOperationDefs
{
	/** Minimum size for node */
	static const FVector2D NullDragPosition( -2000,-2000);
}



class JOINTEDITOR_API FDragJointGraphNode : public FDragNode
{
public:
	DRAG_DROP_OPERATOR_TYPE(FDragJointGraphNode, FDragNode)

	static TSharedRef<FDragJointGraphNode> New(const TSharedRef<SGraphPanel>& InGraphPanel, const TSharedRef<SGraphNode>& InDraggedNode);
	static TSharedRef<FDragJointGraphNode> New(const TSharedRef<SGraphPanel>& InGraphPanel, const TArray< TSharedRef<SGraphNode> >& InDraggedNodes);

	UJointEdGraphNode* GetDropTargetNode() const;

	double StartTime;

public:


	/**
	 * The node slates that are being drag overed by this operation.
	 * We store this to play release animation on the end of the drag operation.
	 */
	TArray<TWeakPtr<SWidget>> DragOverNodes;
	
public:

	virtual void OnDrop(bool bDropWasHandled, const FPointerEvent& MouseEvent) override;

	virtual bool AffectedByPointerEvent(const FPointerEvent& PointerEvent) override;
	
	virtual void NotifyDropAction();
	
protected:
	typedef FDragNode Super;
};


/**
 * SDS2: 
 * New layout for the whole slate:
 *
 *  LeftNodeBox                  NodeBody                  RightNodeBox
 *  ___________       ________________________________     ____________
 * |           |    |         CenterWholeBox         |    |            |
 * |           |    |   __________________________   |    |            | 
 * |           |    |  |   _____________________  |  |    |            |
 * |           |    |  |  |NameBox             |  |  |    |            |
 * |Input Pin 1|    |  |  |____________________|  |  |    |Output Pin 1|
 * |Input Pin 2|    |  |   _____________________  |  |    |Output Pin 2|
 * |Input Pin 3|    |  |  |CenterContentBox    |  |  |    |Output Pin 3|
 * |Input Pin 4|    |  |  |____________________|  |  |    |Output Pin 4|
 * |...        |    |  |   _____________________  |  |    |...         |
 * |           |    |  |  |SubNodeBox          |  |  |    |            |
 * |           |    |  |  |____________________|  |  |    |            |
 * |           |    |  |__________________________|  |    |            |
 * |___________|    |________________________________|    |____________| 
 */
	
/**
 * Don't touch LeftNodeBox, RightNodeBox if possible.
 * Those are related with the pin actions and difficult to manage without deep understanding of its structure.
 *
 * Consider overriding CenterWholeBox if possible.
 * SJointGraphNodeBase also provide a static functions that help you to populate the default slate set by each section.
 * Use it on your slate customization code.
 *
 * Check out how the default provided Joint Editor Graph Node classes. (especially UJointEdFragment_Context.)
 */

/**
 * Joint 2.8 : It has been seriously overhauled; now the most of the attributes are gone, and rely on the manual update for better performance 
 */
class JOINTEDITOR_API SJointGraphNodeBase : public SGraphNodeResizable
{
public:
	SLATE_BEGIN_ARGS(SJointGraphNodeBase) {}
	SLATE_END_ARGS()

public:
	
	void Construct(const FArguments& InArgs, UEdGraphNode* InNode);
	
	virtual ~SJointGraphNodeBase() override;

public:

	/**
	 * Initialize the slate's variable and sub-slates for the layout.
	 * Not recommended to be executed multiple time due to the performance cost.
	 * Instead, Consider executing UpdateGraphNode() or following update function for each section of the slate. (ex, PopulateSubNodeSlates)
	 */
	virtual void InitializeSlate();

public:
	/**
	 * Populate the basic slates that consists the graph node slate. This includes overlay widgets.
	 * If this function is triggered again then it will flush the original slates and repopulate all the layouts.
	 */
	virtual void PopulateNodeSlates();

	virtual TSharedRef<SWidget> CreateCenterWholeBox();

	virtual TSharedRef<SWidget> CreateCenterContentBox();

public:

	virtual TSharedRef<SWidget> CreateNameBox();

	virtual TSharedRef<SWidget> CreateDissolvedSubNodeIndication();

public:

	virtual TSharedRef<SWidget> CreateSubNodePanelSection();

	virtual TSharedRef<SWidget> CreateNodeTagBox();

	/**
	 * Populates the simple display for the properties according to the node instance's PropertyDataForSimpleDisplayOnGraphNode.
	 */
	virtual TSharedRef<SWidget> PopulateSimpleDisplayForProperties();

public:
	
	virtual TSharedRef<SBorder> CreateNodeBody(const bool bSphere = false);
	
	virtual TSharedRef<SJointOutlineBorder> CreateNodeBackground(const bool bSphere = false);

public:

	//Pin Slate Related

	/**
	 * Populate and update the pin widgets.
	 */
	void PopulatePinWidgets();

	void CreateSubNodePinWidgets();

	virtual void CreatePinWidgets() override;

	virtual void AddPin( const TSharedRef<SGraphPin>& PinToAdd ) override;

public:

	//Sub node slate related

	/**
	 * Populate and update sub node slates and add on the SubNodeBox.
	 * This action will not implement the sub nodes with GetCanBeAttachedAtSubNodeBox() value as false.
	 */
	virtual void PopulateSubNodeSlates();

	/**
	 * Assign sub node slates as this node's sub node slates.
	 */
	void AssignSubNode(const TSharedPtr<SGraphNode>& SubNodeWidget);

	/**
	 * Create sub node slate with provided EdGraphNode Instance and graph panel.
	 * @param OwnerGraphPanel the graph panel that will have the new node slate.
	 * @param EdFragment the Editor fragment instance that will be represented by the new sub node slate.
	 * @return Newly create sub node slate.
	 */
	static TSharedPtr<SGraphNode> CreateSubNodeWidget(const TSharedPtr<SGraphPanel>& OwnerGraphPanel, UJointEdGraphNode_Fragment* EdFragment);

public:

	/**
	 * SGraphNode slates that are assigned on this slate as sub nodes.
	 * Only the slates assigned on this array will be processed correctly as sub node slates.
	 */
	TArray< TSharedPtr<SGraphNode> > SubNodes;
	
public:

	/**
	 * Now we use SPanel instead of SWrapBox due to its performance concern.
	 */
	TSharedPtr<SPanel> SubNodePanel;

public:

	/**
	 * Populate and assign sub node panel.
	 * Override this function to make it use different slate then SWrapBox. (You might need this due to the performance)
	 * @see ClearChildrenOnSubNodePanel, AddSlateOnSubNodePanel. you must override those as well if you overriden it to use other classes.
	 */
	virtual void PopulateSubNodePanel();

	/**
	 * Clear sub node slates from the panel. You must override this function if your slate is utilizing other than SWrapBox.
	 */
	virtual void ClearChildrenOnSubNodePanel();

	/**
	 * Add a slate on the Sub Node panel. You must override this function if your slate is utilizing other than SWrapBox.
	 */
	virtual void AddSlateOnSubNodePanel(const TSharedRef<SWidget>& Slate);

public:

	/**
	 * Check whether this slate is a sub node widget or not.
	 * @return true if this slate is a sub node widget, false otherwise.
	 */
	virtual bool IsSubNodeWidget() const;

public:


	/**
	 * Cast and return subnode panel to the provided template typename.
	 * Please notice this is not a safe action that can be called anywhere, you must ensure what the class of the content is.
	 * @tparam SlateClass target slate class we are going to static cast.
	 * @return cast widget.
	 */
	template<typename SlateClass>
	FORCEINLINE TSharedPtr<SlateClass> INLINE_GetCastedSubNodePanel();

protected:

	//Overlay widget related
	
	virtual TArray<FOverlayWidgetInfo> GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const override;

	static void AttachWidgetOnOverlayInfo(const TSharedPtr<SWidget>& WidgetToAdd, TArray<FOverlayWidgetInfo>& Widgets, FVector2D& OverlayOverallOffset, FVector2D& OverlayOffset);

protected:
	
	void AssignBuildTargetPresetOverlay();
	void RemoveBuildPresetOverlay();
	
	void AssignBreakpointOverlay();
	void RemoveBreakpointOverlay();
	
	void AssignCompileResultOverlay();
	void RemoveCompileResultOverlay();

public:
	
	void GetNodeColorScheme(const bool bIsSelected, FLinearColor& NormalColor, FLinearColor& HoverColor, FLinearColor& OutlineNormalColor, FLinearColor& OutlineHoverColor) const;

public:

	//Overlay Related
	virtual void UpdateErrorInfo() override;
	virtual void UpdateBreakpoint();
	virtual void UpdateBuildTargetPreset();

public:

	//Changed the slates in the name box. This works better than attributes.
	virtual void UpdateNameBox();

public:

	TSharedPtr<STextBlock> NodeTitleHintTextBlock = nullptr;
	
	TSharedPtr<SBorder> NodeTitleSeparatorBorder = nullptr;

public:

	TSharedPtr<SJointDetailsView> JointDetailsView = nullptr;

public:
	
	TSharedPtr<SJointBuildPreset> BuildPresetOverlay = nullptr;

	TSharedPtr<SWidget> BreakpointOverlay = nullptr;
	
	TSharedPtr<SJointGraphNodeCompileResult> CompileResultOverlay = nullptr;

public:
	
	TSharedPtr<SJointOutlineBorder> NodeBackground = nullptr;

	TSharedPtr<SBorder> NodeBody = nullptr;
	
	TSharedPtr<SHorizontalBox> NameBox = nullptr;
	
	TSharedPtr<SHorizontalBox> DissolveIndicator = nullptr;

	TSharedPtr<SVerticalBox> CenterWholeBox = nullptr;

	TSharedPtr<SVerticalBox> CenterContentBox = nullptr;

public:

	TSharedPtr<SBorder> NodeTagBox = nullptr;
	
	TSharedPtr<SVerticalBox> NodeTagContentBox = nullptr;
	
public:

	TArray<TSharedPtr<SJointGraphNodeInsertPoint>> InsertPoints;

public:

	const FSlateBrush* NodeBodyBorderImage = nullptr;
	const FSlateBrush* NodeBackgroundInBorderImage = nullptr;
	const FSlateBrush* NodeBackgroundOutBorderImage = nullptr;

public:

	void ClearSlates();

public:

	/**
	 * Get the editor graph node instance that this slate refers to.
	 * @return The editor graph node instance that this slate refers to.
	 */
	UJointEdGraphNode* GetCastedGraphNode() const;
	
	/**
	 * Modify slate from the editor graph node instance.
	 */
	void ModifySlateFromGraphNode() const;

public:

	/**
	 * An event for the property change detection on the node instance.
	 * @param PropertyChangedEvent 
	 * @param PropertyName 
	 */
	virtual void OnNodeInstancePropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, const FString& PropertyName);

	virtual void OnGraphNodePropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent);
	
	virtual void OnDebugDataChanged(const FJointNodeDebugData* Data);

	
public:

	/**
	 * Update the node tag box to validate.
	 */
	void UpdateNodeTagBox() const;

protected:

	/**
	 * Feed this slate instance to the editor graph node to make the editor node able to access this graph node slate.
	 */
	void AssignSlateToGraphNode();

public:

	FText GetIndexTooltipText();

	EVisibility GetIndexVisibility();

	FText GetIndexText();

	FText GetGraphNodeName() const;

public:
	
	const FSlateBrush* GetIconicNodeSlateBrush() const;
	
	const FText GetIconicNodeText() const;

	const bool GetWhetherToDisplayIconicNodeText() const;

	const uint16 GetDissolvedSubnodeCounts() const;

private:

	/**
	 * Only for cosmetics
	 */
	uint16 DissolvedSubnodeCounts = 0;


public:
	
	virtual FLinearColor GetNodeBodyBackgroundColor() const;

public:
	
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual FVector2D GetNodeMinimumSize() const override;
	virtual FVector2D GetNodeMaximumSize() const override;
	
public:
	
	/**
	 * Fully update the slates. This is the equivalent of the refresh action.
	 */
	virtual void UpdateGraphNode() override;
	void UpdateOwnerOfPinWidgets();

public:
	
	/** @param OwnerPanel  The GraphPanel that this node belongs to */
	virtual void SetOwner(const TSharedRef<SGraphPanel>& OwnerPanel) override;
	
public:

	/**
	 * Get the actual graph node slate the mouse cursor is located at. Don't override this function.
	 */
	virtual TSharedRef<SGraphNode> GetNodeUnderMouse(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

public:

	//Input & Interaction related

	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	virtual FReply OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent) override;

	virtual void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	
	virtual void OnMouseLeave( const FPointerEvent& MouseEvent ) override;

	virtual FReply OnMouseButtonUp( const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent ) override;

	virtual FReply OnMouseButtonDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent) override;

public:

	virtual FChildren* GetChildren() override;
	
public:

	//Drag & Drop related
	
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;

	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	
public:

	/**
	 * If mouse leaves the widget while mouse down, it will be considered as a dragdrop operation.
	 * Otherwise, it will check the distance where the drag has been started off, and if that exceed, it will starts off the drag drop.
	 */
	
	virtual void OnDragStarted();
	
	virtual void OnDragEnded();
	
	void ResetDragMousePos();
	
	void CacheMouseDownPos(const FPointerEvent& MouseEvent);

public:
	
	void OnRenameTextCommited(const FText& InText, ETextCommit::Type CommitInfo);
	
	bool VerifyRenameNameOnTextChanged(const FText& InText, FText& OutErrorMessage);
	
public:
	
	/**
	 * Cached mouse position on the screen when the user mouse button down on the node.
	 */
	FVector2D MouseDownScreenPosition = JointGraphNodeDragDropOperationDefs::NullDragPosition;

	/**
	 * start distance for the drag drop action.
	 */
	float DragStartDistance = 10;
	
public:

	FGraphSelectionManager* GetSelectionManager() const;

public:
	
	/**
	 * Check whether the editor graph node instance for this graph node slate has been selected on the graph.
	 * @return true if the editor graph node is selected
	 */
	bool CheckWhetherNodeSelected() const;

public:
	
	/**
	 * Check whether the node instance can be renamed on the graph.
	 */
	bool CheckRenameNodeInstance() const;

public:

	virtual void OnGraphSelectionChanged(const TSet<UObject*>& NewSelection);

public:

	/**
	 * Play highlighting animation on the node body.
	 * @param bBlinkForOnce Whether to highlight the node once for once or play it endlessly until StopHighlightAnimation() executed.
	 */
	void PlayHighlightAnimation(bool bBlinkForOnce, float SpeedMultiplier = 1.0f);
	
	/**
	 * Stop highlighting animation.
	 */
	void StopHighlightAnimation();
	
public:
	
	
	/**
	 * Play the node background color reset animation if there is no other states that are handling the color of it.
	 */
	void PlayNodeBackgroundColorResetAnimationIfPossible(bool bInstant = false);

	/**
	 * Play the selection animation by the current selection state.
	 */
	void PlaySelectionAnimation();

	void PlayDebuggerAnimation(bool bIsPausedFrom, bool bIsBeginPlay, bool bIsPending, bool bIsEndPlay);
	
	void UpdateDebuggerAnimationByState();
	
	void SetNodeBodyToDebuggerExecutedImage();

	
	void PlayNodeBodyScaleAnimation(float Scale = 1.1f, float Duration = 0.3f);
	
	void PlayNodeBodyColorAnimation(const FLinearColor Color, const FLinearColor BlinkTargetColor, float Duration, const bool bBlink);
	
	void ResetNodeBodyColorAnimation();

	
	void PlayDropAnimation();

	void PlayInsertAnimation();

	void PlayInsertPointHighlightAnimation();

public:

	virtual void InitializeVoltVariables();
	
	
	FVoltAnimationTrack NodeBodyTransformTrack;
	
	FVoltAnimationTrack NodeBodyColorTrack;

public:
	
	virtual const FSlateBrush* GetShadowBrush(bool bSelected) const override;

public:

	const EJointEdSlateDetailLevel::Type GetSlateDetailLevel() const;
};

template <typename SlateClass>
TSharedPtr<SlateClass> SJointGraphNodeBase::INLINE_GetCastedSubNodePanel()
{
	if(SubNodePanel.IsValid())
	{
		return StaticCastSharedPtr<SlateClass>(SubNodePanel);
	}
	
	return nullptr;
}

