//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

#include "UObject/Interface.h"

#include "EdGraph/EdGraphNode.h" // For ed pin structures.
#include "EdGraph/EdGraphPin.h" // For ed pin structures.

#include "Styling/SlateBrush.h" // For FSlateBrush
#include "Logging/TokenizedMessage.h" // For FTokenizedMessage

#include "JointSharedTypes.generated.h"

class UJointNodeBase;
class AJointActor;
class UTexture2D;

class UActorComponent;
class UDataTable;

class IPropertyHandle;



/**
 * enum for the execution context of the Joint actor.
 */
UENUM(BlueprintType)
enum class EJointActorExecutionType : uint8
{
	None UMETA(DisplayName="None"), // Default value - helps to abort any execution.
	PreBeginPlay UMETA(DisplayName="Pre Begin Play"),
	PostBeginPlay UMETA(DisplayName="Post Begin Play"),
	PrePending UMETA(DisplayName="Pre Pending"),
	PostPending UMETA(DisplayName="Post Pending"),
	PreEndPlay UMETA(DisplayName="Pre End Play"),
	PostEndPlay UMETA(DisplayName="Post End Play"),
};

/**
 * Struct for the execution context of the Joint actor.
 * Joint 2.12.0 : introduced it to support multiple queue based execution (as a replacement of the previous version's direct node playing system).
 */
USTRUCT(BlueprintType)
struct FJointActorExecutionElement
{
	GENERATED_BODY()
	
public:
	
	FJointActorExecutionElement();
	
	FJointActorExecutionElement(const EJointActorExecutionType InExecutionType, UJointNodeBase* InTargetNode);
	
public:
	 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joint")
	EJointActorExecutionType ExecutionType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joint")
	TWeakObjectPtr<class UJointNodeBase> TargetNode;
	
public:
	
	/**
	 * A unique identifier for this execution element.
	 * Used for the debugger to identify the execution element.
	 * Note: This is cheap - Guid is 16 bytes only.
	 */
	UPROPERTY(Transient)
	FGuid ExecutionElementGuid = FGuid::NewGuid();
	
public:
	
	bool operator==(const FJointActorExecutionElement& Other) const
	{
		return ExecutionElementGuid == Other.ExecutionElementGuid;
	}
	
};


/**
 * A data structure that contains the setting data for a property that will be used to display on the graph node by automatically generated slates.
 */
USTRUCT()
struct JOINT_API FJointGraphNodePropertyData
{
	GENERATED_BODY()

public:
	FJointGraphNodePropertyData();

	FJointGraphNodePropertyData(const FName& InPropertyName);

public:
	UPROPERTY(EditAnywhere, Category = "Data")
	FName PropertyName;
};


UENUM(BlueprintType)
enum class EJointBuildPresetBehavior : uint8
{
	Include,
	Exclude
};


/**
 * A wrapper struct for the Joint node instance reference on the graph.
 * This is useful when you have to create a node that requires a reference to a specific node on the graph.
 * 
 * NOTE: Notice that == operator will use the GetTypeHash() function to compare the two structures - which will make two different instances with the same data to be treated as different.
 */
USTRUCT(BlueprintType)
struct JOINT_API FJointNodePointer
{
	GENERATED_BODY()

public:
	FJointNodePointer();

	~FJointNodePointer();

public:
	/**
	 * The node instance that this structure refers to.
	 * Please notice that this property can refer to the nodes that are not originated from the parent Joint manager, and the node on that graph might not be loaded at the time.
	 * We are still testing out whether this behavior is safe to use on the actual product, and not sure about it yet. If something goes wrong, it can be changed at the future update.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node")
	TSoftObjectPtr<UJointNodeBase> Node;

public:
#if WITH_EDITORONLY_DATA

	/**
	 * The editor node that this structure refers to. This is used as backup plan when the node instance has been invalidated.
	 */
	UPROPERTY()
	TSoftObjectPtr<class UEdGraphNode> EditorNode;

#endif

public:

