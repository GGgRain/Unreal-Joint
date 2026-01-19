//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Editor/Debug/JointNodeDebugData.h"
#include "EdGraph/EdGraphNode.h"
#include "Layout/Visibility.h"
#include "Node/JointNodeBase.h"
#include "EdGraph/EdGraphSchema.h"
#include "Editor/SharedType/JointEditorSharedTypes.h"
#include "SharedType/JointSharedTypes.h"

#include "Misc/EngineVersionComparison.h"
#if UE_VERSION_OLDER_THAN(5, 3, 0)
#include "AIGraph/Classes/AIGraphTypes.h"
#else

class SGraphPanel;class SGraphPanel;
#include "Editor/AIGraph/Classes/AIGraphTypes.h"
#endif

#include "JointEdGraphNode.generated.h"

class SGraphPanel;
class FJointEditorToolkit;
class UJointEdGraph;
class SJointGraphNodeBase;
class UJointManager;
class UJointEdGraphNode;
class FReply;

/**
 * The implementation of the Joint nodes in the Joint manager graph and editor.
 * Override this class to make your own graph node for your fragments or Joint nodes.
 * You must override SupportedNodeClass() and provide the class of the Joint node that you want to use this editor node class with.
 * Note for SDS1 users: Now if you want to implement a new node slate to your Joint fragments and nodes, derive this class and override GenerateGraphNodeSlate() to make slates for the node.
 *
 * Note: This class is designed to work on the C++ only. So, for the full customization of the editor node's action, you must start to override it on the native code side.
 */
UCLASS(Abstract)
class JOINTEDITOR_API UJointEdGraphNode : public UEdGraphNode, public IJointEdNodeInterface
{
	GENERATED_BODY()

public:
	
	UJointEdGraphNode();

	virtual ~UJointEdGraphNode() override;

public:
	
	//The instance of the runtime Joint node that this editor graph node represent on the graph.
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="Developer Mode")
	TObjectPtr<UObject> NodeInstance;


#if WITH_EDITOR

	//The class data of the instance of the runtime Joint node. It is used to restore the node instance if the instance becomes invalid.
	//Joint 2.9 : deprecated. Use NodeClassData instead.
	UPROPERTY(VisibleAnywhere, Category="Developer Mode")
	struct FGraphNodeClassData ClassData;

#endif
	
	//The class data of the instance of the runtime Joint node. It is used to restore the node instance if the instance becomes invalid. 
	UPROPERTY(VisibleAnywhere, Category="Developer Mode")
	struct FJointGraphNodeClassData NodeClassData;

public:
	
	/**
	 * The parent node that this node is attached at. If this node is the highest node on the hierarchy, It is nullptr.
	 * */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="Developer Mode")
	TObjectPtr<UJointEdGraphNode> ParentNode;

	/**
	 * The list of the sub nodes attached on this node.
	 */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="Developer Mode")
	TArray<TObjectPtr<UJointEdGraphNode>> SubNodes;

	//The data of the pins that this graph node has. Modifying this property will affect the pins list of the node.
	//This is necessary due to the accessing the pins by its category and name manually is difficult to manage.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Editor Node")
	TArray<FJointEdPinData> PinData;

public:

	/**
	 * The orientation of the sub nodes' alignment on this node.
	 */
	UPROPERTY(EditAnywhere, Category="Visual")
	TEnumAsByte<EOrientation> SubNodeBoxOrientation = Orient_Horizontal;
	
	/**
	 * The Slate Detail Level to use to display elements on the graph slate. This will not do anything for the base nodes.
	 */
	UPROPERTY(EditAnywhere, Category="Visual")
	TEnumAsByte<EJointEdSlateDetailLevel::Type> SlateDetailLevel = EJointEdSlateDetailLevel::SlateDetailLevel_Maximum;
	
public:

	/**
	 * The compile message to show on the graph node and compiler result on the Joint editor.
	 */
	TArray<TSharedPtr<class FTokenizedMessage>> CompileMessages;

public:
	
	/**
	 * Return the node class this editor node class supports. Editor module will automatically find the Ed class for the provided node class.
	 * @return the class of the Joint node that this node supports.
	 */
	virtual TSubclassOf<UJointNodeBase> SupportedNodeClass();
	
