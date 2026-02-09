// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JointEdGraphNode.h"
#include "Engine/DataTable.h"
#include "UObject/NoExportTypes.h"
#include "JointScriptParser.generated.h"

class UJointManager;
class UEdGraph;
class UJointNodeBase;

/**
 * A data table row structure for Joint Script Parser.
 * It can hold up to 16 fields, you can extend it as needed.
 */
USTRUCT(BlueprintType, Blueprintable, Category = "Script Parser Row")
struct FJointScriptParserTable16 : public FTableRowBase
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field2;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field3;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field4;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field5;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field6;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field7;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field8;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field9;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field10;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field11;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field12;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field13;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field14;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field15;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Script Parser Row")
	FString Field16;
};

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
	 * Implement a Joint node from the provided text data.
	 * If the parsing failed, it must return nullptr, to indicate the failure.
	 * @param TargetJointManager The Joint manager that will own the created node.
	 * @param InTextData The text data to parse and create the node.
	 * @return Created Joint node instance.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Script Parser")
	bool ImplementJointNodeFromText(UJointManager* TargetJointManager, const FString& InTextData);
	
	virtual bool ImplementJointNodeFromText_Implementation(UJointManager* TargetJointManager, const FString& InTextData);

	/**
	 * Implement Joint nodes from the provided text data.
	 * If the parsing failed, it must return false, to indicate the failure.
	 * @param TargetJointManager The Joint manager that will own the created nodes.
	 * @param InTextData The text data to parse and create the nodes.
	 * @return true if the parsing succeeded, false otherwise.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Script Parser")
	bool HandleImporting(UJointManager* TargetJointManager, const FString& InTextData);
	
	virtual bool HandleImporting_Implementation(UJointManager* TargetJointManager, const FString& InTextData);
	
public:
	
#if WITH_EDITOR
	virtual bool IsEditorOnly() const override { return true; }
#endif
	
};
