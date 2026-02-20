// Fill out your copyright notice in the Description page of Project Settings.


#include "Script/JointScriptLinker.h"

#include "JointScriptParser.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

FJointScriptLinkerFileEntry::FJointScriptLinkerFileEntry() 
	: FileName(TEXT(""))
	, FilePath(TEXT(""))
{
}

FJointScriptLinkerNodeSet::FJointScriptLinkerNodeSet()
{
}

FJointScriptLinkerMapping::FJointScriptLinkerMapping()
{
}

FJointScriptLinkerParserData::FJointScriptLinkerParserData()
{
}

UJointScriptParser* FJointScriptLinkerParserData::CreateParserInstance() const
{
 	if (!ScriptParser.IsValid()) return nullptr;
	
	UJointScriptParser* ParserInstance = ScriptParser.Get()->GetDefaultObject<UJointScriptParser>();
	DeserializeParserInstanceFromData(ParserInstanceData, ScriptParser);
	
	return ParserInstance;
}

void FJointScriptLinkerParserData::SaveParserInstance(UJointScriptParser* ParserInstance)
{
	if (!ParserInstance) return;
	
	ScriptParser = TSoftClassPtr<UJointScriptParser>(ParserInstance->GetClass());
	SerializeParserInstanceToData(ParserInstance, ParserInstanceData);
}

void FJointScriptLinkerParserData::SerializeParserInstanceToData(UJointScriptParser* ParserInstance, TArray<uint8>& OutData)
{
	OutData.Empty();
		
	if (ParserInstance)
	{
		FMemoryWriter MemoryWriter(OutData, true);
		FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, true);
		Ar.ArIsSaveGame = true;
		ParserInstance->Serialize(Ar);
	}
}

UJointScriptParser* FJointScriptLinkerParserData::DeserializeParserInstanceFromData(const TArray<uint8>& InData, TSoftClassPtr<UJointScriptParser> ParserClass)
{
	if (!ParserClass.IsValid()) return nullptr;
		
	UJointScriptParser* ParserInstance = NewObject<UJointScriptParser>(GetTransientPackage(), ParserClass.Get());
	
	DeserializeParserInstanceFromDataToExistingParser(ParserInstance, InData);
	
	return ParserInstance;
}

void FJointScriptLinkerParserData::DeserializeParserInstanceFromDataToExistingParser(UJointScriptParser* ParserInstance, const TArray<uint8>& InData)
{
	if (!ParserInstance) return;
		
	if (InData.Num() > 0)
	{
		FMemoryReader MemoryReader(InData, true);
		FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);
		Ar.ArIsSaveGame = true;
		ParserInstance->Serialize(Ar);
	}
}

FJointScriptLinkerDataElement::FJointScriptLinkerDataElement()
{
}

//introduced for FindByKey usage
FJointScriptLinkerDataElement::FJointScriptLinkerDataElement(const FJointScriptLinkerFileEntry& InFileEntry)
{
	FileEntry = InFileEntry;
}

FJointScriptLinkerData::FJointScriptLinkerData()
{
	
}