public:
	
	/**
	 * Properties that are hidden for the node on the simple display widget.
	 */
	UPROPERTY(VisibleAnywhere, Category="Developer Mode")
	TSet<FName> SimpleDisplayHiddenProperties;
	
public:

	TWeakPtr<FJointEditorToolkit> OptionalToolkit;

public:
	
	/**
	 * TODO : make a synchronization code
	 */
	UPROPERTY(VisibleAnywhere, Category="Developer Mode")
	bool bFromExternal = true;
	
public:


	//Slate Action
	
	/**
	 * Use this function to populate any default widgets on the graph node body.
	 * Access the slate and change the slate as you want.
	 *
	 * If you must change the slate after it is populated, you can access the slate with GetGraphNodeSlate(). Use this function on further customization and instancing.
	 * 
	 * Note for SDS1 users : You don't need to create new node slate class and mess around the editor to assign the slate anymore because now we have this function.
	 * See how the new native editor fragments utilize this function.
	 */
	virtual void ModifyGraphNodeSlate();
	
	/**
	 * Set the graph node's slate that is populated on the Joint graph editor.
	 */
	void SetGraphNodeSlate(const TSharedPtr<SJointGraphNodeBase>& GraphNodeSlate);

	void ClearGraphNodeSlate();

	/**
	 * Get the graph node's slate that is populated on the Joint graph editor.
	 * In most case, you don't need to cast it to derived slate class but if you need, then you can try to cast it with 'StaticCastSharedPtr<SJointGraphNodeSubNodeBase>(GetGraphNodeSlate());'
	 * But you make sure to whether it is valid first, and whether it has the type you want by GetGraphNodeSlate()->GetType() == "SJointGraphNodeSubNodeBase" (or something else you want).  
	 * 
	 * NOTE: It's about the weak pointer general usage - you have to pin it and store it on a local shared pointer variable before using it - avoid calling functions on .Pin()->Function()!!!!!
	 * This will cause the slate's internal shared pointer to be destroyed. (don't know why)
	 */
	TWeakPtr<SJointGraphNodeBase> GetGraphNodeSlate() const;

	/**
	 * Check whether the graph node's slate can be reused on the provided graph.
	 * If it returns false, the graph editor will throw out the old slate and create a new one.
	 * By default, it checks whether the InGraph is the same as the graph that this node belongs to.
	 * @param InGraphPanel The graph panel to check the reusability of the slate.
	 * @return Whether we can reuse the slate on the provided graph or not.
	 */
	bool CheckGraphNodeSlateReusableOn(TWeakPtr<SGraphPanel> InGraphPanel) const;

private:

	/**
	 * Saved Slate of the graph node. Guaranteed to be cast to SJointGraphNodeBase.
	 * Graph node slate will be reused if it is possible to reuse.
	 */
	TSharedPtr<SJointGraphNodeBase> GraphNodeSlate = nullptr;
	
public:

	//Request the node slate to refresh the slates: it will let it throw out the old internal slates and repopulate them.
	void RequestUpdateSlate();
	
	//Request the node slate to refresh the pin widgets and repopulate it.
	void RequestPopulationOfPinWidgets();

public:
	
	void NotifyNodeInstancePropertyChangeToGraphNodeWidget(const FPropertyChangedEvent& PropertyChangedEvent, const FString& PropertyName);

	void NotifyGraphNodePropertyChangeToGraphNodeWidget(const FPropertyChangedEvent& PropertyChangedEvent);

public:

	/**
	 * Replace current node instance object with the provided node class.
	 * @param Class The class to replace to
	 */
	void ReplaceNodeClassTo(TSubclassOf<UJointNodeBase> Class);

	/**
	 * Return whether the node class can be replaced. If it returns false, it will hide the entry from the context menu.
	 * @See CanPerformReplaceNodeClassTo for the actual check for each node class type.
	 * @return whether the node class can be replaced
	 */
	virtual bool CanReplaceNodeClass();

	virtual bool CanPerformReplaceNodeClassTo(TSubclassOf<UJointNodeBase> Class);

	/**
	 * Replace current graph node object instance with the provided editor node class.
	 * It will be applied only when the provided editor node class supports the node instance type.
	 * @param Class The class to replace to
	 */
	void ReplaceEditorNodeClassTo(TSubclassOf<UJointEdGraphNode> Class);

	/**
	 * Return whether the editor node class can be replaced. If it returns false, it will hide the entry from the context menu.
	 * @See CanPerformReplaceNodeClassTo for the actual check for each node class type.
	 * @return whether the node class can be replaced
	 */
	virtual bool CanReplaceEditorNodeClass();

	virtual bool CanPerformReplaceEditorNodeClassTo(TSubclassOf<UJointEdGraphNode> Class);

