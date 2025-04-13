//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Toolkit/JointEdGraphNodesCustomization.h"

#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Application/SlateWindowHelper.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "JointEdGraph.h"
#include "JointEditorStyle.h"
#include "JointEditorToolkit.h"
#include "JointManager.h"
#include "IDetailsView.h"

#include "Node/JointEdGraphNode.h"
#include "Node/JointNodeBase.h"

#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "JointAdvancedWidgets.h"
#include "JointEditorNodePickingManager.h"
#include "PropertyCustomizationHelpers.h"

#include "Styling/CoreStyle.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GraphNode/JointGraphNodeSharedSlates.h"

#include "Widgets/Notifications/SNotificationList.h"

#include "Windows/WindowsPlatformApplicationMisc.h"


#define LOCTEXT_NAMESPACE "JointManagerEditor"


class FJointEditorToolkit;

UJointManager* JointDetailCustomizationHelpers::GetJointManagerFromNodes(TArray<UObject*> Objs)
{
	UJointManager* Manager = nullptr;

	for (UObject* Obj : Objs)
	{
		if (!Obj || !Obj->GetOuter()) { continue; }

		if (UJointManager* DM = Cast<UJointManager>(Obj->GetOuter())) Manager = DM;

		break;
	}

	return Manager;
}

bool JointDetailCustomizationHelpers::CheckBaseClassForDisplay(TArray<UObject*> Objs, IDetailCategoryBuilder& DataCategory)
{
	UClass* FirstClass = nullptr;

	for (UObject* Obj : Objs)
	{
		if (Obj == nullptr)
		{
			DataCategory.AddCustomRow(LOCTEXT("InvaildData",
			                                  "Data Instance in the graph node is invaild. Please try to refresh the node or remove this graph node."))
				.WholeRowContent()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("InvaildData",
					              "Data Instance in the graph node is invaild. Please try to refresh the node or remove this graph node."))
				];
			return true;
		}

		if (FirstClass == nullptr)
		{
			FirstClass = Obj->GetClass();
		}
		else
		{
			if (FirstClass != Obj->GetClass())
			{
				DataCategory.AddCustomRow(LOCTEXT("NotSameData",
				                                  "Not all of the selected nodes share the same type of parent class."))
					.WholeRowContent()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("NotSameData",
						              "Not all of the selected nodes share the same type of parent class."))
					];
				return true;
			}
		}
	}
	return false;
}

TSharedRef<IDetailCustomization> FJointEdGraphNodesCustomizationBase::MakeInstance()
{
	return MakeShareable(new FJointEdGraphNodesCustomizationBase);
}

bool JointDetailCustomizationHelpers::IsArchetypeOrClassDefaultObject(TWeakObjectPtr<UObject> Object)
{
	if (Object.IsStale() || !Object.IsValid() || Object.Get() == nullptr) return false;

	if (Object->HasAnyFlags(EObjectFlags::RF_ClassDefaultObject) || Object->HasAnyFlags(
		EObjectFlags::RF_ArchetypeObject))
		return true;


	return false;
}

bool JointDetailCustomizationHelpers::HasArchetypeOrClassDefaultObject(TArray<TWeakObjectPtr<UObject>> SelectedObjects)
{
	for (const TWeakObjectPtr<UObject>& Object : SelectedObjects)
	{
		if (Object.IsStale() || !Object.IsValid() || Object.Get() == nullptr) continue;

		if (IsArchetypeOrClassDefaultObject(Object)) return true;
	}

	return false;
}


bool JointDetailCustomizationHelpers::HasArchetypeOrClassDefaultObject(TArray<UObject*> SelectedObjects)
{
	for (const TWeakObjectPtr<UObject>& Object : SelectedObjects)
	{
		if (Object.IsStale() || !Object.IsValid() || Object.Get() == nullptr) continue;

		if (IsArchetypeOrClassDefaultObject(Object)) return true;
	}

	return false;
}


TArray<UObject*> JointDetailCustomizationHelpers::GetNodeInstancesFromGraphNodes(TArray<TWeakObjectPtr<UObject>> SelectedObjects)
{
	TArray<UObject*> Objs;

	for (const TWeakObjectPtr<UObject>& Object : SelectedObjects)
	{
		if (Object.IsStale() || !Object.IsValid() || Object.Get() == nullptr) continue;

		UJointEdGraphNode* TestAsset = Cast<UJointEdGraphNode>(Object.Get());

		// See if this one is good
		if (TestAsset != nullptr && !TestAsset->IsTemplate() && TestAsset->NodeInstance)
		{
			Objs.Add(TestAsset->NodeInstance);
		}
	}

	return Objs;
}

TArray<UObject*> JointDetailCustomizationHelpers::CastToNodeInstance(TArray<TWeakObjectPtr<UObject>> SelectedObjects)
{
	TArray<UObject*> Objs;

	for (const TWeakObjectPtr<UObject>& Object : SelectedObjects)
	{
		if (Object.IsStale() || !Object.IsValid() || Object.Get() == nullptr) continue;

		UJointNodeBase* TestAsset = Cast<UJointNodeBase>(Object.Get());
		// See if this one is good
		if (TestAsset != nullptr) Objs.Add(TestAsset);
	}

	return Objs;
}


