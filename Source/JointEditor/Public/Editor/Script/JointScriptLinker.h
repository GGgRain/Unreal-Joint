// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JointManager.h"
#include "UObject/NoExportTypes.h"
#include "JointScriptLinker.generated.h"

class UJointScriptParser;

USTRUCT(BlueprintType)
struct JOINTEDITOR_API FJointScriptLinkerFileEntry
{
	GENERATED_BODY()
	
public:
	
	FJointScriptLinkerFileEntry();
	
public:
	
	/**
	 * File name of the file entry.
	 * This property is used as the unique identifier for the file entry.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Joint Script Linker File Entry")
	FString FileName;
	
public:
	
	// additional data for the identification

	/**
	 * Full file path of the file entry.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Joint Script Linker File Entry")
	FString FilePath;
	
public:
	
	bool operator==(const FJointScriptLinkerFileEntry& Other) const
	{
		return FileName == Other.FileName;
	}
	
	bool operator!=(const FJointScriptLinkerFileEntry& Other) const
	{
		return !(*this == Other);
	}
	
	bool IsValid() const
	{
		return !FileName.IsEmpty();
	}
	
	bool IsNull() const
	{
		return FileName.IsEmpty() && FilePath.IsEmpty();
	}
};

FORCEINLINE uint32 GetTypeHash(const FJointScriptLinkerFileEntry& Entry)
{
	return GetTypeHash(Entry.FileName);
}

USTRUCT(BlueprintType)
struct JOINTEDITOR_API FJointScriptLinkerNodeSet
{
	GENERATED_BODY()
	
public:
	
	FJointScriptLinkerNodeSet();
	
public:
	
	/**
	 * List of Joint node GUIDs associated.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Joint Script Linker Node Mapping")
	TArray<FGuid> NodeGuids;
};


USTRUCT(BlueprintType)
struct JOINTEDITOR_API FJointScriptLinkerMapping
{
	GENERATED_BODY()
	
public:
	
	FJointScriptLinkerMapping();
	
public:
	
	/**
	 * Joint manager associated with this node mapping.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Joint Script Linker Node Mapping")
	TSoftObjectPtr<UJointManager> JointManager;
	
	/**
	 * Node mappings for the Joint nodes.
	 * Key is the external node identifier (e.g., node name in the script file), value is the corresponding Joint node's GUID.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Joint Script Linker Node Mapping")
	TMap<FString, FJointScriptLinkerNodeSet> NodeMappings;
	
public:
	
	FJointScriptLinkerNodeSet& FindOrAddNodeMappingSetForId(const FString& InId)
	{
		if (FJointScriptLinkerNodeSet* FoundSet = NodeMappings.Find(InId))
		{
			return *FoundSet;
		}
		else
		{
			FJointScriptLinkerNodeSet& NewSet = NodeMappings.Add(InId, FJointScriptLinkerNodeSet());
			return NewSet;
		}
	}
	
	bool RemoveNodeMappingSetForId(const FString& InId)
	{
		return NodeMappings.Remove(InId) > 0;
	}
	
	bool RemoveNodeFromMappingSet(const FString& InId, const FGuid& InNodeGuid)
	{
		if (FJointScriptLinkerNodeSet* FoundSet = NodeMappings.Find(InId))
		{
			FoundSet->NodeGuids.Remove(InNodeGuid);
			
			if (FoundSet->NodeGuids.Num() == 0)
			{
				NodeMappings.Remove(InId);
			}
			
			return true;
		}
		
		return false;
	}
	
	bool RemoveNodeGuidFromAllMappings(const FGuid& InNodeGuid)
	{
		bool bRemoved = false;
		
		TArray<FString> KeysToRemove;
		
		for (TPair<FString, FJointScriptLinkerNodeSet>& NodeMapping : NodeMappings)
		{
			NodeMapping.Value.NodeGuids.Remove(InNodeGuid);
			
			if (NodeMapping.Value.NodeGuids.Num() == 0)
			{
				KeysToRemove.Add(NodeMapping.Key);
			}
			else
			{
				bRemoved = true;
			}
		}
		
		for (const FString& Key : KeysToRemove)
		{
			NodeMappings.Remove(Key);
			bRemoved = true;
		}
		
		return bRemoved;
	}
	
};

USTRUCT(BlueprintType)
struct JOINTEDITOR_API FJointScriptLinkerParserData
{
	GENERATED_BODY()
	
public:
	
	FJointScriptLinkerParserData();
	
public:
	
	/**
	 * Script parser associated with this data element - which is used to parse the script file and generate the node mappings.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Joint Script Linker Data Element")
	TSoftClassPtr<UJointScriptParser> ScriptParser;
	
	/**
	 * Archived parser instance data for the parser.
	 * It will be re-instantiated and deserialized to the parser instance.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Joint Script Linker Data Element")
	TArray<uint8> ParserInstanceData;
	
public:
	
	UJointScriptParser* CreateParserInstance() const;
	
	void SaveParserInstance(UJointScriptParser* ParserInstance);
	
public:
	
	static void SerializeParserInstanceToData(UJointScriptParser* ParserInstance, TArray<uint8>& OutData);
	
	static UJointScriptParser* DeserializeParserInstanceFromData(const TArray<uint8>& InData, TSoftClassPtr<UJointScriptParser> ParserClass);
	
	static void DeserializeParserInstanceFromDataToExistingParser(UJointScriptParser* ParserInstance, const TArray<uint8>& InData);
	
};

USTRUCT(BlueprintType)
struct JOINTEDITOR_API FJointScriptLinkerDataElement
{
	GENERATED_BODY()
	
public:
	
	FJointScriptLinkerDataElement();
	FJointScriptLinkerDataElement(const FJointScriptLinkerFileEntry& InFileEntry);
	
public:
	
	/**
	 * File entry associated with this data element.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Joint Script Linker Data Element")
	FJointScriptLinkerFileEntry FileEntry;
	
public:
	
	/**
	 * Script parser associated with this data element - which is used to parse the script file and generate the node mappings.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Joint Script Linker Data Element")
	FJointScriptLinkerParserData ParserData;
	
public:
	
	/**
	 * List of node mappings for different Joint managers.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Joint Script Linker Node Mapping List")
	TArray<FJointScriptLinkerMapping> Mappings;
	
public:
	
	void CleanUpEmptyMappings()
	{
		for (int32 i = Mappings.Num() - 1; i >= 0; i--)
		{
			if (Mappings[i].NodeMappings.Num() == 0)
			{
				Mappings.RemoveAt(i);
			}
		}
	}
	
	void CleanUpMappingsWithMissingJointManagers()
	{
		for (int32 i = Mappings.Num() - 1; i >= 0; i--)
		{
			if (!Mappings[i].JointManager.IsValid())
			{
				Mappings.RemoveAt(i);
			}
		}
	}
	
public:
	
	bool operator==(const FJointScriptLinkerDataElement& Other) const
	{
		return FileEntry == Other.FileEntry;
	}
	
};

FORCEINLINE uint32 GetTypeHash(const FJointScriptLinkerDataElement& Element)
{
	return GetTypeHash(Element.FileEntry);
}
	
USTRUCT(BlueprintType)
struct JOINTEDITOR_API FJointScriptLinkerData
{
	GENERATED_BODY()
	
public:
	
	FJointScriptLinkerData();
	
public:
	
	/**
	 * Saved Script Links for the project.
	 * It holds the mapping between the file entries and the node mappings for the Joint nodes in different Joint managers.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Joint Script", meta = (DisplayName = "Script Links"))
	TArray<FJointScriptLinkerDataElement> ScriptLinks;
	
};