public:

	/**
	 * Notify parent editor graph that the graph has been changed.
	 * This will rebuild the whole graph.
	 */
	void NotifyGraphChanged();
	
	/**
	 * Notify parent editor graph that the graph has been changed and notify that the graph wants the other sub-objects to be up-to-date.
	 * It will not run the full visual representation rebuild.
	 */
	void NotifyGraphRequestUpdate();

public:

	//Triggered when the node's connections (pin) have been changed. Use this function to grab other node that are connected with the node.
	virtual void NodeConnectionListChanged() override;
	
	/**
	 * Allocates the node instances that this graph node refers to. By default, it allocates the owning node instance.
	 * This function is used to get the actual node instance of the connected graph node refers to in the children graph node classes' NodeConnectionListChanged().
	 * Override this function to implement nodes that must be work as connector or proxy in the Joint. See how UJointEdGraphNode_Connector override this function.
	 * Joint 2.10: Now it has SourcePin parameter to provide the pin that triggered this allocation action - useful when you want to allocate different node instances depending on the pin that triggered this action.
	 */
	virtual void AllocateReferringNodeInstancesOnConnection(TArray<TObjectPtr<UJointNodeBase>>& Nodes, UEdGraphPin* SourcePin = nullptr);

public:


	/**
	 * Reallocate pins for the nodes. Override this function to implement pins that must be implemented dynamically.
	 * Joint 2.3.0 : Logic has been changed, now it contains a part where it triggers the node instance's OnUpdatePinData function.
	 * If your fragment need to be updated in both editor node and node instance then call super function of this func.
	 */
	virtual void ReallocatePins();
	
	/**
	 * Allocate default pins for a given node, based only the NodeType, which should already be filled in.
	 * Use this function to initialize the pin of the node.
	 */
	virtual void AllocateDefaultPins() override;

public:

	
	//Implement or remove or update the changed data and pins. It calls UpdatePinsFromPinData(), ImplementPinDataPins(), RemoveUnnecessaryPins() ,ReplicateSubNodePins() internally.
	void UpdatePins();

	//Find and return if the node has an implemented pin for the provided pin data.
	void UpdatePinsFromPinData();
	
	//Implement pins from the pin data.
	void ImplementPinDataPins();

	//Remove pins if we don't need.
	void RemoveUnnecessaryPins();

private:
	
	/**
	 * Whether the graph notification is locked or not.
	 */
	bool bIsLocked = false;

public:
	
	void Lock();

	void Unlock();

	const bool& IsLocked() const {return bIsLocked;}
	
public:

	
	/**
	 * Implement sub node pins and remove the unnecessary ones.
	 * The sub node pins are automatically replicated on the parent-most node when it is needed.
	 * It's a trick to make it work with the default SGraphPanel code.
	 * Replication action will be allowed only if this node is parent-most node.
	 * Also, it will reorder the sub node replicated pins has its valid order.
	 */
	virtual void ReplicateSubNodePins();

	/**
	 * Find the original pin of that provided node pin is replicated from. If the provided pin is not a replicated pin, it will return the provided pin itself.
	 */
	UEdGraphPin* FindOriginalPin(UEdGraphPin* InReplicatedPin);
	
	/**
	 * Find the original pin pf that provided sub node pin is replicated from. If the provided pin is not a replicated sub node pin, it will return nullptr.
	 * @param InReplicatedSubNodePin The replicated sub node pin on the parent-most node.
	 * @return Found original sub node pin instance.
	 */
	UEdGraphPin* FindOriginalSubNodePin(UEdGraphPin* InReplicatedSubNodePin);
	
	/**
	 * Find the replicated pin that is copied from the provided original sub node pin in the parent-most node.
	 * Use this function on getting connection of the pin on the graph. See how we are using it on the NodeConnectionListChanged().
	 * @param InOriginalSubNodePin The original sub node pin on the sub node.
	 * @return Found replicated parent-most pin instance.
	 */
	UEdGraphPin* FindReplicatedSubNodePin(const UEdGraphPin* InOriginalSubNodePin);
	
	/**
	 * Reset the guid of the pin data's each element.
	 * Used to release invalid pin Guid after the copy action.
	 */
	void ResetPinDataGuid();