//Get the parent Joint manager from the node instances. It will only return a valid Joint manager pointer when the whole nodes return the same Joint manager.
UJointManager* JointDetailCustomizationHelpers::GetParentJointFromNodeInstances(
	TArray<UObject*> InNodeInstanceObjs,
	bool& bFromMultipleManager,
	bool& bFromInvalidJointManagerObject,
	bool& bFromJointManagerItself)
{
	bFromMultipleManager = false;
	bFromInvalidJointManagerObject = false;
	bFromJointManagerItself = false;

	UJointManager* ParentJointManager = nullptr;

	//Node Instances can be UJointNodeBase and UJointManager.
	for (UObject* NodeInstanceObj : InNodeInstanceObjs)
	{
		if (NodeInstanceObj == nullptr || !IsValid(NodeInstanceObj)) continue;

		if (UJointNodeBase* CastedNodeInstance = Cast<UJointNodeBase>(NodeInstanceObj))
		{
			//Check if we have any nodes with invalid Joint manager.
			if (CastedNodeInstance->GetJointManager() == nullptr)
			{
				bFromInvalidJointManagerObject = true;

				break;
			}

			if (ParentJointManager == nullptr)
			{
				ParentJointManager = CastedNodeInstance->GetJointManager();
			}
			//Check if we have nodes from multiple Joint manager.
			else if (CastedNodeInstance->GetJointManager() != ParentJointManager)
			{
				bFromMultipleManager = true;

				break;
			}
		}
		//Check if the NodeInstanceObj itself is a Joint manager.
		else if (UJointManager* CastedJointManagerNodeInstance = Cast<UJointManager>(NodeInstanceObj))
		{
			ParentJointManager = CastedJointManagerNodeInstance;

			bFromJointManagerItself = true;

			break;
		}
	}

	return (bFromMultipleManager || bFromInvalidJointManagerObject) ? nullptr : ParentJointManager;
}


void FJointEdGraphNodesCustomizationBase::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& NodeInstanceCategory = DetailBuilder.EditCategory("Node Instance Data");

	CachedSelectedObjects = DetailBuilder.GetSelectedObjects();

	TArray<UObject*> Objs = JointDetailCustomizationHelpers::GetNodeInstancesFromGraphNodes(CachedSelectedObjects);

	TAttribute<const UClass*> SelectedClass_Attr = TAttribute<const UClass*>::CreateLambda([this]
	{
		UClass* OutClass = nullptr;

		if (CachedSelectedObjects.Num() >= 2)
		{
			return OutClass;
		}

		for (TWeakObjectPtr<UObject> CachedSelectedObject : CachedSelectedObjects)
		{
			if (CachedSelectedObject.IsValid() && CachedSelectedObject.Get() != nullptr)
				return CachedSelectedObject->
					GetClass();
		}

		return OutClass;
	});

	NodeInstanceCategory.AddExternalObjects(
		Objs,
		EPropertyLocation::Common,
		FAddPropertyParams()
		.HideRootObjectNode(true)
	);


	IDetailCategoryBuilder& EditorNodeCategory = DetailBuilder.EditCategory("Editor Node");

	bool bCanShowPinDataProperty = true;

	for (UObject* Obj : Objs)
	{
		if (!Obj) continue;

		if (UJointNodeBase* NodeBase = Cast<UJointNodeBase>(Obj); NodeBase && !NodeBase->
			GetAllowNodeInstancePinControl())
		{
			bCanShowPinDataProperty = false;

			break;
		}
	}

	if (!bCanShowPinDataProperty)
	{
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UJointEdGraphNode, PinData));
	}

	IDetailCategoryBuilder& AdvancedCategory = DetailBuilder.EditCategory("Advanced");
	AdvancedCategory.InitiallyCollapsed(true);
	AdvancedCategory.SetShowAdvanced(true);

	AdvancedCategory.AddCustomRow(LOCTEXT("ChangeEditorNodeClassText", "Change Editor Node Class"))
		.WholeRowWidget
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SJointOutlineBorder)
				.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OutlineNormalColor(FLinearColor(0.04, 0.04, 0.04))
				.OutlineHoverColor(FJointEditorStyle::Color_Selected)
				.ContentMargin(FJointEditorStyle::Margin_Button)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(STextBlock)
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
					.AutoWrapText(true)
					.Text(LOCTEXT("EdNodeClassDataChangeHintText",
					              "Change the class of the editor node.\nIf multiple nodes are selected, it will show none always, but it will work anyway.\nIt will be applied only when the chosen class supports the node instance."))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SClassPropertyEntryBox)
				.AllowNone(false)
				.AllowAbstract(false)
				.MetaClass(UJointEdGraphNode::StaticClass())
				.SelectedClass(SelectedClass_Attr)
				.OnSetClass(this, &FJointEdGraphNodesCustomizationBase::OnSetClass)
			]
		];
}

void FJointEdGraphNodesCustomizationBase::OnSetClass(const UClass* Class)
{
	if (Class != nullptr)
	{
		for (int i = 0; i < CachedSelectedObjects.Num(); ++i)
		{
			if (CachedSelectedObjects[i].Get() == nullptr) continue;

			if (UJointEdGraphNode* CastedGraphNode = Cast<UJointEdGraphNode>(CachedSelectedObjects[i].Get()))
			{
				CastedGraphNode->ReplaceEditorNodeClassTo(Class);
			}
		}
	}
}

UJointNodeBase* GetFirstNodeInstanceFromSelectedNodeInstances(IDetailLayoutBuilder& DetailBuilder,
                                                              TArray<TWeakObjectPtr<UObject>> Objects)
{
	TArray<TWeakObjectPtr<UObject>> NodeInstances = DetailBuilder.GetSelectedObjects();

	if (!NodeInstances.IsEmpty())
	{
		if (const TWeakObjectPtr<UObject> FirstNode = NodeInstances[0]; FirstNode.Get())
		{
			if (UObject* Object = FirstNode.Get())
			{
				return Cast<UJointNodeBase>(Object);
			}
		}
	}

	return nullptr;
}


