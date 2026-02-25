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
#include "IPropertyUtilities.h"
#include "JointAdvancedWidgets.h"
#include "JointEdGraphNode_Fragment.h"
#include "JointEditorNodePickingManager.h"
#include "JointEditorSettings.h"
#include "JointEditorToolkitToastMessages.h"
#include "JointEdUtils.h"
#include "JointNodePreset.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "VoltDecl.h"

#include "EditorTools/SJointNotificationWidget.h"
#include "EditorWidget/JointGraphEditor.h"
#include "EditorWidget/SJointGraphPanel.h"
#include "EditorWidget/SJointGraphPreviewer.h"
#include "EditorWidget/SJointManagerImportingPopup.h"

#include "Styling/CoreStyle.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GraphNode/JointGraphNodeSharedSlates.h"
#include "Misc/MessageDialog.h"
#include "Node/JointFragment.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SScrollBox.h"

#include "Widgets/Notifications/SNotificationList.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Markdown/SJointMDSlate_Admonitions.h"
#include "Module/Volt_ASM_InterpColor.h"
#include "Module/Volt_ASM_Sequence.h"
#include "Module/Volt_ASM_InterpBackgroundColor.h"

#include "Misc/EngineVersionComparison.h"

#if UE_VERSION_OLDER_THAN(5, 4, 0)
#include "PropertyEditor/Private/PropertyNode.h"
#else

#endif


#include "Script/JointScriptSettings.h"
#include "Widgets/Layout/SExpandableArea.h"


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

bool JointDetailCustomizationHelpers::CheckBaseClassForDisplay(TArray<UObject*> Objs,
                                                               IDetailCategoryBuilder& DataCategory)
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


TArray<UObject*> JointDetailCustomizationHelpers::GetNodeInstancesFromGraphNodes(
	TArray<TWeakObjectPtr<UObject>> SelectedObjects)
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


TSharedRef<IDetailCustomization> FJointEdGraphNodesCustomizationBase::MakeInstance()
{
	return MakeShareable(new FJointEdGraphNodesCustomizationBase);
}


void FJointEdGraphNodesCustomizationBase::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& NodeInstanceCategory = DetailBuilder.EditCategory("Node Instance Data");

	CachedSelectedObjects = DetailBuilder.GetSelectedObjects();

	TArray<UObject*> NodeInstanceObjs = JointDetailCustomizationHelpers::GetNodeInstancesFromGraphNodes(CachedSelectedObjects);

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
				if (UJointEdGraphNode* GraphNode = Cast<UJointEdGraphNode>(CachedSelectedObject.Get()))
				{
					return GraphNode->GetCastedNodeInstance()
						       ? GraphNode->GetCastedNodeInstance()->GetClass()
						       : nullptr;
				}
		}

		return OutClass;
	});

	TAttribute<const UClass*> SelectedEditorClass_Attr = TAttribute<const UClass*>::CreateLambda([this]
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
		NodeInstanceObjs,
		EPropertyLocation::Default,
		FAddPropertyParams()
		.HideRootObjectNode(true)
	);


	IDetailCategoryBuilder& EditorNodeCategory = DetailBuilder.EditCategory("Editor Node");

	if (!CheckIfEveryNodeAllowPinDataControl(NodeInstanceObjs))
	{
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UJointEdGraphNode, PinData));
	}


	TSet<UJointNodeBase*> AllDissolvedFragments;

	for (UObject* NodeInstanceObj : NodeInstanceObjs)
	{
		if (!NodeInstanceObj) continue;

		UJointNodeBase* NodeBase = Cast<UJointNodeBase>(NodeInstanceObj);

		if (!NodeBase || !NodeBase->GetJointManager() || !NodeBase->GetJointManager()->JointGraph) continue;

		UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(NodeBase->GetJointManager()->JointGraph);

		if (CastedGraph == nullptr) continue;

		TArray<UJointFragment*> Subnodes = NodeBase->GetAllFragmentsOnLowerHierarchy();

		for (UJointFragment* Subnode : Subnodes)
		{
			if (!Subnode) continue;

			UEdGraphNode* SubnodeGraphNode = CastedGraph->FindGraphNodeForNodeInstance(Subnode);

			if (!SubnodeGraphNode) continue;

			if (UJointEdGraphNode_Fragment* CastedSubnodeGraphNode = Cast<UJointEdGraphNode_Fragment>(SubnodeGraphNode))
			{
				if (CastedSubnodeGraphNode->IsDissolvedSubNode()) AllDissolvedFragments.Add(Subnode);
			}
		}
	}

	TArray<UObject*> DissolvedFragmentsArray;

	for (UJointNodeBase* Item : AllDissolvedFragments.Array())
	{
		DissolvedFragmentsArray.Add(Item);
	}

	if (!DissolvedFragmentsArray.IsEmpty())
	{
		IDetailCategoryBuilder& DissolvedCategory = DetailBuilder.EditCategory("Dissolved Fragments");

		DissolvedCategory.AddExternalObjects(DissolvedFragmentsArray);
	}


	bool bCanShowReplaceNodeContext = false;
	bool bCanShowReplaceEditorNodeContext = false;

	for (TWeakObjectPtr<UObject> CachedSelectedObject : CachedSelectedObjects)
	{
		if (CachedSelectedObject.IsValid() && CachedSelectedObject.Get() != nullptr)
			if (UJointEdGraphNode* GraphNode = Cast<UJointEdGraphNode>(CachedSelectedObject.Get()))
			{
				bCanShowReplaceNodeContext |= GraphNode->CanReplaceNodeClass();
				bCanShowReplaceEditorNodeContext |= GraphNode->CanReplaceEditorNodeClass();
			}

		if (bCanShowReplaceNodeContext && bCanShowReplaceEditorNodeContext) break;
	}

	IDetailCategoryBuilder& AdvancedCategory = DetailBuilder.EditCategory("Advanced");
	AdvancedCategory.InitiallyCollapsed(true);
	AdvancedCategory.SetShowAdvanced(true);

	if (bCanShowReplaceNodeContext)
	{
		AdvancedCategory.AddCustomRow(LOCTEXT("ChangeNodeClassText", "Change Node Class"))
		                .WholeRowWidget
		[
			SNew(SJointOutlineBorder)
			.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OutlineNormalColor(FLinearColor(0.04, 0.04, 0.04))
			.OutlineHoverColor(FJointEditorStyle::Color_Selected)
			.ContentPadding(FJointEditorStyle::Margin_Large)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.Padding(FJointEditorStyle::Margin_Tiny)
				[
					SNew(STextBlock)
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h2")
					.AutoWrapText(true)
					.Text(LOCTEXT("NodeClassDataChangeHintTextTitle", "Change the node instance's class."))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.Padding(FJointEditorStyle::Margin_Tiny)
				[
					SNew(STextBlock)
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
					.AutoWrapText(true)
					.Text(LOCTEXT("NodeClassDataChangeHintText",
					              "If multiple nodes are selected, it will show none always, but it will work anyway."))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Tiny)
				[
					SNew(SClassPropertyEntryBox)
					.AllowNone(false)
					.AllowAbstract(false)
					.MetaClass(UJointNodeBase::StaticClass())
					.SelectedClass(SelectedClass_Attr)
					.OnSetClass(this, &FJointEdGraphNodesCustomizationBase::OnChangeNodeSetClass)
				]
			]
		];
	}

	if (bCanShowReplaceEditorNodeContext)
	{
		AdvancedCategory.AddCustomRow(LOCTEXT("ChangeEditorNodeClassText", "Change Editor Node Class"))
		                .WholeRowWidget
		[
			SNew(SJointOutlineBorder)
			.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OutlineNormalColor(FLinearColor(0.04, 0.04, 0.04))
			.OutlineHoverColor(FJointEditorStyle::Color_Selected)
			.ContentPadding(FJointEditorStyle::Margin_Large)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.Padding(FJointEditorStyle::Margin_Tiny)
				[
					SNew(STextBlock)
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Black.h2")
					.AutoWrapText(true)
					.Text(LOCTEXT("EdNodeClassDataChangeHintTextTitle", "Change the class of the editor node."))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.Padding(FJointEditorStyle::Margin_Tiny)
				[
					SNew(STextBlock)
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
					.AutoWrapText(true)
					.Text(LOCTEXT("EdNodeClassDataChangeHintText",
					              "If multiple nodes are selected, it will show none always, but it will work anyway.\nIt will be applied only when the chosen class supports the node instance."))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Tiny)
				[
					SNew(SClassPropertyEntryBox)
					.AllowNone(false)
					.AllowAbstract(false)
					.MetaClass(UJointEdGraphNode::StaticClass())
					.SelectedClass(SelectedEditorClass_Attr)
					.OnSetClass(this, &FJointEdGraphNodesCustomizationBase::OnChangeEditorNodeSetClass)
				]
			]
		];
	}

	HideDeveloperModeProperties(DetailBuilder);
}

void FJointEdGraphNodesCustomizationBase::OnChangeNodeSetClass(const UClass* Class)
{
	TSubclassOf<UJointNodeBase> CastedClass = Class ? TSubclassOf<UJointNodeBase>(const_cast<UClass*>(Class)) : nullptr;

	if (CastedClass == nullptr) return;


	FScopedTransaction Transaction(FText::Format(
		NSLOCTEXT("JointEdTransaction", "TransactionTitle_ChangeNodeClass", "Change node class: {0}"),
		FText::FromString(Class->GetName())));
	{
		TSet<UObject*> ModifiedObjects;

		for (int i = 0; i < CachedSelectedObjects.Num(); ++i)
		{
			if (CachedSelectedObjects[i].Get() == nullptr) continue;

			if (UJointEdGraphNode* CastedGraphNode = Cast<UJointEdGraphNode>(CachedSelectedObjects[i].Get()))
			{
				//Fallback
				if (!CastedGraphNode->CanReplaceNodeClass())
				{
					FJointEdUtils::FireNotification(
						LOCTEXT("Notification_ReplaceNode_Deny_NotAllowed", "Replace Node Class Action Has Been Denied"),
						LOCTEXT("Notification_Sub_ReplaceNode_Deny_NotAllowed", "This editor node prohibits this action."),
						EJointMDAdmonitionType::Error
					);

					continue;
				}

				if (!CastedGraphNode->CanPerformReplaceNodeClassTo(CastedClass))
				{
					FJointEdUtils::FireNotification(
						LOCTEXT("Notification_ReplaceNode_Deny_NotAllowed_Specific", "Replace Node Class Action Has Been Denied"),
						LOCTEXT("Notification_Sub_ReplaceNode_Deny_NotAllowed_Specific", "Provided class that is not compatible with the editor node type or it was invalid class."),
						EJointMDAdmonitionType::Error
					);

					continue;
				}


				if (!ModifiedObjects.Contains(CastedGraphNode))
				{
					CastedGraphNode->Modify();
					ModifiedObjects.Add(CastedGraphNode);
				}

				if (UJointNodeBase* CastedNode = CastedGraphNode->GetCastedNodeInstance(); CastedNode && !ModifiedObjects.Contains(
					CastedNode))
				{
					CastedNode->Modify();
					ModifiedObjects.Add(CastedNode);
				}

				if (UEdGraph* CastedGraph = CastedGraphNode->GetGraph(); CastedGraph && !ModifiedObjects.Contains(CastedGraph))
				{
					CastedGraph->Modify();
					ModifiedObjects.Add(CastedGraph);
				}

				CastedGraphNode->ReplaceNodeClassTo(CastedClass);
			}
		}
	}
}


void FJointEdGraphNodesCustomizationBase::OnChangeEditorNodeSetClass(const UClass* Class)
{
	TSubclassOf<UJointEdGraphNode> CastedClass = Class ? TSubclassOf<UJointEdGraphNode>(const_cast<UClass*>(Class)) : nullptr;

	if (CastedClass == nullptr) return;

	FScopedTransaction Transaction(

		FText::Format(NSLOCTEXT("JointEdTransaction", "TransactionTitle_ChangeEditorNodeClass",
		                        "Changed editor node class: {0}"),
		              FText::FromString(Class->GetName())));
	{
		TSet<UObject*> ModifiedObjects;

		for (int i = 0; i < CachedSelectedObjects.Num(); ++i)
		{
			if (CachedSelectedObjects[i].Get() == nullptr) continue;

			if (UJointEdGraphNode* CastedGraphNode = Cast<UJointEdGraphNode>(CachedSelectedObjects[i].Get()))
			{
				//Fallback
				if (!CastedGraphNode->CanReplaceEditorNodeClass())
				{
					FJointEdUtils::FireNotification(
						LOCTEXT("Notification_ReplaceEditorNode_Deny_NotAllowed", "Replace Editor Node Class Action Has Been Denied"),
						LOCTEXT("Notification_Sub_ReplaceEditorNode_Deny_NotAllowed", "This editor node prohibits this action."),
						EJointMDAdmonitionType::Error
					);

					continue;
				}

				if (!CastedGraphNode->CanPerformReplaceEditorNodeClassTo(CastedClass))
				{
					FJointEdUtils::FireNotification(
						LOCTEXT("Notification_ReplaceEditorNode_Deny_NotAllowed_Specific", "Replace Editor Node Class Action Has Been Denied"),
						LOCTEXT("Notification_Sub_ReplaceEditorNode_Deny_NotAllowed_Specific", "Provided class doesn't support the node instance type or it was invalid class."),
						EJointMDAdmonitionType::Error
					);

					continue;
				}

				if (!ModifiedObjects.Contains(CastedGraphNode))
				{
					CastedGraphNode->Modify();
					ModifiedObjects.Add(CastedGraphNode);
				}

				if (UJointNodeBase* CastedNode = CastedGraphNode->GetCastedNodeInstance(); CastedNode && !ModifiedObjects.Contains(
					CastedNode))
				{
					CastedNode->Modify();
					ModifiedObjects.Add(CastedNode);
				}

				if (UEdGraph* CastedGraph = CastedGraphNode->GetGraph(); CastedGraph && !ModifiedObjects.Contains(CastedGraph))
				{
					CastedGraph->Modify();
					ModifiedObjects.Add(CastedGraph);
				}

				CastedGraphNode->ReplaceEditorNodeClassTo(CastedClass);
			}
		}
	}
}

void FJointEdGraphNodesCustomizationBase::HideDeveloperModeProperties(IDetailLayoutBuilder& DetailBuilder)
{
	if (!UJointEditorSettings::Get()->bEnableDeveloperMode)
	{
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UJointEdGraphNode, NodeInstance));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UJointEdGraphNode, ClassData));

		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UJointEdGraphNode, NodeClassData));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UJointEdGraphNode, ParentNode));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UJointEdGraphNode, SubNodes));

		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UJointEdGraphNode, SimpleDisplayHiddenProperties));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UJointEdGraphNode, ExternalSourceEntry));
	}
}

