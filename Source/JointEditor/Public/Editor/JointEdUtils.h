//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "ClassViewerFilter.h"
#include "AIGraphTypes.h"
#include "JointManager.h"
#include "STextPropertyEditableTextBox.h"
#include "SharedType/JointSharedTypes.h"

class UJointEdGraphNode;

class JOINTEDITOR_API FJointEdUtils
{
public:
	//Get the editor node's subclasses of the provided class on the ClassCache of the engine instance.
	static void GetEditorNodeSubClasses(const UClass* BaseClass, TArray<FGraphNodeClassData>& ClassData);

	//Get the node's subclasses of the provided class on the ClassCache of the engine instance.
	static void GetNodeSubClasses(const UClass* BaseClass, TArray<FGraphNodeClassData>& ClassData);

	//Find and return the first EditorGraphNode for the provided Joint node class. If there is no class for the node, returns nullptr;
	static TSubclassOf<UJointEdGraphNode> FindEdClassForNode(FGraphNodeClassData Class);
	

	template <typename Type>
	class JOINTEDITOR_API FNewNodeClassFilter : public IClassViewerFilter
	{
	public:
		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass,TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override;
		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions,const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override;
	};

	class JOINTEDITOR_API FJointAssetFilter : public IClassViewerFilter
	{
	public:
		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass,TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override;
		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions,const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override;
	};

	class JOINTEDITOR_API FJointFragmentFilter : public IClassViewerFilter
	{
	public:
		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass,TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override;
		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions,const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override;
	};

	class JOINTEDITOR_API FJointNodeFilter : public IClassViewerFilter
	{
		public:
		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass,TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override;
		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions,const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override;
	};

public:
	

	/**
	 * StaticStableTextId implementation for the system.
	 * It handles some localization error correction like generating init keys for the text while trying its best to keep it stable with the old value.
	 * And it lets the engine knows that we are holding this text asset in the provided package.
	 * This will help you to quickly construct a new text source for the project.
	 * @param InPackage 
	 * @param InEditAction 
	 * @param InTextSource 
	 * @param InProposedNamespace 
	 * @param InProposedKey 
	 * @param OutStableNamespace 
	 * @param OutStableKey 
	 */
	static void JointText_StaticStableTextId(UPackage* InPackage, const IEditableTextProperty::ETextPropertyEditAction InEditAction, const FString& InTextSource, const FString& InProposedNamespace, const FString& InProposedKey, FString& OutStableNamespace, FString& OutStableKey);

	static void JointText_StaticStableTextIdWithObj(UObject* InObject, const IEditableTextProperty::ETextPropertyEditAction InEditAction, const FString& InTextSource, const FString& InProposedNamespace, const FString& InProposedKey, FString& OutStableNamespace, FString& OutStableKey);


public:

	/**
	 * Open editor for the provided Joint manager.
	 * @param Manager 
	 * @param Toolkit 
	 */
	static void OpenEditorFor(UJointManager* Manager, class FJointEditorToolkit*& Toolkit);

public:

	
	/**
	 * Get blueprint class with the name.
	 * @param ClassName The name of the blueprint class asset on the content browser.
	 * @return Found blueprint class.
	 */
	static UClass* GetBlueprintClassWithClassPackageName(const FName& ClassName);

	template<typename PropertyType = FProperty>
	static PropertyType* GetCastedPropertyFromClass(const UClass* Class, const FName& PropertyName);

	static FText GetFriendlyNameOfNode(const UJointNodeBase* Node);

	static FText GetFriendlyNameFromClass(TSubclassOf<UObject> Class);

	static EMessageSeverity::Type ResolveJointEdMessageSeverityToEMessageSeverity(const EJointEdMessageSeverity::Type JointEdLogMessage);

};

