//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoltModuleItem.h"
#include "Templates/SubclassOf.h"
#include "UObject/Interface.h"
#include "VoltSubModuleInterface.generated.h"

UINTERFACE(Blueprintable)
class VOLTCORE_API UVoltSubModuleInterface : public UInterface
{
	GENERATED_BODY()
};


/**
 * An interface for the objects that need any kind of sub-module style hierarchy.
 * Deriving this interface is required when you want to add/remove/find/findOrAdd with declarative syntax.
 * If your module doesn't need any additional hierarchical implementation with sub-modules, You can ignore it.
 * * See UVolt_ASM_Sequence, UVoltAnimation.
 */

/**
	You can use VOLT_SUBMODULE_CONTAINER_ARGUMENT( SubModulesArgumentName ) macro between VOLT_MODULE_BEGIN_ARGS(), VOLT_MODULE_END_ARGS() macro to declare an argument that will grab submodules in the declarative animation making.



	
	For example, See how UVolt_ASM_Sequence declares its arguments on the header:
	
	```
    VOLT_MODULE_BEGIN_ARGS(UVolt_ASM_Sequence) :
		_bShouldLoop(true)
	{}
	VOLT_MODULE_ARGUMENT( bool, bShouldLoop )
	VOLT_SUBMODULE_CONTAINER_ARGUMENT(SubModules) //It let you use () operator on the declaration to grab other modules and pass them into the InArgs in Construct().
	VOLT_MODULE_END_ARGS()
	```



	You still need to declare actual property that will have submodules for your module or another object type, and also have to override some functions of the interface.
	Luckily, We got you! You don't need to override all the functions by yourself. Just use VOLT_DECLARE_SUBMODULE_FUNCTIONS( ModulesContainerPropertyName ) instead!
	
	See how UVolt_ASM_Sequence declares its property and functions on the header:

	```
	public:
		
		UPROPERTY(Instanced, BlueprintReadWrite, Category="Animation")
			TArray<UVoltModuleItem*> Modules; 
		
		VOLT_DECLARE_SUBMODULE_FUNCTIONS(Modules)
	```




	Those actions allow you to do like this in the declaration:
	
	```
	UVoltAnimation* ColorAnimation = VOLT_MAKE_ANIMATION()
		(
			VOLT_MAKE_MODULE(UVolt_ASM_Sequence)
			.bShouldLoop(true)
			(
				VOLT_MAKE_MODULE(UVolt_ASM_InterpColor)
				.InterpSpeed(42)
				.TargetColor(FLinearColor(1, 1, 1, 0.5)),
				VOLT_MAKE_MODULE(UVolt_ASM_InterpColor)
				.InterpSpeed(24)
				.TargetColor(FLinearColor(1, 1, 1, 0))
			)
		);
	```
 */


class VOLTCORE_API IVoltSubModuleInterface
{
	GENERATED_BODY()

public:

	IVoltSubModuleInterface() {}

public:

	//Blueprint support

	/**
	 * Check and return whether the module is already in the container.
	 * @param InModule The module to check
	 * @return whether the module is already in the container
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Sub Module Action", DisplayName="Has Module")
	bool K2_HasModule(UVoltModuleItem* InModule);

	/**
	 * Check and return whether any module with specified class is already in the container.
	 * @param InModuleClass module class to find
	 * @return whether the module is already in the container
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Sub Module Action", DisplayName="Has Module For Class")
	bool K2_HasModuleForClass(TSubclassOf<UVoltModuleItem> InModuleClass);
	
	/**
	 * Get the module at the index
	 * @param Index 
	 * @return retrieved module. nullptr if any doesn't present
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Sub Module Action", DisplayName="Get Module At")
	UVoltModuleItem* K2_GetModuleAt(const int& Index);

	/**
	 * Get the module with specific class. 
	 * @param InModuleClass Class to search for
	 * @return retrieved module. nullptr if any doesn't present
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Sub Module Action", DisplayName="Get Module For Class")
	UVoltModuleItem* K2_GetModuleForClass(TSubclassOf<UVoltModuleItem> InModuleClass);

	/**
	 * Add the module to the container. This action will fail if the module is already in the container.
	 * @param InModule Module to add
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Sub Module Action", DisplayName="Add Module")
	void K2_AddModule(UVoltModuleItem* InModule);

	/**
	 * Insert the module to the container at the specified location. This action will fail if the module is already in the container.
	 * @param InModule Module to add
	 * @param Index location to insert
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Sub Module Action", DisplayName="Insert Module At")
	void K2_InsertModuleAt(UVoltModuleItem* InModule, const int& Index);
	
	/**
	 * Remove the module from the container.
	 * @param InModule Module to remove
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Sub Module Action", DisplayName="Remove Module")
	void K2_RemoveModule(UVoltModuleItem* InModule);

	/**
	 * Remove all modules from the container for the specified class.
	 * @param InModuleClass Module to remove
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Sub Module Action", DisplayName="Remove Module For Class")
	void K2_RemoveAllModuleForClass(TSubclassOf<UVoltModuleItem> InModuleClass);
	
	/**
	 * Remove the module at the specified location of the container. This will fail if the index isn't valid.
	 * @param InModule Module to remove
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Sub Module Action", DisplayName="Remove Module At")
	void K2_RemoveModuleAt(const int& Index);
	
private:

	virtual bool K2_HasModule_Implementation(UVoltModuleItem* InModule);

	virtual bool K2_HasModuleForClass_Implementation(TSubclassOf<UVoltModuleItem> InModuleClass);
	
	virtual UVoltModuleItem* K2_GetModuleAt_Implementation(const int& Index);

	virtual UVoltModuleItem* K2_GetModuleForClass_Implementation(TSubclassOf<UVoltModuleItem> InModuleClass);
	
	virtual void K2_AddModule_Implementation(UVoltModuleItem* InModule);

	virtual void K2_InsertModuleAt_Implementation(UVoltModuleItem* InModule, const int& Index);
	
	virtual void K2_RemoveModule_Implementation(UVoltModuleItem* InModule);

	virtual void K2_RemoveAllModuleForClass_Implementation(TSubclassOf<UVoltModuleItem> InModuleClass);

	virtual void K2_RemoveModuleAt_Implementation(const int& Index);

public:

	/**
	 * Check and return whether the module is already in the container.
	 * @param InModule The module to check
	 * @return whether the module is already in the container
	 */
	virtual bool HasModule(UVoltModuleItem* InModule);