bool FJointEdGraphNodesCustomizationBase::CheckIfEveryNodeAllowPinDataControl(TArray<UObject*> NodeInstanceObjs)
{
	bool bCanShowPinDataProperty = true;

	for (UObject* NodeInstanceObj : NodeInstanceObjs)
	{
		if (!NodeInstanceObj) continue;

		if (UJointNodeBase* NodeBase = Cast<UJointNodeBase>(NodeInstanceObj); NodeBase && !NodeBase->
			GetAllowEditingOfPinDataOnDetailsPanel())
		{
			bCanShowPinDataProperty = false;

			break;
		}
	}

	return bCanShowPinDataProperty;
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

TSharedRef<IDetailCustomization> FJointNodeInstanceCustomizationBase::MakeInstance()
{
	return MakeShareable(new FJointNodeInstanceCustomizationBase);
}

void FJointNodeInstanceCustomizationBase::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<UObject*> NodeInstances = JointDetailCustomizationHelpers::GetNodeInstancesFromGraphNodes(
		DetailBuilder.GetSelectedObjects());

	//If it was empty, try to grab the node instances by itself.
	if (NodeInstances.IsEmpty())
	{
		NodeInstances = JointDetailCustomizationHelpers::CastToNodeInstance(DetailBuilder.GetSelectedObjects());
	}

	//Display class description for the nodes when all the nodes are instanced.
	PopulateNodeClassesDescription(DetailBuilder, NodeInstances);

	//Display class description for the nodes when all the nodes are instanced.
	if (!JointDetailCustomizationHelpers::HasArchetypeOrClassDefaultObject(NodeInstances))
	{
		DetailBuilder.HideCategory("Editor");
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

			DetailBuilder.HideProperty(*PropIt->GetName());
			PropertyHandle->MarkHiddenByCustomization();
		}
	}
}

void FJointNodeInstanceCustomizationBase::PopulateNodeClassesDescription(IDetailLayoutBuilder& DetailBuilder, TArray<UObject*> NodeInstances)
{
	if (!JointDetailCustomizationHelpers::HasArchetypeOrClassDefaultObject(NodeInstances))
	{
		TSet<UClass*> ClassesToDescribe;

		for (UObject* Obj : NodeInstances)
		{
			if (Obj == nullptr) continue;
			if (ClassesToDescribe.Contains(Obj->GetClass())) continue;

			ClassesToDescribe.Add(Obj->GetClass());
		}

		TSharedPtr<SScrollBox> DescriptionBox;

		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Description");

		Category.AddCustomRow(LOCTEXT("NodeInstanceDetailDescription", "Description"))
		        .WholeRowWidget
		[
			SNew(SBox)
			.MaxDesiredHeight(UJointEditorSettings::Get()->DescriptionCardBoxMaxDesiredHeight)
			[
				SAssignNew(DescriptionBox, SScrollBox)
				.AnimateWheelScrolling(true)
				.AllowOverscroll(EAllowOverscroll::Yes)
			]
		];

		for (UClass* ToDescribe : ClassesToDescribe)
		{
			DescriptionBox->AddSlot()
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
}

TSharedRef<IDetailCustomization> FJointEdGraphCustomization::MakeInstance()
{
	return MakeShareable(new FJointEdGraphCustomization());
}

void FJointEdGraphCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	if (UJointEditorSettings::Get()->bEnableDeveloperMode)
	{
		IDetailCategoryBuilder& DeveloperCategory = DetailBuilder.EditCategory("Developer Mode - Internal Data");
		DeveloperCategory.SetCategoryVisibility(true);

		DeveloperCategory.AddCustomRow(LOCTEXT("RecaptureText", "Recapture"))
		                 .NameContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RecaptureTextName", "Recapture Properties"))
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
			]
			.ValueContent()
			[
				SNew(SJointOutlineButton)
				.NormalColor(FLinearColor::Transparent)
				.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
				.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OutlineNormalColor(FLinearColor::Transparent)
				.ContentPadding(FJointEditorStyle::Margin_Normal)
				.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
				.OnPressed(this, &FJointEdGraphCustomization::OnRecaptureButtonPressed)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("RecaptureText", "Recapture"))
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
				]
			];

		TArray<TWeakObjectPtr<UObject>> Objects = DetailBuilder.GetSelectedObjects();

		for (TWeakObjectPtr<UObject> Object : Objects)
		{
			if (!Object.IsValid()) continue;

			if (UJointEdGraph* CastedObject = Cast<UJointEdGraph>(Object))
			{
				CachedGraph.Add(CastedObject);
			}
		}

		OnRecaptureButtonPressed();
	}
	else
	{
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UJointEdGraph, Nodes_Captured));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UJointEdGraph, DebugData_Captured));
	}
}

void FJointEdGraphCustomization::OnRecaptureButtonPressed()
{
	for (TWeakObjectPtr<UJointEdGraph> JointEdGraph : CachedGraph)
	{
		if (!JointEdGraph.IsValid()) continue;

		JointEdGraph->Nodes_Captured = JointEdGraph->Nodes;
		JointEdGraph->DebugData_Captured = JointEdGraph->DebugData;
	}
}

TSharedRef<IDetailCustomization> FJointManagerCustomization::MakeInstance()
{
	return MakeShareable(new FJointManagerCustomization());
}

void FJointManagerCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	if (UJointEditorSettings::Get()->bEnableDeveloperMode)
	{
		//Removed in 2.10
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
		SNew(SJointMDSlate_Admonitions)
		.AdmonitionType(EJointMDAdmonitionType::Note)
		.CustomHeaderText(LOCTEXT("FJointBuildPresetCustomization_ExplanationTitle", "Joint Build Preset"))
		.bUseDescriptionText(true)
		.DescriptionText(LOCTEXT("FJointBuildPresetCustomization_Explanation",
		                         "Joint Build Preset is a preset asset that lets you specify which nodes and fragments of the Joint Manager are included in or excluded from packaging for a given build target. (See the tooltip for each property.)"
		                         "\n\nWe supports client & server preset, but those are not that recommended to use because using only those two has limitation on making a game that can be standalone & multiplayer together."
		                         "\nIf you set any of those to \'exclude\' then standalone section will not contain the nodes with that preset."
		                         "\nFor that cases, using build target based including & excluding will help you better. just make 3 different build target for each sessions (standalone (game), client, server) and set the behavior for each of them."))
	];
}

TSharedRef<IDetailCustomization> FJointNodePresetCustomization::MakeInstance()
{
	return MakeShareable(new FJointNodePresetCustomization);
}

void FJointNodePresetCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& DescriptionCategory = DetailBuilder.EditCategory("Description");
	DescriptionCategory.AddCustomRow(INVTEXT("Explanation"))
	                   .WholeRowContent()
	[
		SNew(SJointMDSlate_Admonitions)
		.AdmonitionType(EJointMDAdmonitionType::Note)
		.CustomHeaderText(LOCTEXT("FJointNodePresetCustomization_ExplanationTitle", "Joint Node Preset"))
		.bUseDescriptionText(true)
		.bAutoWrapDescriptionText(true)
		.DescriptionText(LOCTEXT("FJointNodePresetCustomization_Explanation",
		                         "Joint Node Preset is a preset asset that lets you reuse the base node set configuration (including the values of the properties) as a preset."
		                         "\nIt's just for the better iteration + and you can also take advantage of it on the script parsers as well."
		                         "\nBut keep in mind that changing presets will not apply any changes to the nodes. that already exist in the graph."))
	];

	//UJointEdGraph* PresetGraph;
	JointManagerHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UJointNodePreset, InternalJointManager));


	//Grab the Joint Manager from the preset asset.
	UJointManager* InternalJointManager = nullptr;
	if (JointManagerHandle)
	{
		UObject* JointManagerObject = nullptr;
		JointManagerHandle->GetValue(JointManagerObject);

		InternalJointManager = Cast<UJointManager>(JointManagerObject);
	}

	//Make a graph editor for the PresetGraph property to show the graph of the preset asset.
	IDetailCategoryBuilder& GraphCategory = DetailBuilder.EditCategory("Preset Graph");

	if (InternalJointManager)
	{
		if (UEdGraph* EdGraph = InternalJointManager->JointGraph)
		{
			GraphCategory
				.AddCustomRow(LOCTEXT("PresetGraphRow", "Preset Graph"))
				.WholeRowContent()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SBox)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.HeightOverride(750)
					[
						SNew(SJointGraphPreviewer, EdGraph)
						.Visibility(EVisibility::HitTestInvisible)
						.Clipping(EWidgetClipping::Inherit)
					]
				];
		}
		else
		{
			GraphCategory.AddCustomRow(LOCTEXT("PresetGraphNull", "Preset Graph Null"))
			             .WholeRowContent()
			             .HAlign(HAlign_Fill)
			             .VAlign(VAlign_Fill)
			[
				SNew(SJointMDSlate_Admonitions)
				.AdmonitionType(EJointMDAdmonitionType::Warning)
				.CustomHeaderText(LOCTEXT("PresetGraphNullTitle", "No Graph Found"))
				.bUseDescriptionText(true)
				.DescriptionText(LOCTEXT("PresetGraphNullDescription", "No graph found for this preset. Try pressing the edit button below to generate the graph, if you're first time opening this preset asset."))
			];
		}
	}

	GraphCategory.AddProperty(JointManagerHandle).Visibility(EVisibility::Collapsed);

	GraphCategory.AddCustomRow(LOCTEXT("PresetGraphEditingHint", "Preset Graph Edit"))
	             .WholeRowContent()
	[
		SNew(SJointOutlineBorder)
		.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.OutlineNormalColor(FLinearColor(0.04, 0.04, 0.04))
		.OutlineHoverColor(FJointEditorStyle::Color_Selected)
		.ContentPadding(FJointEditorStyle::Margin_Large)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PresetGraphEditingHint", "Edit the preset graph to change the configuration of the preset"))
					.ColorAndOpacity(FLinearColor::White)
					.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FJointEditorStyle::Margin_Normal)
				[
					SNew(SJointOutlineButton)
					.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
					.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.OutlineNormalColor(FLinearColor::Transparent)
					.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
					.ContentPadding(FJointEditorStyle::Margin_Normal)
					.ToolTipText(LOCTEXT("EditPresetGraphButtonToolTip", "Open the external editor"))
					.OnClicked(this, &FJointNodePresetCustomization::OnEditButtonClicked)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("EditPresetGraphButton", "Edit Preset Graph"))
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
						.ColorAndOpacity(FLinearColor::White)
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SJointMDSlate_Admonitions)
				.AdmonitionType(EJointMDAdmonitionType::Info)
				.CustomHeaderText(LOCTEXT("PresetGraphEditingHintTitle", "Preset Editing Rule"))
				.bUseDescriptionText(true)
				.DescriptionText(LOCTEXT("PresetGraphEditingHintDescription", "1. Fragments that are attached to the root node of the preset graph will be implemented as Manager Fragments."
				                         "\n2. Base Nodes (foundations) and its fragements will be implemented on the target graph. Connection between Base Nodes will be implemented as well."
				                         "\n3. But the connection between Root node and the other nodes will not be implemented."))
			]
		]
	];
}

void FJointNodePresetCustomization::PendingDelete()
{
	IDetailCustomization::PendingDelete();

	//Grab the Joint Manager from the preset asset.
	UJointManager* InternalJointManager = nullptr;
	if (JointManagerHandle)
	{
		UObject* JointManagerObject = nullptr;
		JointManagerHandle->GetValue(JointManagerObject);

		InternalJointManager = Cast<UJointManager>(JointManagerObject);
	}

	if (InternalJointManager)
	{
		if (FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(InternalJointManager, false))
		{
			//	UE_DEPRECATED(5.3, "Use CloseWindow that takes in an EAssetEditorCloseReason instead")
#if UE_VERSION_OLDER_THAN(5, 3, 0)
			Toolkit->CloseWindow();
#else
			Toolkit->CloseWindow(EAssetEditorCloseReason::AssetForceDeleted);
#endif
		}
	}

	JointManagerHandle.Reset();
}

