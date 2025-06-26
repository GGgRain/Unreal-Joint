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
	if (!GetAllPinsFromChildren().IsEmpty())
	{

		TArray<FJointEdPinData> NewPinData;
		NewPinData.Add(FJointEdPinData("In", EEdGraphPinDirection::EGPD_Input));
		PinData = UJointFunctionLibrary::ImplementPins(PinData, NewPinData);
		
		if (GetCastedNodeInstance<UJN_Foundation>()) GetCastedNodeInstance<UJN_Foundation>()->NextNode.Empty();
	}
	else
	{
		bool bShouldAdd = true;

		const int Num = PinData.Num();
		for (int i = 0; i < Num; ++i)
		{
			if (PinData[i].PinName == "Out")
			{
				bShouldAdd = false;
				break;
			}
		}

		if (bShouldAdd) {
			TArray<FJointEdPinData> NewPinData;
			NewPinData.Add(FJointEdPinData("In", EEdGraphPinDirection::EGPD_Input));
			NewPinData.Add(FJointEdPinData("Out", EEdGraphPinDirection::EGPD_Output));
			PinData = UJointFunctionLibrary::ImplementPins(PinData, NewPinData);
		}
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

		if(CheckProvidedPinIsOnPinData(Pin))
		{
			for (const UEdGraphPin* LinkedTo : Pin->LinkedTo)
			{
				if (LinkedTo == nullptr) continue;

				if (LinkedTo->GetOwningNode() == nullptr) continue;

				UEdGraphNode* ConnectedNode = LinkedTo->GetOwningNode();

				if (!ConnectedNode) continue;

				UJointEdGraphNode* CastedGraphNode = Cast<UJointEdGraphNode>(ConnectedNode);

				if (!CastedGraphNode) continue;

				CastedGraphNode->AllocateReferringNodeInstancesOnConnection(CastedNode->NextNode);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
