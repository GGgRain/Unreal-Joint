
// Fill out your copyright notice in the Description page of Project Settings.


#include "Asset/JointScriptParser.h"

#include "JointEdGraphNode.h"
#include "JointEditorFunctionLibrary.h"
#include "JointEdUtils.h"
#include "JointManager.h"
#include "ScopedTransaction.h"
#include "Markdown/SJointMDSlate_Admonitions.h"

#define LOCTEXT_NAMESPACE "JointScriptParser"

UJointScriptParser::UJointScriptParser()
{
}

bool UJointScriptParser::CheckIsSupportedExtension_Implementation(const FString& InExtensionName)
{
	return false;
}

bool UJointScriptParser::CheckIsSupportedTextFormat_Implementation(const FString& InTextData)
{
	return false;
}

TArray<UJointEdGraphNode*> UJointScriptParser::ParseTextData_Implementation(
	UJointManager* TargetJointManager,
	const FString& InTextData,
	const FJointScriptLinkerFileEntry& FileEntry)
{
	//TSubclassOf<UJointEdGraphNode> TargetFragmentEdClass = FJointEdUtils::FindEdClassForNode(NodeClass);
	
	FJointEdUtils::FireNotification(
		LOCTEXT("JointScriptParser_ParseTextData_NotImplemented_Title", "Joint Script Parser Not Implemented"),
		LOCTEXT("JointScriptParser_ParseTextData_NotImplemented_Message", "The ParseTextData function is not implemented in the derived Joint Script Parser class - This will not create any Joint nodes."),
		EJointMDAdmonitionType::Warning,
		10
	);

	return TArray<UJointEdGraphNode*>();
}

bool UJointScriptParser::HandleImporting_Implementation(
	UJointManager* TargetJointManager,
	const FString& InTextData,
	const FJointScriptLinkerFileEntry& FileEntry)
{
	TArray<FString> TextData;
	InTextData.ParseIntoArrayLines(TextData);
	
	FScopedTransaction Transaction(
		LOCTEXT("JointScriptParser_HandleImporting_Transaction", "Import Joint Script Text Data")
	);
	
	UEdGraph* GraphToAdd = TargetJointManager->JointGraph;
	
	TargetJointManager->Modify();
	GraphToAdd->Modify();
	
	// Get the asset - because the 'this' object is the CDO of the parser class, we need to get the asset instance to link with the script linker data element, so that we can find the correct parser instance when parsing the text data to create nodes.
	UJointScriptParser* AssetForCDO = Cast<UJointScriptParser>(this);
	if (!AssetForCDO)
	{
		FJointEdUtils::FireNotification(
			LOCTEXT("JointScriptParser_HandleImporting_Failure_Title", "Joint Script Import Failed"),
			LOCTEXT("JointScriptParser_HandleImporting_Failure_Message_CouldNotGetAsset", "Could not get the asset instance for the Joint Script Parser - This will cause issues with linking the parser with the script linker data element."),
			EJointMDAdmonitionType::Error
		);
		
		return false;
	}
	
	UJointEditorFunctionLibrary::LinkParserWithScript(this, FileEntry);
	
	for (const FString& Line : TextData)
	{
		TArray<UJointEdGraphNode*> RecognizedNode = ParseTextData(TargetJointManager,Line,FileEntry);
	}
	
	return true;
}


#undef LOCTEXT_NAMESPACE