FReply FJointNodePresetCustomization::OnEditButtonClicked()
{
	//Grab the Joint Manager from the preset asset.
	UJointManager* InternalJointManager = nullptr;
	if (JointManagerHandle)
	{
		UObject* JointManagerObject = nullptr;
		JointManagerHandle->GetValue(JointManagerObject);

		InternalJointManager = Cast<UJointManager>(JointManagerObject);
	}

	FJointEditorToolkit* Toolkit = nullptr;
	FJointEdUtils::OpenEditorFor(InternalJointManager, Toolkit);
	Toolkit->SetIsInNodePresetEditingMode(true);

	return FReply::Handled();
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


	JointDetailCustomizationHelpers::GetParentJointFromNodeInstances(NodeInstanceObjs, bFromMultipleManager,
	                                                                 bFromInvalidJointManagerObject,
	                                                                 bFromJointManagerItself);


	//IDetailGroup& Group = StructBuilder.
	//	AddGroup("NodePicker", StructPropertyHandle.Get()->GetPropertyDisplayName());
	// Begin a new line.  All properties below this call will be added to the same line until EndLine() or another BeginLine() is called

	StructBuilder.AddProperty(NodeHandle.ToSharedRef())
	             .CustomWidget()
	             .WholeRowContent()
	             .HAlign(HAlign_Fill)
	[
		SAssignNew(BorderWidget, SJointOutlineBorder)
		.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.OutlineNormalColor(FLinearColor(0.04, 0.04, 0.04))
		.OutlineHoverColor(FLinearColor(0.4, 0.4, 0.5))
		.ContentPadding(FJointEditorStyle::Margin_Tiny)
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
				.Padding(FJointEditorStyle::Margin_Small)
				[
					NodeHandle.Get()->CreatePropertyValueWidget(true)
				]
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(FeatureButtonsSlate, SJointNodePointerSlateFeatureButtons)
				.OnPickupButtonPressed(this, &FJointNodePointerStructCustomization::OnNodePickUpButtonPressed)
				.OnGotoButtonPressed(this, &FJointNodePointerStructCustomization::OnGoToButtonPressed)
				.OnCopyButtonPressed(this, &FJointNodePointerStructCustomization::OnCopyButtonPressed)
				.OnPasteButtonPressed(this, &FJointNodePointerStructCustomization::OnPasteButtonPressed)
				.OnClearButtonPressed(this, &FJointNodePointerStructCustomization::OnClearButtonPressed)
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

FReply FJointNodePointerStructCustomization::OnNodePickUpButtonPressed()
{
	bool bFromMultipleManager = false;
	bool bFromInvalidJointManagerObject = false;
	bool bFromJointManagerItself = false;

	UJointManager* FoundJointManager = JointDetailCustomizationHelpers::GetParentJointFromNodeInstances(
		NodeInstanceObjs, bFromMultipleManager, bFromInvalidJointManagerObject, bFromJointManagerItself);

	//Halt if the Joint manager is not valid.
	if (FoundJointManager == nullptr) return FReply::Handled();

	if (FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(FoundJointManager))
	{
		if (!Toolkit->GetNodePickingManager().IsValid()) return FReply::Handled();

		if (!Toolkit->GetNodePickingManager()->IsInNodePicking())
		{
			Request = Toolkit->GetNodePickingManager()->StartNodePicking(NodeHandle, EditorNodeHandle);
		}
		else
		{
			Toolkit->GetNodePickingManager()->EndNodePicking();
		}
	}

	return FReply::Handled();
}


FReply FJointNodePointerStructCustomization::OnGoToButtonPressed()
{
	UObject* CurrentNode = nullptr;

	NodeHandle.Get()->GetValue(CurrentNode);

	//Revert
	if (CurrentNode == nullptr) return FReply::Handled();;

	bool bFromMultipleManager = false;
	bool bFromInvalidJointManagerObject = false;
	bool bFromJointManagerItself = false;

	UJointManager* FoundJointManager = JointDetailCustomizationHelpers::GetParentJointFromNodeInstances(
		NodeInstanceObjs, bFromMultipleManager, bFromInvalidJointManagerObject, bFromJointManagerItself);

	//Halt if the Joint manager is not valid.
	if (FoundJointManager == nullptr) return FReply::Handled();

	if (FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(FoundJointManager))
	{
		if (Toolkit->GetJointManager() && Toolkit->GetJointManager()->JointGraph)
		{
			if (UJointEdGraph* CastedGraph = Cast<UJointEdGraph>(Toolkit->GetJointManager()->JointGraph))
			{
				TSet<TWeakObjectPtr<UJointEdGraphNode>> GraphNodes = CastedGraph->GetCachedJointGraphNodes();

				for (TWeakObjectPtr<UJointEdGraphNode> Node : GraphNodes)
				{
					if (!Node.IsValid() || Node->GetCastedNodeInstance() == nullptr) continue;

					if (Node->GetCastedNodeInstance() == CurrentNode)
					{
						Toolkit->JumpToHyperlink(Node.Get());

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

	BlinkSelf();

	return FReply::Handled();
}

FReply FJointNodePointerStructCustomization::OnCopyButtonPressed()
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
	if (FoundJointManager == nullptr) return FReply::Handled();

	if (FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(FoundJointManager))
	{
		JointEditorToolkitToastMessages::PopulateNodePickerCopyToastMessage(SharedThis(Toolkit));
	}

	BlinkSelf();

	return FReply::Handled();
}

FReply FJointNodePointerStructCustomization::OnPasteButtonPressed()
{
	FString Value;

	FPlatformApplicationMisc::ClipboardPaste(Value);
	//Check if the provided node is from the same Joint manager.

	TSoftObjectPtr<UJointNodeBase> Node;
	Node = FSoftObjectPath(Value);

	if (!Node.IsValid())
	{
		//Invalid node path provided.
		return FReply::Handled();
	}


	bool bFromMultipleManager = false;
	bool bFromInvalidJointManagerObject = false;
	bool bFromJointManagerItself = false;

	UJointManager* FoundJointManager = JointDetailCustomizationHelpers::GetParentJointFromNodeInstances(
		NodeInstanceObjs, bFromMultipleManager, bFromInvalidJointManagerObject, bFromJointManagerItself);

	//Halt if the Joint manager is not valid.
	if (FoundJointManager == nullptr) return FReply::Handled();

	if (FoundJointManager != Node->GetJointManager())
	{
		//The node is not from the same Joint manager.
		return FReply::Handled();
	}

	NodeHandle->SetValueFromFormattedString(Value, PPF_Copy);

	OnNodeDataChanged();


	if (FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(FoundJointManager))
	{
		JointEditorToolkitToastMessages::PopulateNodePickerPastedToastMessage(SharedThis(Toolkit));
	}

	return FReply::Handled();
}

FReply FJointNodePointerStructCustomization::OnClearButtonPressed()
{
	if (NodeHandle) NodeHandle->ResetToDefault();
	if (EditorNodeHandle) EditorNodeHandle->ResetToDefault();

	return FReply::Handled();
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

		FJointEdUtils::FireNotification(
			LOCTEXT("NotJointNodeInstanceType", "Node Pick Up Canceled"),
			LOCTEXT("NotJointNodeInstanceType_Sub", "Provided node instance was not a valid Joint node instance. Pointer reset to default."),
			EJointMDAdmonitionType::Error
		);

		FText FailedNotificationText = LOCTEXT("NotJointNodeInstanceType", "Node Pick Up Canceled");
		FText FailedNotificationSubText = LOCTEXT("NotJointNodeInstanceType_Sub", "Provided node instance was not a valid Joint node instance. Pointer reset to default.");
		FNotificationInfo NotificationInfo(FailedNotificationText);
		NotificationInfo.Image = FJointEditorStyle::Get().GetBrush("JointUI.Image.Joint3d");
		NotificationInfo.bFireAndForget = true;
		NotificationInfo.FadeInDuration = 0.2f;
		NotificationInfo.FadeOutDuration = 0.2f;
		NotificationInfo.ExpireDuration = 4.5f;
		NotificationInfo.WidthOverride = FOptionalSize();
		NotificationInfo.ContentWidget = SNew(SJointNotificationWidget)
		[
			SNew(SJointMDSlate_Admonitions)
			.AdmonitionType(EJointMDAdmonitionType::Error)
			.CustomHeaderText(FailedNotificationText)
			.bUseDescriptionText(true)
			.DescriptionText(FailedNotificationSubText)
		];

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
				// FText::Format returns FText, convert to FString before appending
				AllowedTypeStr.Append(
					FText::Format(
						LOCTEXT("AllowedTypeTemplate", "{0} ({1})"),
						FJointEdUtils::GetFriendlyNameFromClass(AllowedType.Get()),
						FText::FromString(AllowedType.Get()->GetName())
					).ToString()
				);
			}

			FString DisallowedTypeStr;
			for (TSubclassOf<UJointNodeBase> DisallowedType : Casted->DisallowedType)
			{
				if (DisallowedType == nullptr) continue;
				if (!DisallowedTypeStr.IsEmpty()) DisallowedTypeStr.Append(", ");
				// FText::Format returns FText, convert to FString before appending
				DisallowedTypeStr.Append(
					FText::Format(
						LOCTEXT("DisallowedTypeTemplate", "{0} ({1})"),
						FJointEdUtils::GetFriendlyNameFromClass(DisallowedType.Get()),
						FText::FromString(DisallowedType.Get()->GetName())
					).ToString()
				);
			}

			FText FailedNotificationText = LOCTEXT("NotJointNodeInstanceType", "Node Pick Up Canceled");
			FText FailedNotificationSubText = LOCTEXT("NotJointNodeInstanceType_Sub", "Current structure can not accept the provided node instance.");

			FNotificationInfo NotificationInfo(FailedNotificationText);
			NotificationInfo.bFireAndForget = true;
			NotificationInfo.FadeInDuration = 0.2f;
			NotificationInfo.FadeOutDuration = 0.2f;
			NotificationInfo.ExpireDuration = 4.5f;
			NotificationInfo.WidthOverride = FOptionalSize();

			if (!AllowedTypeStr.IsEmpty() && !DisallowedTypeStr.IsEmpty())
			{
				NotificationInfo.ContentWidget = SNew(SJointNotificationWidget)
				[
					SNew(SJointMDSlate_Admonitions)
					.AdmonitionType(EJointMDAdmonitionType::Error)
					.CustomHeaderText(LOCTEXT("NodePickUpCanceledTitle", "Node Pick Up Canceled"))
					.bUseDescriptionText(false)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(STextBlock)
							.TextStyle(&FJointEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("JointUI.TextBlock.Regular.h3"))
							.Text(LOCTEXT("NodePickUpCanceledDescription", "Current structure can not accept the provided node instance."))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(SJointMDSlate_Admonitions)
							.AdmonitionType(EJointMDAdmonitionType::Info)
							.CustomHeaderText(LOCTEXT("AllowedType", "Allowed Types"))
							.bUseDescriptionText(true)
							.DescriptionText(FText::FromString(AllowedTypeStr))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(SJointMDSlate_Admonitions)
							.AdmonitionType(EJointMDAdmonitionType::Info)
							.CustomHeaderText(LOCTEXT("DisallowedType", "Disallowed Types"))
							.bUseDescriptionText(true)
							.DescriptionText(FText::FromString(DisallowedTypeStr))
						]
					]
				];
			}
			else if (!AllowedTypeStr.IsEmpty())
			{
				NotificationInfo.ContentWidget = SNew(SJointNotificationWidget)
				[
					SNew(SJointMDSlate_Admonitions)
					.AdmonitionType(EJointMDAdmonitionType::Error)
					.CustomHeaderText(LOCTEXT("NodePickUpCanceledTitle", "Node Pick Up Canceled"))
					.bUseDescriptionText(false)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(STextBlock)
							.TextStyle(&FJointEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("JointUI.TextBlock.Regular.h3"))
							.Text(LOCTEXT("NodePickUpCanceledDescription", "Current structure can not accept the provided node instance."))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(SJointMDSlate_Admonitions)
							.AdmonitionType(EJointMDAdmonitionType::Info)
							.CustomHeaderText(LOCTEXT("AllowedType", "Allowed Types"))
							.bUseDescriptionText(true)
							.DescriptionText(FText::FromString(AllowedTypeStr))
						]
					]
				];
			}
			else if (!DisallowedTypeStr.IsEmpty())
			{
				NotificationInfo.ContentWidget = SNew(SJointNotificationWidget)
				[
					SNew(SJointMDSlate_Admonitions)
					.AdmonitionType(EJointMDAdmonitionType::Error)
					.CustomHeaderText(LOCTEXT("NodePickUpCanceledTitle", "Node Pick Up Canceled"))
					.bUseDescriptionText(false)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(STextBlock)
							.TextStyle(&FJointEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("JointUI.TextBlock.Regular.h3"))
							.Text(LOCTEXT("NodePickUpCanceledDescription", "Current structure can not accept the provided node instance."))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FJointEditorStyle::Margin_Normal)
						[
							SNew(SJointMDSlate_Admonitions)
							.AdmonitionType(EJointMDAdmonitionType::Info)
							.CustomHeaderText(LOCTEXT("DisallowedType", "Disallowed Types"))
							.bUseDescriptionText(true)
							.DescriptionText(FText::FromString(DisallowedTypeStr))
						]
					]
				];
			}

			FSlateNotificationManager::Get().AddNotification(NotificationInfo);
		}
	}

	BlinkSelf();
}

void FJointNodePointerStructCustomization::OnNodeResetToDefault()
{
	if (EditorNodeHandle.IsValid()) EditorNodeHandle->ResetToDefault();
}