public:

	/**
	 * Get the map of connections for the existing pin. It only returns for the pins that this node has a pin data of. (created by this node)
	 * @param PinToConnection Output map.
	 */
	void GetPinDataToConnectionMap(TMap<FJointEdPinData, FJointNodes>& PinToConnection);

public:
	
	//Get the parent-most node of the node. parent-most node is the term for the root node of hierarchy that this node is involved in.
	UJointEdGraphNode* GetParentmostNode();

	//Get the Joint manager that have this graph node.
	UJointManager* GetJointManager() const;

public:

	//Pin & Pin Data Related Functions

	//Check if the provided pin is implemented by the pin data of this node by comparing the pin Guid with the stored pin data's implemented pin Guid.
	bool CheckPinIsOriginatedFromThis(const UEdGraphPin* Pin);

	/**
	 * Get the pin that is implemented by the provided pin data from this node or its sub nodes.
	 * @param InPinData The pin data to find the pin for.
	 * @return Found pin instance. nullptr if not found or the pin data is not from this node.
	 */
	UEdGraphPin* GetPinForPinDataFromHierarchy(const FJointEdPinData& InPinData);
	
	/**
	 * Get the pin that is implemented by the provided pin data from this node.
	 * @param InPinData The pin data to find the pin for.
	 * @return Found pin instance. nullptr if not found or the pin data is not from this node.
	 */
	UEdGraphPin* GetPinForPinDataFromThis(const FJointEdPinData& InPinData);

	/**
	 * Get the pin that is implemented by the provided pin data from its sub nodes.
	 * @param InPinData The pin data to find the pin for.
	 * @return Found pin instance. nullptr if not found or the pin data is not from this node or its sub nodes.
	 */
	UEdGraphPin* GetPinForPinDataFromSubNodes(const FJointEdPinData& InPinData);

public:

	/**
	 * Get the pin data that is implementing the provided pin from this node or its sub nodes.
	 * @param Pin The pin to find the pin data for.
	 * @return Found pin data instance. nullptr if not found or the pin is not from this node.
	 */
	FJointEdPinData* GetPinDataForPinFromHierarchy(const UEdGraphPin* Pin);
	
	/**
	 * Get the pin data that is implementing the provided pin from this node only.
	 * @param Pin The pin to find the pin data for.
	 * @return Found pin data instance. nullptr if not found or the pin is not from this node.
	 */
	FJointEdPinData* GetPinDataForPinFromThis(const UEdGraphPin* Pin);

	/**
	 * Get the pin data that is implementing the provided pin from this node but from its sub nodes.
	 * @param Pin The pin to find the pin data for.
	 * @return Found pin data instance. nullptr if not found or the pin is not from this node or its sub nodes.
	 */
	FJointEdPinData* GetPinDataForPinFromSubNodes(const UEdGraphPin* Pin);

public:

	//Grab all the pins from whole hierarchy including this node and its sub nodes.
	TArray<UEdGraphPin*> GetPinsFromHierarchy();

	//Grab all the pins from this node only.
	TArray<UEdGraphPin*> GetPinsFromThis();
	
	//Grab all the pins from whole sub nodes.
	TArray<UEdGraphPin*> GetPinsFromSubNodes();

public:

	//Grab all the pin data from whole hierarchy including this node and its sub nodes.
	TArray<FJointEdPinData*> GetPinDataFromHierarchy();

	//Grab all the pins from this node only.
	TArray<FJointEdPinData>& GetPinDataFromThis();

	//Grab all the pin data from whole sub nodes.
	TArray<FJointEdPinData*> GetPinDataFromSubNodes() const;


public:

	static bool TryCastPinOwnerToJointEdGraphNode(const UEdGraphPin* Pin, UJointEdGraphNode*& OutGraphNode);

	static UJointEdGraphNode* CastPinOwnerToJointEdGraphNode(const UEdGraphPin* Pin);
	
