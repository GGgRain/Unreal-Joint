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
#include "JointEdGraphNode_Fragment.h"
#include "JointEditorNodePickingManager.h"
#include "JointEditorSettings.h"
#include "JointEdUtils.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"

#include "Styling/CoreStyle.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GraphNode/JointGraphNodeSharedSlates.h"
#include "Misc/MessageDialog.h"
#include "Node/JointFragment.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SScrollBox.h"

#include "Widgets/Notifications/SNotificationList.h"
#include "HAL/PlatformApplicationMisc.h"


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


bool FJointEdGraphNodesCustomizationBase::CheckIfEveryNodeAllowPinDataControl(TArray<UObject*> NodeInstanceObjs)
{
	bool bCanShowPinDataProperty = true;

	for (UObject* NodeInstanceObj : NodeInstanceObjs)
	{
		if (!NodeInstanceObj) continue;

		if (UJointNodeBase* NodeBase = Cast<UJointNodeBase>(NodeInstanceObj); NodeBase && !NodeBase->
			GetAllowNodeInstancePinControl())
		{
			bCanShowPinDataProperty = false;

			break;
		}
	}

	return bCanShowPinDataProperty;
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

		if ( CastedGraph == nullptr ) continue;
		
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

		if(bCanShowReplaceNodeContext && bCanShowReplaceEditorNodeContext) break;
	}

	IDetailCategoryBuilder& AdvancedCategory = DetailBuilder.EditCategory("Advanced");
	AdvancedCategory.InitiallyCollapsed(true);
	AdvancedCategory.SetShowAdvanced(true);

	if(bCanShowReplaceNodeContext)
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

	if(bCanShowReplaceEditorNodeContext)
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
					FText NotificationText = LOCTEXT("Notification_ReplaceNode_Deny_NotAllowed","Replace Node Class Action Has Been Denied");
					FText NotificationSubText = LOCTEXT("Notification_Sub_ReplaceNode_Deny_NotAllowed","This editor node prohibits this action.");

					FNotificationInfo NotificationInfo(NotificationText);
					NotificationInfo.SubText = NotificationSubText;
					NotificationInfo.Image = FJointEditorStyle::Get().GetBrush("JointUI.Image.JointManager");
					NotificationInfo.bFireAndForget = true;
					NotificationInfo.FadeInDuration = 0.3f;
					NotificationInfo.FadeOutDuration = 1.3f;
					NotificationInfo.ExpireDuration = 4.5f;

					FSlateNotificationManager::Get().AddNotification(NotificationInfo);

					continue;
				}
				
				if (!CastedGraphNode->CanPerformReplaceNodeClassTo(CastedClass))
				{
					FText NotificationText = LOCTEXT("Notification_ReplaceNode_Deny_NotAllowed_Specific","Replace Node Class Action Has Been Denied");
					FText NotificationSubText = LOCTEXT("Notification_Sub_ReplaceNode_Deny_NotAllowed_Specific","Provided class that is not compatible with the editor node type or it was invalid class.");

					FNotificationInfo NotificationInfo(NotificationText);
					NotificationInfo.SubText = NotificationSubText;
					NotificationInfo.Image = FJointEditorStyle::Get().GetBrush("JointUI.Image.JointManager");
					NotificationInfo.bFireAndForget = true;
					NotificationInfo.FadeInDuration = 0.3f;
					NotificationInfo.FadeOutDuration = 1.3f;
					NotificationInfo.ExpireDuration = 4.5f;

					FSlateNotificationManager::Get().AddNotification(NotificationInfo);

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

				if (UEdGraph* CastedGraph = CastedGraphNode->GetGraph(); CastedGraph &&!ModifiedObjects.Contains(CastedGraph))
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
					FText NotificationText = LOCTEXT("Notification_ReplaceEditorNode_Deny_NotAllowed","Replace Editor Node Class Action Has Been Denied");
					FText NotificationSubText = LOCTEXT("Notification_Sub_ReplaceEditorNode_Deny_NotAllowed","This editor node prohibits this action.");

					FNotificationInfo NotificationInfo(NotificationText);
					NotificationInfo.SubText = NotificationSubText;
					NotificationInfo.Image = FJointEditorStyle::Get().GetBrush("JointUI.Image.JointManager");
					NotificationInfo.bFireAndForget = true;
					NotificationInfo.FadeInDuration = 0.3f;
					NotificationInfo.FadeOutDuration = 1.3f;
					NotificationInfo.ExpireDuration = 4.5f;

					FSlateNotificationManager::Get().AddNotification(NotificationInfo);

					continue;
				}
				
				if (!CastedGraphNode->CanPerformReplaceEditorNodeClassTo(CastedClass))
				{
					FText NotificationText = LOCTEXT("Notification_ReplaceEditorNode_Deny_NotAllowed_Specific","Replace Editor Node Class Action Has Been Denied");
					FText NotificationSubText = LOCTEXT("Notification_Sub_ReplaceEditorNode_Deny_NotAllowed_Specific","Provided class doesn't support the node instance type or it was invalid class.");

					FNotificationInfo NotificationInfo(NotificationText);
					NotificationInfo.SubText = NotificationSubText;
					NotificationInfo.Image = FJointEditorStyle::Get().GetBrush("JointUI.Image.JointManager");
					NotificationInfo.bFireAndForget = true;
					NotificationInfo.FadeInDuration = 0.3f;
					NotificationInfo.FadeOutDuration = 1.3f;
					NotificationInfo.ExpireDuration = 4.5f;

					FSlateNotificationManager::Get().AddNotification(NotificationInfo);

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

				if (UEdGraph* CastedGraph = CastedGraphNode->GetGraph(); CastedGraph &&!ModifiedObjects.Contains(CastedGraph))
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
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UJointEdGraphNode, bFromExternal));
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
		SNew(SJointOutlineBorder)
		.OuterBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.InnerBorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
		.OutlineNormalColor(FLinearColor(0.04, 0.04, 0.04))
		.OutlineHoverColor(FJointEditorStyle::Color_Selected)
		.ContentPadding(FJointEditorStyle::Margin_Normal)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(STextBlock)
			.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h3")
			.AutoWrapText(true)
			.Text(LOCTEXT("FJointBuildPresetCustomization_Explanation",
			              "We supports client & server preset, but those are not that recommended to use because using only those two has limitation on making a game that can be standalone & multiplayer together.\nIf you set any of those to \'exclude\' then standalone section will not contain the nodes with that preset.\nFor that cases, using build target based including & excluding will help you better. just make 3 different build target for each sessions (standalone (game), client, server) and set the behavior for each of them."))
		]
	];
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
		SNew(SJointOutlineBorder)
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
		Toolkit->PopulateNodePickerCopyToastMessage();
	}

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
		Toolkit->PopulateNodePickerPastedToastMessage();
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

#undef LOCTEXT_NAMESPACE