void FJointNodePointerStructCustomization::OnMouseHovered()
{
	FeatureButtonsSlate->UpdateVisualOnHovered();

	if (BackgroundBox.IsValid()) BackgroundBox->SetRenderOpacity(0.5);
	if (ButtonBox.IsValid()) ButtonBox->SetVisibility(EVisibility::SelfHitTestInvisible);

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
		TSet<TWeakObjectPtr<UJointEdGraphNode>> GraphNodes = CastedGraph->GetCachedJointGraphNodes();

		for (TWeakObjectPtr<UJointEdGraphNode> Node : GraphNodes)
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
	FeatureButtonsSlate->UpdateVisualOnUnhovered();

	if (BackgroundBox.IsValid()) BackgroundBox->SetRenderOpacity(1);
	if (ButtonBox.IsValid()) ButtonBox->SetVisibility(EVisibility::Collapsed);

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
		TSet<TWeakObjectPtr<UJointEdGraphNode>> GraphNodes = CastedGraph->GetCachedJointGraphNodes();

		for (TWeakObjectPtr<UJointEdGraphNode> Node : GraphNodes)
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

void FJointNodePointerStructCustomization::BlinkSelf()
{
	if (BorderWidget == nullptr) return;

	VOLT_STOP_ANIM(BlinkAnimTrack);

	//play blink animation with background color change
	const UVoltAnimation* BlinkAnimation = VOLT_MAKE_ANIMATION()
	(
		VOLT_MAKE_MODULE(UVolt_ASM_Sequence)
		.bShouldLoop(true)
		.MaxLoopCount(1)
		                                    (
			                                    VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
			.InterpolationMode(EVoltInterpMode::AlphaBased)
			.AlphaBasedDuration(0.16)
			.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
			.AlphaBasedBlendExp(6)
			.bUseStartColor(true)
			.StartColor(BorderWidget->NormalColor)
			.TargetColor(FLinearColor(1, 1, 1, 0.5)),
			                                    VOLT_MAKE_MODULE(UVolt_ASM_InterpBackgroundColor)
			.InterpolationMode(EVoltInterpMode::AlphaBased)
			.AlphaBasedDuration(0.16)
			.AlphaBasedEasingFunction(EEasingFunc::ExpoOut)
			.AlphaBasedBlendExp(6)
			.bUseStartColor(true)
			.StartColor(FLinearColor(1, 1, 1, 0.5))
			.TargetColor(BorderWidget->NormalColor)
		                                    )
	);

	BlinkAnimTrack = VOLT_PLAY_ANIM(BorderWidget->InnerBorder, BlinkAnimation);
}


//JointScriptLinkerCustomizationHelpers - namespace

namespace JointScriptLinkerCustomizationHelpers
{
	void Reimport_FJointScriptLinkerDataElement(
		FString FilePath,
		TSharedPtr<IPropertyHandle> StructPropertyHandle_FJointScriptLinkerDataElement
	)
	{
		if (StructPropertyHandle_FJointScriptLinkerDataElement.IsValid())
		{
			TSharedPtr<IPropertyHandle> ParserDataHandle = StructPropertyHandle_FJointScriptLinkerDataElement->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerDataElement, ParserData));
			TSharedPtr<IPropertyHandle> ParserData_ScriptParser = ParserDataHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerParserData, ScriptParser));
			TSharedPtr<IPropertyHandle> ParserData_ParserInstanceData = ParserDataHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerParserData, ParserInstanceData));

			FString OutputString;

			ParserData_ScriptParser->GetValueAsFormattedString(OutputString);
			// make a soft object path and try to load it, if it fails, skip and fire notification
			FSoftClassPath SoftClassPath(OutputString);
			UClass* ParserClass = SoftClassPath.TryLoadClass<UJointScriptParser>();
			
			//TArray<uint8> ParserInstanceData -> we have to get raw data.
			TArray<uint8> ParserInstanceData;
			void* ParserInstanceDataPtr = nullptr;

			if (ParserData_ParserInstanceData->GetValueData(ParserInstanceDataPtr) == FPropertyAccess::Success && ParserInstanceDataPtr != nullptr)
			{
				const TArray<uint8>* SourceArrayPtr = static_cast<TArray<uint8>*>(ParserInstanceDataPtr);
				ParserInstanceData = *SourceArrayPtr;
			}

			UJointScriptParser* ParserCasted = ParserClass->GetDefaultObject<UJointScriptParser>();

			if (ParserCasted == nullptr)
			{
				FJointEdUtils::FireNotification(
					LOCTEXT("ReimportFileFailedTitle_InvalidParser", "Reimport File Skipped"),
					LOCTEXT("ReimportFileFailedDesc_InvalidParser", "The parser reference provided is not valid. Skipping reimport for that mapping."),
					EJointMDAdmonitionType::Warning
				);

				return;
			}
			
			FJointScriptLinkerParserData::DeserializeParserInstanceFromDataToExistingParser(
				ParserCasted, 
				ParserInstanceData
			);

			//TArray<FJointScriptLinkerMapping> Mappings
			TSharedPtr<IPropertyHandle> MappingsHandle = StructPropertyHandle_FJointScriptLinkerDataElement->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerDataElement, Mappings));

			//iterate through each element and call ImportFileToJointManager
			TSharedPtr<IPropertyHandleArray> MappingsArrayHandle = MappingsHandle->AsArray();

			uint32 NumElements = 0;
			MappingsArrayHandle->GetNumElements(NumElements);

			for (uint32 i = 0; i < NumElements; ++i)
			{
				TSharedPtr<IPropertyHandle> ElementHandle = MappingsArrayHandle->GetElement(i);
				TSharedPtr<IPropertyHandle> JointManagerHandle = ElementHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, JointManager));

				if (!JointManagerHandle.IsValid()) continue;

				UObject* JointManagerObj = nullptr;
				JointManagerHandle->GetValue(JointManagerObj);

				if (JointManagerObj == nullptr)
				{
					FJointEdUtils::FireNotification(
					   LOCTEXT("ReimportFileFailedTitle_NullJointManager", "Reimport File Skipped"),
					   FText::Format(LOCTEXT("ReimportFileFailedDesc_NullJointManager", "Mapping index [{0}] has a null Joint Manager. Skipping."), FText::AsNumber(i)),
					   EJointMDAdmonitionType::Warning
					);
					continue;
				}
				if (UJointManager* JointManagerCasted = Cast<UJointManager>(JointManagerObj))
				{
					FJointEdUtils::ImportFileToJointManager(
						JointManagerCasted,
						FilePath,
						ParserCasted,
						true
					);
				}
			}
		}
	}

	void Reimport_FJointScriptLinkerDataElement_Array(
		FString FilePath,
		TSharedPtr<IPropertyHandle> StructPropertyHandle_FJointScriptLinkerDataElement_Array
	)
	{
		if (StructPropertyHandle_FJointScriptLinkerDataElement_Array.IsValid())
		{
			TSharedPtr<IPropertyHandleArray> ArrayHandle = StructPropertyHandle_FJointScriptLinkerDataElement_Array->AsArray();

			uint32 NumElements = 0;
			ArrayHandle->GetNumElements(NumElements);

			for (uint32 i = 0; i < NumElements; ++i)
			{
				TSharedPtr<IPropertyHandle> ElementHandle = ArrayHandle->GetElement(i);
				Reimport_FJointScriptLinkerDataElement(FilePath, ElementHandle);
			}
		}
	}

	bool CheckPathValidity(
		TSharedPtr<IPropertyHandle> StructPropertyHandle_FJointScriptLinkerFileEntry
	)
	{
		if (StructPropertyHandle_FJointScriptLinkerFileEntry.IsValid())
		{
			const TSharedPtr<IPropertyHandle> FilePathHandle = StructPropertyHandle_FJointScriptLinkerFileEntry->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath));

			FString OutputString;

			FilePathHandle->GetValueAsFormattedString(OutputString);

			return !OutputString.IsEmpty() && FPaths::FileExists(OutputString);
		}

		return false;
	}


	void GetAllNodeGuidsFromJointManager(UJointManager* JM, TSet<FGuid>& OutNodeGuids)
	{
		if (!JM) return;

		for (TObjectPtr<UJointNodeBase>& ManagerFragment : JM->ManagerFragments)
		{
			if (!ManagerFragment) continue;

			const FGuid& FragmentNodeGuid = ManagerFragment->GetNodeGuid();

			OutNodeGuids.Add(FragmentNodeGuid);

			const TArray<UJointFragment*>& FragmentsOnLowerHierarchy = ManagerFragment->GetAllFragmentsOnLowerHierarchy();
			for (const UJointFragment* Fragment : FragmentsOnLowerHierarchy)
			{
				if (!Fragment) continue;

				const FGuid& ManagerFragmentNodeGuid = Fragment->GetNodeGuid();

				OutNodeGuids.Add(ManagerFragmentNodeGuid);
			}
		}

		for (TObjectPtr<UJointNodeBase>& JointNodeBase : JM->Nodes)
		{
			if (!JointNodeBase) continue;

			const FGuid& NodeGuid = JointNodeBase->GetNodeGuid();

			OutNodeGuids.Add(NodeGuid);

			const TArray<UJointFragment*>& FragmentsOnLowerHierarchy = JointNodeBase->GetAllFragmentsOnLowerHierarchy();
			for (const UJointFragment* Fragment : FragmentsOnLowerHierarchy)
			{
				if (!Fragment) continue;

				const FGuid& FragmentNodeGuid = Fragment->GetNodeGuid();

				OutNodeGuids.Add(FragmentNodeGuid);
			}
		}
	}


	UJointManager* LoadJointManager(const TSharedPtr<IPropertyHandle>& JointManagerHandle)
	{
		FString SoftReferenceString;
		JointManagerHandle->GetValueAsFormattedString(SoftReferenceString);
		FSoftObjectPath SoftObjectPath(SoftReferenceString);

		//load the object if it's not already loaded
		UObject* LoadedObject = SoftObjectPath.TryLoad();
		UJointManager* JM = Cast<UJointManager>(LoadedObject);
		return JM;
	}
	
	TSharedRef<SWidget> CreateImportButtonContentWidget()
	{
		return SNew(SBox)
			.WidthOverride(14)
			.HeightOverride(14)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SImage)
				.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Import"))
				.DesiredSizeOverride(FVector2D(12, 12))
				.ColorAndOpacity(FLinearColor::White)
			];
	}

	TSharedRef<SWidget> CreateReimportButtonContentWidget()
	{
		return SNew(SBox)
			.WidthOverride(14)
			.HeightOverride(14)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Import"))
					.DesiredSizeOverride(FVector2D(10, 10))
					.ColorAndOpacity(FLinearColor::White)
					.RenderTransform(FSlateRenderTransform(FVector2D(-3, -3)))
				]
				+ SOverlay::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Refresh"))
					.DesiredSizeOverride(FVector2D(10, 10))
					.ColorAndOpacity(FLinearColor::White)
					.RenderTransform(FSlateRenderTransform(FVector2D(3, 3)))
				]
			];
	}
	
	TSharedRef<SWidget> CreateQuickReimportButtonContentWidget()
	{
		return SNew(SBox)
			.WidthOverride(14)
			.HeightOverride(14)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Import"))
					.DesiredSizeOverride(FVector2D(10, 10))
					.ColorAndOpacity(FLinearColor::White)
					.RenderTransform(FSlateRenderTransform(FVector2D(-3, -3)))
				]
				+ SOverlay::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Refresh"))
					.DesiredSizeOverride(FVector2D(10, 10))
					.ColorAndOpacity(FLinearColor::White)
					.RenderTransform(FSlateRenderTransform(FVector2D(3, 3)))
				]
				+ SOverlay::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("EditorViewport.ToggleRealTime"))
					.DesiredSizeOverride(FVector2D(7, 7))
					.ColorAndOpacity(FLinearColor::White)
					.RenderTransform(FSlateRenderTransform(FVector2D(2, -4)))
				]
			];
	}

	TSharedRef<SWidget> CreateReallocateButtonContentWidget()
	{
		return 
			SNew(SBox)
			.WidthOverride(14)
			.HeightOverride(14)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SImage)
				.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.EyeDropper"))
				.DesiredSizeOverride(FVector2D(12, 12))
				.ColorAndOpacity(FLinearColor::White)
			];
	}
}


TSharedRef<IPropertyTypeCustomization> FJointScriptLinkerDataElementCustomization::MakeInstance()
{
	return MakeShareable(new FJointScriptLinkerDataElementCustomization);
}

void FJointScriptLinkerDataElementCustomization::CustomizeStructHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils)
{
	//No special header needed - create default on
	HeaderRow.NameContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FJointEditorStyle::Margin_Tiny)
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FJointEditorStyle::Margin_Tiny)
		[
			CreateReimportButtonWidget(StructPropertyHandle).ToSharedRef()
		]
	];
}

void FJointScriptLinkerDataElementCustomization::CustomizeStructChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils)
{
	ChildBuilder.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerDataElement, FileEntry)).ToSharedRef());
	ChildBuilder.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerDataElement, ParserData)).ToSharedRef());
	ChildBuilder.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerDataElement, Mappings)).ToSharedRef());
}

TSharedPtr<SWidget> FJointScriptLinkerDataElementCustomization::CreateReimportButtonWidget(TSharedPtr<IPropertyHandle> StructPropertyHandle)
{
	
	TWeakPtr<IPropertyTypeCustomization> WeakThisPtr = AsShared();
	TWeakPtr<IPropertyHandle> WeakStructPropertyHandle = StructPropertyHandle;
	
	auto ValidityCheck = [WeakStructPropertyHandle]() -> const bool
	{
		if (!WeakStructPropertyHandle.IsValid()) return false;
		
		const TSharedPtr<IPropertyHandle> FilePathHandle = WeakStructPropertyHandle.Pin()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath));

		FString OutputString;

		FilePathHandle->GetValueAsFormattedString(OutputString);

		return !OutputString.IsEmpty() && FPaths::FileExists(OutputString);
	};

	//Reimport button
	return SNew(SJointOutlineButton)
		.NormalColor(FLinearColor::Transparent)
		.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
		.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.OutlineNormalColor(FLinearColor::Transparent)
		.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
		.ContentPadding(FJointEditorStyle::Margin_Normal)
		.ToolTipText(LOCTEXT("QuickReimportButtonTooltip", "Reimport from the file path specified in this entry. Will use the same parser as the initial import."))
		.IsEnabled_Lambda([WeakStructPropertyHandle]() -> const bool
		{
			if (!WeakStructPropertyHandle.IsValid()) return false;
			
			return JointScriptLinkerCustomizationHelpers::CheckPathValidity(WeakStructPropertyHandle.Pin());
		})
		.OnClicked_Lambda([WeakStructPropertyHandle]()
		{
			if (!WeakStructPropertyHandle.IsValid()) return FReply::Handled();
			
			const TSharedPtr<IPropertyHandle> FilePathHandle = WeakStructPropertyHandle.Pin()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath));

			FString OutputString;

			FilePathHandle->GetValueAsFormattedString(OutputString);

			if (OutputString.IsEmpty() || !FPaths::FileExists(OutputString))
			{
				FJointEdUtils::FireNotification(
					LOCTEXT("ReimportFileFailedTitle", "Reimport File Failed"),
					LOCTEXT("ReimportFileFailedDesc", "File path is empty or file does not exist at the specified path."),
					EJointMDAdmonitionType::Error
				);

				return FReply::Handled();
			}

			JointScriptLinkerCustomizationHelpers::Reimport_FJointScriptLinkerDataElement(OutputString,  WeakStructPropertyHandle.Pin());

			return FReply::Handled();
		})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Padding(FJointEditorStyle::Margin_Small)
			[
				JointScriptLinkerCustomizationHelpers::CreateQuickReimportButtonContentWidget()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Padding(FJointEditorStyle::Margin_Small)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
				.Text(LOCTEXT("QuickReimportAllButtonLabel", "Quick Reimport All"))
			]
		];
}


TSharedRef<IPropertyTypeCustomization> FJointScriptLinkerFileEntryCustomization::MakeInstance()
{
	return MakeShareable(new FJointScriptLinkerFileEntryCustomization);
}

/*
void FJointScriptLinkerFileEntryCustomization::BuildFilterString()
{
	if (!HeaderRowPtr) return;
	
	const TSharedPtr<IPropertyHandle> FilePathHandle = ThisStructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath));

	FString FilePathString;

	FilePathHandle->GetValueAsFormattedString(FilePathString);

	if (FilePathString.IsEmpty())
	{
		FilePathString = TEXT("N/A");
	}
	
	HeaderRowPtr->RowTagName = FName(FText::Format(
		LOCTEXT("FileEntryNameWithPathTemplate", "({0})"),
		FText::FromString(FilePathString)
	).ToString());
}
*/

TSharedRef<IPropertyTypeCustomization> FJointScriptLinkerDataCustomization::MakeInstance()
{
	return MakeShareable(new FJointScriptLinkerDataCustomization);
}

void FJointScriptLinkerDataCustomization::CustomizeStructHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils)
{
	ThisStructPropertyHandle = StructPropertyHandle;
	PropertyUtils = StructCustomizationUtils.GetPropertyUtilities();
	HeaderRow.WholeRowContent()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FJointEditorStyle::Margin_Tiny)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Tiny)
			[
				StructPropertyHandle->CreatePropertyNameWidget()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Tiny)
			[
				CreateImportButtonWidget().ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Tiny)
			[
				CreateQuickReimportAllButtonWidget().ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Tiny)
			[
				CreateRefreshWidgetButtonWidget().ToSharedRef()
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FJointEditorStyle::Margin_Tiny)
		[
			StructPropertyHandle->CreatePropertyValueWidget()
		]
	];
}

void FJointScriptLinkerDataCustomization::CustomizeStructChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils)
{
	ChildBuilder.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerData, ScriptLinks)).ToSharedRef());
}

TSharedPtr<SWidget> FJointScriptLinkerDataCustomization::CreateImportButtonWidget()
{
	//Reimport button
	return SNew(SJointOutlineButton)
		.NormalColor(FLinearColor::Transparent)
		.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
		.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.OutlineNormalColor(FLinearColor::Transparent)
		.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
		.ContentPadding(FJointEditorStyle::Margin_Normal)
		.ToolTipText(LOCTEXT("ImportJointManagerToolTip", "Import a Joint Manager by selecting a script file. This will create a new Joint Manager asset based on the provided script and parser."))
		.OnClicked_Lambda([this]()
		{
			FScopedTransaction ImportTransaction(LOCTEXT("ImportJointManagerTransaction", "Import Joint Manager"));
	
			UJointScriptSettings::Get()->Modify();
			
			// Create a simple modal window with a list and OK/Cancel
			TSharedPtr<SWindow> ImportWindow = SNew(SWindow)
				.Title(LOCTEXT("ImportJointManagerWindowTitle", "Import Joint Manager"))
				.ClientSize(FVector2D(1200, 800))
				.SupportsMinimize(false)
				.SupportsMaximize(false)
				.FocusWhenFirstShown(true);

			// List view widget
			ImportWindow->SetContent(
				SNew(SJointManagerImportingPopup)
				.ParentWindow(ImportWindow)
			);
			
			// Show the window modally
			FSlateApplication::Get().AddModalWindow(ImportWindow.ToSharedRef(), nullptr);

			UJointScriptSettings::Save();

			return FReply::Handled();
		})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Padding(FJointEditorStyle::Margin_Small)
			[
				JointScriptLinkerCustomizationHelpers::CreateImportButtonContentWidget()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Padding(FJointEditorStyle::Margin_Small)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
				.Text(LOCTEXT("ImportButtonLabel", "Import Joint Manager"))
			]
		];
}