public:

	//Begin of IJointEdNodeInterface implementation

	virtual UObject* JointEdNodeInterface_GetNodeInstance() override;
	
	virtual TArray<FJointEdPinData> JointEdNodeInterface_GetPinData() override;
	
	//End of IJointEdNodeInterface implementation

public:

	/**
	 * Returns the response when this node is being tested for attachment to a target parent node.
	 *
	 * Prior to Joint 2.12, this function was evaluated only for the node being attached and its direct parent, validating the attachment rules strictly at that boundary.
	 *
	 * Starting from Joint 2.12, this function may also be triggered for sub-nodes of the node being attached. In this case, each sub-node receives the attachment target node reference and is allowed to participate in the validation logic.
	 * As a result, InParentNode may not represent the direct parent of this node, but rather the actual attachment target node.
	 */
	virtual FPinConnectionResponse CanAttachThisAtParentNode(const UJointEdGraphNode* InParentNode) const; 
	
	/**
	 * Returns the response when a sub-node is being tested for attachment to this node.
	 *
	 * Prior to Joint 2.12, this function was evaluated only for direct sub-node attachments.
	 *
	 * Starting from Joint 2.12, this function may also be invoked for the parent nodes that belong to a node that we want to attach a sub node, allowing indirect parent nodes to participate in the attachment validation process.
	 */
	virtual FPinConnectionResponse CanAttachSubNodeOnThis(const UJointEdGraphNode* InSubNode) const;

public:

	//return whether this node can have sub node.
	virtual bool CanHaveSubNode() const;
	
	//Add specific node as sub node of this node.
	virtual void AddSubNode(UJointEdGraphNode* SubNode, bool bIsUpdateLocked = false);

	//Find the index of the sub node in the array for the drop action.
	virtual int32 FindSubNodeDropIndex(UJointEdGraphNode* SubNode) const;
	void SetParentNodeTo(UJointEdGraphNode* NewParentNode);
	void Update();

	//Rearrange the sub node at the specified index. This action will fail if the provided node is not added on this node prior to this action. 
	virtual void RearrangeSubNodeAt(UJointEdGraphNode* SubNode, int32 DropIndex, bool bIsUpdateLocked = false);
	
	//Remove specific node from this node's sub node.
	virtual void RemoveSubNode(UJointEdGraphNode* SubNode, bool bIsUpdateLocked = false);

	//Remove all sub nodes of this node.
	virtual void RemoveAllSubNodes(bool bIsUpdateLocked);

	/**
	 * Refresh node instance's the sub node array from the graph node's sub nodes.
	 */
	virtual void SyncNodeInstanceSubNodeListFromGraphNode();

	//Refresh all the sub nodes to have valid parent node reference.
	virtual void UpdateSubNodeChain();
	
	//Check if node is sub node. It will return if the parent node instance is valid.
	virtual bool IsSubNode() const;
	
	//Grab all the parent nodes in the hierarchy.
	virtual TArray<UJointEdGraphNode*> GetAllParentNodesInHierarchy() const;

	//Grab all the sub nodes in the hierarchy.
	virtual TArray<UJointEdGraphNode*> GetAllSubNodesInHierarchy() const;

public:

	/**
	 * Get whether this node can have breakpoint.
	 * @return Whether this node can have breakpoint. returning false will also remove the access to the breakpoint action on the menu
	 */
	virtual bool CanHaveBreakpoint() const;
	
public:

	UObject* GetNodeInstance() const;

	//Get cast version of the node instance. this is just for the clean code.
	template<typename NodeClass=UJointNodeBase>
	FORCEINLINE NodeClass* GetCastedNodeInstance() const
	{
		return NodeInstance ? Cast<NodeClass>(NodeInstance) : nullptr;
	}

	UJointEdGraph* GetCastedGraph() const;
	
public:
	
	FText GetPriorityIndicatorTooltipText() const;

	EVisibility GetPriorityIndicatorVisibility() const;

	FText GetPriorityIndicatorText() const;