void SJointNodeInstanceSimpleDisplayPropertyName::Construct(const FArguments& InArgs)
{
	EdNode = InArgs._EdNode;
	NameWidget = InArgs._NameWidget;
	PropertyHandle = InArgs._PropertyHandle;

	TAttribute<const FSlateBrush*> CheckBoxImage = TAttribute<const FSlateBrush*>::CreateLambda([this]
	{
		return
			PropertyHandle.IsValid()
			&& PropertyHandle->IsValidHandle()
			&& PropertyHandle->GetProperty()
			&& EdNode
			&& EdNode->SimpleDisplayHiddenProperties.Contains(PropertyHandle->GetProperty()->GetFName())
				? FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Kismet.VariableList.HideForInstance")
				: FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Kismet.VariableList.ExposeForInstance");
	});

	TAttribute<FSlateColor> CheckBoxColor = TAttribute<FSlateColor>::CreateLambda([this]
	{
		return
			PropertyHandle.IsValid()
			&& PropertyHandle->IsValidHandle()
			&& PropertyHandle->GetProperty()
			&& EdNode
			&& EdNode->SimpleDisplayHiddenProperties.Contains(PropertyHandle->GetProperty()->GetFName())
				? FLinearColor(0.2, 0.2, 0.2)
				: FLinearColor(1, 0.5, 0.2);
	});

	TAttribute<EVisibility> CheckBoxVisibility = TAttribute<EVisibility>::CreateLambda([this]
	{
		return
			EdNode
			&& EdNode->OptionalToolkit.IsValid()
			&& EdNode->OptionalToolkit.Pin()->GetCheckedToggleVisibilityChangeModeForSimpleDisplayProperty()
				? EVisibility::Visible
				: EVisibility::Collapsed;
	});

	this->ChildSlot[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0, 0, 4, 0))
		[
			SNew(SCheckBox)
			.Visibility(CheckBoxVisibility)
			.Style(FJointEditorStyle::GetUEEditorSlateStyleSet(), "TransparentCheckBox")
			.OnCheckStateChanged(this, &SJointNodeInstanceSimpleDisplayPropertyName::OnVisibilityCheckStateChanged)
			[
				SNew(SImage)
				.Visibility(EVisibility::SelfHitTestInvisible)
				.Image(CheckBoxImage)
				.ColorAndOpacity(CheckBoxColor)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			NameWidget.ToSharedRef()
		]
	];
}

void SJointNodeInstanceSimpleDisplayPropertyName::OnVisibilityCheckStateChanged(ECheckBoxState CheckBoxState)
{
	if (EdNode && PropertyHandle->IsValidHandle() && PropertyHandle->GetProperty())
	{
		if (CheckBoxState == ECheckBoxState::Checked)
		{
			EdNode->SimpleDisplayHiddenProperties.Add(PropertyHandle->GetProperty()->GetFName());
		}
		else if (CheckBoxState == ECheckBoxState::Unchecked)
		{
			EdNode->SimpleDisplayHiddenProperties.Remove(PropertyHandle->GetProperty()->GetFName());
		}
	}
}

TSharedRef<IDetailCustomization> FJointNodeInstanceSimpleDisplayCustomizationBase::MakeInstance()
{
	return MakeShareable(new FJointNodeInstanceSimpleDisplayCustomizationBase);
}



void FJointNodeInstanceSimpleDisplayCustomizationBase::AddCategoryProperties(
	UJointEdGraphNode* EdNode, IDetailCategoryBuilder& CategoryBuilder,
	TArray<TSharedRef<IPropertyHandle>> OutAllProperties)
{
	for (TSharedRef<IPropertyHandle> OutPropertyHandle : OutAllProperties)
	{
		if (!OutPropertyHandle->IsValidHandle()) continue;
		if (!OutPropertyHandle->GetProperty()) continue;
		
		TAttribute<EVisibility> RowVisibility = TAttribute<EVisibility>::CreateLambda([EdNode, OutPropertyHandle, this]
		{
			return OutPropertyHandle->IsValidHandle()
			       && OutPropertyHandle->GetProperty()
			       && EdNode
			       &&
			       (
				       !EdNode->SimpleDisplayHiddenProperties.Contains(OutPropertyHandle->GetProperty()->GetFName())
				       ||
				       EdNode->OptionalToolkit.IsValid() && EdNode->OptionalToolkit.Pin()->
				       GetCheckedToggleVisibilityChangeModeForSimpleDisplayProperty()
			       )
				       ? EVisibility::Visible
				       : EVisibility::Collapsed;
		});

		TAttribute<const FSlateBrush*> CheckBoxImage = TAttribute<const FSlateBrush*>::CreateLambda(
			[EdNode, OutPropertyHandle, this]
			{
				return OutPropertyHandle->IsValidHandle()
				       && OutPropertyHandle->GetProperty()
				       && EdNode
				       && EdNode->SimpleDisplayHiddenProperties.Contains(OutPropertyHandle->GetProperty()->GetFName())
					       ? FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(
						       "Kismet.VariableList.ExposeForInstance")
					       : FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(
						       "Kismet.VariableList.HideForInstance");
			});

		IDetailPropertyRow& Row = CategoryBuilder.AddProperty(OutPropertyHandle);

		Row.Visibility(RowVisibility);

		TSharedPtr<SWidget> OutNameWidget;
		TSharedPtr<SWidget> OutValueWidget;

		Row.GetDefaultWidgets(OutNameWidget, OutValueWidget);

		Row.CustomWidget(/*bShowChildren*/ true)
			.NameContent()
			[
				SNew(SJointNodeInstanceSimpleDisplayPropertyName)
				.NameWidget(OutNameWidget.ToSharedRef())
				.EdNode(EdNode)
				.PropertyHandle(OutPropertyHandle)
			]
			.ValueContent()
			[
				OutValueWidget.ToSharedRef()
			]
			.ExtensionContent()
			[
				SNullWidget::NullWidget
			];
	}
}

