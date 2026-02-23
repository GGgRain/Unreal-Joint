#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/CoreRedirects.h"
#include "JointEditorSharedTypes.generated.h"


/**
 * Data structure for the redirect system of Joint. 
 */

USTRUCT()
struct JOINTEDITOR_API FJointCoreRedirectObjectName
{
	GENERATED_BODY()

	FJointCoreRedirectObjectName();

	FJointCoreRedirectObjectName(const FCoreRedirectObjectName& CoreRedirectObjectName);

public:

	/** Raw name of object */
	UPROPERTY(VisibleAnywhere, Category="Redirect")
	FName ObjectName;

	/** String of outer chain, may be empty */
	UPROPERTY(VisibleAnywhere, Category="Redirect")
	FName OuterName;

	/** Package this was in before, may be extracted out of OldName */
	UPROPERTY(VisibleAnywhere, Category="Redirect")
	FName PackageName;
	
public:

	/** Checks for exact equality */
	bool operator==(const FJointCoreRedirectObjectName& Other) const
	{
		return ObjectName == Other.ObjectName && OuterName == Other.OuterName && PackageName == Other.PackageName;
	}

	bool operator!=(const FJointCoreRedirectObjectName& Other) const
	{
		return !(*this == Other);
	}

public:

	static struct FCoreRedirectObjectName ConvertToCoreRedirectObjectName(const FJointCoreRedirectObjectName& InObjectName);
	
};

USTRUCT()
struct JOINTEDITOR_API FJointCoreRedirect
{
	GENERATED_BODY()

	FJointCoreRedirect() {}
	
	FJointCoreRedirect(const FJointCoreRedirectObjectName& InOldName, const FJointCoreRedirectObjectName& InNewName)
		: OldName(InOldName)
		, NewName(InNewName)
	{
	}

	FJointCoreRedirect(const FJointCoreRedirect& Other)
		: OldName(Other.OldName)
		, NewName(Other.NewName)
	{
	}
	
public:
	
	/** Name of object to look for */
	UPROPERTY(VisibleAnywhere, Category="Redirect")
	FJointCoreRedirectObjectName OldName;

	/** Name to replace with */
	UPROPERTY(VisibleAnywhere, Category="Redirect")
	FJointCoreRedirectObjectName NewName;
	
public:
	
	/** Checks for exact equality */
	bool operator==(const FJointCoreRedirect& Other) const
	{
		return OldName == Other.OldName && NewName == Other.NewName;
	}

	bool operator!=(const FJointCoreRedirect& Other) const
	{
		return !(*this == Other);
	}
};

USTRUCT()
struct JOINTEDITOR_API FJointGraphNodeClassData
{
	GENERATED_USTRUCT_BODY()

	FJointGraphNodeClassData() {}
	FJointGraphNodeClassData(UClass* InClass);
	FJointGraphNodeClassData(UClass* InClass, const FString& InDeprecatedMessage);
	FJointGraphNodeClassData(const FString& InAssetName, const FString& InGeneratedClassPackage, const FString& InClassName, UClass* InClass);

	FString ToString() const;
	FString GetClassName() const;
	FText GetCategory() const;
	FString GetDisplayName() const;
	UClass* GetClass(bool bSilent = false);
	bool IsAbstract() const;

	FORCEINLINE bool IsBlueprint() const { return AssetName.Len() > 0; }
	FORCEINLINE bool IsDeprecated() const { return DeprecatedMessage.Len() > 0; }
	FORCEINLINE FString GetDeprecatedMessage() const { return DeprecatedMessage; }
	FORCEINLINE FString GetPackageName() const { return GeneratedClassPackage; }

	/** set when child class masked this one out (e.g. always use game specific class instead of engine one) */
	uint32 bIsHidden : 1;

	/** set when class wants to hide parent class from selection (just one class up hierarchy) */
	uint32 bHideParent : 1;

public:

	/** pointer to uclass */
	TWeakObjectPtr<UClass> Class;

