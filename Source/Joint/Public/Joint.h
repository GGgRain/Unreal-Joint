//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystem/JointSubsystem.h"
#include "Modules/ModuleManager.h"

class FJointModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
public:
	
#if WITH_EDITORONLY_DATA
	
	DECLARE_DELEGATE_RetVal_TwoParams(bool, FCheckJointExecutionException, AJointActor*, const FJointActorExecutionElement&);

	DECLARE_DELEGATE_TwoParams(FJointDebuggerNodePlaybackNotification, AJointActor*, UJointNodeBase*);

	DECLARE_DELEGATE_TwoParams(FJointDebuggerJointPlaybackNotification, AJointActor*, const FGuid&);
	
	FCheckJointExecutionException OnJointExecutionExceptionDelegate;

	FJointDebuggerNodePlaybackNotification JointDebuggerNodeBeginPlayNotification;

	FJointDebuggerNodePlaybackNotification JointDebuggerNodeEndPlayNotification;
	
	FJointDebuggerNodePlaybackNotification JointDebuggerNodePendingNotification;

	FJointDebuggerJointPlaybackNotification JointDebuggerJointBeginPlayNotification;

	FJointDebuggerJointPlaybackNotification JointDebuggerJointEndPlayNotification;

#endif
	
};