void FJointNodeInstanceSimpleDisplayCustomizationBase::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	UJointEdGraphNode* EdNode = nullptr;

	if (UJointNodeBase* NodeInstance = GetFirstNodeInstanceFromSelectedNodeInstances(
		DetailBuilder, DetailBuilder.GetSelectedObjects()))
	{
		if (NodeInstance->EdGraphNode.Get())
		{
			EdNode = Cast<UJointEdGraphNode>(NodeInstance->EdGraphNode.Get());
		}
	}

	if (!EdNode) return;


	TArray<FName> OutCategoryNames;

	DetailBuilder.GetCategoryNames(OutCategoryNames);

	for (const FName& OutCategoryName : OutCategoryNames)
	{
		IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory(OutCategoryName);

		//CategoryBuilder.InitiallyCollapsed(EdNode->SimpleDisplayNotExpandingCategory.Contains(OutCategoryName));
		CategoryBuilder.RestoreExpansionState(false);

		TArray<TSharedRef<IPropertyHandle>> OutAllProperties;

		CategoryBuilder.GetDefaultProperties(OutAllProperties);

		AddCategoryProperties(EdNode, CategoryBuilder, OutAllProperties);
	}
}

void FJointNodeInstanceSimpleDisplayCustomizationBase::CustomizeDetails(
	const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	CustomizeDetails(*DetailBuilder);
}

TSharedRef<IDetailCustomization> FJointNodeInstanceCustomizationBase::MakeInstance()
{
	return MakeShareable(new FJointNodeInstanceCustomizationBase);
}

void FJointNodeInstanceCustomizationBase::HideDisableEditOnInstanceProperties(
	IDetailLayoutBuilder& DetailBuilder, TArray<UObject*> NodeInstances)
{
	for (UObject* Object : NodeInstances)
	{
		if (Object == nullptr) continue;

		if (JointDetailCustomizationHelpers::IsArchetypeOrClassDefaultObject(Object)) continue;

		for (TFieldIterator<FProperty> PropIt(Object->GetClass()); PropIt; ++PropIt)
		{
			if (!PropIt->IsValidLowLevel()) continue;

			if (!PropIt->HasAnyPropertyFlags(CPF_DisableEditOnInstance)) continue;

			TSharedRef<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(
				*PropIt->GetName(), Object->GetClass());

			PropertyHandle->MarkHiddenByCustomization();
		}
	}
}

TSharedRef<IDetailCustomization> FJointBuildPresetCustomization::MakeInstance()
{
	return MakeShareable(new FJointBuildPresetCustomization());
}

void FJointBuildPresetCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& DescriptionCategory = DetailBuilder.EditCategory("Description");
	DescriptionCategory.AddCustomRow(INVTEXT("Explanation"))
		.WholeRowContent()
		[
			SNew(SJointOutlineBorder)
			.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OutlineNormalColor(FLinearColor(0.04, 0.04, 0.04))
			.OutlineHoverColor(FJointEditorStyle::Color_Selected)
			.ContentMargin(FJointEditorStyle::Margin_Button)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(STextBlock)
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
					.AutoWrapText(true)
					.Text(LOCTEXT("FJointBuildPresetCustomization_Explanation", "We supports client & server preset, but those are not that recommended to use because using only those two has limitation on making a game that can be standalone & multiplayer together.\nIf you set any of those to \'exclude\' then standalone section will not contain the nodes with that preset.\nFor that cases, using build target based including & excluding will help you better. just make 3 different build target for each sessions (standalone (game), client, server) and set the behavior for each of them."))
			]
		];
}

void FJointNodeInstanceCustomizationBase::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<UObject*> NodeInstances = JointDetailCustomizationHelpers::GetNodeInstancesFromGraphNodes(DetailBuilder.GetSelectedObjects());

	//If it was empty, try to grab the node instances by itself.
	if (NodeInstances.IsEmpty())
	{
		NodeInstances = JointDetailCustomizationHelpers::CastToNodeInstance(DetailBuilder.GetSelectedObjects());
	}

	//Display class description for the nodes when all the nodes are instanced.
	if (!JointDetailCustomizationHelpers::HasArchetypeOrClassDefaultObject(NodeInstances))
	{
		TSet<UClass*> ClassesToDescribe;

		for (UObject* Obj : NodeInstances)
		{
			if (Obj != nullptr)
			{
				if (!ClassesToDescribe.Contains(Obj->GetClass())) ClassesToDescribe.Add(Obj->GetClass());
			}
		}

		TSharedPtr<SVerticalBox> DescriptionBox;

		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Description");

		Category.AddCustomRow(LOCTEXT("NodeInstanceDetailDescription", "Description"))
			.WholeRowWidget
			[
				SAssignNew(DescriptionBox, SVerticalBox)
			];

		for (UClass* ToDescribe : ClassesToDescribe)
		{
			DescriptionBox->AddSlot()
				.AutoHeight()
				[
					SNew(SJointNodeDescription)
					.ClassToDescribe(ToDescribe)
				];
		}
	}
	else
	{
		//DetailBuilder.HideCategory("Description");
	}

	//Display class description for the nodes when all the nodes are instanced.
	if (!JointDetailCustomizationHelpers::HasArchetypeOrClassDefaultObject(NodeInstances))
	{
		DetailBuilder.HideCategory("Editor Node");
	}

	//Hide the properties that are not instance editable.
	HideDisableEditOnInstanceProperties(DetailBuilder, NodeInstances);

	/*
	IDetailCategoryBuilder& AdvancedCategory = DetailBuilder.EditCategory("Advanced");

	for (UObject* NodeInstance : NodeInstances)
	{
		if(!NodeInstance) continue;
		
		AdvancedCategory.AddCustomRow(LOCTEXT("OuterRow","Outer"))
		.ValueContent()
		[
			SNew(STextBlock)
			.Text(NodeInstance->GetOuter() ? FText::FromString(NodeInstance->GetOuter()->GetName()) : LOCTEXT("OuterNull","Nullptr"))
		];
	}
	*/
}


