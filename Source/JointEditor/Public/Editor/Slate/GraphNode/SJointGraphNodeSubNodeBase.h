//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SJointGraphNodeBase.h"

/**
 *
 */

class JOINTEDITOR_API SJointGraphNodeSubNodeBase : public SJointGraphNodeBase
{
public:
	SLATE_BEGIN_ARGS(SJointGraphNodeSubNodeBase) {}
	SLATE_END_ARGS()
		void Construct(const FArguments& InArgs, UEdGraphNode* InNode);

	~SJointGraphNodeSubNodeBase();

public:

	//for the overlay widget rendering
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

public:
	
	virtual TSharedRef<SWidget> CreateNodeContentArea() override;

	virtual TSharedRef<SWidget> CreateSubNodePanelSection() override;


	//virtual TSharedRef<SWidget> CreateNameBox() override;

public:

	virtual void PopulateNodeSlates() override;

public:
	
	virtual FVector2D GetNodeMinimumSize() const override;
	
	virtual FVector2D GetNodeMaximumSize() const override;

public:
	
	/**
	 * Populate and assign sub node panel.
	 * Override this function to make it use different slate then SWrapBox. (You might need this due to the performance)
	 * @see ClearSubNodeSlatesFromPanel, you must override this as well if you overriden it to use other classes.
	 */
	virtual void PopulateSubNodePanel() override;

	/**
	 * Clear sub node slates from the panel. You must override this function if your slate is utilizing other than SWrapBox.
	 */
	virtual void ClearChildrenOnSubNodePanel() override;

	
	virtual void AddSlateOnSubNodePanel(const TSharedRef<SWidget>& Slate) override;

public:

	virtual bool IsSubNodeWidget() const override;

public:
	
	/**
	 * Event for the mouse button up. 
	 * @param SenderGeometry Same as OnMouseButtonUp.
	 * @param MouseEvent Same as OnMouseButtonUp.
	 * @return Same as OnMouseButtonUp.
	 */
	virtual FReply OnMouseButtonUp( const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent ) override;

	/**
	 * Event for the mouse button down. 
	 * @param SenderGeometry Same as OnMouseButtonDown.
	 * @param MouseEvent Same as OnMouseButtonDown.
	 * @return Same as OnMouseButtonDown.
	 */
	virtual FReply OnMouseButtonDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent) override;


	//To handle transaction issue.
	virtual FReply OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent) override;


public:

	virtual FLinearColor GetNodeBodyBackgroundColor() const override;

private:

	EOrientation LastSeenSubNodeOrientation = EOrientation::Orient_Horizontal;

public:

	const bool IsDissolvedSubNode() const;
};
