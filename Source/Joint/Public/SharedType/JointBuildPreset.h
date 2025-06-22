// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SharedType/JointSharedTypes.h"
#include "UObject/NoExportTypes.h"
#include "JointBuildPreset.generated.h"

/**
 * A preset asset that decides whether nodes will be included or excluded on specific build condition on packaging.
 * Please notice that if parent node get excluded, then sub nodes will be excluded as well for that build condition.
 * All behavior will work with AND operator - if any of the setting tells the system to exclude the node, that will be excluded.
 */
UCLASS()
class JOINT_API UJointBuildPreset : public UObject
{
	GENERATED_BODY()

public:

	UJointBuildPreset();

public:

#if WITH_EDITORONLY_DATA
	
	/**
	 * Whether the node will be included on the client side.
	 * Please notice 
	 */
	UPROPERTY(EditAnywhere, Category="Settings")
	EJointBuildPresetBehavior OnClientBehavior;

	/**
	 * Whether the node will be excluded on the server side.
	 */
	UPROPERTY(EditAnywhere, Category="Settings")
	EJointBuildPresetBehavior OnServerBehavior;

	/**
	 * Whether the node will be excluded on Build Targets. If you don't override it on PerBuildTargetSetting, this behavior will be used for every setting.
	 */
	UPROPERTY(EditAnywhere, Category="Settings")
	EJointBuildPresetBehavior DefaultBuildTargetBehavior;
	
	/**
	 * Name of the build target -> behavior.
	 */
	UPROPERTY(EditAnywhere, Category="Settings")
	TMap<FName, EJointBuildPresetBehavior> PerBuildTargetSetting;
	
#endif

public:

	const bool AllowForClient() const;
	const bool AllowForServer() const;
	const bool AllowForBuildTarget(const ITargetPlatform* TargetPlatform);

public:
	
#if WITH_EDITORONLY_DATA

	UPROPERTY(EditAnywhere, Category="Visual")
	FLinearColor PresetColor;

	UPROPERTY(EditAnywhere, Category="Visual")
	FText PresetInitial;

#endif
	
};
