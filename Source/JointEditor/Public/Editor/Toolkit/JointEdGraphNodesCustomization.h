//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DetailWidgetRow.h"
#include "IDetailCustomization.h"
#include "JointEditorNodePickingManager.h"
#include "JointEditorToolkit.h"
#include "VoltAnimationTrack.h"


class SJointNodePointerSlateFeatureButtons;
class UJointEdGraphNode;
class UJointNodeBase;
class SContextTextEditor;
struct FTextRunParseResults;
class STextPropertyEditableTextBox;
class SMultiLineEditableTextBox;
class IDetailLayoutBuilder;
class UJointManager;

class SWidget;
class IPropertyHandle;

//EdGraphNode Customization

class JOINTEDITOR_API JointDetailCustomizationHelpers
{
	
public:
	
	static UJointManager* GetJointManagerFromNodes(TArray<UObject*> Objs);

	static bool CheckBaseClassForDisplay(TArray<UObject*> Objs, IDetailCategoryBuilder& DataCategory);

	static bool IsArchetypeOrClassDefaultObject(TWeakObjectPtr<UObject> Object);

	static bool HasArchetypeOrClassDefaultObject(TArray<TWeakObjectPtr<UObject>> SelectedObjects);
	
	static bool HasArchetypeOrClassDefaultObject(TArray<UObject*> SelectedObjects);
	
	static TArray<UObject*> GetNodeInstancesFromGraphNodes(TArray<TWeakObjectPtr<UObject>> SelectedObjects);

	static UJointManager* GetParentJointFromNodeInstances(TArray<UObject*> InNodeInstanceObjs, bool& bFromMultipleManager, bool& bFromInvalidJointManagerObject, bool& bFromJointManagerItself);

	static TArray<UObject*> CastToNodeInstance(TArray<TWeakObjectPtr<UObject>> SelectedObjects);
};

class JOINTEDITOR_API FJointEdGraphNodesCustomizationBase: public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of IDetailCustomization interface

public:
	
	void OnChangeNodeSetClass(const UClass* Class);
	void OnChangeEditorNodeSetClass(const UClass* Class);

	TArray< TWeakObjectPtr<UObject> > CachedSelectedObjects;

public:
	
	void HideDeveloperModeProperties(IDetailLayoutBuilder& DetailBuilder);

public:

	static bool CheckIfEveryNodeAllowPinDataControl(TArray<UObject*> NodeInstanceObjs);


	
};

// class FSimpleDisplayCategorySlot : public TSharedFromThis<FSimpleDisplayCategorySlot>
// {
// public:
// 	virtual ~FSimpleDisplayCategorySlot() {}
// };

class JOINTEDITOR_API SJointNodeInstanceSimpleDisplayPropertyName : public SCompoundWidget
{
	
public:

	SLATE_BEGIN_ARGS(SJointNodeInstanceSimpleDisplayPropertyName) :
		_EdNode(nullptr)
		{ }
		SLATE_ARGUMENT(TSharedPtr<SWidget>, NameWidget)
		SLATE_ARGUMENT(TSharedPtr<IPropertyHandle>, PropertyHandle)
		SLATE_ARGUMENT(UJointEdGraphNode*, EdNode)
	SLATE_END_ARGS()

public:
	
	void Construct(const FArguments& InArgs);

public:

	void OnVisibilityCheckStateChanged(ECheckBoxState CheckBoxState);

public:

	UJointEdGraphNode* EdNode = nullptr;
	TSharedPtr<class SWidget> NameWidget;
	TSharedPtr<class IPropertyHandle> PropertyHandle;
	
};
		
class JOINTEDITOR_API FJointNodeInstanceSimpleDisplayCustomizationBase: public IDetailCustomization
{
	
public:
	
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) override;
	virtual void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override;
	// End of IDetailCustomization interface

public:
	
	void AddCategoryProperties(UJointEdGraphNode* EdNode, IDetailCategoryBuilder& CategoryBuilder, TArray<TSharedRef<IPropertyHandle>> OutAllProperties);

};



//NodeInstance Customization

class JOINTEDITOR_API FJointNodeInstanceCustomizationBase: public IDetailCustomization
{
	
public:
	
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of IDetailCustomization interface

public:
	
	void HideDisableEditOnInstanceProperties(IDetailLayoutBuilder& DetailBuilder, TArray<UObject*> NodeInstances);

	void PopulateNodeClassesDescription(IDetailLayoutBuilder& DetailBuilder, TArray<UObject*> NodeInstances);

};

class JOINTEDITOR_API FJointEdGraphCustomization: public IDetailCustomization
{
	
public:
	
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of IDetailCustomization interface

public:

	void OnRecaptureButtonPressed();

public:

	TArray<TWeakObjectPtr<UJointEdGraph>> CachedGraph;
};


class JOINTEDITOR_API FJointManagerCustomization: public IDetailCustomization
{
	
public:
	
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of IDetailCustomization interface
public:

	TArray<TWeakObjectPtr<UJointManager>> CachedManager;

};

class JOINTEDITOR_API FJointBuildPresetCustomization: public IDetailCustomization
{
	
public:
	
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of IDetailCustomization interface
	
};

//Type Customization (usually structures)

class JOINTEDITOR_API FJointNodePointerStructCustomization : public IStructCustomization
{
public:
	
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

public:
	
	virtual void CustomizeStructHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeStructChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils) override;

private:

	/** slate color struct handle */
	TSharedPtr<IPropertyHandle> StructPropertyHandle;

	TSharedPtr<IPropertyHandle> NodeHandle;
	TSharedPtr<IPropertyHandle> EditorNodeHandle;

	TSharedPtr<IPropertyHandle> AllowedTypeHandle;
	TSharedPtr<IPropertyHandle> DisallowedTypeHandle;

private:

	TWeakPtr<class FJointEditorNodePickingManagerRequest> Request = nullptr;

private:

	TArray<UObject*> NodeInstanceObjs;

private:
	
	FReply OnNodePickUpButtonPressed();
	FReply OnGoToButtonPressed();
	FReply OnCopyButtonPressed();
	FReply OnPasteButtonPressed();
	FReply OnClearButtonPressed();

	
	void OnNodeDataChanged();
	void OnNodeResetToDefault();

private:

	void OnMouseHovered();
	void OnMouseUnhovered();

private:

	TSharedPtr<SVerticalBox> BackgroundBox;
	TSharedPtr<SHorizontalBox> ButtonBox;
	TSharedPtr<SJointNodePointerSlateFeatureButtons> FeatureButtonsSlate;
	
public:
	
	void BlinkSelf();
	
public:
	
	TSharedPtr<class SJointOutlineBorder> BorderWidget = nullptr;
	
public:
	
	FVoltAnimationTrack BlinkAnimTrack;
	
};
