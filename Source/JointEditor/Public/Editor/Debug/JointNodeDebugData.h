//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointNodeDebugData.generated.h"

/**
 * A data set for the debug data.
 * This can be used any sort of the debug related to the node. (though I can only come up with the breakpoint action yet, but can be extended.)
 * Basically, the graph will hold this data for each node, but it will not create FJointNodeDebugData instances for the nodes that doesn't need any kind of debug.
 *
 * The system will assume multiple FJointNodeDebugData with same Node property value as same instance for iteration and find actions and hashing. Please notice that.
 */

class UJointEdGraphNode;

USTRUCT()
struct JOINTEDITOR_API FJointNodeDebugData
{
	GENERATED_BODY()

public:

	/**
	 * The graph node reference that uses this debug data.
	 */
	UPROPERTY(EditAnywhere, Category="Debug")
	TObjectPtr<UJointEdGraphNode> Node = nullptr;
	
public:

	/**
	 * Whether the node has a breakpoint.
	 */
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bHasBreakpoint = false;

	/**
	 * Whether the node has a breakpoint.
	 */
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bIsBreakpointEnabled = false;

public:

	/**
	 * Whether the node will not be played on this playback.
	 */
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bDisabled = false;

public:

	/**
	 * Test if this data still need to be stored or not.
	 * It checks whether there is any property for the debug still true so need to keep this instance.
	 */
	bool CheckWhetherNecessary() const;

public:
	
	//Operator overload for the hashing.

	//The system will assume multiple FJointNodeDebugData with same Node property value as same instance for iteration and find actions and hashing. Please notice that.
	bool operator==(const FJointNodeDebugData& other) const;
	
};

inline bool FJointNodeDebugData::operator==(const FJointNodeDebugData& OtherData) const {
	return Node == OtherData.Node;
}