public:
	
	/**
	 * Get the orientation of the sub node box. Return EOrientation::Orient_Horizontal by default.
	 * We don't provide a variable to this feature by default because we thought having this feature on all the nodes might harm the readability of the graph.
	 * So instead, we provided it to only some of the nodes that we think it really needs.
	 *
	 * + If the node uses fixed size then it will have multiple rows, but if it is not then all the sub nodes will be populated in a single row or column.
	 * @return Orientation of the sub node box organization.
	 */
	virtual EOrientation GetSubNodeBoxOrientation();

protected:
	
	/**
	 * Whether this node can use fixed custom size.
	 * If false, it will wrapped down to its minimum size.
	 */
	UPROPERTY()
	bool bIsNodeResizeable = true;
	
	/**
	 * Depth of the node in the graph. Used on coloring the node.
	 */
	UPROPERTY(Transient)
	uint16 NodeDepth = 0;

public:

	/**
	 * Whether this node can use fixed custom size.
	 * @return true if the node uses fixed size for the slate.
	 */
	bool GetUseFixedNodeSize() const;


	void RecalculateNodeDepth();

	const uint16& GetNodeDepth() const;

	
public:

	//Update node size to new value.
	virtual void ResizeNode(const FVector2D& NewSize) override;

	//Get the node's size on the graph. Changing this function will manipulate the size of the node's slate.
	virtual FVector2D GetSize() const;
	
	//Get the node's maximum size on the graph. Changing this function will manipulate the size of the node's slate.
	virtual FVector2D GetNodeMaximumSize() const;

	//Get the node's minimum size on the graph. Changing this function will manipulate the size of the node's slate.
	virtual FVector2D GetNodeMinimumSize() const;
	
public:
	
	//Check whether we can add the provided sub node on this node.
	bool CheckCanAddSubNode(const UJointEdGraphNode* SubNode, FPinConnectionResponse& OutResponse, bool bAllowNotification = true) const;

public:
	
	//~ Begin UEdGraphNode Interface
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	//~ End UEdGraphNode Interface

public:

	/**
	 * Whether to hide the name of the node on the graph
	 * There are some cases that you might want to hide the name while still enabling renaming feature to get the event of renaming. This is for that case.
	 * @return whether to hide the name of the node on the graph.
	 */
	virtual bool GetShouldHideNameBox() const;

public:

	/**
	 * Check whether this graph node must use the node instance specified color.
	 * @return whether this graph node must use the node instance specified color.
	 */
	bool CheckShouldUseNodeInstanceSpecifiedBodyColor() const;

	void GrabSlateDetailLevelFromNodeInstance();

protected:

	//hold the outer chain of the node instances and ed nodes on its hierarchy to be able to copy them properly.
	void HoldOuterChainToCopy();

	//Resolve the outer chain of the node instances and ed nodes on its hierarchy after the copy action.
	void RestoreOuterChainFromCopy();
	
public:
	
	virtual void PostPlacedNewNode() override;
	virtual void PrepareForCopying() override;
	virtual void PostCopyNode();
	virtual void PostPasteNode() override;
	
	virtual void PostEditImport() override;
	virtual void PostEditUndo() override;

	virtual void ReconstructNode() override;
	virtual void DestroyNode() override;

	virtual void ImportCustomProperties(const TCHAR* SourceText, FFeedbackContext* Warn) override;
	
public:

	virtual void ReconstructNodeInHierarchy();

public:

	/**
	 * Determine if this node can be created under the specified schema
	 */
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override;
	
	
public:

	virtual void OnRenameNode(const FString& DesiredNewName) override;

	/**
	 * Start to rename this node on the graph.
	 * It will call RequestRename on the graph node slate if it is valid.
	 * Override this function to implement any additional logic when the renaming is requested. (see UJointEdGraphNode_Composite for example)
	 */
	virtual void RequestStartRenaming();

public:
	
	void FeedEditorNodeToNodeInstance();
	
	void BindNodeInstance();

	void UnbindNodeInstance();

private:
	
	//Bind editor node's 'NodeInstancePropertyChanged' function to the node instance's PropertyChangedNotifiers.
	void BindNodeInstancePropertyChangeEvents();
	
	//Unbind editor node's 'NodeInstancePropertyChanged' function from the node instance's PropertyChangedNotifiers.
	void UnbindNodeInstancePropertyChangeEvents();