	/** path to class if it's not loaded yet */
	UPROPERTY(VisibleAnywhere, Category="Developer Mode")
	FString AssetName;
	
	UPROPERTY(VisibleAnywhere, Category="Developer Mode")
	FString GeneratedClassPackage;

	/** resolved name of class from asset data */
	UPROPERTY(VisibleAnywhere, Category="Developer Mode")
	FString ClassName;

	/** User-defined category for this class */
	UPROPERTY(VisibleAnywhere, Category="Developer Mode")
	FText Category;

	/** message for deprecated class */
	FString DeprecatedMessage;

public:

	bool operator==(const FJointGraphNodeClassData& Other) const
	{
		return Class == Other.Class && AssetName == Other.AssetName && GeneratedClassPackage == Other.GeneratedClassPackage
			&& ClassName == Other.ClassName && Category.EqualTo(Other.Category) && DeprecatedMessage == Other.DeprecatedMessage;
	}

	bool operator!=(const FJointGraphNodeClassData& Other) const
	{
		return !(*this == Other);
	}
};

FORCEINLINE uint32 GetTypeHash(const FJointGraphNodeClassData& Struct)
{
	return FCrc::MemCrc32(&Struct, sizeof(FJointGraphNodeClassData));
}


struct JOINTEDITOR_API FJointGraphNodeClassNode
{
	FJointGraphNodeClassData Data;
	FString ParentClassName;

	TSharedPtr<FJointGraphNodeClassNode> ParentNode;
	TArray<TSharedPtr<FJointGraphNodeClassNode> > SubNodes;

	void AddUniqueSubNode(TSharedPtr<FJointGraphNodeClassNode> SubNode);
};

struct JOINTEDITOR_API FJointGraphNodeClassHelper
{
	DECLARE_MULTICAST_DELEGATE(FOnPackageListUpdated);

	FJointGraphNodeClassHelper(UClass* InRootClass);
	~FJointGraphNodeClassHelper();

	void GatherClasses(const UClass* BaseClass, TArray<FJointGraphNodeClassData>& AvailableClasses);
	static FString GetDeprecationMessage(const UClass* Class);

	void OnAssetAdded(const struct FAssetData& AssetData);
	void OnAssetRemoved(const struct FAssetData& AssetData);
	void InvalidateCache();
	void OnReloadComplete(EReloadCompleteReason Reason);

public:

	static void RemoveUnknownClass(const FJointGraphNodeClassData& ClassData);
	static void AddUnknownClass(const FJointGraphNodeClassData& ClassData);
	static bool IsClassKnown(const FJointGraphNodeClassData& ClassData);
	static FOnPackageListUpdated OnPackageListUpdated;

	static int32 GetObservedBlueprintClassCount(UClass* BaseNativeClass);
	static void AddObservedBlueprintClasses(UClass* BaseNativeClass);
	void UpdateAvailableBlueprintClasses();

public:
	
	UClass* RootNodeClass;
	
	TSharedPtr<FJointGraphNodeClassNode> RootNode;

public:
	
	static TArray<FJointGraphNodeClassData> UnknownPackages;
	
	static TMap<UClass*, int32> BlueprintClassCount;
	
private:
	
	TSharedPtr<FJointGraphNodeClassNode> CreateClassDataNode(const struct FAssetData& AssetData);
	TSharedPtr<FJointGraphNodeClassNode> FindBaseClassNode(TSharedPtr<FJointGraphNodeClassNode> Node, const FString& ClassName);
	void FindAllSubClasses(TSharedPtr<FJointGraphNodeClassNode> Node, TArray<FJointGraphNodeClassData>& AvailableClasses);

	UClass* FindAssetClass(const FString& GeneratedClassPackage, const FString& AssetName);
	void BuildClassGraph();
	void AddClassGraphChildren(TSharedPtr<FJointGraphNodeClassNode> Node, TArray<TSharedPtr<FJointGraphNodeClassNode> >& NodeList);

	bool IsHidingClass(UClass* Class);
	bool IsHidingParentClass(UClass* Class);
	bool IsPackageSaved(FName PackageName);
};