TSharedRef<IPropertyTypeCustomization> FJointNodePointerStructCustomization::MakeInstance()
{
	return MakeShareable(new FJointNodePointerStructCustomization);
}

void FJointNodePointerStructCustomization::CustomizeStructHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle,
                                                                 FDetailWidgetRow& HeaderRow,
                                                                 IPropertyTypeCustomizationUtils&
                                                                 StructCustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle;

	NodeHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointNodePointer, Node));
	EditorNodeHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointNodePointer, EditorNode));

	AllowedTypeHandle = StructPropertyHandle->
		GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointNodePointer, AllowedType));
	DisallowedTypeHandle = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FJointNodePointer, DisallowedType));
	
	HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		].ValueContent()
		[
			StructPropertyHandle->CreatePropertyValueWidget(false)
		];
}

void FJointNodePointerStructCustomization::CustomizeStructChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle,
                                                                   IDetailChildrenBuilder& StructBuilder,
                                                                   IPropertyTypeCustomizationUtils&
                                                                   StructCustomizationUtils)
{
	//Grab the object that holds this structure.
	NodeInstanceObjs = JointDetailCustomizationHelpers::GetNodeInstancesFromGraphNodes(
		StructBuilder.GetParentCategory().GetParentLayout().GetSelectedObjects());

	//If it was empty, try to grab the node instances by itself.
	if (NodeInstanceObjs.IsEmpty())
		NodeInstanceObjs = JointDetailCustomizationHelpers::CastToNodeInstance(
			StructBuilder.GetParentCategory().GetParentLayout().GetSelectedObjects());

	bool bFromMultipleManager = false;
	bool bFromInvalidJointManagerObject = false;
	bool bFromJointManagerItself = false;

	FText PickingTooltipText =
		bFromInvalidJointManagerObject
			? LOCTEXT("NodePickUpDeniedToolTip_null",
			          "Some of the objects you selected doesn't have a valid outermost object.")
			: bFromMultipleManager
			? LOCTEXT("NodePickUpDeniedToolTip_Multiple", "Selected objects must be from the same Joint manager.")
			: LOCTEXT("NodePickUpToolTip", "click to pick-up the node.");


	JointDetailCustomizationHelpers::GetParentJointFromNodeInstances(NodeInstanceObjs, bFromMultipleManager, bFromInvalidJointManagerObject,
	                                bFromJointManagerItself);


	//IDetailGroup& Group = StructBuilder.
	//	AddGroup("NodePicker", StructPropertyHandle.Get()->GetPropertyDisplayName());
	// Begin a new line.  All properties below this call will be added to the same line until EndLine() or another BeginLine() is called

	StructBuilder.AddProperty(NodeHandle.ToSharedRef())
		.CustomWidget()
		.WholeRowContent()
		.HAlign(HAlign_Fill)
		[
			SNew(SJointOutlineBorder)
			.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OutlineNormalColor(FLinearColor(0.04, 0.04, 0.04))
			.OutlineHoverColor(FJointEditorStyle::Color_Selected)
			.ContentMargin(FJointEditorStyle::Margin_PinGap)
			.OnHovered(this, &FJointNodePointerStructCustomization::OnMouseHovered)
			.OnUnhovered(this, &FJointNodePointerStructCustomization::OnMouseUnhovered)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SAssignNew(BackgroundBox, SVerticalBox)
					.RenderOpacity(1)
					.Visibility(EVisibility::SelfHitTestInvisible)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Frame * 0.5)
					[
						NodeHandle.Get()->CreatePropertyValueWidget(true)
					]
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SAssignNew(ButtonBox, SHorizontalBox)
					.Visibility(EVisibility::Collapsed)
					+ SHorizontalBox::Slot()
					.FillWidth(1)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SJointOutlineButton)
						.NormalColor(FLinearColor::Transparent)
						.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
						.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.OutlineNormalColor(FLinearColor::Transparent)
						.ContentMargin(FJointEditorStyle::Margin_Button)
						.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
						.ToolTipText(PickingTooltipText)
						.IsEnabled(!(bFromMultipleManager || bFromInvalidJointManagerObject || bFromJointManagerItself))
						.OnPressed(this, &FJointNodePointerStructCustomization::OnNodePickUpButtonPressed)
						[
							SNew(SImage)
							.DesiredSizeOverride(FVector2D(20, 20))
							.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.EyeDropper"))
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SJointOutlineButton)
						.NormalColor(FLinearColor::Transparent)
						.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
						.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.OutlineNormalColor(FLinearColor::Transparent)
						.ContentMargin(FJointEditorStyle::Margin_Button)
						.ToolTipText(LOCTEXT("GotoTooltip", "Go to the seleced node on the graph."))
						.IsEnabled(
							!(bFromMultipleManager || bFromInvalidJointManagerObject ||
								bFromJointManagerItself))
						.OnPressed(this, &FJointNodePointerStructCustomization::OnGoToButtonPressed)
						[

							SNew(SImage)
							.DesiredSizeOverride(FVector2D(20, 20))
							.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.ArrowRight"))
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SJointOutlineButton)
						.NormalColor(FLinearColor::Transparent)
						.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
						.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.OutlineNormalColor(FLinearColor::Transparent)
						.ContentMargin(FJointEditorStyle::Margin_Button)
						.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
						.ToolTipText(LOCTEXT("CopyTooltip", "Copy the structure to the clipboard"))
						.IsEnabled(
							!(bFromMultipleManager || bFromInvalidJointManagerObject ||
								bFromJointManagerItself))
						.OnPressed(this, &FJointNodePointerStructCustomization::OnCopyButtonPressed)
						[
							SNew(SImage)
							.DesiredSizeOverride(FVector2D(20, 20))
							.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("GenericCommands.Copy"))
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SJointOutlineButton)
						.NormalColor(FLinearColor::Transparent)
						.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
						.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.OutlineNormalColor(FLinearColor::Transparent)
						.ContentMargin(FJointEditorStyle::Margin_Button)
						.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
						.ToolTipText(LOCTEXT("PasteTooltip", "Paste the structure from the clipboard"))
						.IsEnabled(
							!(bFromMultipleManager || bFromInvalidJointManagerObject ||
								bFromJointManagerItself))
						.OnPressed(this, &FJointNodePointerStructCustomization::OnPasteButtonPressed)
						[
							SNew(SImage)
							.DesiredSizeOverride(FVector2D(20, 20))
							.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("GenericCommands.Paste"))
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SJointOutlineButton)
						.NormalColor(FLinearColor::Transparent)
						.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
						.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
						.OutlineNormalColor(FLinearColor::Transparent)
						.ContentMargin(FJointEditorStyle::Margin_Button)
						.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
						.ToolTipText(LOCTEXT("ResetTooltip", "Reset the structure"))
						.IsEnabled(
							!(bFromMultipleManager || bFromInvalidJointManagerObject ||
								bFromJointManagerItself))
						.OnPressed(this, &FJointNodePointerStructCustomization::OnClearButtonPressed)
						[
							SNew(SImage)
							.DesiredSizeOverride(FVector2D(20, 20))
							.ColorAndOpacity(FLinearColor(1,0.5,0.3))
							.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Unlink"))
						]
					]
				]
			]
		];

	if (NodeInstanceObjs.IsEmpty())
	{
		StructBuilder.AddProperty(AllowedTypeHandle.ToSharedRef());
		StructBuilder.AddProperty(DisallowedTypeHandle.ToSharedRef());
	}

	FSimpleDelegate OnDataChanged = FSimpleDelegate::CreateSP(
		this, &FJointNodePointerStructCustomization::OnNodeDataChanged);

	FSimpleDelegate OnNodeResetTo = FSimpleDelegate::CreateSP(
		this, &FJointNodePointerStructCustomization::OnNodeResetToDefault);

	NodeHandle.Get()->SetOnPropertyValueChanged(OnDataChanged);
	NodeHandle.Get()->SetOnPropertyResetToDefault(OnNodeResetTo);
}

