//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "ClassViewerFilter.h"
#include "EdGraphSchema_K2_Actions.h"
#include "Editor/Graph/JointEdGraph.h"
#include "JointManager.h"
#include "STextPropertyEditableTextBox.h"
#include "Editor/SharedType/JointEditorSharedTypes.h"
#include "SharedType/JointSharedTypes.h"

class UJointEdGraphNode;

class JOINTEDITOR_API FJointEdUtils
{
public:
	//Get the editor node's subclasses of the provided class on the ClassCache of the engine instance.
	static void GetEditorNodeSubClasses(const UClass* BaseClass, TArray<FJointGraphNodeClassData>& ClassData);

	//Get the node's subclasses of the provided class on the ClassCache of the engine instance.
	static void GetNodeSubClasses(const UClass* BaseClass, TArray<FJointGraphNodeClassData>& ClassData);

	//Find and return the first EditorGraphNode for the provided Joint node class. If there is no class for the node, returns nullptr;
	static TSubclassOf<UJointEdGraphNode> FindEdClassForNode(FJointGraphNodeClassData Class);
	

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

	static void HandleNewAssetActionClassPicked(FString BasePath,UClass* InClass);

public:

	/**
	 * Open editor for the provided Joint manager.
	 * @param Manager 
	 * @param Toolkit 
	 */
	static void OpenEditorFor(UJointManager* Manager, class FJointEditorToolkit*& Toolkit);

	/**
	 * Find or add Joint Editor Toolkit for the provided asset or object.
	 * @param ObjectRelatedTo The joint manager related object.
	 * @param bOpenIfNotPresent If not present, open up.
	 * @return Found toolkit for the provided asset.
	 */
	static FJointEditorToolkit* FindOrOpenJointEditorInstanceFor(UObject* ObjectRelatedTo, const bool& bOpenIfNotPresent = true, const bool& bFocusIfOpen = true);

public:

	/**
	 * Find graph for the provided node instance. Not quite good for performance. Use it wisely.
	 * @param NodeInstance Provided node instance for the search action.
	 * @return Found graph instance
	 */
	static UJointEdGraph* FindGraphForNodeInstance(const UJointNodeBase* NodeInstance);

	/**
	 * Find a graph node for the provided node instance. Not quite good for performance. Use it wisely.
	 * @param NodeInstance Provided node instance for the search action.
	 * @return Found graph node instance
	 */
	static UEdGraphNode* FindGraphNodeForNodeInstance(const UJointNodeBase* NodeInstance);

	/**
	 * Find the editor node that has a node instance with the provided guid. warning: it literally iterates through all the nodes on the Joint manager. Not quite good for performance. Use it wisely.
	 * @param NodeGuid Node instance guid to search with.
	 * @param JointManager Manager that will search with.
	 * @return Found editor node.
	 */
	static UJointEdGraphNode* FindGraphNodeWithProvidedNodeInstanceGuid(UJointManager* JointManager, const FGuid& NodeGuid);

public:
	
	/**
	 * Return the corresponding Joint graph node from the provided Joint graph node for the target Joint manager.
	 * @param SearchFor Key object to use
	 * @param TargetManager Target Joint manager to find the corresponding node for.
	 * @return Corresponding Joint Graph Node.
	 */
	static UJointEdGraphNode* GetCorrespondingJointGraphNodeForJointManager(UJointEdGraphNode* SearchFor, UJointManager* TargetManager);

public:
	
	/**
	 * Return the Joint manager asset from the provided Joint manager object instance (transient - copied one). If itself is an asset, then it will return itself.
	 * @param InJointManager Key object to use
	 * @return Original Joint Manager Asset.
	 */
	static UJointManager* GetOriginalJointManager(UJointManager* InJointManager);

	/**
	 * Return the Joint graph node from the provided Joint graph node from Joint manager object instance (transient - copied one). If itself is from an asset, then it will return itself.
	 * @param InJointEdGraphNode Key object to use
	 * @return Original Joint Graph Node.
	 */
	static UJointEdGraphNode* GetOriginalJointGraphNodeFromJointGraphNode(UJointEdGraphNode* InJointEdGraphNode);
	
	
	/**
	 * Return the Joint node from the provided Joint node from Joint manager object instance (transient - copied one). If itself is from an asset, then it will return itself.
	 * @param InJointNode Key object to use
	 * @return Original Joint Node.
	 */
	static UJointNodeBase* GetOriginalJointNodeFromJointNode(UJointNodeBase* InJointNode);

public:
	
	static void MarkNodesAsModifiedAndValidateName(TSet<UEdGraphNode*> InNodes);

	static void MoveNodesAtLocation(TSet<UEdGraphNode*> InNodes, const FVector2D& PasteLocation);

public:
	
	/**
	 * Get blueprint class with the name.
	 * @param ClassName The name of the blueprint class asset on the content browser.
	 * @return Found blueprint class.
	 */
	static UClass* GetBlueprintClassWithClassPackageName(const FName& ClassName);

	template<typename PropertyType = FProperty>
	static PropertyType* GetCastedPropertyFromClass(const UClass* Class, const FName& PropertyName);

	static FText GetFriendlyNameOfNode(const UJointEdGraphNode* Node);

	static FText GetFriendlyNameFromClass(TSubclassOf<UObject> Class);

	static EMessageSeverity::Type ResolveJointEdMessageSeverityToEMessageSeverity(const EJointEdMessageSeverity::Type JointEdLogMessage);

public:
	
	static void RemoveGraph(UJointEdGraph* GraphToRemove);

	static void RemoveNode(class UObject* NodeRemove);

	static void RemoveNodes(TArray<class UObject*> NodesToRemove);

public:
	
	/**
	 * Rename the provided object safely. It will check for the name conflict and rename it with a unique name if there is a conflict.
	 */
	static bool GetSafeNameForObjectRenaming(FString& InNewNameOutValidatedName, UObject* ObjectToRename, UObject* InOuter);

	static bool IsNameSafeForObjectRenaming(const FString& InName, UObject* ObjectToRename, UObject* InOuter, FText& OutErrorMessage);

	static bool GetSafeNameForObject(FString& InNewNameOutValidatedName, UObject* InOuter);

public:
	
	// Editor cosmetic related
	static void GetGraphIconForAction(FEdGraphSchemaAction_K2Graph const* const ActionIn, FSlateBrush const*& IconOut, FSlateColor& ColorOut, FText& ToolTipOut);
	static void GetGraphIconFor(const UEdGraph* Graph, FSlateBrush const*& IconOut);

public:

	//Templates

	template< class T > 
		static inline void GetAllNodesOfClass( const UJointManager* Manager, TArray<T*>& OutNodes )
	{
		TArray<UJointEdGraph*> AllGraphs;
		
		if (UEdGraph* Graph = Manager->GetJointGraphAs())
		{
			if (UJointEdGraph* JointGraph = Cast<UJointEdGraph>(Graph))
			{
				AllGraphs.Add(JointGraph);
				AllGraphs.Append(JointGraph->GetAllSubGraphsRecursively());
		
				for(int32 i=0; i< AllGraphs.Num(); i++)
				{
					check(AllGraphs[i] != NULL);
					TArray<T*> GraphNodes;
					AllGraphs[i]->GetNodesOfClass<T>(GraphNodes);
					OutNodes.Append(GraphNodes);
				}
			}
		}
		
	}
	
};

