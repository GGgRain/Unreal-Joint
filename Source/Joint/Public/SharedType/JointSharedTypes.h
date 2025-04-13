//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

#include "UObject/Interface.h"

#include "EdGraph/EdGraphNode.h" // for ed pin structures.
#include "EdGraph/EdGraphPin.h" // for ed pin structures.

#include "JointSharedTypes.generated.h"

class AJointActor;
class UTexture2D;

class UActorComponent;
class UDataTable;

class UJointNodeBase;

class IPropertyHandle;

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
 */
USTRUCT(BlueprintType)
struct JOINT_API FJointNodePointer
{
	GENERATED_BODY()

public:
	
	FJointNodePointer():
#if WITH_EDITORONLY_DATA
		Node(nullptr),
		EditorNode(nullptr)
#else
		Node(nullptr)
#endif
	
	{}

	~FJointNodePointer() {}

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
	
#if WITH_EDITORONLY_DATA
	
	/**
	 * The node instance classes that can be picked up on this structure.
	 * This is useful when want to make a filter for this instance.
	 * This property will be exposed in the detail tab and be editable for the possible modification.
	 */
	UPROPERTY(AdvancedDisplay, EditAnywhere, Category = "Node")
	TSet<TSubclassOf<UJointNodeBase>> AllowedType;

	/**
	 * The node instance classes that can be picked up on this structure.
	 * This is useful when want to make a filter for this instance.
	 * This property will be exposed in the detail tab and be editable for the possible modification.
	 */
	UPROPERTY(AdvancedDisplay, EditAnywhere, Category = "Node")
	TSet<TSubclassOf<UJointNodeBase>> DisallowedType;
	
#endif


#if WITH_EDITOR
	
	static bool CanSetNodeOnProvidedJointNodePointer(FJointNodePointer Structure, UJointNodeBase* Node);

	static TArray<TSharedPtr<FTokenizedMessage>> GetCompilerMessage(FJointNodePointer& Structure, UJointNodeBase* Node, const FString& OptionalStructurePropertyName);

#endif


	bool operator==(const FJointNodePointer& Other) const
	{
		return GetTypeHash(this) == GetTypeHash(&Other);
	}
	
};

FORCEINLINE uint32 GetTypeHash(const FJointNodePointer& Struct)
{
	return FCrc::MemCrc32(&Struct, sizeof(FGuid));
}



/**
 * A wrapper struct for the nodes for the BP.
 */
USTRUCT(BlueprintType)
struct JOINT_API FJointNodes
{
	GENERATED_BODY()

public:

	FJointNodes() {}

	FJointNodes(const TArray<UJointNodeBase*>& InNodes) : Nodes(InNodes) {}


public:

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Objects")
	TArray<UJointNodeBase*> Nodes;
	
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

	FJointEdPinData(const FName& InPinName, const EEdGraphPinDirection& InDirection, const FEdGraphPinType& InType);

	FJointEdPinData(const FName& InPinName, const EEdGraphPinDirection& InDirection, const FEdGraphPinType& InType, const FJointEdPinDataSetting& InSettings);
	
	FJointEdPinData(const FName& InPinName, const EEdGraphPinDirection& InDirection, const FEdGraphPinType& InType, const FJointEdPinDataSetting& InSettings,  const FGuid& InGuid);

	
	FJointEdPinData(const FJointEdPinData& Other);



public:

	static const FJointEdPinData PinData_Null;

	bool HasSameSignature(const FJointEdPinData& Other) const
	{
		return PinName == Other.PinName && Direction == Other.Direction && Type == Other.Type;
	}

	void CopyPropertiesFrom(const FJointEdPinData& Other)
	{
		PinName = Other.PinName;
		Direction = Other.Direction;
		Type = Other.Type;
		Settings = Other.Settings;
	}

	bool operator==(const FJointEdPinData& Other) const
	{
		return PinName == Other.PinName && Direction == Other.Direction && Type == Other.Type && ImplementedPinId == Other.ImplementedPinId;
	}

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


inline bool operator!=(const FJointEdPinData& A, const FJointEdPinData& B)
{
	return A == B;
}


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

	IJointEdNodeInterface() {}

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