	/**
	 * The node instance classes that can be picked up on this structure.
	 * This is useful when want to make a filter for this instance.
	 * This property will be exposed in the detail tab and be editable for the possible modification.
	 */
	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "Node")
	TSet<TSubclassOf<UJointNodeBase>> AllowedType;

	/**
	 * The node instance classes that can be picked up on this structure.
	 * This is useful when want to make a filter for this instance.
	 * This property will be exposed in the detail tab and be editable for the possible modification.
	 */
	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category = "Node")
	TSet<TSubclassOf<UJointNodeBase>> DisallowedType;

	bool operator==(const FJointNodePointer& Other) const;

	
	static bool CanSetNodeOnProvidedJointNodePointer(FJointNodePointer Structure, UJointNodeBase* Node);

	static TArray<TSharedPtr<FTokenizedMessage>> GetCompilerMessage(FJointNodePointer& Structure, UJointNodeBase* Node,
	                                                                const FString& OptionalStructurePropertyName);
	
	/**
	 * Check whether the node pointer has the same restrictions as the provided node pointer.
	 */
	bool HasSameRestrictionsAs(const FJointNodePointer& Other) const;

	/**
	 * Check whether the node pointer has the same restrictions as the provided set of allowed and disallowed types.
	 */
	bool CheckMatchRestrictions(const TSet<TSubclassOf<UJointNodeBase>>& AllowedClass, const TSet<TSubclassOf<UJointNodeBase>>& DisallowedClasses) const;
	
	
	/**
	 * Check whether the node pointer is valid.
	 * @return true if the node pointer is valid.
	 */
	bool IsValid() const;
	
};

FORCEINLINE uint32 GetTypeHash(const FJointNodePointer& Struct)
{
	return FCrc::MemCrc32(&Struct, sizeof(FGuid));
}

#define RESOLVE_JOINT_POINTER(Pointer, T) \
((Pointer.Node && Cast<T>(Pointer.Node.Get())) ? Cast<T>(Pointer.Node.Get()) : nullptr)


/**
 * A wrapper struct for the nodes for the BP.
 */
USTRUCT(BlueprintType)
struct JOINT_API FJointNodes
{
	GENERATED_BODY()

public:
	FJointNodes();

	FJointNodes(const TArray<UJointNodeBase*>& InNodes);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objects")
	TArray<TObjectPtr<UJointNodeBase>> Nodes;
};


/**
 * The level of the graph node slate details to display.
 */

UENUM(BlueprintType)
namespace EJointEdSlateDetailLevel
{
	enum Type
	{
		SlateDetailLevel_Stow UMETA(DisplayName="Stowed"),
		SlateDetailLevel_Minimal_Content UMETA(DisplayName="Minimal"),
		SlateDetailLevel_Maximum UMETA(DisplayName="Max"),
	};
}


/**
 * Node setting data for the Joint editor. Mainly related to the visuals.
 */
USTRUCT(BlueprintType)
struct JOINT_API FJointEdNodeSetting
{
	GENERATED_BODY()

public:
	FJointEdNodeSetting();

public:
	