TSharedPtr<SWidget> FJointScriptLinkerDataCustomization::CreateQuickReimportAllButtonWidget()
{
	//Reimport button
	return SNew(SJointOutlineButton)
		.NormalColor(FLinearColor::Transparent)
		.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
		.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.OutlineNormalColor(FLinearColor::Transparent)
		.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
		.ContentPadding(FJointEditorStyle::Margin_Normal)
		.ToolTipText(LOCTEXT("QuickReimportAllToolTip",
		                     "Quickly reimport all Joint Managers in this script linker data. This will reimport all files associated with each mapping without changing the file paths, and will overwrite existing node mappings. Please make sure to backup your data before using this function."))
		.OnClicked_Lambda([this]()
		{
			TSharedPtr<IPropertyHandle> ScriptLinksHandle = ThisStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerData, ScriptLinks));

			//iterate through each element and call ImportFileToJointManager
			TSharedPtr<class IPropertyHandleArray> ArrayPH = ScriptLinksHandle->AsArray();

			if (!ArrayPH.IsValid()) return FReply::Handled();

			uint32 NumElements = 0;
			ArrayPH->GetNumElements(NumElements);

			for (uint32 Index = 0; Index < NumElements; ++Index)
			{
				//FJointScriptLinkerDataElement
				TSharedPtr<IPropertyHandle> ElementHandle = ArrayPH->GetElement(Index);

				if (!ElementHandle.IsValid()) continue;

				TSharedPtr<IPropertyHandle> JointScriptLinkerFileEntryHandle = ElementHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerDataElement, FileEntry));
				TSharedPtr<IPropertyHandle> JointScriptLinkerMappingArrayHandle = ElementHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerDataElement, Mappings));
				// get the path string from the FileEntry struct
				TSharedPtr<IPropertyHandle> FilePathHandle = JointScriptLinkerFileEntryHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath));
				FString FilePathString;
				FilePathHandle->GetValueAsFormattedString(FilePathString);

				JointScriptLinkerCustomizationHelpers::Reimport_FJointScriptLinkerDataElement(FilePathString, ElementHandle);
			}

			FJointEdUtils::FireNotification(
				LOCTEXT("QuickReimportAllSuccessTitle", "Quick Reimport All Completed"),
				LOCTEXT("ReimportAllSuccessDesc", "All files have been reimported. Please verify the node mappings in the Joint Manager assets."),
				EJointMDAdmonitionType::Mention
			);

			// Simply refresh the details panel. 
			if (PropertyUtils.IsValid()) PropertyUtils.Pin()->ForceRefresh();
			
			UJointScriptSettings::Save();

			return FReply::Handled();
		})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Padding(FJointEditorStyle::Margin_Small)
			[
				JointScriptLinkerCustomizationHelpers::CreateQuickReimportButtonContentWidget()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Padding(FJointEditorStyle::Margin_Small)
			[
				SNew(STextBlock)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
				.Text(LOCTEXT("QuickReimportAllButtonLabel", "Quick Reimport All"))
			]
		];
}

TSharedPtr<SWidget> FJointScriptLinkerDataCustomization::CreateRefreshWidgetButtonWidget()
{
	//Refresh button
	return SNew(SJointOutlineButton)
		.NormalColor(FLinearColor::Transparent)
		.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
		.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.OutlineNormalColor(FLinearColor::Transparent)
		.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
		.ContentPadding(FJointEditorStyle::Margin_Normal)
		.ToolTipText(LOCTEXT("RefreshToolTip", "Refresh the details panel to reflect any changes in the data"))
		.OnClicked_Lambda([this]()
		{
			// Simply refresh the details panel. 
			if (PropertyUtils.IsValid()) PropertyUtils.Pin()->ForceRefresh();

			return FReply::Handled();
		})
		[
			SNew(SBox)
			.WidthOverride(14)
			.HeightOverride(14)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SImage)
				.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Refresh"))
				.DesiredSizeOverride(FVector2D(12, 12))
				.ColorAndOpacity(FLinearColor::White)
			]
		];
}

void FJointScriptLinkerFileEntryCustomization::CustomizeStructHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils)
{
	HeaderRowPtr = &HeaderRow;
	ThisStructPropertyHandle = StructPropertyHandle;
	//No special header needed - create default on

	//if (const TSharedPtr<IPropertyHandle> FileNameHandle =StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath)))
	//{
	//	FileNameHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FJointScriptLinkerFileEntryCustomization::BuildFilterString));
	//}

	if (HeaderRowPtr)
	{
		TSharedPtr<IPropertyHandle> FilePathHandle = StructPropertyHandle->GetChildHandle(
			GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath));

		HeaderRowPtr->PropertyHandles = TArray<TSharedPtr<IPropertyHandle>>{
			StructPropertyHandle,
			FilePathHandle
		};

		HeaderRowPtr->WholeRowContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				CreateValidityIconWidget(StructPropertyHandle).ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Tiny)
			[
				CreateNameTextWidget(StructPropertyHandle).ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Tiny)
			[
				CreatePathTextWidget(StructPropertyHandle).ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Tiny)
			[
				CreateOpenOnFileExplorerButtonWidget(StructPropertyHandle).ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Tiny)
			[
				CreateReassignFileButtonWidget(StructPropertyHandle).ToSharedRef()
			]
		];
	}
}

void FJointScriptLinkerFileEntryCustomization::CustomizeStructChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils)
{
	ChildBuilder.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath)).ToSharedRef()); //.Visibility(EVisibility::Collapsed);
}


TSharedPtr<SWidget> FJointScriptLinkerFileEntryCustomization::CreateValidityIconWidget(TSharedPtr<IPropertyHandle> StructPropertyHandle)
{
	TWeakPtr<IPropertyHandle> WeakStructPropertyHandle = StructPropertyHandle;
	
	//lambda for validity check
	auto ValidityCheck = [WeakStructPropertyHandle]() -> const bool
	{
		if (!WeakStructPropertyHandle.IsValid()) return false;
		
		const TSharedPtr<IPropertyHandle> FilePathHandle = WeakStructPropertyHandle.Pin()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath));

		FString OutputString;

		FilePathHandle->GetValueAsFormattedString(OutputString);

		return !OutputString.IsEmpty() && FPaths::FileExists(OutputString);
	};

	//Validity icon - change the icon based on whether the file exists or not
	return SNew(SImage)
		.ColorAndOpacity_Lambda([ValidityCheck]() -> const FSlateColor
		{
			return ValidityCheck() ? FLinearColor::Green + FLinearColor(0.15, 0.15, 0.15) : FLinearColor::Red + FLinearColor(0.15, 0.15, 0.15);
		})
		.Image_Lambda([ValidityCheck]() -> const FSlateBrush*
		{
			return ValidityCheck() ? FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Check") : FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Error");
		})
		.DesiredSizeOverride(FVector2D(16, 16))
		.ToolTipText_Lambda([ValidityCheck]() -> const FText
		{
			return ValidityCheck() ? LOCTEXT("FileEntryValidToolTip", "The specified file path is valid.") : LOCTEXT("FileEntryInvalidToolTip", "The specified file path is missing or invalid.");
		});
}

TSharedPtr<SWidget> FJointScriptLinkerFileEntryCustomization::CreateNameTextWidget(TSharedPtr<IPropertyHandle> StructPropertyHandle)
{
	TWeakPtr<IPropertyTypeCustomization> WeakThisPtr = AsShared();
	TWeakPtr<IPropertyHandle> WeakStructPropertyHandle = StructPropertyHandle;

	// show name of the file first and then the path in ()
	return SNew(STextBlock)
		.TextStyle(&FJointEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("JointUI.TextBlock.Black.h4"))
		.HighlightText_Lambda([WeakThisPtr]() -> const FText
		{
			if (!WeakThisPtr.IsValid()) return FText::GetEmpty();
			
			TSharedPtr<FJointScriptLinkerFileEntryCustomization> CastedPtr = StaticCastSharedPtr<FJointScriptLinkerFileEntryCustomization>(WeakThisPtr.Pin());
			
			return (CastedPtr->HeaderRowPtr != nullptr) ? CastedPtr->HeaderRowPtr->FilterTextString : FText::GetEmpty();
		})
		.Text_Lambda([WeakStructPropertyHandle]() -> const FText
		{
			if (!WeakStructPropertyHandle.IsValid()) return FText::FromString(TEXT("N/A"));
			
			const TSharedPtr<IPropertyHandle> FilePathHandle = WeakStructPropertyHandle.Pin()->GetChildHandle(
				GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath));

			FString FileNameString;

			FilePathHandle->GetValueAsFormattedString(FileNameString);

			if (FileNameString.IsEmpty())
			{
				FileNameString = TEXT("N/A");
			}

			return FText::Format(
				LOCTEXT("FileEntryNameWithPathTemplate", "{0}"),
				FText::FromString(FPaths::GetCleanFilename(FileNameString))
			);
		});
}

TSharedPtr<SWidget> FJointScriptLinkerFileEntryCustomization::CreatePathTextWidget(TSharedPtr<IPropertyHandle> StructPropertyHandle)
{
	TWeakPtr<IPropertyTypeCustomization> WeakThisPtr = AsShared();
	TWeakPtr<IPropertyHandle> WeakStructPropertyHandle = StructPropertyHandle;

	// show name of the file first and then the path in ()
	return SNew(STextBlock)
		.TextStyle(&FJointEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("JointUI.TextBlock.Regular.h5"))
		.ColorAndOpacity(FLinearColor(0.66, 0.66, 0.66))
		.HighlightText_Lambda([WeakThisPtr]() -> const FText
		{
			if (!WeakThisPtr.IsValid()) return FText::GetEmpty();
			TSharedPtr<FJointScriptLinkerFileEntryCustomization> CastedPtr = StaticCastSharedPtr<FJointScriptLinkerFileEntryCustomization>(WeakThisPtr.Pin());
			return (CastedPtr->HeaderRowPtr != nullptr) ? CastedPtr->HeaderRowPtr->FilterTextString : FText::GetEmpty();
		})
		.Text_Lambda([WeakStructPropertyHandle]() -> const FText
		{
			if (!WeakStructPropertyHandle.IsValid()) return FText::FromString(TEXT("N/A"));
			
			const TSharedPtr<IPropertyHandle> FilePathHandle = WeakStructPropertyHandle.Pin()->GetChildHandle(
				GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath));

			FString FilePathString;

			FilePathHandle->GetValueAsFormattedString(FilePathString);

			if (FilePathString.IsEmpty())
			{
				FilePathString = TEXT("N/A");
			}

			return FText::Format(
				LOCTEXT("FileEntryNameWithPathTemplate", "({0})"),
				FText::FromString(FilePathString)
			);
		});
}

TSharedPtr<SWidget> FJointScriptLinkerFileEntryCustomization::CreateOpenOnFileExplorerButtonWidget(TSharedPtr<IPropertyHandle> StructPropertyHandle)
{
	TWeakPtr<IPropertyHandle> WeakStructPropertyHandle = StructPropertyHandle;
	
	//lambda for validity check
	auto ValidityCheck = [WeakStructPropertyHandle]() -> const bool
	{
		if (!WeakStructPropertyHandle.IsValid()) return false;
		
		const TSharedPtr<IPropertyHandle> FilePathHandle = WeakStructPropertyHandle.Pin()->GetChildHandle(
			GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath));

		FString OutputString;

		FilePathHandle->GetValueAsFormattedString(OutputString);

		return !OutputString.IsEmpty() && FPaths::FileExists(OutputString);
	};

	TSharedPtr<SWidget> Widget = SNew(SJointOutlineButton)
		.NormalColor(FLinearColor::Transparent)
		.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
		.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.OutlineNormalColor(FLinearColor::Transparent)
		.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
		.ContentPadding(FJointEditorStyle::Margin_Normal)
		.ToolTipText(LOCTEXT("OpenInExplorerToolTip", "Open file location in Explorer"))
		.OnClicked_Lambda([WeakStructPropertyHandle]()
		{
			if (!WeakStructPropertyHandle.IsValid()) return FReply::Handled();
			
			TSharedPtr<IPropertyHandle> FilePathHandle = WeakStructPropertyHandle.Pin()->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath));

			FString OutputString;

			FilePathHandle->GetValueAsFormattedString(OutputString);

			if (OutputString.IsEmpty())
			{
				FJointEdUtils::FireNotification(
					LOCTEXT("OpenInExplorerFailedTitle", "Open in Explorer Failed"),
					LOCTEXT("OpenInExplorerFailedDesc", "File path is empty."),
					EJointMDAdmonitionType::Error
				);

				return FReply::Handled();
			}
			else if (!FPaths::FileExists(OutputString))
			{
				FJointEdUtils::FireNotification(
					LOCTEXT("OpenInExplorerFailedTitle", "Open in Explorer Failed"),
					LOCTEXT("OpenInExplorerFailedDesc_NotExist", "File does not exist at the specified path."),
					EJointMDAdmonitionType::Error
				);

				return FReply::Handled();
			}

			FJointEdUtils::OpenFileInExplorer(OutputString);

			return FReply::Handled();
		})
		[
			SNew(SBox)
			.WidthOverride(14)
			.HeightOverride(14)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SImage)
				.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.FolderOpen"))
				.DesiredSizeOverride(FVector2D(16, 16))
				.ColorAndOpacity_Lambda([ValidityCheck]() -> const FSlateColor
				{
					return ValidityCheck() ? FLinearColor(1, 1, 1) : FLinearColor(0.5, 0.5, 0.5);
				})
			]
		];

	return Widget;
}

TSharedPtr<SWidget> FJointScriptLinkerFileEntryCustomization::CreateReassignFileButtonWidget(TSharedPtr<IPropertyHandle> StructPropertyHandle)
{
	TSharedPtr<SWidget> Widget = SNew(SJointOutlineButton)
		.NormalColor(FLinearColor::Transparent)
		.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
		.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.OutlineNormalColor(FLinearColor::Transparent)
		.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
		.ContentPadding(FJointEditorStyle::Margin_Normal)
		.ToolTipText(LOCTEXT("ReassignFileToolTip", "Reassign file for this entry"))
		.OnClicked_Lambda([StructPropertyHandle, this]()
		{
			TArray<FString> OutFilePaths;

			FJointEdUtils::OpenJointScriptFileSelectionWindow(
				OutFilePaths, 
				false);

			if (!OutFilePaths.IsEmpty())
			{
				const FString FirstFilePath = OutFilePaths[0];

				const TSharedPtr<IPropertyHandle> FilePathHandle = StructPropertyHandle->GetChildHandle(
					GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath));

				FilePathHandle->SetValueFromFormattedString(FirstFilePath);

				// get the parent struct handle to trigger OnValueChanged event
				TSharedPtr<IPropertyHandle> ParentStructHandle = StructPropertyHandle->GetParentHandle();
				if (ParentStructHandle.IsValid())
				{
					ParentStructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
				}
				
				FJointEdUtils::FireNotification(
					LOCTEXT("FileReassignSuccessTitle", "File Reassigned"),
					FText::Format(LOCTEXT("FileReassignSuccessDesc", "File has been reassigned to {0}."), FText::FromString(FirstFilePath)),
					EJointMDAdmonitionType::Mention
				);
			}
			else
			{
				FJointEdUtils::FireNotification(
					LOCTEXT("FileReassignCanceledTitle", "File Reassign Canceled"),
					LOCTEXT("FileReassignCanceledDesc", "No file was selected for reassignment."),
					EJointMDAdmonitionType::Warning
				);
			}
			
			UJointScriptSettings::Save();

			return FReply::Handled();
		})
		[
			SNew(SBox)
			.WidthOverride(14)
			.HeightOverride(14)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SImage)
				.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Audit"))
				.DesiredSizeOverride(FVector2D(16, 16))
				.ColorAndOpacity(FLinearColor::White)
			]
		];

	return Widget;
}

