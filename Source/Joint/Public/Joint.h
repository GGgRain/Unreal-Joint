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
	
	DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnJointExecutionException, AJointActor*, UJointNodeBase*);

	DECLARE_DELEGATE_TwoParams(FOnJointDebuggerMentionNodePlayback, AJointActor*, UJointNodeBase*);

	DECLARE_DELEGATE_TwoParams(FOnJointDebuggerMentionJointPlayback, AJointActor*, const FGuid&);
	
	FOnJointExecutionException OnJointExecutionExceptionDelegate;

	FOnJointDebuggerMentionNodePlayback OnJointDebuggerMentionNodeBeginPlay;

	FOnJointDebuggerMentionNodePlayback OnJointDebuggerMentionNodeEndPlay;
	
	FOnJointDebuggerMentionNodePlayback OnJointDebuggerMentionNodePending;

	FOnJointDebuggerMentionJointPlayback OnJointDebuggerMentionJointBeginPlay;

	FOnJointDebuggerMentionJointPlayback OnJointDebuggerMentionJointEndPlay;

#endif
	
};
