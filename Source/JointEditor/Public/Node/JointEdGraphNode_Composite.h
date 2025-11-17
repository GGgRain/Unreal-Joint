//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointEdGraphNode_Tunnel.h"
#include "JointEditorSettings.h"
#include "JointEditorStyle.h"
#include "Node/JointEdGraphNode.h"
#include "JointEdGraphNode_Composite.generated.h"

class UJointEditorSettings;
class FJointEditorStyle;
class SJointSlateDrawer;
class UK2Node_Tunnel;
class UJointManager;
/**
 * This is a editor node that represents a sub graph.
 */
UCLASS()
class JOINTEDITOR_API UJointEdGraphNode_Composite : public UJointEdGraphNode_Tunnel
{
	GENERATED_BODY()

public:

	UJointEdGraphNode_Composite();

public:
	
	/**
	 * Updates the tunnel node pins to match this node's pins.
	 */
	virtual void SynchronizeTunnelNodePins() override;
	
public:

	//UObject Interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

public:
	
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FLinearColor GetNodeBodyTintColor() const override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool GetShouldHideNameBox() const override;

public:
	
	virtual FVector2D GetNodeMinimumSize() const override;

public:

	virtual void AllocateDefaultPins() override;

	virtual void PrepareForCopying() override;
	virtual void PostCopyNode() override;
	virtual void PostPasteNode() override;
	virtual bool CanDuplicateNode() const override;
	
	virtual void ReconstructNode() override;
	virtual void PostPlacedNewNode() override;
	virtual bool CanHaveSubNode() const override;
	virtual bool CanReplaceNodeClass() override;
	virtual bool CanReplaceEditorNodeClass() override;
	
	virtual void NodeConnectionListChanged() override;
	virtual void AllocateReferringNodeInstancesOnConnection(TArray<TObjectPtr<UJointNodeBase>>& Nodes, UEdGraphPin* SourcePin) override;
	
	virtual void UpdateNodeInstance() override;
	virtual bool CanUserDeleteNode() const override;
	virtual void DestroyNode() override;
	virtual void ModifyGraphNodeSlate() override;

public:

	virtual bool CanHaveBreakpoint() const override;
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

public:

	bool UseLowDetailedRendering() const;
	
	bool UseCaptureDetailedRendering() const;
	
public:

	void OnPreviewerHovered();
	void OnPreviewerUnhovered();
	

	FReply OnGraphClicked();
	FReply OnPreviewerOpenCloseClicked();
	FReply OnLockPreviewerClicked();
	FReply OnFitPreviewerClicked();

public:

	void OnRenameTextCommited(const FText& Text, ETextCommit::Type Arg);
	bool OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage);


	virtual void RequestStartRenaming() override;

public:

	UPROPERTY()
	TObjectPtr<class UEdGraph> BoundGraph;

public:
	
	TSharedPtr<class SJointSlateDrawer> ButtonDrawer;
	TSharedPtr<class SInlineEditableTextBlock> BoundGraphNameEditableTextBlock;
	TWeakPtr<class SJointGraphPreviewer> JointGraphPreviewer;

public:
	
	/**
	 * The color of the node on the graph.
	 */
	UPROPERTY(EditAnywhere, Category = "Composite", meta = (DisplayName = "Node Color"))
	FLinearColor NodeColor;

	/**
	 * Whether to show the previewer on the node.
	 */
	UPROPERTY(EditAnywhere, Category = "Composite", meta = (DisplayName = "Show Previewer"))
	bool bShowPreviewer = true;

	/**
	 * Whether to lock the interaction with the previewer.
	 */
	UPROPERTY(EditAnywhere, Category = "Composite", meta = (DisplayName = "Lock Previewer"))
	bool bLockPreviewer = false;

protected:

	UPROPERTY()
	FVector2D SavedViewOffset;

	UPROPERTY()
	float SavedZoomAmount = 1.0f;
};
