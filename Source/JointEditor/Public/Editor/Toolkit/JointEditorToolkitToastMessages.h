#pragma once

class SJointToolkitToastMessage;
class FJointEditorToolkit;

namespace EJointEditorToastMessage
{
	static const FString NodePicking = "NodePickingToastMessage";
	static const FString QuickNodePicking = "QuickNodePickingToastMessage";
	static const FString TransientEditingWarning = "TransientEditingWarningToastMessage";
	static const FString NonStandaloneAssetEditingWarning = "NonStandaloneAssetEditingWarningToastMessage";
	static const FString VisibilityChangeModeForSimpleDisplayProperty = "VisibilityChangeModeForSimpleDisplayPropertyToastMessage";
	static const FString NodePickerCopy = "NodePickerCopyToastMessage";
	static const FString NodePickerPasted = "NodePickerPastedToastMessage";
	static const FString NeedReopening = "NeedReopeningToastMessage";
	static const FString NodePresetEditing = "NodePresetEditingToastMessage";
}

namespace JointEditorToolkitToastMessages
{
	
	void PopulateNodePickingToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr);
	
	void PopulateQuickNodePickingToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr);
	
	void PopulateTransientEditingWarningToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr);
	
	void PopulateNonStandaloneAssetEditingWarningToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr);

	void PopulateVisibilityChangeModeForSimpleDisplayPropertyToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr);

	void PopulateNodePickerCopyToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr);
	
	void PopulateNodePickerPastedToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr);

	void PopulateNeedReopeningToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr);
	
	void PopulateNodePresetEditingToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr);
	
	
	
	void PopulateToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr, const FString& MessageKey, TSharedRef<SJointToolkitToastMessage> Widget);
	
	void ClearToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr, const FGuid& MessageGuid);
	
	void ClearToastMessage(TWeakPtr<FJointEditorToolkit> ToolkitPtr, const FString& MessageKey);
};
