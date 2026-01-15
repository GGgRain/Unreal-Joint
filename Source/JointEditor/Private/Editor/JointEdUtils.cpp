//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointEdUtils.h"

#include "AssetToolsModule.h"
#include "EdGraphSchema_K2_Actions.h"
#include "JointActor.h"
#include "JointEdGraph.h"
#include "JointEdGraphNode_Fragment.h"
#include "JointEdGraphNode_Reroute.h"
#include "JointEdGraphSchema.h"
#include "JointEditorToolkit.h"
#include "Modules/ModuleManager.h"

#include "JointEditor.h"
#include "JointEditorNameValidator.h"
#include "JointEditorSettings.h"
#include "JointEditorStyle.h"
#include "JointFunctionLibrary.h"
#include "SNodePanel.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Internationalization/TextPackageNamespaceUtil.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Node/JointFragment.h"
#include "Node/JointNodeBase.h"
#include "Serialization/TextReferenceCollector.h"

#include "Misc/EngineVersionComparison.h"


#define LOCTEXT_NAMESPACE "JointEdUtils"

class FJointEditorModule;

void FJointEdUtils::GetEditorNodeSubClasses(const UClass* BaseClass, TArray<FJointGraphNodeClassData>& ClassData)
{
	FJointEditorModule& EditorModule = FModuleManager::GetModuleChecked<
		FJointEditorModule>(TEXT("JointEditor"));
	FJointGraphNodeClassHelper* ClassCache = EditorModule.GetEdClassCache().Get();

	if(ClassCache)
	{
		ClassCache->GatherClasses(BaseClass, ClassData);
	}
}

void FJointEdUtils::GetNodeSubClasses(const UClass* BaseClass, TArray<FJointGraphNodeClassData>& ClassData)
{
	FJointEditorModule& EditorModule = FModuleManager::GetModuleChecked<
		FJointEditorModule>(TEXT("JointEditor"));

	FJointGraphNodeClassHelper* ClassCache = EditorModule.GetClassCache().Get();

	if(ClassCache)
	{
		ClassCache->GatherClasses(BaseClass, ClassData);
	}
}

TSubclassOf<UJointEdGraphNode> FJointEdUtils::FindEdClassForNode(FJointGraphNodeClassData Class)
{
	TArray<FJointGraphNodeClassData> ClassData;

	FJointEdUtils::GetEditorNodeSubClasses(UJointEdGraphNode::StaticClass(), ClassData);

	for (FJointGraphNodeClassData& SubclassData : ClassData)
	{
		TSubclassOf<UJointEdGraphNode> EdNodeClass = SubclassData.GetClass();

		if (EdNodeClass == nullptr) continue;

		if (EdNodeClass->GetDefaultObject<UJointEdGraphNode>()->SupportedNodeClass() != Class.GetClass()) continue;

		return EdNodeClass;
	}

	return nullptr;
}

template <typename Type>
bool FJointEdUtils::FNewNodeClassFilter<Type>::IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions,
                                                              const UClass* InClass,
                                                              TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
{
	if (InClass != nullptr) { return InClass->IsChildOf(Type::StaticClass()); }
	return false;
}

