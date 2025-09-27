#pragma once

#include "CoreMinimal.h"
#include "Framework/Docking/TabManager.h"
#include "JointEditorToolkit.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "WorkflowOrientedApp/ApplicationMode.h"

/** Application mode for main behavior tree editing mode */
class FJointEditorApplicationMode : public FApplicationMode
{
public:
	FJointEditorApplicationMode(TSharedPtr<class FJointEditorToolkit> InToolkit);

public:

	void Initialize();

	virtual void RegisterTabFactories(TSharedPtr<class FTabManager> InTabManager) override;
	virtual void PreDeactivateMode() override;
	virtual void PostActivateMode() override;

protected:
	TWeakPtr<class FJointEditorToolkit> JointEditorToolkit;

	// Set of spawnable tabs in behavior tree editing mode
	FWorkflowAllowedTabSet EditorTabFactories;

public:

	/** Factory that spawns graph editors; used to look up all tabs spawned by it. */
	TWeakPtr<FDocumentTabFactory> GraphEditorTabFactoryPtr;

public:

	TSharedPtr<FTabManager::FLayout> GetTabLayout();
	
};