FJointArrayElementBuilder::FJointArrayElementBuilder(TSharedPtr<IPropertyHandle> InPropertyHandle)
{
	PropertyHandle = InPropertyHandle;
}

void FJointArrayElementBuilder::SetOnRebuildChildren(FSimpleDelegate InOnRebuildChildren)
{
	IDetailCustomNodeBuilder::SetOnRebuildChildren(InOnRebuildChildren);
}

void FJointArrayElementBuilder::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{
	/*
	//Create PropertyHandle widget
	NodeRow.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	].ValueContent()
	[
		PropertyHandle->CreatePropertyValueWidget()
	];
	*/
}


void FJointArrayElementBuilder::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	TSharedPtr<class IPropertyHandleArray> ArrayPH = PropertyHandle->AsArray();

	if (!ArrayPH) return;

	uint32 Num = 0;
	ArrayPH->GetNumElements(Num);

	for (uint32 Index = 0; Index < Num; ++Index)
	{
		TSharedPtr<IPropertyHandle> ElementHandle = ArrayPH->GetElement(Index);

		ChildrenBuilder.AddProperty(ElementHandle.ToSharedRef());
	}
}

bool FJointArrayElementBuilder::RequiresTick() const
{
	return IDetailCustomNodeBuilder::RequiresTick();
}


void FJointArrayElementBuilder::Tick(float DeltaTime)
{
	IDetailCustomNodeBuilder::Tick(DeltaTime);
}

FName FJointArrayElementBuilder::GetName() const
{
	return FName("ArrayPropertyBuilder");
}

bool FJointArrayElementBuilder::InitiallyCollapsed() const
{
	return false;
}


TSharedRef<IPropertyTypeCustomization> FJointScriptLinkerMappingCustomization::MakeInstance()
{
	return MakeShareable(new FJointScriptLinkerMappingCustomization);
}

FJointScriptLinkerMappingCustomization::SCandidateRow::FArguments::FArguments()
{
}

void FJointScriptLinkerMappingCustomization::SCandidateRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
{
	Item = InArgs._Item;

	SMultiColumnTableRow<TSharedPtr<FCandidateItem>>::Construct(
		FSuperRowType::FArguments(),
		OwnerTable
	);
}

TSharedRef<SWidget> FJointScriptLinkerMappingCustomization::SCandidateRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == "Name")
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->Name))
				.ColorAndOpacity(FLinearColor::White)
				.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Tiny)
			[
				SNew(SJointOutlineButton)
				.NormalColor(FLinearColor::Transparent)
				.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
				.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OutlineNormalColor(FLinearColor::Transparent)
				.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
				.ContentPadding(FJointEditorStyle::Margin_Tiny)
				.ToolTipText(LOCTEXT("GoToButtonToolTip", "Go to this Joint Manager asset in the Content Browser"))
				.OnClicked(this, &FJointScriptLinkerMappingCustomization::SCandidateRow::OnGoToButtonPressed)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.ArrowRight"))
					.DesiredSizeOverride(FVector2D(12, 12))
					.ColorAndOpacity(FLinearColor::White)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FJointEditorStyle::Margin_Tiny)
			[
				SNew(SJointOutlineButton)
				.NormalColor(FLinearColor::Transparent)
				.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
				.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.OutlineNormalColor(FLinearColor::Transparent)
				.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
				.ContentPadding(FJointEditorStyle::Margin_Tiny)
				.ToolTipText(LOCTEXT("OpenEditorButtonToolTip", "Open this Joint Manager asset in the editor"))
				.OnClicked(this, &FJointScriptLinkerMappingCustomization::SCandidateRow::OnOpenEditorButtonPressed)
				[
					SNew(SImage)
					.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Edit"))
					.DesiredSizeOverride(FVector2D(12, 12))
					.ColorAndOpacity(FLinearColor::White)
				]
			];
	}
	else if (ColumnName == "Path")
	{
		return SNew(STextBlock)
			.Text(FText::FromString(Item->Path))
			.ColorAndOpacity(FLinearColor::Gray)
			.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h4");
	}
	else if (ColumnName == "Match")
	{
		return SNew(STextBlock) // display "no matching nodes" if MatchingNodeCount is 0, otherwise display the count
			.Text(Item->MatchingNodeCount > 0 ? FText::AsNumber(Item->MatchingNodeCount) : FText::FromString(TEXT("No Matching Nodes")))
			.ColorAndOpacity(Item->MatchingNodeCount > 0 ? FLinearColor::Green : FLinearColor::Red);
	}

	return SNullWidget::NullWidget;
}

FReply FJointScriptLinkerMappingCustomization::SCandidateRow::OnOpenEditorButtonPressed()
{
	if (Item->Manager.IsValid())
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Item->Manager.Get());
	}

	return FReply::Handled();
}


FReply FJointScriptLinkerMappingCustomization::SCandidateRow::OnGoToButtonPressed()
{
	if (Item->Manager.IsValid())
	{
		TArray<UObject*> ObjectsToSync;
		ObjectsToSync.Add(Item->Manager.Get());
		GEditor->SyncBrowserToObjects(ObjectsToSync);
	}

	return FReply::Handled();
}

// Add missing FArguments constructor for SReallocationPopup to resolve linker error
FJointScriptLinkerMappingCustomization::SReallocationPopup::FArguments::FArguments()
{
}

void FJointScriptLinkerMappingCustomization::SReallocationPopup::Construct(const FArguments& InArgs)
{
	ParentWindow = InArgs._ParentWindow;
	ParentJointManagerHandle = InArgs._JointManagerHandle.Pin();
	CandidateItems = InArgs._CandidateItems;
	ParentCustomizationPtr = InArgs._ParentCustomizationPtr;


	this->ChildSlot.AttachWidget(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FJointEditorStyle::Margin_Normal)
		[
			SNew(SJointMDSlate_Admonitions)
			.AdmonitionType(EJointMDAdmonitionType::Info)
			.bAutoWrapDescriptionText(true)
			.bUseDescriptionText(true)
			.DescriptionText(
				LOCTEXT(
					"ReallocateJointManager_Info",
					"Select a Joint Manager to reassign for this mapping and press OK to proceed the reallocation. The number of matching nodes (compared to the current mapping) is shown to help identify the correct one, but please verify by checking the Joint Manager's graph assets may have similar node counts."
				)
			)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1)
		.Padding(FJointEditorStyle::Margin_Normal)
		[
			SAssignNew(ReallocationPopupListView, SListView<TSharedPtr<FCandidateItem>>)
			.ListItemsSource(&CandidateItems)
			.SelectionMode(ESelectionMode::Single)
			.OnGenerateRow_Lambda([](TSharedPtr<FCandidateItem> InItem, const TSharedRef<STableViewBase>& Owner)
			{
				return SNew(SCandidateRow, Owner)
					.Item(InItem);
			})
			.OnSelectionChanged_Lambda([this](TSharedPtr<FCandidateItem> NewSelection, ESelectInfo::Type)
			{
				SelectedItem = NewSelection;
			})
			.HeaderRow
			(
				SNew(SHeaderRow)
				+ SHeaderRow::Column("Name")
				.DefaultLabel(LOCTEXT("Column_Name", "Name"))
				.FillWidth(0.3f)
				.OnSort(this, &SReallocationPopup::OnColumnSort)
				.SortMode(
					TAttribute<EColumnSortMode::Type>::Create(
						TAttribute<EColumnSortMode::Type>::FGetter::CreateSP(
							this,
							&SReallocationPopup::GetColumnSortMode,
							FName("Name")
						)
					)
				)
				+ SHeaderRow::Column("Path")
				.DefaultLabel(LOCTEXT("Column_Path", "Path"))
				.FillWidth(0.5f)
				.OnSort(this, &SReallocationPopup::OnColumnSort)
				.SortMode(
					TAttribute<EColumnSortMode::Type>::Create(
						TAttribute<EColumnSortMode::Type>::FGetter::CreateSP(
							this,
							&SReallocationPopup::GetColumnSortMode,
							FName("Path")
						)
					)
				)
				+ SHeaderRow::Column("Match")
				.DefaultLabel(LOCTEXT("Column_Match", "Matching Nodes"))
				.FillWidth(0.2f)
				.OnSort(this, &SReallocationPopup::OnColumnSort)
				.SortMode(
					TAttribute<EColumnSortMode::Type>::Create(
						TAttribute<EColumnSortMode::Type>::FGetter::CreateSP(
							this,
							&SReallocationPopup::GetColumnSortMode,
							FName("Match")
						)
					)
				)
			)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.Padding(FJointEditorStyle::Margin_Normal)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SButton)
				.Text(LOCTEXT("Cancel", "Cancel"))
				.OnClicked_Lambda([this]() -> FReply
				{
					if (ParentWindow.IsValid()) ParentWindow.Pin()->RequestDestroyWindow();

					if (ParentCustomizationPtr.Pin()) ParentCustomizationPtr.Pin()->RefreshVisual();

					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FJointEditorStyle::Margin_Normal)
			[
				SNew(SButton)
				.Text(LOCTEXT("Proceed", "Proceed"))
				.ButtonColorAndOpacity(FLinearColor(0.5, 0.7, 1.0))
				.IsEnabled_Lambda([this]() -> bool
				{
					return SelectedItem.IsValid() && SelectedItem.Pin()->Manager.IsValid();
				})
				.OnClicked_Lambda([this]() -> FReply
				{
					if (SelectedItem.IsValid() && SelectedItem.Pin()->Manager.IsValid() && ParentJointManagerHandle.IsValid())
					{
						// Assign the selected JointManager to the JointManager property handle
						UObject* ObjToSet = SelectedItem.Pin()->Manager.Get();
						FPropertyAccess::Result Result = ParentJointManagerHandle->SetValue(ObjToSet);
					}

					FJointEdUtils::FireNotification(
						LOCTEXT("ReallocationSuccessTitle", "Joint Manager Reallocated"),
						LOCTEXT("ReallocationSuccessDesc", "The Joint Manager reference has been successfully reallocated."),
						EJointMDAdmonitionType::Mention
					);

					if (ParentWindow.IsValid()) ParentWindow.Pin()->RequestDestroyWindow();

					if (ParentCustomizationPtr.Pin()) ParentCustomizationPtr.Pin()->RefreshVisual();
					
					UJointScriptSettings::Save();

					return FReply::Handled();
				})
			]
		]
	);
}

void FJointScriptLinkerMappingCustomization::SReallocationPopup::OnColumnSort(
	EColumnSortPriority::Type,
	const FName& ColumnId,
	EColumnSortMode::Type SortMode)
{
	ReallocationCurrentSortColumn = ColumnId;
	ReallocationCurrentSortMode = SortMode;

	CandidateItems.Sort([&](const TSharedPtr<FCandidateItem>& A, const TSharedPtr<FCandidateItem>& B)
	{
		int32 Result = 0;

		if (ColumnId == "Name")
			Result = A->Name.Compare(B->Name);
		else if (ColumnId == "Path")
			Result = A->Path.Compare(B->Path);
		else if (ColumnId == "Match")
			Result = A->MatchingNodeCount - B->MatchingNodeCount;

		return SortMode == EColumnSortMode::Ascending ? Result < 0 : Result > 0;
	});

	if (ReallocationPopupListView.IsValid())
	{
		ReallocationPopupListView->RequestListRefresh();
	}
}

EColumnSortMode::Type FJointScriptLinkerMappingCustomization::SReallocationPopup::GetColumnSortMode(FName ColumnId) const
{
	return (ReallocationCurrentSortColumn == ColumnId) ? ReallocationCurrentSortMode : EColumnSortMode::None;
}