template <typename Type>
bool FJointEdUtils::FNewNodeClassFilter<Type>::IsUnloadedClassAllowed(
	const FClassViewerInitializationOptions& InInitOptions,
	const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData,
	TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
{
	return InUnloadedClassData->IsChildOf(Type::StaticClass()) && !InUnloadedClassData->HasAnyClassFlags(CLASS_Abstract);
}

bool FJointEdUtils::FJointAssetFilter::IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions,
                                                      const UClass* InClass, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
{
	if (InClass != nullptr) { 
		return InClass == UJointFragment::StaticClass();
	}
	return false;
}


bool FJointEdUtils::FJointAssetFilter::IsUnloadedClassAllowed(
	const FClassViewerInitializationOptions& InInitOptions,
	const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData,
	TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
{
	return InUnloadedClassData->IsA(UJointFragment::StaticClass());
}

inline bool FJointEdUtils::FJointFragmentFilter::IsClassAllowed(
	const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass,
	TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
{
	if (InClass != nullptr) { 
		return InClass == UJointFragment::StaticClass();
	}
	return false;
}

inline bool FJointEdUtils::FJointFragmentFilter::IsUnloadedClassAllowed(
	const FClassViewerInitializationOptions& InInitOptions,
	const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData,
	TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
{
	return InUnloadedClassData->IsA(UJointFragment::StaticClass());
}

inline bool FJointEdUtils::FJointNodeFilter::IsClassAllowed(
	const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass,
	TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
{
	if (InClass != nullptr)
	{
		return InClass->IsChildOf(UJointNodeBase::StaticClass()) && !InClass->HasAnyClassFlags(CLASS_Abstract);
	}
	return false;
}





inline bool FJointEdUtils::FJointNodeFilter::IsUnloadedClassAllowed(
	const FClassViewerInitializationOptions& InInitOptions,
	const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData,
	TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
{
	return InUnloadedClassData->IsChildOf(UJointNodeBase::StaticClass()) && !InUnloadedClassData->HasAnyClassFlags(CLASS_Abstract);
}

void FJointEdUtils::JointText_StaticStableTextId(UPackage* InPackage,
                                                 const IEditableTextProperty::ETextPropertyEditAction InEditAction, const FString& InTextSource,
                                                 const FString& InProposedNamespace, const FString& InProposedKey, FString& OutStableNamespace,
                                                 FString& OutStableKey)
{
	bool bPersistKey = false;

	const FString PackageNamespace = TextNamespaceUtil::EnsurePackageNamespace(InPackage);
	if (!PackageNamespace.IsEmpty())
	{
		// Make sure the proposed namespace is using the correct namespace for this package
		OutStableNamespace = TextNamespaceUtil::BuildFullNamespace(InProposedNamespace, PackageNamespace, /*bAlwaysApplyPackageNamespace*/true);

		if (InProposedNamespace.Equals(OutStableNamespace, ESearchCase::CaseSensitive) || InEditAction == IEditableTextProperty::ETextPropertyEditAction::EditedNamespace)
		{
			// If the proposal was already using the correct namespace (or we just set the namespace), attempt to persist the proposed key too
			if (!InProposedKey.IsEmpty())
			{
				// If we changed the source text, then we can persist the key if this text is the *only* reference using that ID
				// If we changed the identifier, then we can persist the key only if doing so won't cause an identify conflict
				const FTextReferenceCollector::EComparisonMode ReferenceComparisonMode = InEditAction == IEditableTextProperty::ETextPropertyEditAction::EditedSource ? FTextReferenceCollector::EComparisonMode::MatchId : FTextReferenceCollector::EComparisonMode::MismatchSource;
				const int32 RequiredReferenceCount = InEditAction == IEditableTextProperty::ETextPropertyEditAction::EditedSource ? 1 : 0;

				int32 ReferenceCount = 0;
				FTextReferenceCollector(InPackage, ReferenceComparisonMode, OutStableNamespace, InProposedKey, InTextSource, ReferenceCount);

				if (ReferenceCount == RequiredReferenceCount)
				{
					bPersistKey = true;
					OutStableKey = InProposedKey;
				}
			}
		}
		else if (InEditAction != IEditableTextProperty::ETextPropertyEditAction::EditedNamespace)
		{
			// If our proposed namespace wasn't correct for our package, and we didn't just set it (which doesn't include the package namespace)
			// then we should clear out any user specified part of it
			OutStableNamespace = TextNamespaceUtil::BuildFullNamespace(FString(), PackageNamespace, /*bAlwaysApplyPackageNamespace*/true);
		}
	}

	if (!bPersistKey)
	{
		OutStableKey = FGuid::NewGuid().ToString();
	}
}

void FJointEdUtils::JointText_StaticStableTextIdWithObj(UObject* InObject,
                                                        const IEditableTextProperty::ETextPropertyEditAction InEditAction, const FString& InTextSource,
                                                        const FString& InProposedNamespace, const FString& InProposedKey, FString& OutStableNamespace,
                                                        FString& OutStableKey)
{
	UPackage* Package = InObject ? InObject->GetOutermost() : nullptr;
	JointText_StaticStableTextId(Package, InEditAction, InTextSource, InProposedNamespace, InProposedKey, OutStableNamespace, OutStableKey);
}


void FJointEdUtils::HandleNewAssetActionClassPicked(FString BasePath, UClass* InClass)
{
	if (InClass == nullptr) return;

	FString ClassName = FBlueprintEditorUtils::GetClassNameWithoutSuffix(InClass);
	
	FString PathName = !BasePath.IsEmpty() ? FPaths::GetPath(BasePath) : TEXT("/Game");

	PathName = PathName / ClassName;

	FString Name;
	FString PackageName;
	const FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(PathName, TEXT("_New"), PackageName, Name);

	UPackage* Package = CreatePackage(*PackageName);
	if (ensure(Package))
	{
		// Create and init a new Blueprint
		if (UBlueprint* NewBP = FKismetEditorUtilities::CreateBlueprint(InClass
		                                                                , Package
		                                                                , FName(*Name)
		                                                                , BPTYPE_Normal
		                                                                , UBlueprint::StaticClass()
		                                                                , UBlueprintGeneratedClass::StaticClass()))
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(NewBP);

			// Notify the asset registry
			FAssetRegistryModule::AssetCreated(NewBP);

			// Mark the package dirty...
			Package->MarkPackageDirty();
		}
	}


	FSlateApplication::Get().DismissAllMenus();
}

void FJointEdUtils::OpenEditorFor(UJointManager* Manager, FJointEditorToolkit*& Toolkit)
{
	Toolkit = nullptr;

	if (Manager == nullptr) return;

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Manager);

	IAssetEditorInstance* EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->
	                                                FindEditorForAsset(Manager, true);

	if (EditorInstance != nullptr)
	{
		Toolkit = static_cast<FJointEditorToolkit*>(EditorInstance);
	}
}

FJointEditorToolkit* FJointEdUtils::FindOrOpenJointEditorInstanceFor(UObject* ObjectRelatedTo, const bool& bOpenIfNotPresent, const bool& bFocusIfOpen)
{
	if (!ObjectRelatedTo) return nullptr;
	
	UAssetEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();

	auto FindOrOpenEditor = [&](UJointManager* Manager) -> FJointEditorToolkit*
	{
		if (!Manager) return nullptr;
		
		if (IAssetEditorInstance* Editor = EditorSubsystem->FindEditorForAsset(Manager, bFocusIfOpen)) return static_cast<FJointEditorToolkit*>(Editor);

		if (!bOpenIfNotPresent) return nullptr;
		
		EditorSubsystem->OpenEditorForAsset(Manager);

		if (IAssetEditorInstance* Editor = EditorSubsystem->FindEditorForAsset(Manager, bFocusIfOpen))
		{
			return static_cast<FJointEditorToolkit*>(Editor);
		}

		return nullptr;
	};

	if (UJointManager* Manager = Cast<UJointManager>(ObjectRelatedTo)) return FindOrOpenEditor(Manager);
	if (UJointEdGraph* Graph = Cast<UJointEdGraph>(ObjectRelatedTo)) return FindOrOpenEditor(Graph->GetJointManager());
	if (UJointEdGraphNode* EdNode = Cast<UJointEdGraphNode>(ObjectRelatedTo)) return FindOrOpenEditor(EdNode->GetJointManager());
	if (UJointNodeBase* NodeBase = Cast<UJointNodeBase>(ObjectRelatedTo)) return FindOrOpenEditor(NodeBase->GetJointManager());

	return nullptr;
}

UJointEdGraph* FJointEdUtils::FindGraphForNodeInstance(const UJointNodeBase* NodeInstance)
{
	if (NodeInstance == nullptr) return nullptr;

	if (UJointManager* Manager = NodeInstance->GetJointManager())
	{
		
		if (Manager->JointGraph == nullptr) return nullptr;

		if (UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(Manager->JointGraph))
		{
			TSet<TWeakObjectPtr<UObject>> Nodes = CastedGraph->GetCachedJointNodeInstances(false);

			// do const_cast here - due to TSet not supporting const types in older UE versions
			if (Nodes.Contains(const_cast<UJointNodeBase*>(NodeInstance)))
			{
				return CastedGraph;
			}

			//if not found, search in sub graphs
			TArray<UJointEdGraph*> Graphs = CastedGraph->GetAllSubGraphsRecursively();
			
			for (UJointEdGraph* Graph : Graphs)
			{
				if (Graph == nullptr) continue;

				Nodes = Graph->GetCachedJointNodeInstances(false);

				// do const_cast here - due to TSet not supporting const types in older UE versions
				if (Nodes.Contains(const_cast<UJointNodeBase*>(NodeInstance)))
				{
					return Graph;
				}
			}

		}
	}

	return nullptr;
	
	
}

UEdGraphNode* FJointEdUtils::FindGraphNodeForNodeInstance(const UJointNodeBase* NodeInstance)
{
	
	UEdGraphNode* OutNode = nullptr;
	
	if (NodeInstance == nullptr) return nullptr;

	if (UJointManager* Manager = NodeInstance->GetJointManager())
	{
		if (Manager->JointGraph == nullptr) return nullptr;

		if (UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(Manager->JointGraph))
		{
			OutNode =  CastedGraph->FindGraphNodeForNodeInstance(NodeInstance);

			if (OutNode)
			{
				return OutNode;
			}
			
			//if not found, search in sub graphs
			TArray<UJointEdGraph*> Graphs = CastedGraph->GetAllSubGraphsRecursively();
			
			for (UJointEdGraph* Graph : Graphs)
			{
				if (Graph == nullptr) continue;

				if (UEdGraphNode* FoundNode2 = Graph->FindGraphNodeForNodeInstance(NodeInstance))
				{
					return FoundNode2;
				}
			}
		}
	}

	return nullptr;
}

UJointEdGraphNode* FJointEdUtils::FindGraphNodeWithProvidedNodeInstanceGuid(UJointManager* JointManager, const FGuid& NodeGuid)
{
	if (JointManager == nullptr) return nullptr;

	TArray<UJointEdGraph*> AllGraphs = UJointEdGraph::GetAllGraphsFrom(JointManager);

	for (UJointEdGraph* Graph : AllGraphs)
	{
		if (Graph == nullptr) continue;

		TSet<TWeakObjectPtr<UJointEdGraphNode>> GraphNodes = Graph->GetCachedJointGraphNodes();

		for (TWeakObjectPtr<UJointEdGraphNode> GraphNode : GraphNodes)
		{
			if (GraphNode == nullptr) continue;

			UJointNodeBase* NodeInstance = GraphNode->GetCastedNodeInstance();
			
			if (NodeInstance && NodeInstance->NodeGuid == NodeGuid) return GraphNode.Get();
		}
	}

	return nullptr;
}

UJointEdGraphNode* FJointEdUtils::GetCorrespondingJointGraphNodeForJointManager(UJointEdGraphNode* SearchFor, UJointManager* TargetManager)
{
	if (SearchFor != nullptr && SearchFor->GetJointManager() != nullptr)
	{
		UJointManager* SearchForJointManager = SearchFor->GetJointManager();
		
		//if this node is from the target Joint manager : return itself.
		if (SearchForJointManager == TargetManager) return SearchFor;
		
		if (!TargetManager || !TargetManager->JointGraph) return nullptr;
		
		// UJointEdGraph::Nodes contains only the base node on the graph, not sub nodes. So we need to iterate through all nodes to find the matching one - but in a clever way.
		// Cache the hierarchy paths of the provided node of the attachment tree, from base node to itself.
			
		TArray<FString> InJointNodeHierarchyPaths;
			
		UJointEdGraphNode* CurrentNode = SearchFor;
		while (CurrentNode != nullptr)
		{
			FString CurrentPath = CurrentNode->GetPathName(SearchForJointManager);
			InJointNodeHierarchyPaths.Insert(CurrentPath, 0); // insert at the beginning to maintain order from base to leaf.

			CurrentNode = CurrentNode->ParentNode;
		}
			
			
		// Now iterate the actual nodes on the asset side and 'step-in' for the hierarchy path stages to find the matching node.
			
		TArray<UJointEdGraph*> AllGraphs = UJointEdGraph::GetAllGraphsFrom(TargetManager);
			
		TArray<UEdGraphNode*> CandidateNodes;
			
		// initialize candidate nodes from all graphs' base nodes.
		for (UJointEdGraph* Graph : AllGraphs)
		{
			if (Graph == nullptr) continue;

			for (TObjectPtr<class UEdGraphNode> EdGraphNode : Graph->Nodes)
			{
				CandidateNodes.Add(EdGraphNode);
			}
		}
			
		while (CandidateNodes.Num() > 0 && InJointNodeHierarchyPaths.Num() > 0)
		{
			FString TargetPath = InJointNodeHierarchyPaths[0];
			InJointNodeHierarchyPaths.RemoveAt(0);
				
			TArray<UEdGraphNode*> NextCandidateNodes;
			for (UEdGraphNode* Node : CandidateNodes)
			{
				if (Node == nullptr) continue;

				FString NodePath = Node->GetPathName(TargetManager);

				if (NodePath == TargetPath)
				{
					// if this is the last stage, we found it.
					if (InJointNodeHierarchyPaths.Num() == 0)
					{
						return Cast<UJointEdGraphNode>(Node);
					}
					else
					{
						// step into children for next stage.
						if (UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(Node))
						{
							TArray<TObjectPtr<UJointEdGraphNode>> SubNodes = CastedNode->SubNodes;
								
							for (TObjectPtr<UJointEdGraphNode> JointEdGraphNode : SubNodes)
							{
								NextCandidateNodes.Add(JointEdGraphNode);
							}
						}
					}
				}
			}
			
			CandidateNodes = NextCandidateNodes;
		}
	}
	
	return nullptr;
}

UJointManager* FJointEdUtils::GetOriginalJointManager(UJointManager* InJointManager)
{
	if (InJointManager != nullptr && !InJointManager->IsAsset())
	{
		if (UObject* Outer = InJointManager->GetOuter())
		{
			if (AJointActor* JointActor = Cast<AJointActor>(Outer))
			{
				if (JointActor->OriginalJointManager && JointActor->OriginalJointManager->IsValidLowLevel())
				{
					return JointActor->OriginalJointManager;
				}
			}
		}
	}
	else if (InJointManager != nullptr && InJointManager->IsAsset())
	{
		return InJointManager;
	}

	return nullptr;
}

UJointEdGraphNode* FJointEdUtils::GetOriginalJointGraphNodeFromJointGraphNode(UJointEdGraphNode* InJointEdGraphNode)
{
	
	//use GetCorrespondingJointGraphNodeForJointManager
	
	return GetCorrespondingJointGraphNodeForJointManager(InJointEdGraphNode, FJointEdUtils::GetOriginalJointManager(InJointEdGraphNode->GetJointManager()));
}

UJointNodeBase* FJointEdUtils::GetOriginalJointNodeFromJointNode(UJointNodeBase* InJointNode)
{
	// use GetCorrespondingJointNodeForJointManager
	
	return UJointFunctionLibrary::GetCorrespondingJointNodeForJointManager(InJointNode, FJointEdUtils::GetOriginalJointManager(InJointNode->GetJointManager()));
}

void FJointEdUtils::MarkNodesAsModifiedAndValidateName(
	TSet<UEdGraphNode*> InNodes
)
{
	// Modify the nodes to prepare for reattachment of sub nodes.

	for (UEdGraphNode*& PreImportNode : InNodes)
	{
		if (!PreImportNode) continue;

		if (UJointEdGraphNode* CastedGraphNode = Cast<UJointEdGraphNode>(PreImportNode))
		{
			CastedGraphNode->Modify();

			if (!CastedGraphNode->GetCastedNodeInstance()) continue;

			CastedGraphNode->GetCastedNodeInstance()->Modify();
		}
	}

	// rename the node instances to avoid name conflicts
	for (UEdGraphNode* NewPastedGraphNode : InNodes)
	{
		if (UJointEdGraphNode* CastedGraphNode = Cast<UJointEdGraphNode>(NewPastedGraphNode))
		{
			TObjectPtr<UObject>& Instance = CastedGraphNode->NodeInstance;

			if (CastedGraphNode == nullptr || Instance == nullptr) continue;

			FString Name = Instance->GetName();
			
			if (FJointEdUtils::GetSafeNameForObjectRenaming(Name, Instance, Instance->GetOuter()))
			{
				Instance->Rename(*Name, Instance->GetOuter(), REN_NonTransactional);
			}else
			{
				ensureMsgf(false, TEXT("Failed to rename the node instance '%s' during paste operation. This is not admirable, and can break the asset."), *Instance->GetPathName());
			}
		}
	}
	
}


void FJointEdUtils::MoveNodesAtLocation(TSet<UEdGraphNode*> InNodes, const FVector2D& PasteLocation)
{
	// Recenter pasted nodes around PasteLocation
	int64 SumX = 0, SumY = 0;
	int32 Count = 0;
		
	for (UEdGraphNode* Node : InNodes)
	{
		UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(Node);
		
		//If it is UJointEdGraphNode type, then check whether it is a fragment to decide whether to add it on the avg point calculation.
		//If it was a graph node but not a UJointEdGraphNode type node, then just add it to the calculation.
		if (!CastedNode || (CastedNode && !Cast<UJointEdGraphNode_Fragment>(CastedNode)))
		{
			SumX += Node->NodePosX;
			SumY += Node->NodePosY;
			++Count;
		}
	}

	if (Count > 0)
	{
		const int32 CenterX = static_cast<int32>(SumX / Count);
		const int32 CenterY = static_cast<int32>(SumY / Count);
		const FVector2D CurrentCenter((float)CenterX, (float)CenterY);
		const FVector2D Offset = PasteLocation - CurrentCenter;

		for (UEdGraphNode* Node : InNodes)
		{
			Node->NodePosX = static_cast<int32>(Node->NodePosX + Offset.X);
			Node->NodePosY = static_cast<int32>(Node->NodePosY + Offset.Y);
			Node->SnapToGrid(UJointEditorSettings::GetJointGridSnapSize());
		}
	}
}


UClass* FJointEdUtils::GetBlueprintClassWithClassPackageName(const FName& ClassName)
{
	FString FinalClassName = ClassName.ToString();

	if(!FinalClassName.EndsWith("_C")) FinalClassName.Append("_C");

#if UE_VERSION_OLDER_THAN(5,1,0)
	
	if (UClass* Result = FindObject<UClass>(ANY_PACKAGE,*ClassName.ToString())) return Result;

	if (UObjectRedirector* RenamedClassRedirector = FindObject<UObjectRedirector>(ANY_PACKAGE, *ClassName.ToString())) return CastChecked<UClass>(RenamedClassRedirector->DestinationObject);

#else
	
	if (UClass* Result = FindFirstObjectSafe<UClass>(*ClassName.ToString())) return Result;

	if (UObjectRedirector* RenamedClassRedirector = FindFirstObjectSafe<UObjectRedirector>(*ClassName.ToString())) return CastChecked<UClass>(RenamedClassRedirector->DestinationObject);
	
#endif
	return nullptr;
}

template <typename PropertyType>
PropertyType* FJointEdUtils::GetCastedPropertyFromClass(const UClass* Class, const FName& PropertyName)
{
	if(Class == nullptr || PropertyName.IsNone()) return nullptr;

	if(FProperty* FoundProperty = Class->FindPropertyByName(PropertyName))
	{
		if(PropertyType* CastedProperty = CastField<PropertyType>(FoundProperty)) return CastedProperty;
	}
	
	return nullptr;
}


FText FJointEdUtils::GetFriendlyNameOfNode(const UJointEdGraphNode* Node)
{
	if(Node)
	{
		if(Node->GetEdNodeSetting().bUseSimplifiedDisplayClassFriendlyNameText)
		{
			return Node->GetEdNodeSetting().SimplifiedClassFriendlyNameText;
		}else
		{
			if (Node->GetCastedNodeInstance())
			{
				return GetFriendlyNameFromClass(Node->GetCastedNodeInstance()->GetClass());
			}

			//Fallback to class data if no instance
			return GetFriendlyNameFromClass(const_cast<UJointEdGraphNode*>(Node)->NodeClassData.GetClass(true));
		}
	}

	return FText::GetEmpty();
}

FText FJointEdUtils::GetFriendlyNameFromClass(const TSubclassOf<UObject> Class)
{
	if (Class != nullptr)
	{
		FString DisplayName = Class->GetMetaData(TEXT("DisplayName"));

		if (!DisplayName.IsEmpty())
		{
			return FText::FromString(DisplayName);
		}

		UClass* MyClass = Class.Get();
		if (MyClass)
		{
			FString ClassDesc = MyClass->GetName();

			if (MyClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
			{
				return FText::FromString(ClassDesc.LeftChop(2));
			}

			const int32 ShortNameIdx = ClassDesc.Find(TEXT("_"), ESearchCase::CaseSensitive);
			if (ShortNameIdx != INDEX_NONE)
			{
#if UE_VERSION_OLDER_THAN(5,5,0)
				ClassDesc.MidInline(ShortNameIdx + 1, MAX_int32, false);
#else
				ClassDesc.MidInline(ShortNameIdx + 1, MAX_int32, EAllowShrinking::No);
#endif
			}

			return FText::FromString(ClassDesc);
		}
	}

	return FText::GetEmpty();
}

EMessageSeverity::Type FJointEdUtils::ResolveJointEdMessageSeverityToEMessageSeverity(
	const EJointEdMessageSeverity::Type JointEdLogMessage)
{
	switch (JointEdLogMessage)
	{
	case EJointEdMessageSeverity::Info:
		return EMessageSeverity::Info;
	case EJointEdMessageSeverity::Warning:
		return EMessageSeverity::Warning;
	case EJointEdMessageSeverity::PerformanceWarning:
		return EMessageSeverity::PerformanceWarning;
	case EJointEdMessageSeverity::Error:
		return EMessageSeverity::Error;
	}
	
	return EMessageSeverity::Info;
}





void FJointEdUtils::RemoveGraph(UJointEdGraph* GraphToRemove)
{
	if (GraphToRemove == nullptr) return;

	if (UJointManager* Manager = GraphToRemove->GetJointManager())
	{
		if (Manager->GetJointGraphAs() == GraphToRemove)
		{
			//We cannot delete the main graph.
			return;
		}

		Manager->Modify();

		GraphToRemove->Modify();

		// Can't just call Remove, the object is wrapped in a struct
		for(int EditedDocIdx = 0; EditedDocIdx < Manager->LastEditedDocuments.Num(); ++EditedDocIdx)
		{
			if(Manager->LastEditedDocuments[EditedDocIdx].EditedObjectPath.ResolveObject() == GraphToRemove)
			{
				Manager->LastEditedDocuments.RemoveAt(EditedDocIdx);
				break;
			}
		}

		// traverse the graphs to find the parent graph and remove the subgraph reference

		if (UEdGraph* ParentGraph = UEdGraph::GetOuterGraph(GraphToRemove))
		{
			if (UJointEdGraph* ParentJointGraph = Cast<UJointEdGraph>(ParentGraph))
			{
				ParentJointGraph->Modify();
				ParentJointGraph->SubGraphs.Remove(GraphToRemove);
			}
		}

		GraphToRemove->GetSchema()->HandleGraphBeingDeleted(*GraphToRemove);

		GraphToRemove->Rename(nullptr, GetTransientPackage(), REN_DoNotDirty | REN_DontCreateRedirectors);
		GraphToRemove->ClearFlags(RF_Standalone | RF_Public);
		GraphToRemove->RemoveFromRoot();
	}
	
}

void FJointEdUtils::RemoveNode(class UObject* NodeRemove)
{
	if (NodeRemove == nullptr) return;
		
	if (UEdGraphNode* Node = Cast<UEdGraphNode>(NodeRemove))
	{
		if (!Node->CanUserDeleteNode()) return;

		Node->Modify();

		if (UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(Node))
		{
			if (CastedNode->GetCastedNodeInstance()) CastedNode->GetCastedNodeInstance()->Modify();

			UJointEdGraphNode* SavedParentNode = CastedNode->GetParentmostNode();

			if (SavedParentNode)
			{
				SavedParentNode->Modify();

				if (SavedParentNode->GetCastedNodeInstance())
				{
					SavedParentNode->GetCastedNodeInstance()->Modify();
				}

				for (UJointEdGraphNode* SubNodes : SavedParentNode->GetAllSubNodesInHierarchy())
				{
					if (SubNodes)
					{
						SubNodes->Modify();

						if (SubNodes->GetCastedNodeInstance())
						{
							SubNodes->GetCastedNodeInstance()->Modify();
						}
					}
				}
			}

			Node->DestroyNode();

			if (SavedParentNode)
			{
				SavedParentNode->RequestUpdateSlate();
			}
		}
		else
		{
			Node->DestroyNode();
		}
	}
}

void FJointEdUtils::RemoveNodes(TArray<class UObject*> NodesToRemove)
{
	
	for (UObject* ToRemove : NodesToRemove){

		if (ToRemove == nullptr) continue;
		
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(ToRemove))
		{
			if (!Node->CanUserDeleteNode()) continue;

			Node->Modify();

			if (UJointEdGraphNode* CastedNode = Cast<UJointEdGraphNode>(Node))
			{
				if (CastedNode->GetCastedNodeInstance()) CastedNode->GetCastedNodeInstance()->Modify();

				UJointEdGraphNode* SavedParentNode = CastedNode->GetParentmostNode();

				if (SavedParentNode)
				{
					SavedParentNode->Modify();

					if (SavedParentNode->GetCastedNodeInstance())
					{
						SavedParentNode->GetCastedNodeInstance()->Modify();
					}

					for (UJointEdGraphNode* SubNodes : SavedParentNode->GetAllSubNodesInHierarchy())
					{
						if (SubNodes)
						{
							SubNodes->Modify();

							if (SubNodes->GetCastedNodeInstance())
							{
								SubNodes->GetCastedNodeInstance()->Modify();
							}
						}
					}
				}

				Node->DestroyNode();

				if (SavedParentNode && SavedParentNode != Node)
				{
					SavedParentNode->RequestUpdateSlate();
				}
			}
			else
			{
				Node->DestroyNode();
			}
		}
	}
}



bool FJointEdUtils::GetSafeNameForObjectRenaming(FString& InNewNameOutValidatedName, UObject* ObjectToRename, UObject* InOuter)
{
	//Revert if the node instance was nullptr.
	if (!ObjectToRename) return false;

	//Revert if the new outer is the same with the object.
	if (ObjectToRename == InOuter) return false;

	//for the case of renaming in the same outer, we need to ignore the existing name of the object, and if the outer is not the same, we must check it as well.
	FName ExistingName = ObjectToRename->GetOuter() == InOuter ? ObjectToRename->GetFName() : NAME_None;
	
	TSharedPtr<FJointEditorNameValidator> NameValidator = MakeShareable(new FJointEditorNameValidator(InOuter, ExistingName));
	NameValidator->FindValidStringPruningSuffixes(InNewNameOutValidatedName);

	return true;
}

bool FJointEdUtils::IsNameSafeForObjectRenaming(const FString& InName, UObject* ObjectToRename, UObject* InOuter, FText& OutErrorMessage)
{
	//Revert if the node instance was nullptr.
	if (!ObjectToRename) return false;

	//Revert if the new outer is the same with the object.
	if (ObjectToRename == InOuter) return false;

	//for the case of renaming in the same outer, we need to ignore the existing name of the object, and if the outer is not the same, we must check it as well.
	FName ExistingName = ObjectToRename->GetOuter() == InOuter ? ObjectToRename->GetFName() : NAME_None;
	
	TSharedPtr<FJointEditorNameValidator> NameValidator = MakeShareable(new FJointEditorNameValidator(InOuter, ExistingName));

	const EValidatorResult Result = NameValidator->Validate(InName, OutErrorMessage);
	
	return Result == EValidatorResult::Ok || Result == EValidatorResult::ExistingName;
}

bool FJointEdUtils::GetSafeNameForObject(FString& InNewNameOutValidatedName, UObject* InOuter)
{
	TSharedPtr<FJointEditorNameValidator> NameValidator = MakeShareable(new FJointEditorNameValidator(InOuter, NAME_None));
	NameValidator->FindValidStringPruningSuffixes(InNewNameOutValidatedName);

	return true;
}


void FJointEdUtils::GetGraphIconForAction(FEdGraphSchemaAction_K2Graph const* const ActionIn, FSlateBrush const*& IconOut, FSlateColor& ColorOut, FText& ToolTipOut)
{
	switch (ActionIn->GraphType)
	{
	case EEdGraphSchemaAction_K2Graph::Graph:
		{
			IconOut = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(TEXT("GraphEditor.EventGraph_16x"));
			ToolTipOut = LOCTEXT("EventGraph_ToolTip", "Joint Graph");
		}
		break;
	case EEdGraphSchemaAction_K2Graph::Subgraph:
		{
			if (Cast<UJointEdGraph>(ActionIn->EdGraph)) // TODO: MAKE IT WORK MORE WITH SUBGRAPH CLASS IF NEEDED
			{
				IconOut = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(TEXT("GraphEditor.SubGraph_16x") );
				ToolTipOut = LOCTEXT("JointSubGraph_ToolTip", "Joint Sub Graph");
			}
		}
		break;
	}
}

void FJointEdUtils::GetGraphIconFor(const UEdGraph* Graph, FSlateBrush const*& IconOut)
{

	if (!Graph) return;

	const UJointEdGraph* JointGraph = Cast<const UJointEdGraph>(Graph);

	if (JointGraph->IsRootGraph())
	{
		IconOut = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(TEXT("GraphEditor.EventGraph_16x"));
	}else
	{
		IconOut = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(TEXT("GraphEditor.SubGraph_16x") );
	}

	// fallback
	if (IconOut == nullptr) IconOut = FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(TEXT("GraphEditor.EventGraph_16x"));
	
}

#undef LOCTEXT_NAMESPACE
