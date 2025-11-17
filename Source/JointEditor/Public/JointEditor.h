//Copyright 2022~2024 DevGrain. All Rights Reserved.


#pragma once

#include "Editor/SharedType/JointEditorSharedTypes.h"

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Modules/ModuleInterface.h"


class FJointManagementTabHandler;
class FJointEditorToolkit;
class UJointDebugger;
class FJointGraphPinSlateFactory;
struct FJointGraphNodeSlateFactory;

class IAssetTypeActions;
class IAssetTools;

namespace JointToolTabNames
{
	static const FName JointManagementTab("JointManagement");
	static const FName JointBulkSearchReplaceTab("JointBulkSearchReplace");
	static const FName JointCompilerTab("JointCompiler");
}

class JOINTEDITOR_API FJointEditorModule : 
	public IHasMenuExtensibility, 
	public IHasToolBarExtensibility, 
	public IModuleInterface
{
public:
	
	//~ IHasMenuExtensibility interface

	virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager() override;

public:

	//~ IHasToolBarExtensibility interface

	virtual TSharedPtr<FExtensibilityManager> GetToolBarExtensibilityManager() override;

public:

	//~ IModuleInterface interface

	virtual void StartupModule() override;


	virtual void ShutdownModule() override;

	virtual bool SupportsDynamicReloading() override;

protected:

	/** Registers asset tool actions. */
	void RegisterAssetTools();

	void RegisterSequencerTrack();

	/**
	 * Registers a single asset type action.
	 *
	 * @param AssetTools The asset tools object to register with.
	 * @param Action The asset type action to register.
	 */
	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action);

	void RegisterModuleCommand();

protected:

	void AppendActiveJointRedirects();

public:
	
	static void OpenJointManagementTab();
	static void OpenJointBulkSearchReplaceTab();
	static void OpenJointCompilerTab();
	
	TSharedRef<SDockTab> OnSpawnJointManagementTab(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnSpawnJointBulkSearchReplaceTab(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> OnSpawnJointCompilerTab(const FSpawnTabArgs& SpawnTabArgs);

private:
	
	/** Unregisters asset tool actions. */
	void UnregisterAssetTools();

	void RegisterClassLayout();
	
	void UnregisterSequencerTrack();
	
	/** Unregisters asset tool actions. */
	void UnregisterClassLayout();

public:

	//Collect and store class editor graph node cache and node cache. This function will be called in the FJointManagerActions::OpenAssetEditor for the snyc loading.
	void StoreClassCaches();
	
	//Flush editor graph node cache and node cache.
	void FlushClassCaches();

protected:

	/** Registers main menu and tool bar menu extensions. */
	void RegisterMenuExtensions();

	/** Unregisters main menu and tool bar menu extensions. */
	void UnregisterMenuExtensions();
	
public:

	/** Registers debugger. */
	void RegisterDebugger();

	/** Unregisters debugger. */
	void UnregisterDebugger();

public:

	FDelegateHandle JointNativeMovieTrackCreateEditorHandle;

private:

	/** Holds the menu extensibility manager. */
	TSharedPtr<FExtensibilityManager> MenuExtensibilityManager;

	/** The collection of registered asset type actions. */
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;

	/** Holds the tool bar extensibility manager. */
	TSharedPtr<FExtensibilityManager> ToolBarExtensibilityManager;

	/** Holds the tool bar extensibility manager. */
	TSharedPtr<FJointGraphNodeSlateFactory> JointNodeStyleFactory;

	/** Holds the tool bar extensibility manager. */
	TSharedPtr<FJointGraphPinSlateFactory> JointGraphPinFactory;

public:
	
	/** 
	 * Holds the FJointManagementTabHandler manager.
	 * Access this when you want to attach your own tab for Joint Management tab.
	 */
	TSharedPtr<FJointManagementTabHandler> JointManagementTabHandler;

public:
	
	TSharedPtr<FJointGraphNodeClassHelper> GetEdClassCache();

	TSharedPtr<FJointGraphNodeClassHelper> GetClassCache();

private:

	TSharedPtr<FJointGraphNodeClassHelper> EdClassCache;
	
	TSharedPtr<FJointGraphNodeClassHelper> ClassCache;

public:
	
	/**
	 * The Joint debugger object for the editor module.
	 * A single Joint debugger will handle all the actions related to the debugging from all Joint instance on the level.
	 *
	 * This object is a singleton object, you can also access to it by UJointDebugger::Get();
	 */
	TObjectPtr<UJointDebugger> JointDebugger;

public:

	static FJointEditorModule* Get();


};

//IMPLEMENT_MODULE(FJointEditorModule, JointEditor)

