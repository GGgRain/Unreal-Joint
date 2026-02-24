//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DetailWidgetRow.h"
#include "IDetailCustomization.h"
#include "JointAdvancedWidgets.h"
#include "JointEditorNodePickingManager.h"
#include "JointEditorStyle.h"
#include "JointEditorToolkit.h"
#include "VoltAnimationTrack.h"
#include "PropertyHandle.h"


class SJointNodePointerSlateFeatureButtons;
class UJointEdGraphNode;
class UJointNodeBase;
class SContextTextEditor;
struct FTextRunParseResults;
class STextPropertyEditableTextBox;
class SMultiLineEditableTextBox;
class IDetailLayoutBuilder;
class UJointManager;

class SWindow;
class SWidget;

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

class JOINTEDITOR_API FJointNodePresetCustomization: public IDetailCustomization
{
	
public:
	
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of IDetailCustomization interface
	
	virtual void PendingDelete() override;

public:
	
	FReply OnEditButtonClicked();
	
public:
	
	TSharedPtr<IPropertyHandle> JointManagerHandle;

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






class JOINTEDITOR_API FJointScriptLinkerDataCustomization : public IStructCustomization
{
public:
	
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	
public:
	
	TSharedPtr<IPropertyHandle> ThisStructPropertyHandle;

public:
	virtual void CustomizeStructHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeStructChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils) override;
	
public:
	
	TSharedPtr<SWidget> CreateImportButtonWidget();
	TSharedPtr<SWidget> CreateQuickReimportAllButtonWidget();
	TSharedPtr<SWidget> CreateRefreshWidgetButtonWidget();
	
public:
	
	TWeakPtr<IPropertyUtilities> PropertyUtils;

};

class JOINTEDITOR_API FJointScriptLinkerDataElementCustomization : public IStructCustomization
{
public:
	
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

public:
	
	virtual void CustomizeStructHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeStructChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils) override;
	
public:

	TSharedPtr<SWidget> CreateReimportButtonWidget(TSharedPtr<IPropertyHandle> StructPropertyHandle);

};


class JOINTEDITOR_API FJointScriptLinkerFileEntryCustomization : public IStructCustomization
{
public:
	
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	
public:
	
	FDetailWidgetRow* HeaderRowPtr = nullptr;
	TSharedPtr<IPropertyHandle> ThisStructPropertyHandle;
	
public:
	
	virtual void CustomizeStructHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeStructChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils) override;

public:
	
	TSharedPtr<SWidget> CreateValidityIconWidget(TSharedPtr<IPropertyHandle> StructPropertyHandle);
	TSharedPtr<SWidget> CreateNameTextWidget(TSharedPtr<IPropertyHandle> StructPropertyHandle);
	TSharedPtr<SWidget> CreatePathTextWidget(TSharedPtr<IPropertyHandle> StructPropertyHandle);
	
public:
	
	TSharedPtr<SWidget> CreateOpenOnFileExplorerButtonWidget(TSharedPtr<IPropertyHandle> StructPropertyHandle);
	TSharedPtr<SWidget> CreateReassignFileButtonWidget(TSharedPtr<IPropertyHandle> StructPropertyHandle);

};


class JOINTEDITOR_API FJointArrayElementBuilder : public IDetailCustomNodeBuilder, public TSharedFromThis<FJointArrayElementBuilder>
{
public:

	FJointArrayElementBuilder(TSharedPtr<IPropertyHandle> InPropertyHandle);

	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRebuildChildren ) override;

	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) override;

	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) override;

	virtual bool RequiresTick() const override;

	virtual void Tick( float DeltaTime ) override;

	virtual FName GetName() const override;

	virtual bool InitiallyCollapsed() const override;

public:
	
 	TSharedPtr<IPropertyHandle> PropertyHandle;	
};


class JOINTEDITOR_API FJointScriptLinkerMappingCustomization : public IStructCustomization
{
public:
	
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

public:
	
	struct FNodeStatusItem
	{
		FString Key;              // Map Key
		FGuid Guid;
		bool bExistsInGraph = false;
	};

public:
	
	// Build UI data for list view
	struct FCandidateItem
	{
		TWeakObjectPtr<UJointManager> Manager;
		FString Name;
		FString Path;
		uint32 MatchingNodeCount;
	};
	
	class SCandidateRow : public SMultiColumnTableRow<TSharedPtr<FCandidateItem>>
	{
	public:
		SLATE_BEGIN_ARGS(SCandidateRow);
		SLATE_ARGUMENT(TSharedPtr<FCandidateItem>, Item)
		SLATE_END_ARGS()
		
	public:

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable);
		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
	
	public:
	
		FReply OnOpenEditorButtonPressed();
		FReply OnGoToButtonPressed();

	private:
		TSharedPtr<FCandidateItem> Item;
	};
	
	
	class SReallocationPopup : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SReallocationPopup);
		SLATE_ARGUMENT(TWeakPtr<IPropertyHandle>, JointManagerHandle)
		SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)
		SLATE_ARGUMENT(TArray<TSharedPtr<FCandidateItem>>, CandidateItems)
		SLATE_ARGUMENT(TWeakPtr<FJointScriptLinkerMappingCustomization>, ParentCustomizationPtr)
		SLATE_END_ARGS()
		
	public:
		
		virtual void Construct(const FArguments& InArgs);
		
	public:
		
		void OnColumnSort(EColumnSortPriority::Type, const FName& ColumnId, EColumnSortMode::Type SortMode);
		EColumnSortMode::Type GetColumnSortMode(FName ColumnId) const;
		
	public:
		
		// Parent window and property handle for performing reallocation
		TWeakPtr<SWindow> ParentWindow;
		TSharedPtr<IPropertyHandle> ParentJointManagerHandle;
		TWeakPtr<FJointScriptLinkerMappingCustomization> ParentCustomizationPtr;
		
	public:
		
		// Slate components
		TSharedPtr<SListView<TSharedPtr<FCandidateItem>>> ReallocationPopupListView;

	public:
		
		// Data
		TArray<TSharedPtr<FCandidateItem>> CandidateItems;
		TWeakPtr<FCandidateItem> SelectedItem;
		
		// Sorting
		FName ReallocationCurrentSortColumn = "Match";
		EColumnSortMode::Type ReallocationCurrentSortMode = EColumnSortMode::Descending;
	};
public:

	// widget constructions
	TSharedRef<SWidget> CreateContentWidget();

public:
	
	virtual void CustomizeStructHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeStructChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils) override;
	
public:
	
	void CleanUp();

public:
	
	TSharedPtr<IPropertyHandle> ThisStructPropertyHandle;
	FDetailWidgetRow* HeaderRowPtr;
	
public:
	
	TSharedPtr<SJointOutlineBorder> BodyBorder;
	
public:

	TArray<TSharedPtr<FNodeStatusItem>> NodeStatusItems;

public:
	
	// Reallocation
	FReply OnReallocateButtonPressed();
	TSharedPtr<SWindow> ReallocationWindow;
	
	FReply OnReimportButtonPressed();
	FReply OnQuickReimportButtonPressed();

	
	FReply OnCleanAllButtonClicked();
	FReply OnCleanInvalidButtonClicked();
	
public:
	
	void RefreshVisual();
	
public:
	
	void BuildNodeStatusItems();
	void BuildFilterString();

public:
	
	FScriptMapHelper GetMapHelpers(
		TSharedPtr<IPropertyHandle> InNodeMappingsHandle,
		FMapProperty*& OutMapProp,
		void*& OutMapContainer
	);
};
