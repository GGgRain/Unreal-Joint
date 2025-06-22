//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Node/Derived/JN_Foundation.h"

UJN_Foundation::UJN_Foundation()
{
}

TArray<UJointNodeBase*> UJN_Foundation::SelectNextNodes_Implementation(AJointActor* InHostingJointInstance)
{
	TArray<UJointNodeBase*> Nodes = Super::SelectNextNodes_Implementation(InHostingJointInstance);

	if(!Nodes.IsEmpty()) return Nodes;
	
	return NextNode;
}

bool UJN_Foundation::IsSupportedForNetworking() const
{
	return true; //For function call spaces, but it should not be used with other functionalities.
}