void FJointNodePointerStructCustomization::OnNodePickUpButtonPressed()
{
	bool bFromMultipleManager = false;
	bool bFromInvalidJointManagerObject = false;
	bool bFromJointManagerItself = false;

	UJointManager* FoundJointManager = JointDetailCustomizationHelpers::GetParentJointFromNodeInstances(
		NodeInstanceObjs, bFromMultipleManager, bFromInvalidJointManagerObject, bFromJointManagerItself);

	//Halt if the Joint manager is not valid.
	if (FoundJointManager == nullptr) return;

	if (FJointEditorToolkit* Toolkit = FJointEditorToolkit::FindOrOpenEditorInstanceFor(FoundJointManager))
	{
		if(!Toolkit->GetNodePickingManager().IsValid()) return;
		
		if (!Toolkit->GetNodePickingManager()->IsInNodePicking())
		{
			Request = Toolkit->GetNodePickingManager()->StartNodePicking(NodeHandle, EditorNodeHandle);
		}
		else
		{
			Toolkit->GetNodePickingManager()->EndNodePicking();
		}
	}
}


void FJointNodePointerStructCustomization::OnGoToButtonPressed()
{
	UObject* CurrentNode = nullptr;

	NodeHandle.Get()->GetValue(CurrentNode);

	//Revert
	if (CurrentNode == nullptr) return;

	bool bFromMultipleManager = false;
	bool bFromInvalidJointManagerObject = false;
	bool bFromJointManagerItself = false;

	UJointManager* FoundJointManager = JointDetailCustomizationHelpers::GetParentJointFromNodeInstances(
		NodeInstanceObjs, bFromMultipleManager, bFromInvalidJointManagerObject, bFromJointManagerItself);

	//Halt if the Joint manager is not valid.
	if (FoundJointManager == nullptr) return;

	if (FJointEditorToolkit* Toolkit = FJointEditorToolkit::FindOrOpenEditorInstanceFor(FoundJointManager))
	{
		if (Toolkit->GetJointManager() && Toolkit->GetJointManager()->JointGraph)
		{
			if (UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(Toolkit->GetJointManager()->JointGraph))
			{
				TSet<TSoftObjectPtr<UJointEdGraphNode>> GraphNodes = CastedGraph->GetCacheJointGraphNodes();

				for (TSoftObjectPtr<UJointEdGraphNode> Node : GraphNodes)
				{
					if (!Node.IsValid() || Node->GetCastedNodeInstance() == nullptr) continue;

					if (Node->GetCastedNodeInstance() == CurrentNode)
					{
						Toolkit->JumpToNode(Node.Get());

						Toolkit->StartHighlightingNode(Node.Get(), true);
						
						// if(!Toolkit->GetNodePickingManager().IsValid()) return;
						//
						// if (!Toolkit->GetNodePickingManager()->IsInNodePicking() || Toolkit->GetNodePickingManager()->GetActiveRequest() != Request)
						// {
						// 	Toolkit->StartHighlightingNode(Node.Get(), true);
						// }
						
						break;
					}
				}
			}
		}
	}
}

