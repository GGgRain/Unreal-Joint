
// Fill out your copyright notice in the Description page of Project Settings.


#include "Asset/JointScriptParser.h"

#include "JointEdGraphNode.h"
#include "JointEdUtils.h"
#include "JointManager.h"
#include "Markdown/SJointMDSlate_Admonitions.h"

#define LOCTEXT_NAMESPACE "JointScriptParser"

UJointScriptParser::UJointScriptParser()
{
}

bool UJointScriptParser::CheckIsSupportedTextFormat_Implementation(const FString& InTextData)
{
	return false;
}

bool UJointScriptParser::CheckIsSupportedExtension_Implementation(const FString& InExtensionName)
{
	return false;
}

bool UJointScriptParser::ImplementJointNodeFromText_Implementation(UJointManager* TargetJointManager, const FString& InTextData)
{
	//TSubclassOf<UJointEdGraphNode> TargetFragmentEdClass = FJointEdUtils::FindEdClassForNode(NodeClass);
	
	FJointEdUtils::FireNotification(
		LOCTEXT("JointScriptParser_ImplementJointNodeFromText_NotImplemented_Title", "Joint Script Parser Not Implemented"),
		LOCTEXT("JointScriptParser_ImplementJointNodeFromText_NotImplemented_Message", "The ImplementJointNodeFromText function is not implemented in the derived Joint Script Parser class - This will not create any Joint nodes."),
		EJointMDAdmonitionType::Warning
	);

	return false;
}

bool UJointScriptParser::HandleImporting_Implementation(
	UJointManager* TargetJointManager,
	const FString& InTextData)
{
	TArray<FString> TextData;
	InTextData.ParseIntoArrayLines(TextData);

	bool bImportedAny = false;

	UEdGraph* GraphToAdd = TargetJointManager->JointGraph;

	for (const FString& Line : TextData)
	{
		if (!ImplementJointNodeFromText(TargetJointManager, Line))continue;

		bImportedAny = true;
	}

	return bImportedAny;
}


#undef LOCTEXT_NAMESPACE