TSharedRef<SWidget> FJointScriptLinkerMappingCustomization::CreateContentWidget()
{
	TSharedPtr<IPropertyHandle> JointManagerHandle = ThisStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, JointManager));
	TSharedPtr<IPropertyHandle> NodeMappingsHandle = ThisStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, NodeMappings));

	//iterate NodeStatusItems and count bExistsInGraph
	int32 RecognizedCount = 0;
	int32 TotalCount = NodeStatusItems.Num();
	for (const TSharedPtr<FNodeStatusItem>& Item : NodeStatusItems)
	{
		if (!Item->bExistsInGraph) continue;

		++RecognizedCount;
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FJointEditorStyle::Margin_Small)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0)
				[
					SNew(SJointOutlineButton)
					.NormalColor(FLinearColor::Transparent)
					.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
					.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.OutlineNormalColor(FLinearColor::Transparent)
					.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
					.ContentPadding(FJointEditorStyle::Margin_Normal)
					.ToolTipText(LOCTEXT("ReallocateToolTip", "Reallocate the Joint Manager reference for this mapping (useful if the Joint Manager was deleted and recreated)"))
					.OnClicked(this, &FJointScriptLinkerMappingCustomization::OnReallocateButtonPressed)
					[
						JointScriptLinkerCustomizationHelpers::CreateReallocateButtonContentWidget()
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0)
				[
					SNew(SJointOutlineButton)
					.NormalColor(FLinearColor::Transparent)
					.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
					.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.OutlineNormalColor(FLinearColor::Transparent)
					.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
					.ContentPadding(FJointEditorStyle::Margin_Normal)
					.ToolTipText(LOCTEXT("ReimportToolTip", "Reimport the file for this specific mapping"))
					.OnClicked(this, &FJointScriptLinkerMappingCustomization::OnReimportButtonPressed)
					[
						JointScriptLinkerCustomizationHelpers::CreateReimportButtonContentWidget()
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0)
				[
					SNew(SJointOutlineButton)
					.NormalColor(FLinearColor::Transparent)
					.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
					.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.OutlineNormalColor(FLinearColor::Transparent)
					.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
					.ContentPadding(FJointEditorStyle::Margin_Normal)
					.ToolTipText(LOCTEXT("QuickReimportToolTip", "Quickly reimport the file for this mapping using the same parser settings as the last import (if available)"))
					.OnClicked(this, &FJointScriptLinkerMappingCustomization::OnQuickReimportButtonPressed)
					[
						JointScriptLinkerCustomizationHelpers::CreateQuickReimportButtonContentWidget()
					]
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FJointEditorStyle::Margin_Small)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			[
				ThisStructPropertyHandle->GetChildHandle(
					GET_MEMBER_NAME_CHECKED(
						FJointScriptLinkerMapping,
						JointManager
					))->CreatePropertyValueWidget()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FJointEditorStyle::Margin_Small)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			[
				ThisStructPropertyHandle->CreateDefaultPropertyButtonWidgets()
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding(FJointEditorStyle::Margin_Small)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right) // indicator for the summaries
			[
				SNew(SImage) //if TotalCount == 0, show "Icons.Info" with white color to indicate no nodes are mapped, instead of showing "0/0 Recognized" with red color which can be misleading
				.Image_Lambda([RecognizedCount, TotalCount]() -> const FSlateBrush*
				{
					if (TotalCount == 0)
					{
						return FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Info");
					}
					return RecognizedCount == TotalCount
						       ? FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Check")
						       : (RecognizedCount == 0
							          ? FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Error")
							          : FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Warning"));
				})
				.DesiredSizeOverride(FVector2D(16, 16))
				.ColorAndOpacity_Lambda([RecognizedCount, TotalCount]() -> FLinearColor
				{
					if (TotalCount == 0)
					{
						return FLinearColor::White;
					}
					return RecognizedCount == TotalCount
						       ? FLinearColor::Green
						       : (RecognizedCount == 0 ? FLinearColor::Red : FLinearColor::Yellow);
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FJointEditorStyle::Margin_Small)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right) // indicator for the summaries
			[
				SNew(STextBlock)
				.TextStyle(&FJointEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("JointUI.TextBlock.Regular.h3"))
				.Text_Lambda([this, RecognizedCount, TotalCount]() -> FText
				{
					//if TotalCount == 0, show "No Nodes Mapped" instead of "0/0 Recognized" which can be misleading
					if (TotalCount == 0)
					{
						return LOCTEXT("NoNodesMapped", "No Nodes Mapped");
					}
					return FText::Format(LOCTEXT("NodeMappingSummaryFormat", "{0}/{1} Recognized"), FText::AsNumber(RecognizedCount), FText::AsNumber(TotalCount));
				})
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0)
		[
			SNew(SVerticalBox)
			.Visibility_Lambda([this]() -> EVisibility
			{
				return NodeStatusItems.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
			})
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FJointEditorStyle::Margin_Small)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(STextBlock)
					.TextStyle(&FJointEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("JointUI.TextBlock.Black.h5"))
					.Text(LOCTEXT("NodeMappingsLabel", "Node Mappings"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FJointEditorStyle::Margin_Small)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(SJointOutlineButton)
					.NormalColor(FLinearColor::Transparent)
					.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
					.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.OutlineNormalColor(FLinearColor::Transparent)
					.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
					.ContentPadding(FJointEditorStyle::Margin_Tiny)
					.ToolTipText(LOCTEXT("CleanAllButtonToolTip", "Clean all node mappings."))
					.OnClicked(this, &FJointScriptLinkerMappingCustomization::OnCleanAllButtonClicked)
					[
						SNew(SImage)
						.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Delete"))
						.DesiredSizeOverride(FVector2D(12, 12))
						.ColorAndOpacity(FLinearColor::White)
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FJointEditorStyle::Margin_Small)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(SJointOutlineButton)
					.NormalColor(FLinearColor::Transparent)
					.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
					.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
					.OutlineNormalColor(FLinearColor::Transparent)
					.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
					.ContentPadding(FJointEditorStyle::Margin_Tiny)
					.ToolTipText(LOCTEXT("CleanInvalidButtonToolTip", "Clean invalid node mappings that do not exist in the Joint Manager's graph."))
					.OnClicked(this, &FJointScriptLinkerMappingCustomization::OnCleanInvalidButtonClicked)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SImage)
							.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.Delete"))
							.DesiredSizeOverride(FVector2D(12, 12))
							.ColorAndOpacity(FLinearColor::White)
							.RenderTransform(FSlateRenderTransform(FVector2D(0, 0)))
						]
						+ SOverlay::Slot()
						[
							SNew(SImage)
							.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("GraphEditor.Macro.IsValid_16x"))
							.DesiredSizeOverride(FVector2D(12, 12))
							.ColorAndOpacity(FLinearColor::White)
							.RenderTransform(FSlateRenderTransform(FVector2D(-5, -5)))
						]
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0)
			[
				SNew(SExpandableArea)
				.AreaTitle(LOCTEXT("NodeMappingStatusTitle", "Node Mapping Status"))
				.InitiallyCollapsed(true)
				.BodyContent()
				[
					SNew(SListView<TSharedPtr<FNodeStatusItem>>)
					.ListItemsSource(&NodeStatusItems)
					.ItemHeight(22)
					.OnGenerateRow_Lambda([JointManagerHandle](TSharedPtr<FNodeStatusItem> InItem, const TSharedRef<STableViewBase>& Owner)
					{
						return SNew(STableRow<TSharedPtr<FNodeStatusItem>>, Owner)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(0.4f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(InItem->Key))
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.HAlign(HAlign_Left)
								[
									SNew(STextBlock)
									.Text(FText::FromString(InItem->Guid.ToString()))
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.HAlign(HAlign_Left)
								[
									SNew(SJointOutlineButton)
									.NormalColor(FLinearColor::Transparent)
									.HoverColor(FLinearColor(0.06, 0.06, 0.1, 1))
									.OutlineBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
									.OutlineNormalColor(FLinearColor::Transparent)
									.ButtonStyle(FJointEditorStyle::Get(), "JointUI.Button.Round.White")
									.ContentPadding(FJointEditorStyle::Margin_Tiny)
									.ToolTipText(LOCTEXT("GoToButtonToolTip", "Go to this node in the Joint Manager's graph"))
									.IsEnabled(InItem->bExistsInGraph)
									.OnClicked_Lambda([InItem, JointManagerHandle]()
									{
										UObject* JointManagerObj = nullptr;
										JointManagerHandle->GetValue(JointManagerObj);

										UJointManager* JM = Cast<UJointManager>(JointManagerObj);
										if (!JM)
										{
											FJointEdUtils::FireNotification(
												LOCTEXT("GoToNodeFailedTitle", "Go To Node Failed"),
												LOCTEXT("GoToNodeFailedDesc", "Unable to retrieve the Joint Manager reference."),
												EJointMDAdmonitionType::Error
											);
											return FReply::Handled();
										}

										FJointEditorToolkit* Toolkit = FJointEdUtils::FindOrOpenJointEditorInstanceFor(JM, true, true);

										if (Toolkit)
										{
											Toolkit->JumpToNodeGuid(InItem->Guid);
										}

										return FReply::Handled();
									})
									[
										SNew(SImage)
										.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush("Icons.ArrowRight"))
										.DesiredSizeOverride(FVector2D(16, 16))
										.ColorAndOpacity(FLinearColor::White)
									]
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.HAlign(HAlign_Right)
								.VAlign(VAlign_Center)
								[
									SNew(SImage)
									.Image(FJointEditorStyle::GetUEEditorSlateStyleSet().GetBrush(InItem->bExistsInGraph ? "Icons.Check" : "Icons.Error"))
									.ColorAndOpacity(InItem->bExistsInGraph ? FLinearColor::Green : FLinearColor::Red)
									.ToolTipText(
										InItem->bExistsInGraph ? LOCTEXT("NodeExistsToolTip", "This node exists in the Joint Manager's graph.") : LOCTEXT("NodeMissingToolTip", "This node is missing from the Joint Manager's graph."))
									.DesiredSizeOverride(FVector2D(12, 12))
								]
							];
					})
				]

			]
		];
}

void FJointScriptLinkerMappingCustomization::CustomizeStructHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils)
{
	//StructPropertyHandle->AddChildStructure();

	// Store the struct property handle for later use
	ThisStructPropertyHandle = StructPropertyHandle;
	HeaderRowPtr = &HeaderRow;

	CleanUp();

	BuildNodeStatusItems();

	if (HeaderRowPtr)
	{
		HeaderRowPtr->WholeRowContent()
		[
			SAssignNew(BodyBorder, SJointOutlineBorder)
			.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
			.OutlineNormalColor(FLinearColor(0.04, 0.04, 0.04))
			.OutlineHoverColor(FJointEditorStyle::Color_Selected)
			.ContentPadding(FJointEditorStyle::Margin_Large)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				CreateContentWidget()
			]
		];
	}

	BuildFilterString();
}


void FJointScriptLinkerMappingCustomization::CustomizeStructChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils)
{
	ChildBuilder.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, JointManager)).ToSharedRef()).IsEnabled(false);
	ChildBuilder.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, NodeMappings)).ToSharedRef()).IsEnabled(false);
}

void FJointScriptLinkerMappingCustomization::CleanUp()
{
	NodeStatusItems.Reset();
}

FReply FJointScriptLinkerMappingCustomization::OnReallocateButtonPressed()
{
	if (!ThisStructPropertyHandle) return FReply::Handled();

	//array
	TArray<TSharedPtr<FCandidateItem>> Items;

	// Get the child handle for JointManager and NodeMappings (if present)
	TSharedPtr<IPropertyHandle> JointManagerHandle = ThisStructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, JointManager));
	TSharedPtr<IPropertyHandle> NodeMappingsHandle = ThisStructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, NodeMappings));


	// Gather candidate JointManager assets via Asset Registry (best-effort).
	TArray<UJointManager*> CandidateManagers;
	{
		// Try Asset Registry first (falls back gracefully if module not available)
		bool bFoundAny = false;

		TArray<FAssetData> AssetDataList;
		FJointEdUtils::GetAssetOfType<UJointManager>(AssetDataList);
			
		for (const FAssetData& AD : AssetDataList)
		{
			UObject* Obj = AD.GetAsset();
			if (UJointManager* JM = Cast<UJointManager>(Obj))
			{
				CandidateManagers.Add(JM);
			}
		}
		bFoundAny = AssetDataList.Num() > 0;
	}

	// collect node id from the mapping to compare with candidate Joint Managers' mappings for display purposes
	// TMap<FString, FJointScriptLinkerNodeSet> NodeMappings;
	TSet<FGuid> RecognizedNodeIDs;
	if (NodeMappingsHandle.IsValid())
	{
		TSharedPtr<IPropertyHandleMap> MapHandle;
		FMapProperty* MapProperty;
		void* MapContainer;
		FScriptMapHelper MapHelper = GetMapHelpers(NodeMappingsHandle, MapProperty, MapContainer);

		for (int32 Index = 0; Index < MapHelper.GetMaxIndex(); ++Index)
		{
			if (!MapHelper.IsValidIndex(Index)) continue;

			FJointScriptLinkerNodeSet Value;
			MapProperty->ValueProp->CopyCompleteValue(&Value, MapHelper.GetValuePtr(Index));

			for (const FGuid& Guid : Value.NodeGuids)
			{
				RecognizedNodeIDs.Add(Guid);
			}
		}
	}


	for (auto& JMRef : CandidateManagers)
	{
		UJointManager* JM = JMRef;
		if (!JM) continue;

		FString Name = JM->GetName();
		FString Path = JM->GetPathName();
		int32 MatchingNodeCount = 0;

		// iterate through the Joint Manager's NodeMappings and count how many entries match the recognized node IDs from the current mapping

		TSet<FGuid> CandidateNodeIDs;
		JointScriptLinkerCustomizationHelpers::GetAllNodeGuidsFromJointManager(JM, CandidateNodeIDs);

		for (const FGuid& CandidateNodeID : CandidateNodeIDs)
		{
			if (!RecognizedNodeIDs.Contains(CandidateNodeID)) continue;

			MatchingNodeCount++;
		}

		// Display the mapping entries count as a heuristic for resolvable nodes
		TSharedPtr<FCandidateItem> Item = MakeShared<FCandidateItem>();
		Item->Manager = JM;
		Item->Name = Name;
		Item->Path = Path;
		Item->MatchingNodeCount = MatchingNodeCount;
		Items.Add(Item);
	}

	// If no candidates, notify and return
	if (Items.Num() == 0)
	{
		FJointEdUtils::FireNotification(
			LOCTEXT("NoCandidateManagersTitle", "No Candidate Joint Managers Found"),
			LOCTEXT("NoCandidateManagersDesc", "No Joint Manager assets were found in the project. Please create a Joint Manager asset before reallocating."),
			EJointMDAdmonitionType::Warning
		);
	}

	//sort candidates by number of matching nodes.
	Items.Sort([&](const TSharedPtr<FCandidateItem>& A, const TSharedPtr<FCandidateItem>& B)
	{
		int32 Result = 0;

		Result = A->MatchingNodeCount - B->MatchingNodeCount;

		return Result > 0;
	});


	// Create a simple modal window with a list and OK/Cancel
	ReallocationWindow = SNew(SWindow)
		.Title(LOCTEXT("ReallocateJointManager_Title", "Reallocate Joint Manager"))
		.ClientSize(FVector2D(1200, 800))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		.FocusWhenFirstShown(true);

	// List view widget
	ReallocationWindow->SetContent(
		SNew(SReallocationPopup)
		.CandidateItems(Items)
		.ParentWindow(ReallocationWindow)
		.JointManagerHandle(JointManagerHandle)
		.ParentCustomizationPtr(SharedThis(this))
	);


	// Show modal
	FSlateApplication::Get().AddWindow(ReallocationWindow.ToSharedRef(), true);
	
	UJointScriptSettings::Save();


	return FReply::Handled();
}

FReply FJointScriptLinkerMappingCustomization::OnReimportButtonPressed()
{
	if (!ThisStructPropertyHandle) return FReply::Handled();

	//check if we have a valid Joint Manager reference. If not, fire a notification and return handled.
	TSharedPtr<IPropertyHandle> JointManagerHandle = ThisStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, JointManager));
	UObject* JointManagerObj = nullptr;
	JointManagerHandle->GetValue(JointManagerObj);

	//Get the parent struct property handle and get FJointScriptLinkerDataElement from it, then get the file path and reimport to the Joint Manager in this mapping only.
	//ParentStructHandle will refer 'ScriptLinkData->ScriptLinks[i]->Mappings'
	TSharedPtr<IPropertyHandle> ParentStructHandle = ThisStructPropertyHandle->GetParentHandle();
	if (!ParentStructHandle) return FReply::Handled();

	//ParentStructHandle will now refer to 'ScriptLinkData->ScriptLinks[i]'
	ParentStructHandle = ParentStructHandle->GetParentHandle();
	if (!ParentStructHandle) return FReply::Handled();

	TSharedPtr<IPropertyHandle> FileEntryHandle = ParentStructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerDataElement, FileEntry));
	if (!FileEntryHandle) return FReply::Handled();


	TSharedPtr<IPropertyHandle> ParserDataHandle = ParentStructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerDataElement, ParserData));
	TSharedPtr<IPropertyHandle> ParserData_ScriptParser = ParserDataHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerParserData, ScriptParser));
	TSharedPtr<IPropertyHandle> ParserInstanceData_ScriptParser = ParserDataHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerParserData, ParserInstanceData));


	if (!ParserData_ScriptParser) return FReply::Handled();

	TSharedPtr<IPropertyHandle> FilePathHandle = FileEntryHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath));
	if (!FilePathHandle) return FReply::Handled();

	//get class from ParserData_ScriptParser
	UClass* ScriptParserClass = nullptr;
	FString SoftClassPath;
	ParserData_ScriptParser->GetValueAsFormattedString(SoftClassPath);
	
	
	//TArray<uint8> ParserInstanceData -> we have to get raw data.
	TArray<uint8> ParserInstanceData;
	void* ParserInstanceDataPtr = nullptr;

	if (ParserInstanceData_ScriptParser->GetValueData(ParserInstanceDataPtr) == FPropertyAccess::Success && ParserInstanceDataPtr != nullptr)
	{
		const TArray<uint8>* SourceArrayPtr = static_cast<TArray<uint8>*>(ParserInstanceDataPtr);
		ParserInstanceData = *SourceArrayPtr;
	}
	
	

	FSoftClassPath SoftClassPathObj(SoftClassPath);
	if (SoftClassPathObj.IsValid())
	{
		ScriptParserClass = Cast<UClass>(SoftClassPathObj.TryLoad());
	}

	FString FilePathString;
	FilePathHandle->GetValueAsFormattedString(FilePathString);


	FScopedTransaction ReimportTransaction(LOCTEXT("ReimportJointManagersTransaction", "Re-import Joint Managers from external files"));

	UJointScriptSettings::Get()->Modify();

	// Create a simple modal window with a list and OK/Cancel
	TSharedPtr<SWindow> ImportWindow = SNew(SWindow)
		.Title(LOCTEXT("ImportJointManagerWindowTitle", "Import Joint Manager"))
		.ClientSize(FVector2D(1200, 800))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		.FocusWhenFirstShown(true);

	// List view widget
	ImportWindow->SetContent(
		SNew(SJointManagerImportingPopup)
		.bIsReimporting(true)
		.OptionalJointManagerToImport(JointManagerObj)
		.InitiallySelectedParserClass(ScriptParserClass)
		.InitiallySelectedParserInstanceData(ParserInstanceData)
		.InitialImportMode(SJointManagerImportingPopup::EJointImportMode::ToSpecifiedJointManager)
		.ExternalFilePaths(TArray<FString>{FilePathString})
		.ParentWindow(ImportWindow)
	);

	// Show the window modally
	FSlateApplication::Get().AddModalWindow(ImportWindow.ToSharedRef(), nullptr);

	UJointScriptSettings::Save();

	RefreshVisual();

	return FReply::Handled();
}