void FJointNodePointerStructCustomization::OnCopyButtonPressed()
{
	FString Value;
	if (NodeHandle->GetValueAsFormattedString(Value, PPF_Copy) == FPropertyAccess::Success)
	{
		FPlatformApplicationMisc::ClipboardCopy(*Value);
	}

	bool bFromMultipleManager = false;
	bool bFromInvalidJointManagerObject = false;
	bool bFromJointManagerItself = false;

	UJointManager* FoundJointManager = JointDetailCustomizationHelpers::GetParentJointFromNodeInstances(
		NodeInstanceObjs, bFromMultipleManager, bFromInvalidJointManagerObject, bFromJointManagerItself);

	//Halt if the Joint manager is not valid.
	if (FoundJointManager == nullptr) return;

	if (FJointEditorToolkit* Toolkit = FJointEditorToolkit::FindOrOpenEditorInstanceFor(FoundJointManager))
	{
		Toolkit->PopulateNodePickerCopyToastMessage();
	}
}

void FJointNodePointerStructCustomization::OnPasteButtonPressed()
{
	FString Value;

	FPlatformApplicationMisc::ClipboardPaste(Value);

	NodeHandle->SetValueFromFormattedString(Value, PPF_Copy);
	
	OnNodeDataChanged();

	bool bFromMultipleManager = false;
	bool bFromInvalidJointManagerObject = false;
	bool bFromJointManagerItself = false;

	UJointManager* FoundJointManager = JointDetailCustomizationHelpers::GetParentJointFromNodeInstances(
		NodeInstanceObjs, bFromMultipleManager, bFromInvalidJointManagerObject, bFromJointManagerItself);

	//Halt if the Joint manager is not valid.
	if (FoundJointManager == nullptr) return;

	if (FJointEditorToolkit* Toolkit = FJointEditorToolkit::FindOrOpenEditorInstanceFor(FoundJointManager))
	{
		Toolkit->PopulateNodePickerPastedToastMessage();
	}
}

void FJointNodePointerStructCustomization::OnClearButtonPressed()
{
	if(NodeHandle) NodeHandle->ResetToDefault();
	if(EditorNodeHandle) EditorNodeHandle->ResetToDefault();
}

void FJointNodePointerStructCustomization::OnNodeDataChanged()
{
	UObject* CurrentNode = nullptr;

	NodeHandle.Get()->GetValue(CurrentNode);

	//Abort if the node is not valid.
	if (!CurrentNode) return;

	if (!Cast<UJointNodeBase>(CurrentNode))
	{
		NodeHandle.Get()->ResetToDefault();

		FText FailedNotificationText = LOCTEXT("NotJointNodeInstanceType", "Node Pick Up Canceled");
		FText FailedNotificationSubText = LOCTEXT("NotJointNodeInstanceType_Sub",
		                                          "Provided node instance was not UJointNodeBase type. Pointer reseted.");

		FNotificationInfo NotificationInfo(FailedNotificationText);
		NotificationInfo.SubText = FailedNotificationSubText;
		NotificationInfo.Image = FJointEditorStyle::Get().GetBrush("JointUI.Image.JointManager");
		NotificationInfo.bFireAndForget = true;
		NotificationInfo.FadeInDuration = 0.2f;
		NotificationInfo.FadeOutDuration = 0.2f;
		NotificationInfo.ExpireDuration = 2.5f;
		NotificationInfo.bUseThrobber = true;

		FSlateNotificationManager::Get().AddNotification(NotificationInfo);

		return;
	}

	void* Struct = nullptr;

	StructPropertyHandle->GetValueData(Struct);

	if (Struct != nullptr)
	{
		FJointNodePointer* Casted = static_cast<FJointNodePointer*>(Struct);

		if (!FJointNodePointer::CanSetNodeOnProvidedJointNodePointer(
			*Casted, Cast<UJointNodeBase>(CurrentNode)))
		{
			NodeHandle.Get()->ResetToDefault();

			FString AllowedTypeStr;
			for (TSubclassOf<UJointNodeBase> AllowedType : Casted->AllowedType)
			{
				if (AllowedType == nullptr) continue;
				if (!AllowedTypeStr.IsEmpty()) AllowedTypeStr.Append(", ");
				AllowedTypeStr.Append(AllowedType.Get()->GetName());
			}

			FString DisallowedTypeStr;
			for (TSubclassOf<UJointNodeBase> DisallowedType : Casted->DisallowedType)
			{
				if (DisallowedType == nullptr) continue;
				if (!DisallowedTypeStr.IsEmpty()) DisallowedTypeStr.Append(", ");
				DisallowedTypeStr.Append(DisallowedType.Get()->GetName());
			}

			FText FailedNotificationText = LOCTEXT("NotJointNodeInstanceType", "Node Pick Up Canceled");
			FText FailedNotificationSubText = FText::Format(
				LOCTEXT("NotJointNodeInstanceType_Sub",
				        "Current structure can not have the provided node type. Pointer reseted.\n\nAllowd Types: {0}\nDisallowed Types: {1}"),
				FText::FromString(AllowedTypeStr),
				FText::FromString(DisallowedTypeStr));

			FNotificationInfo NotificationInfo(FailedNotificationText);
			NotificationInfo.SubText = FailedNotificationSubText;
			NotificationInfo.Image = FJointEditorStyle::Get().
				GetBrush("JointUI.Image.JointManager");
			NotificationInfo.bFireAndForget = true;
			NotificationInfo.FadeInDuration = 0.2f;
			NotificationInfo.FadeOutDuration = 0.2f;
			NotificationInfo.ExpireDuration = 2.5f;
			NotificationInfo.bUseThrobber = true;

			FSlateNotificationManager::Get().AddNotification(NotificationInfo);
		}
	}
}

