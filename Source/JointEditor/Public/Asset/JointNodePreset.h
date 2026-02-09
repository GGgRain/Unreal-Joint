// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "JointNodePreset.generated.h"

class UJointNodeBase;

/**
 * Joint Node Preset is an asset that stores a template of a Joint Node.
 * You can create a Joint Node Preset from any base Joint Node in the Joint Editor. it will copy all the properties of the node (including sub nodes) into the preset asset.
 * 
 * You can use preset nodes to quickly create new nodes with pre-configured settings and sub nodes - just by dragging and dropping the preset asset into the Joint Editor graph.
 * Any nodes that are created from a preset will be synchronized with the preset asset - if you update the preset asset, all the nodes that are created from it will be updated as well.
 * You can still, break the synchronization by converting the node back to a normal node.
 * 
 * Joint Node Preset is the core component of external import & export system of Joint Editor because it works with the node preset level instead of the whole joint node level - each row in external csv and json files represent a node preset, not a node instance.
 * You can specify which preset to use for each row in the external file, and Joint Editor will create nodes based on the specified presets.
 */
UCLASS(Blueprintable)
class JOINTEDITOR_API UJointNodePreset : public UObject
{
	GENERATED_BODY()
	
public:
	
	UJointNodePreset();

private:
	
	UPROPERTY(VisibleAnywhere, Category = "Joint Node Preset")
	UObject* NodeTemplate;
	
	// only FJointEdUtils can access.
	friend class FJointEdUtils;
	
#if WITH_EDITOR
	virtual bool IsEditorOnly() const override { return true; }
#endif
	
};