FReply FJointScriptLinkerMappingCustomization::OnQuickReimportButtonPressed()
{
	if (!ThisStructPropertyHandle) return FReply::Handled();

	//check if we have a valid Joint Manager reference. If not, fire a notification and return handled.
	TSharedPtr<IPropertyHandle> JointManagerHandle = ThisStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, JointManager));

	UObject* JointManagerObj = nullptr;
	JointManagerHandle->GetValue(JointManagerObj);

	if (JointManagerObj == nullptr)
	{
		FJointEdUtils::FireNotification(
			LOCTEXT("QuickReimportFileFailedTitle_NullJointManager", "Quick Reimport File Skipped"),
			LOCTEXT("QuickReimportFileFailedDesc_NullJointManager", "Joint Manager reference is null. Please assign a valid Joint Manager before reimporting."),
			EJointMDAdmonitionType::Warning
		);

		return FReply::Handled();
	}


	//Get the parent struct property handle and get FJointScriptLinkerDataElement from it, then get the file path and reimport to the Joint Manager in this mapping only.
	//ParentStructHandle will refer 'ScriptLinkData->ScriptLinks[i]->Mappings'
	TSharedPtr<IPropertyHandle> ParentStructHandle = ThisStructPropertyHandle->GetParentHandle();
	if (!ParentStructHandle) return FReply::Handled();

	//ParentStructHandle will now refer to 'ScriptLinkData->ScriptLinks[i]'
	ParentStructHandle = ParentStructHandle->GetParentHandle();
	if (!ParentStructHandle) return FReply::Handled();

	TSharedPtr<IPropertyHandle> FileEntryHandle = ParentStructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerDataElement, FileEntry));
	if (!FileEntryHandle) return FReply::Handled();

	TSharedPtr<IPropertyHandle> FilePathHandle = FileEntryHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerFileEntry, FilePath));
	if (!FilePathHandle) return FReply::Handled();

	FString FilePathString;
	FilePathHandle->GetValueAsFormattedString(FilePathString);

	if (FilePathString.IsEmpty() || !FPaths::FileExists(FilePathString))
	{
		FJointEdUtils::FireNotification(
			LOCTEXT("QuickReimportFileFailedTitle", "Quick Reimport File Failed"),
			LOCTEXT("QuickReimportFileFailedDesc", "File path is empty or file does not exist at the specified path."),
			EJointMDAdmonitionType::Error
		);

		return FReply::Handled();
	}

	//Check if the parent struct is valid and the struct is FJointScriptLinkerDataElement, if so call the reimport function in JointScriptLinkerCustomizationHelpers, if not return handled.
	if (ParentStructHandle->GetProperty() && ParentStructHandle->GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(FJointScriptLinkerDataElement, Mappings))
	{
		FJointEdUtils::FireNotification(
			LOCTEXT("QuickReimportFileFailedTitle_InvalidStruct", "Quick Reimport File Failed"),
			LOCTEXT("QuickReimportFileFailedDesc_InvalidStruct", "Parent struct is not valid or is not of type FJointScriptLinkerDataElement."),
			EJointMDAdmonitionType::Error
		);
	}

	//FJointScriptLinkerDataElement is the parent struct of FJointScriptLinkerMapping, so we can call the reimport function in
	JointScriptLinkerCustomizationHelpers::Reimport_FJointScriptLinkerDataElement(
		FilePathString,
		ParentStructHandle);
	
	UJointScriptSettings::Save();

	RefreshVisual();

	return FReply::Handled();
}

FReply FJointScriptLinkerMappingCustomization::OnCleanAllButtonClicked()
{
	// it will simply just empty the NodeMappings map in this mapping, which will effectively clean all node mappings. The user can then reassign nodes manually or reimport to repopulate the mappings.

	if (!ThisStructPropertyHandle) return FReply::Handled();

	// ask if the user is sure about this action with a confirmation dialog, as this cannot be undone.
	EAppReturnType::Type Result = FMessageDialog::Open(
		EAppMsgType::YesNo,
		LOCTEXT("CleanAllMappingsConfirmationDesc", "Are you sure you want to clean all node mappings for this mapping? You can undo (ctrl + z) or reimport to repopulate the mappings if needed.")
	);

	if (Result != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	// ask if the user want to remove the nodes from the graph as well, which will permanently remove the nodes and cannot be undone.
	EAppReturnType::Type RemoveNodesResult = FMessageDialog::Open(
		EAppMsgType::YesNo,
		LOCTEXT("CleanAllMappingsRemoveNodesConfirmationDesc",
		        "Do you also want to remove the linked nodes from the Joint Manager's graph? This will permanently remove the nodes and CANNOT be undone. You can manually add the nodes back in the Joint Manager's graph if needed.")
	);

	TSharedPtr<IPropertyHandle> JointManagerHandle = ThisStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, JointManager));
	UJointManager* JointManager = JointScriptLinkerCustomizationHelpers::LoadJointManager(JointManagerHandle);

	FScopedTransaction Transaction(LOCTEXT("CleanAllMappingsTransaction", "Clean All Node Mappings"));
	ThisStructPropertyHandle->NotifyPreChange();

	TSharedPtr<IPropertyHandle> NodeMappingsHandle = ThisStructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, NodeMappings));

	TArray<void*> RawData;
	ThisStructPropertyHandle->AccessRawData(RawData);

	for (void* RawPtr : RawData)
	{
		if (!RawPtr) continue;

		FJointScriptLinkerMapping* Mapping = static_cast<FJointScriptLinkerMapping*>(RawPtr);

		if (RemoveNodesResult == EAppReturnType::Yes)
		{
			for (TPair<FString, FJointScriptLinkerNodeSet>& NodeMapping : Mapping->NodeMappings)
			{
				FJointEdUtils::RemoveNodesWithGuid(JointManager, NodeMapping.Value.NodeGuids);
			}
		}

		Mapping->NodeMappings.Empty();
	}

	ThisStructPropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	ThisStructPropertyHandle->NotifyFinishedChangingProperties();

	UJointScriptSettings::Save();

	//refresh the details panel to reflect the changes
	RefreshVisual();

	FJointEdUtils::FireNotification(
		LOCTEXT("CleanAllMappingsSuccessTitle", "All Mappings Cleaned"),
		LOCTEXT("CleanAllMappingsSuccessDesc", "All node mappings have been removed. Please reimport to repopulate the mappings if needed"),
		EJointMDAdmonitionType::Mention
	);

	return FReply::Handled();
}

FReply FJointScriptLinkerMappingCustomization::OnCleanInvalidButtonClicked()
{
	// iterate through the NodeMappings and check if the node guids exist in the Joint Manager's graph. If not, remove those entries from the map.
	if (!ThisStructPropertyHandle) return FReply::Handled();

	// ask if the user is sure about this action with a confirmation dialog, as this cannot be undone.
	EAppReturnType::Type Result = FMessageDialog::Open(
		EAppMsgType::YesNo,
		LOCTEXT("CleanInvalidMappingsConfirmationDesc",
		        "Are you sure you want to clean invalid node mappings for this mapping? This will remove any node mappings that do not exist in the Joint Manager's graph. You can undo (ctrl + z) if needed.")
	);

	if (Result != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	FScopedTransaction Transaction(LOCTEXT("CleanInvalidMappingsTransaction", "Clean Invalid Node Mappings"));

	TSharedPtr<IPropertyHandle> NodeMappingsHandle = ThisStructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, NodeMappings));

	TSharedPtr<IPropertyHandle> JointManagerHandle = ThisStructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, JointManager));

	UJointManager* JointManager = JointScriptLinkerCustomizationHelpers::LoadJointManager(JointManagerHandle);

	if (!JointManager)
	{
		FJointEdUtils::FireNotification(
			LOCTEXT("CleanInvalidMappingsFailedTitle", "Clean Invalid Mappings Failed"),
			LOCTEXT("CleanInvalidMappingsFailedDesc", "Failed to load Joint Manager reference. Please ensure the reference is valid and try again."),
			EJointMDAdmonitionType::Error
		);
		return FReply::Handled();
	}

	TSet<FGuid> JointManagerNodeGuids;
	JointScriptLinkerCustomizationHelpers::GetAllNodeGuidsFromJointManager(JointManager, JointManagerNodeGuids);

	ThisStructPropertyHandle->NotifyPreChange();

	TArray<void*> RawData;
	ThisStructPropertyHandle->AccessRawData(RawData);

	for (void* RawPtr : RawData)
	{
		if (!RawPtr) continue;

		FJointScriptLinkerMapping* Mapping =
			static_cast<FJointScriptLinkerMapping*>(RawPtr);

		for (auto MapIt = Mapping->NodeMappings.CreateIterator(); MapIt; ++MapIt)
		{
			FJointScriptLinkerNodeSet& NodeSet = MapIt.Value();

			for (int32 i = NodeSet.NodeGuids.Num() - 1; i >= 0; --i)
			{
				if (!JointManagerNodeGuids.Contains(NodeSet.NodeGuids[i]))
				{
					NodeSet.NodeGuids.RemoveAt(i);
				}
			}
		}
	}

	ThisStructPropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	ThisStructPropertyHandle->NotifyFinishedChangingProperties();

	FJointEdUtils::FireNotification(
		LOCTEXT("CleanInvalidMappingsSuccessTitle", "Invalid Mappings Cleaned"),
		LOCTEXT("CleanInvalidMappingsSuccessDesc", "All invalid node mappings have been removed. Please verify the remaining mappings in the Joint Manager asset."),
		EJointMDAdmonitionType::Mention
	);
	
	UJointScriptSettings::Save();

	//refresh the details panel to reflect the changes
	RefreshVisual();

	return FReply::Handled();
}

void FJointScriptLinkerMappingCustomization::RefreshVisual()
{
	BuildNodeStatusItems();

	if (BodyBorder.IsValid()) BodyBorder->SetContent(CreateContentWidget());

	BuildFilterString();
}


void FJointScriptLinkerMappingCustomization::BuildNodeStatusItems()
{
	TSharedPtr<IPropertyHandle> JointManagerHandle = ThisStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, JointManager));
	TSharedPtr<IPropertyHandle> NodeMappingsHandle = ThisStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, NodeMappings));

	//Clear existing items first before rebuilding.
	NodeStatusItems.Reset();

	//JointManagerHandle refers TSoftObjectPtr<UJointManager>, so we need to get the reference first and resolve it to get the actual UJointManager object.
	UJointManager* JM = JointScriptLinkerCustomizationHelpers::LoadJointManager(JointManagerHandle);

	if (JM && NodeMappingsHandle.IsValid())
	{
		TSet<FGuid> ExistingNodeIDs;

		JointScriptLinkerCustomizationHelpers::GetAllNodeGuidsFromJointManager(JM, ExistingNodeIDs);

		TSharedPtr<IPropertyHandleMap> MapHandle;
		FMapProperty* MapProperty = nullptr;
		void* MapContainer = nullptr;
		FScriptMapHelper MapHelper = GetMapHelpers(NodeMappingsHandle, MapProperty, MapContainer);


		for (int32 Index = 0; Index < MapHelper.GetMaxIndex(); ++Index)
		{
			if (!MapHelper.IsValidIndex(Index)) continue;
			FString Key;
			MapProperty->KeyProp->CopyCompleteValue(&Key, MapHelper.GetKeyPtr(Index));

			FJointScriptLinkerNodeSet Value;
			MapProperty->ValueProp->CopyCompleteValue(&Value, MapHelper.GetValuePtr(Index));

			for (const FGuid& Guid : Value.NodeGuids)
			{
				TSharedPtr<FNodeStatusItem> Item = MakeShared<FNodeStatusItem>();
				Item->Key = Key;
				Item->Guid = Guid;
				Item->bExistsInGraph = ExistingNodeIDs.Contains(Guid);
				NodeStatusItems.Add(Item);
			}
		}
	}
}

void FJointScriptLinkerMappingCustomization::BuildFilterString()
{
	if (HeaderRowPtr)
	{
		FString JointManagerString;
		ThisStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, JointManager))->GetValueAsDisplayString(JointManagerString);
		FString NodeMappingsString;
		ThisStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJointScriptLinkerMapping, NodeMappings))->GetValueAsDisplayString(NodeMappingsString);

		// Set the filter string
		const FString FilterString =
			JointManagerString
			+ TEXT("|") // separator for better readability in filter results
			+ NodeMappingsString
			+ TEXT("|")
			+ LOCTEXT("NodeMappingsFilterStringSuffix", "Node Mappings").ToString();

		HeaderRowPtr->FilterString(
			FText::FromString(FilterString)
		);
	}
}

FScriptMapHelper FJointScriptLinkerMappingCustomization::GetMapHelpers(
	TSharedPtr<IPropertyHandle> InNodeMappingsHandle,
	FMapProperty*& OutMapProp,
	void*& OutMapContainer)
{
	if (InNodeMappingsHandle.IsValid())
	{
		OutMapProp = CastFieldChecked<FMapProperty>(InNodeMappingsHandle->GetProperty());

		InNodeMappingsHandle->GetValueData(OutMapContainer);

		if (OutMapContainer)
		{
			return FScriptMapHelper(OutMapProp, OutMapContainer);
		}
	}

	return FScriptMapHelper(nullptr, nullptr);
}

#undef LOCTEXT_NAMESPACE