protected:

	/**
	 * Callback for the node instance's property change event. Override and use this function to implement actions for the property change.
	 */
	virtual void OnNodeInstancePropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent, const FString& PropertyName);

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
	/**
	 * Callback for debug data change.
	 * @param DebugData optional debug data. nullptr if not present ( removed )
	 */
	virtual void OnDebugDataChanged(const struct FJointNodeDebugData* DebugData);
	
public:

	//Update node instance to make it has valid data.
	virtual void UpdateNodeInstance();

	//Check if the stored class data is referring to the existing class.
	virtual bool CheckClassDataIsKnown();
	
	//Notify this node is holding an invalid class data to the FGraphNodeClassHelper.
	virtual void NotifyClassDataUnknown();

	//Let the FGraphNodeClassHelper knows this node is holding an invalid class data.
	virtual bool PatchNodeInstanceFromClassDataIfNeeded();
	
	//Update ClassData from node instance.
	virtual void UpdateNodeClassData();

	//Update ClassData from provided UClass.
	static void UpdateNodeClassDataFrom(UClass* InstanceClass, FJointGraphNodeClassData& UpdatedData);

protected:

	//Allocate node instance's Guid if not set yet.
	virtual void AllocateNodeInstanceGuidIfNeeded() const;
	
	//Reallocate node instance's Guid.
	virtual void ReallocateNodeInstanceGuid() const;

	//Update node outer to the current graph's Joint manager.
	virtual void UpdateNodeInstanceOuterToJointManager() const;

	//Update sub nodes outer to its parent node.
	virtual void UpdateSubNodesInstanceOuterToJointManager() const;

protected:

	//Set the node instance's outer to the provided object.
	virtual bool SetNodeInstanceOuterAs(UObject* NewOuter) const;

public:

	void SetOuterAs(UObject* NewOuter);

public:

	virtual void PatchNodeInstanceFromClassData(FArchive& Ar);
	
	/**
	 * Serialize the node instance data. It also tries to patch up the node instance if the node instance is invalid.
	 */
	virtual void Serialize(FArchive& Ar) override;
	
public:

	//In Joint 2.2, Update Error and UEdGraphNode related error message functions and properties are no longer used, since those are not that useful to display some advanced data for the system.
	//Please Update your API to use 'OnCompileNode' instead.
	
	/**
	 * Compile and check out whether this node has any issues.
	 */
	void CompileNode(const TSharedPtr<class IMessageLogListing>& CompileResultMessage);

	/**
	 * You can override this function and attach any additional logic to update the errors for the node.
	 * You can update the CompileMessage property on here to display the error message for the node on the compilation.
	 *
	 * IMPORTANT NOTE : DO NOT USE CriticalError verbosity! because it will raise an assertion and abort the whole engine on the callsite, and that's not ideal in every perspective (Epic Games deprecated CriticalError verbosity on the ue 5.1 eventually)
	 *
	 * Every log and msg that has been generated on this function will be displayed on the compile and cooking & packaging process, and might halt those actions.
	 */
	virtual void OnCompileNode();

	//Return true if this node has any compile result messages to display.
	bool HasCompileIssues() const;

private:

	FORCEINLINE void AttachDeprecationCompilerMessage();
	FORCEINLINE void AttachPropertyCompilerMessage();
	FORCEINLINE void CompileAndAttachNodeInstanceCompilationMessages();

public:

	/**
	 * Get the color of the node body. override this function to apply a custom color.
	 * @return Color of the node body.
	 */
	virtual FLinearColor GetNodeBodyTintColor() const override;

#if WITH_EDITORONLY_DATA
	
	/**
	 * Joint 2.10 : Now Editor nodes also have their own settings. This will be used only when the editor node doesn't have a valid node instance.
	 * This is only accessible on the code side only - because we don't support BP derived editor node classes.
	 */
	UPROPERTY(Transient)
	FJointEdNodeSetting DefaultEdNodeSetting;

	const FJointEdNodeSetting& GetEdNodeSetting() const;
	
#endif
	
public:

	//Declare the actions the users will get when they right-click on the graph node. By default, it implements add fragment action.
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

	void CreateAddFragmentSubMenu(UToolMenu* Menu, UEdGraph* Graph) const;
	
	void AddContextMenuActions_Fragments(UToolMenu* Menu, const FName SectionName, UGraphNodeContextMenuContext* Context) const;

};