	/**
	 * Node's iconic color. This color will be used for the color scheme of the node for the identification on search tab and other purposes.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Node Body")
	FLinearColor NodeIconicColor = FLinearColor(0.026715, 0.025900, 0.035, 1);

public:
	/**
	 * The brush image to display inside the Iconic Node (a little colored block next to the node name) of the graph node slate.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Iconic Node")
	FSlateBrush IconicNodeImageBrush;

public:
	
	/**
	 * Whether to use specified graph node body color.
	 * If it is false, the node body will use the common color scheme of the node body on the graph
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Node Body")
	bool bUseSpecifiedGraphNodeBodyColor = false;

	/**
	 * Whether to use the Iconic color for the node body on the stow node mode.
	 * by default, when the node is stowed, the body will use NodeIconicColor instead of NodeBodyColor - for the better identification of the node.
	 * But if you want to use NodeBodyColor instead of NodeIconicColor, set this to false.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Node Body")
	bool bUseIconicColorForNodeBodyOnStow = true;

	/**
	 * Node body's color. Change this to customize the color of the node.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Node Body")
	FLinearColor NodeBodyColor = FLinearColor::White;

public:
	
	/**
	 * Whether to use NodeShadowImageBrush instead of the default brush.
	 * Tip: if you want to hide the node's shadow, you can set this to true and set NodeShadowImageBrush's DrawAs to ESlateBrushDrawType::NoDrawType.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Node Body")
	bool bUseCustomNodeShadowImageBrush = false;

	/**
	 * Whether to use InnerNodeBodyImageBrush instead of the default brush.
	 * Tip: if you want to hide the node's InnerNodeBody, you can set this to true and set InnerNodeBodyImageBrush's DrawAs to ESlateBrushDrawType::NoDrawType.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Node Body")
	bool bUseCustomInnerNodeBodyImageBrush = false;

	/**
	 * Whether to use bUseCustomOuterNodeBodyImageBrush instead of the default brush.
	 * Tip: if you want to hide the node's OuterNodeBody, you can set this to true and set OuterNodeBodyImageBrush's DrawAs to ESlateBrushDrawType::NoDrawType.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Node Body")
	bool bUseCustomOuterNodeBodyImageBrush = false;


	/**
	 * The brush for the node shadow.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Node Body")
	FSlateBrush NodeShadowImageBrush;

	/**
	 * The brush for Inner Node Body Image.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Node Body")
	FSlateBrush InnerNodeBodyImageBrush;

	/**
	 * The brush for Outer Node Body Image.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Node Body")
	FSlateBrush OuterNodeBodyImageBrush;

public:
	
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Node Body")
	bool bUseCustomShadowNodePadding = false;
	
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Node Body")
	bool bUseCustomContentNodePadding = false;
	
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Node Body")
	FMargin ShadowNodePadding = FMargin(4);
	
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Node Body")
	FMargin ContentNodePadding = FMargin(4);
	
public:
	/**
	 * Whether to display a ClassFriendlyName hint text next to the Iconic node that will be displayed only when SlateDetailLevel is not SlateDetailLevel_Maximum.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Name")
	bool bAllowDisplayClassFriendlyNameText = true;


	/**
	 * Whether to use the simplified display of the class friendly name text instead of original class friendly name text on the graph node.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Name")
	bool bUseSimplifiedDisplayClassFriendlyNameText = false;

	/**
	 * The simplified class friendly name text that will be displayed on the graph node.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual|Name")
	FText SimplifiedClassFriendlyNameText;

public:
	/**
	 * The SlateDetailLevel that the graph node of this node instance will use by default.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Visual")
	TEnumAsByte<EJointEdSlateDetailLevel::Type> DefaultEdSlateDetailLevel =
		EJointEdSlateDetailLevel::SlateDetailLevel_Maximum;


	/**
	 * The data for the properties that will be displayed on the graph node by automatically generated slate.
	 * Useful when you want to show some properties on the graph, but don't afford to implement a custom editor node class for the node.
	 * Also, you can still use this value to implement a simple display for a property on custom editor node class.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Editing")
	TArray<FJointGraphNodePropertyData> PropertyDataForSimpleDisplayOnGraphNode;

public:
	
	/**
	 * If true, The detail tab of the node will show off the editor node's pin data property.
	 * Note: Joint 2.12: It no longer prohibits the execution of OnPinDataChanged, OnPinConnectionChanged delegates. It only controls whether to show the pin data property on the detail tab.
	 */
	UPROPERTY(EditDefaultsOnly, Transient, Category="Editor|Pin")
	bool bAllowEditingOfPinDataOnDetailsPanel = false;

public:
	void UpdateFromNode(const UJointNodeBase* Node);
};


USTRUCT(BlueprintType)
struct JOINT_API FJointEdPinDataSetting
{
	GENERATED_BODY()

public:
	FJointEdPinDataSetting();

public:
	/**
	 * Whether to display the name text of the pin all the time, without hovering.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pin Data Setting")
	bool bAlwaysDisplayNameText = false;
};

/**
 * Data structure for the pin data for the Joint editor.
 * Joint 2.3.0: We moved it to the runtime module to support full customization of the pins on the node instance side.
 */
USTRUCT(BlueprintType)
struct JOINT_API FJointEdPinData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Editor Node")
	FName PinName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Editor Node")
	TEnumAsByte<enum EEdGraphPinDirection> Direction;

	UPROPERTY(VisibleAnywhere, Category="Editor Node")
	FEdGraphPinType Type;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Editor Node - Visual")
	FJointEdPinDataSetting Settings;

public:
	
	//Guid of the pin instance that has been implemented by this data.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Editor Node")
	FGuid ImplementedPinId;

public:
	
	FJointEdPinData();

	FJointEdPinData(const FName& InPinName, const EEdGraphPinDirection& InDirection);

