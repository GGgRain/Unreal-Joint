//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "JointEdUtils.h"

#include "JointEdGraph.h"
#include "JointEditorToolkit.h"
#include "Modules/ModuleManager.h"

#include "JointEditor.h"
#include "Internationalization/TextPackageNamespaceUtil.h"
#include "Node/JointFragment.h"
#include "Node/JointNodeBase.h"
#include "Serialization/TextReferenceCollector.h"

class FJointEditorModule;

void FJointEdUtils::GetEditorNodeSubClasses(const UClass* BaseClass, TArray<FGraphNodeClassData>& ClassData)
{
	FJointEditorModule& EditorModule = FModuleManager::GetModuleChecked<
		FJointEditorModule>(TEXT("JointEditor"));
	FGraphNodeClassHelper* ClassCache = EditorModule.GetEdClassCache().Get();

	if(ClassCache)
	{
		ClassCache->GatherClasses(BaseClass, ClassData);
	}
}

void FJointEdUtils::GetNodeSubClasses(const UClass* BaseClass, TArray<FGraphNodeClassData>& ClassData)
{
	FJointEditorModule& EditorModule = FModuleManager::GetModuleChecked<
		FJointEditorModule>(TEXT("JointEditor"));

	FGraphNodeClassHelper* ClassCache = EditorModule.GetClassCache().Get();

	if(ClassCache)
	{
		ClassCache->GatherClasses(BaseClass, ClassData);
	}
}

TSubclassOf<UJointEdGraphNode> FJointEdUtils::FindEdClassForNode(FGraphNodeClassData Class)
{
	TArray<FGraphNodeClassData> ClassData;

	FJointEdUtils::GetEditorNodeSubClasses(UJointEdGraphNode::StaticClass(), ClassData);

	for (FGraphNodeClassData& SubclassData : ClassData)
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

#include "Misc/EngineVersionComparison.h"

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


FText FJointEdUtils::GetFriendlyNameOfNode(const UJointNodeBase* Node)
{
	if(Node)
	{
		if(Node->bUseSimplifiedDisplayClassFriendlyNameText)
		{
			return Node->SimplifiedClassFriendlyNameText;
		}else
		{
			return GetFriendlyNameFromClass(Node->GetClass());
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
