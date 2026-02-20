// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JointEdGraphNode.h"
#include "Engine/DataTable.h"
#include "UObject/NoExportTypes.h"
#include "JointNodePreset.generated.h"

class UJointManager;
class UEdGraph;
class UJointNodeBase;


/**
 * A preset asset for Joint nodes, which can be used to save and apply commonly used settings for Joint nodes.
 * This has been introduced to provide a convenient way for users to manage and reuse their preferred configurations for Joint nodes across different projects or within the same project. Especially with the script parsers.
 */
UCLASS(BlueprintType)
class JOINTEDITOR_API UJointNodePreset : public UObject
{
public:
	
	GENERATED_BODY()
	
public:
	
	UJointNodePreset();

private:
	
#if WITH_EDITOR
	virtual bool IsEditorOnly() const override { return true; }
#endif

public:
	
	/**
	 * Editor only Joint Manager that will be used to store the settings for the Joint nodes.
	 * This will not be used at runtime.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Joint Node Preset")
	UJointManager* InternalJointManager;
	
public:
	
	/**
	 * The display name of the preset, which will be shown in the UI when selecting a preset to apply to a Joint node.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Joint Node Preset")
	FText PresetDisplayName;
	
	/**
	 * A brief description of the preset, which can provide additional information about the settings and use cases of the preset to users when selecting a preset to apply to a Joint node.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Joint Node Preset")
	FText PresetDescription;
	
	/**
	 * The category of the preset, which can be used to organize presets into different groups in the UI when selecting a preset to apply to a Joint node.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Joint Node Preset")
	FText PresetCategory;
};