	/**
	 * Check and return whether the module is already in the container.
	 * @param InModuleClass Class to search for
	 * @return whether the module is already in the container
	 */
	virtual bool HasModuleForClass(TSubclassOf<UVoltModuleItem> InModuleClass);

	/**
	 * Get the module at the index
	 * @param Index 
	 * @return retrieved module. nullptr if any doesn't present
	 */
	virtual UVoltModuleItem* GetModuleAt(const int& Index);
	
	/**
	 * Get the module with specific class. 
	 * @param InModuleClass Class to search for
	 * @return retrieved module. nullptr if any doesn't present
	 */
	virtual UVoltModuleItem* GetModuleForClass(TSubclassOf<UVoltModuleItem> InModuleClass);

	/**
	 * Add the module to the container. This action will fail if the module is already in the container.
	 * @param InModule Module to add
	 */
	virtual void AddModule(UVoltModuleItem* InModule);
	
	/**
	 * Insert the module to the container at the specified location. This action will fail if the module is already in the container.
	 * @param InModule Module to add
	 * @param Index location to insert
	 */
	virtual void InsertModuleAt(UVoltModuleItem* InModule, const int& Index);
		
	/**
	 * Remove the module from the container.
	 * @param InModule Module to remove
	 */
	virtual void RemoveModule(UVoltModuleItem* InModule);

	/**
	 * Remove all modules from the container.
	 * @param InModuleClass Class to search for
	 */
	virtual void RemoveAllModuleForClass(TSubclassOf<UVoltModuleItem> InModuleClass);
	
	/**
	 * Remove the module at the specified location of the container. This will fail if the index isn't valid.
	 * @param Index location to remove
	 */
	virtual void RemoveModuleAt(const int& Index);
	

public:

	//Template functions

	/**
	 * Get the module with specific class. 
	 * @return retrieved module. nullptr if any doesn't present
	 */
	template<typename AnimClass>
	bool HasModuleForClass();

	/**
	 * Get the module with specific class. 
	 * @return retrieved module. nullptr if any doesn't present
	 */
	template<typename AnimClass>
	AnimClass* GetModuleForClass();

	/**
	 * Get the module at the index
	 * @return retrieved module. nullptr if any doesn't present
	 */
	template<typename AnimClass>
	AnimClass* GetModuleAt(const int& Index);

	/**
	 * Remove all modules from the container for the specified class.
	 * @param InModuleClass Module to remove
	 */
	template<typename AnimClass>
	void RemoveAllModuleForClass(TSubclassOf<UVoltModuleItem> InModuleClass);

public:
	
	virtual TArray<TObjectPtr<UVoltModuleItem>>* GetModuleContainer() { return nullptr; }
	
};


template <typename AnimClass>
bool IVoltSubModuleInterface::HasModuleForClass()
{
	return HasModuleForClass(AnimClass::StaticClass());
}

template <typename AnimClass>
AnimClass* IVoltSubModuleInterface::GetModuleForClass()
{
	UVoltModuleItem* Module = GetModuleForClass(AnimClass::StaticClass());
	
	return Module ? Cast<AnimClass>(Module) : nullptr;
}

template <typename AnimClass>
AnimClass* IVoltSubModuleInterface::GetModuleAt(const int& Index)
{
	UVoltModuleItem* Module = GetModuleAt(Index);
	
	return Module ? Cast<AnimClass>(Module) : nullptr;
}

template <typename AnimClass>
void IVoltSubModuleInterface::RemoveAllModuleForClass(TSubclassOf<UVoltModuleItem> InModuleClass)
{
	RemoveAllModuleForClass(AnimClass::StaticClass());
}


/**
 * If you want to make your object support children volt modules, then you have to declare properties and call macro such as this:
 *
 * UPROPERTY(Instanced, BlueprintReadWrite, Category="Animation")
 *		TArray<UVoltModuleItem*> Modules; 
 *
 * VOLT_DECLARE_SUBMODULE_FUNCTIONS(Modules)
 *
 * @See UVoltAnimation, UVolt_ASM_Sequence
 */ 
	

/**
 * Declare basic functions that work with IVoltSubModuleInterface. All of the functions support BP access as well.
 * @param SubModuleArrayPropertyName The name of the TArray property that contains sub-modules.
 */
#define VOLT_DECLARE_SUBMODULE_FUNCTIONS( SubModuleArrayPropertyName )\
	virtual TArray<TObjectPtr<UVoltModuleItem>>* GetModuleContainer() override { return &SubModuleArrayPropertyName; }