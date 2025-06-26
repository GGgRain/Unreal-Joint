//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VoltModuleItem.generated.h"




class IVoltInterface;
class SWidget;

/*
 * A base object class that handles modification of variables as intended.
 * It is inspired by the niagara stack modules' structure and designed to perform as same as that.
 * Override this class to define custom actions for the variable modification.
 */
UCLASS(EditInlineNew, Blueprintable, BlueprintType, Abstract)
class VOLTCORE_API UVoltModuleItem : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * Modify the slate's variable. This function will be triggered every frame. (equivalent of Tick())
	 * @param DeltaTime Delta time from the last frame
	 * @param Volt Volt interface to animated
	 */
	UFUNCTION(BlueprintCallable, Category="Module Action")
	virtual void ModifySlateVariable(const float DeltaTime, const TScriptInterface<IVoltInterface>& Volt);

	UFUNCTION(BlueprintImplementableEvent, Category="Module Action", meta = (DisplayName = "Modify Slate Variable"))
	void K2_ModifySlateVariable(const float DeltaTime, const TScriptInterface<IVoltInterface>& Volt);

public:
	
	/**
	 * Get volt slate that this module is modifying.
	 * @return get the volt slate (volt interface) that this module is affecting.
	 */
	UFUNCTION(BlueprintCallable, Category="Module Action")
	const TScriptInterface<IVoltInterface>& GetVoltSlate();

	/**
	 * Set volt slate that this module is modifying. 
	 * @param Slate the volt slate (volt interface) that this module will affect.
	 */
	UFUNCTION(BlueprintCallable, Category="Module Action")
	void SetVoltSlate(const TScriptInterface<IVoltInterface>& Slate);

protected:

	/**
	 * Set the submodules' volt slate to the provided slate. It will only work when this module is derived from IVoltSubModuleInterface, and property provided module container for the instance.
	 * The function SetVoltSlate also call this as well.
	 * @param Slate the volt slate (volt interface) that this module will affect.
	 */
	void SetSubModulesVoltSlate(const TScriptInterface<IVoltInterface>& Slate);

public:
	
	UPROPERTY(Transient)
	TScriptInterface<IVoltInterface> TargetVoltSlate;
	
	//transient
	TWeakPtr<SWidget> TargetSlate;

public:

	/**
	 * Reload the node to make it able to be played again.
	 */
	void ReloadModule();

	/**
	 * initialize the module and boot up. 
	 */
	virtual void BeginPlayModule();

	/**
	 * Force the module to end.
	 */
	virtual void EndPlayModule();

protected:

	//Only events - No, you can't have a delegate on here! that's a huge overkill!
	
	UFUNCTION(BlueprintNativeEvent, Category = "Module")
	void OnModuleBeginPlay();
	
	virtual void OnModuleBeginPlay_Implementation();
	
	UFUNCTION(BlueprintNativeEvent, Category = "Module")
	void OnModuleEndPlay();

	virtual void OnModuleEndPlay_Implementation();

private:
	
	UPROPERTY(BlueprintGetter=IsBegunPlay, Category = "Data", Transient)
	bool bIsModuleBegunPlay = false;
	
	UPROPERTY(BlueprintGetter=IsEndedPlay, Category = "Data", Transient)
	bool bIsModuleEndedPlay = false;
	
public:
	
	UFUNCTION(BlueprintPure, Category = "Module")
	bool IsBegunPlay() const;

	UFUNCTION(BlueprintPure, Category = "Module")
	bool IsEndedPlay() const;

public:
	
	/**
	 * Whether the module should continue to work and modification of the slate variables.
	 * @return false when this nodule's action is finished.
	 */
	UFUNCTION(BlueprintCallable, Category="Module Life Cycle")
	virtual bool IsActive();

	//Return false when this nodule's action is finished.
	UFUNCTION(BlueprintImplementableEvent, Category="Module Action", meta = (DisplayName = "Is Active"))
	bool K2_IsActive();
	
};
