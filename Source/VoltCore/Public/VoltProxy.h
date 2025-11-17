//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoltInterface.h"
#include "VoltVariableCollection.h"
#include "VoltProxy.generated.h"

/**
 * A proxy object that wraps a slate reference and a variable collection to apply to the slate.
 * You can use this object to animate a slate that is not derived from IVoltInterface.
 */

UCLASS()
class VOLTCORE_API UVoltProxy : public UObject, public IVoltInterface
{

	GENERATED_BODY()
	
public:
	
	UVoltProxy()
	{
		VariableCollection = CreateDefaultSubobject<UVoltVariableCollection>(TEXT("VariableCollection"));
	};
	
	virtual ~UVoltProxy() override = default;

public:

	virtual UVoltVariableCollection* GetVoltVariableCollection() override
	{
		return VariableCollection;
	}

	virtual TWeakPtr<SWidget> GetTargetSlate() override
	{
		return Widget;
	}

public:

	UPROPERTY(Transient)
	TObjectPtr<UVoltVariableCollection> VariableCollection;

	TWeakPtr<SWidget> Widget;

};