	FJointEdPinData(const FName& InPinName, const EEdGraphPinDirection& InDirection, const FJointEdPinDataSetting& InSettings);

	FJointEdPinData(const FName& InPinName, const EEdGraphPinDirection& InDirection, const FEdGraphPinType& InType);

	FJointEdPinData(const FName& InPinName, const EEdGraphPinDirection& InDirection, const FEdGraphPinType& InType,
	                const FJointEdPinDataSetting& InSettings);

	FJointEdPinData(const FName& InPinName, const EEdGraphPinDirection& InDirection, const FEdGraphPinType& InType,
	                const FJointEdPinDataSetting& InSettings, const FGuid& InImplementedPinGuid);


	FJointEdPinData(const FJointEdPinData& Other);

public:
	static const FJointEdPinData PinData_Null;

	bool HasSameSignature(const FJointEdPinData& Other) const;

	void CopyPropertiesFrom(const FJointEdPinData& Other);
	
	bool operator==(const FJointEdPinData& Other) const;

public:
	/**
	 * Pin Type declaration.
	 *
	 * Joint Editor has only those 2 type of pin, and any additional pin types will not be handled by the system correctly.
	 * All the types share the same pin slate, and used for the identification only.
	 */

	//Type of the pin for every normal usages like branch and etc.
	static const FEdGraphPinType PinType_Joint_Normal;

	//Type of the pin for parent pin of the sub node. (Don't use it, it only needs for the system.)
	static const FEdGraphPinType PinType_Joint_SubNodeParent;
};

FORCEINLINE uint32 GetTypeHash(const FJointEdPinData& Struct)
{
	return FCrc::MemCrc32(&Struct, sizeof(FGuid));
}

inline bool operator!=(const FJointEdPinData& A, const FJointEdPinData& B);


// A wrapper struct for the pin data on the copy-paste actions.
USTRUCT()
struct JOINT_API FJointPinDataConnection
{
	GENERATED_BODY()

	FJointPinDataConnection() {}
	FJointPinDataConnection(const FGuid& InToNodeGuid, const FJointEdPinData& InToPinData)
		: ToNodeGuid(InToNodeGuid), ToPinData(InToPinData)
	{}

public:

	UPROPERTY(EditAnywhere, Category="Editor Node")
	FGuid ToNodeGuid;

	UPROPERTY(EditAnywhere, Category="Editor Node")
	FJointEdPinData ToPinData;
	
};

USTRUCT()
struct JOINT_API FJointPinDataConnections
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category="Editor Node")
	TArray<FJointPinDataConnection> Connections;
};

UENUM(BlueprintType)
namespace EJointEdMessageSeverity
{
	enum Type
	{
		Info UMETA(DisplayName="Info"),
		Warning UMETA(DisplayName="Warning"),
		PerformanceWarning UMETA(DisplayName="PerformanceWarning"),
		Error UMETA(DisplayName="Error"),
	};
}

/**
 * Data structure for the log message of runtime nodes.
 */
USTRUCT(BlueprintType)
struct JOINT_API FJointEdLogMessage
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Message")
	TEnumAsByte<EJointEdMessageSeverity::Type> Severity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Message")
	FText Message;

public:
	FJointEdLogMessage();

	FJointEdLogMessage(const EJointEdMessageSeverity::Type& InVerbosity, const FText& InMessage);
};

FORCEINLINE uint32 GetTypeHash(const FJointEdLogMessage& Struct)
{
	return FCrc::MemCrc32(&Struct, sizeof(FJointEdLogMessage));
}


/**
 * An interface class for the runtime node -> editor node communication.
 * Ed-nodes can be cast into this class.
 * This will be probably the best way to handle the data.
 */

UINTERFACE(Blueprintable)
class JOINT_API UJointEdNodeInterface : public UInterface
{
	GENERATED_BODY()
};

class JOINT_API IJointEdNodeInterface
{
	GENERATED_BODY()

public:
	IJointEdNodeInterface();

public:
	/**
	 * Get the node instance the ed node has.
	 * @return node instance of the graph node.
	 */
	virtual UObject* JointEdNodeInterface_GetNodeInstance();

	/**
	 * Get the pin data from the editor node.
	 * @return pin data of the editor node.
	 */
	virtual TArray<FJointEdPinData> JointEdNodeInterface_GetPinData();
};