void FJointNodePointerStructCustomization::OnNodeResetToDefault()
{
	if (EditorNodeHandle.IsValid()) EditorNodeHandle->ResetToDefault();
}

void FJointNodePointerStructCustomization::OnMouseHovered()
{
	if(BackgroundBox.IsValid()) BackgroundBox->SetRenderOpacity(0.5);
	if(ButtonBox.IsValid()) ButtonBox->SetVisibility(EVisibility::SelfHitTestInvisible);
		
	UObject* CurrentNode = nullptr;

	NodeHandle.Get()->GetValue(CurrentNode);

	//Revert
	if (CurrentNode == nullptr) return;

	bool bFromMultipleManager = false;
	bool bFromInvalidJointManagerObject = false;
	bool bFromJointManagerItself = false;

	UJointManager* FoundJointManager = JointDetailCustomizationHelpers::GetParentJointFromNodeInstances(
		NodeInstanceObjs, bFromMultipleManager, bFromInvalidJointManagerObject, bFromJointManagerItself);

	//Halt if the Joint manager is not valid.
	if (FoundJointManager == nullptr) return;

	IAssetEditorInstance* EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(
		FoundJointManager, true);

	if (EditorInstance == nullptr)
	{
		TArray<UObject*> Objects;

		Objects.Add(FoundJointManager);

		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(Objects);

		EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(
			FoundJointManager, true);

		return;
	}

	FJointEditorToolkit* Toolkit = static_cast<FJointEditorToolkit*>(EditorInstance);

	if (!Toolkit->GetJointManager() || !Toolkit->GetJointManager()->JointGraph) return;

	if (UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(Toolkit->GetJointManager()->JointGraph))
	{
		TSet<TSoftObjectPtr<UJointEdGraphNode>> GraphNodes = CastedGraph->GetCacheJointGraphNodes();

		for (TSoftObjectPtr<UJointEdGraphNode> Node : GraphNodes)
		{
			if (!Node.IsValid() || Node->GetCastedNodeInstance() == nullptr) continue;

			if (Node->GetCastedNodeInstance() == CurrentNode)
			{

				Toolkit->StartHighlightingNode(Node.Get(), false);
				
				// if(!Toolkit->GetNodePickingManager().IsValid()) return;
				//
				// if (!Toolkit->GetNodePickingManager()->IsInNodePicking() || Toolkit->GetNodePickingManager()->GetActiveRequest() != Request)
				// {
				// 	Toolkit->StartHighlightingNode(Node.Get(), false);
				// }
				
				break;
			}
		}
	}
}

void FJointNodePointerStructCustomization::OnMouseUnhovered()
{
	if(BackgroundBox.IsValid()) BackgroundBox->SetRenderOpacity(1);
	if(ButtonBox.IsValid()) ButtonBox->SetVisibility(EVisibility::Collapsed);
	
	UObject* CurrentNode = nullptr;

	NodeHandle.Get()->GetValue(CurrentNode);

	//Revert
	if (CurrentNode == nullptr) return;

	bool bFromMultipleManager = false;
	bool bFromInvalidJointManagerObject = false;
	bool bFromJointManagerItself = false;

	UJointManager* FoundJointManager = JointDetailCustomizationHelpers::GetParentJointFromNodeInstances(
		NodeInstanceObjs, bFromMultipleManager, bFromInvalidJointManagerObject, bFromJointManagerItself);


	//Abort if we couldn't find a valid Joint manager.
	if (FoundJointManager == nullptr) return;

	IAssetEditorInstance* EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(
		FoundJointManager, true);


	if (EditorInstance == nullptr)
	{
		TArray<UObject*> Objects;

		Objects.Add(FoundJointManager);

		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(Objects);

		EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(
			FoundJointManager, true);

		return;
	}

	//Guaranteed to have a valid pointer.
	FJointEditorToolkit* Toolkit = static_cast<FJointEditorToolkit*>(EditorInstance);

	if (!Toolkit->GetJointManager() || !Toolkit->GetJointManager()->JointGraph)return;

	if (UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(Toolkit->GetJointManager()->JointGraph))
	{
		TSet<TSoftObjectPtr<UJointEdGraphNode>> GraphNodes = CastedGraph->GetCacheJointGraphNodes();

		for (TSoftObjectPtr<UJointEdGraphNode> Node : GraphNodes)
		{
			if (!Node.IsValid() || Node->GetCastedNodeInstance() == nullptr) continue;

			if (Node->GetCastedNodeInstance() == CurrentNode)
			{
				Toolkit->StopHighlightingNode(Node.Get());
				
				// if(!Toolkit->GetNodePickingManager().IsValid()) return;
				//
				// if (!Toolkit->GetNodePickingManager()->IsInNodePicking() || Toolkit->GetNodePickingManager()->GetActiveRequest() != Request)
				// {
				// 	Toolkit->StopHighlightingNode(Node.Get());
				// }
				break;
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
