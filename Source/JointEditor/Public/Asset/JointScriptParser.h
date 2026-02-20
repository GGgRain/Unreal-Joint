// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JointEdGraphNode.h"
#include "Engine/DataTable.h"
#include "Script/JointScriptLinker.h"
#include "UObject/NoExportTypes.h"
#include "JointScriptParser.generated.h"

class UJointManager;
class UEdGraph;
class UJointNodeBase;


/**
 * A parser for Joint Graph and nodes.
 * It can work with csv, json, xml and other text formats.
 * It's designed to be extended as blueprint or c++ class to implement custom parsing logic.
 * 
 * It should support both exporting and importing Joint graph and nodes. (ideally)
 * @see JSP_CSVParser for the reference implementation.
 */
UCLASS(Abstract)
class JOINTEDITOR_API UJointScriptParser : public UObject
{
	GENERATED_BODY()
	
public:
	
	UJointScriptParser();

private:
	
	/**
	 * Whether to check the extension name when importing.
	 * It will call CheckIsSupportedExtension function to validate the extension name.
	 */
	UPROPERTY(EditAnywhere, Category = "Script Parser")
	bool bCheckExtensionOnImport = true;

	/**
	 * Whether to check the text format when importing.
	 * It will call CheckIsSupportedTextFormat function to validate the text data.
	 */
	UPROPERTY(EditAnywhere, Category = "Script Parser", meta = (EditCondition="bCheckExtensionOnImport"))
	bool bCheckTextFormatOnImport = true;
	
public:
	
	/**
	 * List of option property names that can be used to customize the parsing behavior.
	 * The properties in this list will be displayed in the importer/exporter UI for the user to set.
	 * Use this to add custom options for your parser.
	 */
	UPROPERTY(EditAnywhere, Category = "Script Parser")
	TArray<FString> OptionPropertyNames;
	
public:
	
	/**
	 * Check if the provided text data's extension is supported by this parser.
	 * @param InExtensionName The extension name to check.
	 * @return true if the extension is supported, false otherwise.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Script Parser")
	bool CheckIsSupportedExtension(const FString& InExtensionName);
	
	virtual bool CheckIsSupportedExtension_Implementation(const FString& InExtensionName);
	
	/**
	 * Check if the provided text data is supported by this parser.
	 * @param InTextData The text data to check.
	 * @return true if the text data is supported, false otherwise.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Script Parser")
	bool CheckIsSupportedTextFormat(const FString& InTextData);

	virtual bool CheckIsSupportedTextFormat_Implementation(const FString& InTextData);
	
public:
	
	//node implementation
	
	/**
	 * Parse the provided text data to create Joint nodes in the provided Joint manager, and update them.
	 * You must override this function to implement your own parsing logic.
	 * @param TargetJointManager The Joint manager that will own the created node.
	 * @param InTextData The text data to parse and create the node.
	 * @return Created Joint node instance.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Script Parser")
	TArray<UJointEdGraphNode*> ParseTextData(
		UJointManager* TargetJointManager,
		const FString& InTextData,
		const FJointScriptLinkerFileEntry& FileEntry
	);
	
	virtual TArray<UJointEdGraphNode*> ParseTextData_Implementation(
		UJointManager* TargetJointManager,
		const FString& InTextData,
		const FJointScriptLinkerFileEntry& FileEntry
	);

	/**
	 * Handle importing the provided text data to create Joint nodes in the provided Joint manager.
	 * You can also use this function to initialize any data before parsing.
	 * @param TargetJointManager The Joint manager that will own the created nodes.
	 * @param InTextData The text data to parse and create the nodes.
	 * @return true if the parsing succeeded, false otherwise.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Script Parser")
	void HandleImporting(
		UJointManager* TargetJointManager,
		const FString& InTextData,
		const FJointScriptLinkerFileEntry& FileEntry
	);
	
	virtual void HandleImporting_Implementation(
		UJointManager* TargetJointManager,
		const FString& InTextData,
		const FJointScriptLinkerFileEntry& FileEntry
	);
	
public:
	
#if WITH_EDITOR
	virtual bool IsEditorOnly() const override { return true; }
#endif
	
};
