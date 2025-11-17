//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

#include "UObject/Interface.h"

#include "EdGraph/EdGraphNode.h" // for ed pin structures.
#include "EdGraph/EdGraphPin.h" // for ed pin structures.

#include "JointEdSharedTypes.generated.h"

/**
 * This is the type of response the graph editor should take when making a connection
 * It has been introduced to support BP, because the original ECanCreateConnectionResponse is not exposed to BP and Editor only.
 */

UENUM(BlueprintType)
namespace EJointEdCanCreateConnectionResponse
{
	/** Make the connection; there are no issues (message string is displayed if not empty). */
	enum Type {
		CONNECT_RESPONSE_MAKE,

		/** Cannot make this connection; display the message string as an error. */
		CONNECT_RESPONSE_DISALLOW,

		/** Break all existing connections on A and make the new connection (it's exclusive); display the message string as a warning/notice. */
		CONNECT_RESPONSE_BREAK_OTHERS_A,

		/** Break all existing connections on B and make the new connection (it's exclusive); display the message string as a warning/notice. */
		CONNECT_RESPONSE_BREAK_OTHERS_B,

		/** Break all existing connections on A and B, and make the new connection (it's exclusive); display the message string as a warning/notice. */
		CONNECT_RESPONSE_BREAK_OTHERS_AB,

		/** Make the connection via an intermediate cast node, or some other conversion node. */
		CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE,

		/** Make the connection by promoting a lower type to a higher type. Ex: Connecting a Float -> Double, float should become a double */
		CONNECT_RESPONSE_MAKE_WITH_PROMOTION
	};
}

/**
 * This is a response from CanCreateConnection, indicating if the connecting action is legal and what the result will be
 * It has been introduced to support BP.
 */

USTRUCT(BlueprintType)
struct JOINT_API FJointEdPinConnectionResponse
{
public:
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Connection Response")
	FText Message;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Connection Response")
	TEnumAsByte<EJointEdCanCreateConnectionResponse::Type> Response;

public:
	FJointEdPinConnectionResponse()
		: Message(FText::GetEmpty())
		  , Response(EJointEdCanCreateConnectionResponse::CONNECT_RESPONSE_MAKE)
		  , bIsFatal(false)
	{
	}

	FJointEdPinConnectionResponse(const EJointEdCanCreateConnectionResponse::Type InResponse, FText InMessage)
		: Message(MoveTemp(InMessage))
		  , Response(InResponse)
		  , bIsFatal(false)
	{
	}

	/** If a connection can be made without breaking existing connections */
	bool CanSafeConnect() const
	{
		return (Response == EJointEdCanCreateConnectionResponse::CONNECT_RESPONSE_MAKE || Response ==
			EJointEdCanCreateConnectionResponse::CONNECT_RESPONSE_MAKE_WITH_PROMOTION);
	}

	bool IsFatal() const
	{
		return (Response == EJointEdCanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW) && bIsFatal;
	}

	void SetFatal()
	{
		Response = EJointEdCanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW;
		bIsFatal = true;
	}

private:
	bool bIsFatal = true;
};

FORCEINLINE bool operator==(const FJointEdPinConnectionResponse& A, const FJointEdPinConnectionResponse& B)
{
	return (A.Message.ToString() == B.Message.ToString()) && (A.Response == B.Response);
}
