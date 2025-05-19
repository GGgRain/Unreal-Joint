//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Widgets/SWidget.h"
#include "VoltInterface.generated.h"

class UVoltVariableCollection;

/**
 * Basic interface for the animated slate. it can be used both in non UCLASS, USTRUCT based classes and UObject based classes.
 * It must be used to represent a single slate.
 * Check out the UVoltProxy and Volt native slate types.
 */

UINTERFACE(Blueprintable)
class VOLTCORE_API UVoltInterface : public UInterface
{
	GENERATED_BODY()
};


class VOLTCORE_API IVoltInterface
{
	GENERATED_BODY()

public:

	IVoltInterface() {}

public:

	//Get the variable collection. It will trigger the native GetVoltVariableCollection() internally.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Slate Interface Action", DisplayName="Get Animated Slate Variable Collection")
	UVoltVariableCollection* K2_GetVoltVariableCollection();

	//Get the variable collection. It will trigger the native GetVoltVariableCollection() internally.
	virtual UVoltVariableCollection* K2_GetVoltVariableCollection_Implementation() { return GetVoltVariableCollection(); }

	//Get the variable collection.
	virtual UVoltVariableCollection* GetVoltVariableCollection() { return nullptr; }

public:
	
	//Get the slate reference.
	virtual TWeakPtr<SWidget> GetTargetSlate() { return nullptr; }
};



/**
 * Wrapper structure for hashing of the interface.
 * This is introduced for the hash table querying of the Volt Interfaces
 */
USTRUCT(BlueprintType)
struct FVoltInterfaceElement
{
	GENERATED_BODY()

public:

	FVoltInterfaceElement() {}
	FVoltInterfaceElement(const TScriptInterface<IVoltInterface>& InVoltInterface) : VoltInterface(InVoltInterface) {}
	
public:

	UPROPERTY(BlueprintReadWrite, Category="VoltInterface")
	TScriptInterface<IVoltInterface> VoltInterface;

public:

	FORCEINLINE const bool IsValid() const
	{
		return VoltInterface && VoltInterface->GetTargetSlate().IsValid();
	}
	
};

FORCEINLINE uint32 GetTypeHash(const FVoltInterfaceElement& Struct)
{
	return FCrc::MemCrc32(Struct.VoltInterface->GetTargetSlate().Pin().Get(), sizeof(SWidget));
}

FORCEINLINE bool operator==(const FVoltInterfaceElement& A, const FVoltInterfaceElement& B)
{
	return A.VoltInterface == B.VoltInterface;
}

FORCEINLINE bool operator!=(const FVoltInterfaceElement& A, const FVoltInterfaceElement& B)
{
	return !(A == B);
}
