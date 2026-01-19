//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "JointEdGraphNode_Foundation.h"

#include "JointFunctionLibrary.h"
#include "Node/Derived/JN_Foundation.h"


#define LOCTEXT_NAMESPACE "UJointEdFragment_Foundation"

UJointEdGraphNode_Foundation::UJointEdGraphNode_Foundation()
{
}

TSubclassOf<UJointNodeBase> UJointEdGraphNode_Foundation::SupportedNodeClass()
{
	return UJN_Foundation::StaticClass();
}

void UJointEdGraphNode_Foundation::ReallocatePins()
{
	if (!GetPinsFromSubNodes().IsEmpty())
	{

		TArray<FJointEdPinData> NewPinData;
		NewPinData.Add(FJointEdPinData("In", EEdGraphPinDirection::EGPD_Input));
		
		if (bMakeOutputPinAlways)
		{
			NewPinData.Add(FJointEdPinData("Out", EEdGraphPinDirection::EGPD_Output));
		}
		
		PinData = UJointFunctionLibrary::ImplementPins(PinData, NewPinData);
		
		// purge the next node if the pin is hidden.
		if (!bMakeOutputPinAlways)
		{
			if (GetCastedNodeInstance<UJN_Foundation>()) GetCastedNodeInstance<UJN_Foundation>()->NextNode.Empty();
		}
	}
	else
	{
		TArray<FJointEdPinData> NewPinData;
		NewPinData.Add(FJointEdPinData("In", EEdGraphPinDirection::EGPD_Input));
		NewPinData.Add(FJointEdPinData("Out", EEdGraphPinDirection::EGPD_Output));
		PinData = UJointFunctionLibrary::ImplementPins(PinData, NewPinData);
	}
}

void UJointEdGraphNode_Foundation::AllocateDefaultPins()
{
	PinData.Empty();
	PinData.Add(FJointEdPinData("In", EEdGraphPinDirection::EGPD_Input));
	PinData.Add(FJointEdPinData("Out", EEdGraphPinDirection::EGPD_Output));
}

void UJointEdGraphNode_Foundation::NodeConnectionListChanged()
{
	Super::NodeConnectionListChanged();

	UJN_Foundation* CastedNode = GetCastedNodeInstance<UJN_Foundation>();

	if (CastedNode == nullptr) return;

	CastedNode->NextNode.Empty();

	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin == nullptr) continue;

		if (Pin->Direction == EEdGraphPinDirection::EGPD_Input) continue;

		if(CheckPinIsOriginatedFromThis(Pin))
		{
			for ( UEdGraphPin* LinkedTo : Pin->LinkedTo)
			{
				if (UJointEdGraphNode* LinkedPinOwner = CastPinOwnerToJointEdGraphNode(LinkedTo))
				{
					LinkedPinOwner->AllocateReferringNodeInstancesOnConnection(CastedNode->NextNode, LinkedTo);
				}
			}
		}
	}
}

void UJointEdGraphNode_Foundation::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UpdatePins();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#undef LOCTEXT_NAMESPACE
