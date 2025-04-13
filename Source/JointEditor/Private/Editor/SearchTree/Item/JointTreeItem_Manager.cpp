//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Item/JointTreeItem_Manager.h"

#include "JointEditorStyle.h"
#include "JointManagerActions.h"
#include "IDocumentation.h"
#include "ItemTag/IJointTreeItemTag.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

//////////////////////////////////////////////////////////////////////////
// SJointList

#define LOCTEXT_NAMESPACE "JointPropertyList"


FJointTreeItem_Manager::FJointTreeItem_Manager(UJointManager* InManagerPtr,
                                                     const TSharedRef<SJointTree>& InTree)
	: FJointTreeItem(InTree)
	  , JointManagerPtr(InManagerPtr)
{
}

FReply FJointTreeItem_Manager::OnMouseDoubleClick(const FGeometry& Geometry, const FPointerEvent& PointerEvent)
{
	OnItemDoubleClicked();

	return FReply::Handled();
}


void FJointTreeItem_Manager::GenerateWidgetForNameColumn(TSharedPtr<SHorizontalBox> Box,
                                                            const TAttribute<FText>& InFilterText,
                                                            FIsSelected InIsSelected)
{
	TAttribute<FText> Name_Attr = TAttribute<FText>::CreateLambda([this]
		{
			return JointManagerPtr ? FText::FromName(JointManagerPtr->GetFName()) : LOCTEXT("InvalidRef","INVALID");
		});

	TAttribute<FText> Path_Attr = TAttribute<FText>::CreateLambda(
			[this] { return JointManagerPtr ? FText::FromString(JointManagerPtr->GetPathName()) : LOCTEXT("InvalidRef","INVALID");});


	Box->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.NodeShadow"))
			.BorderBackgroundColor(FJointEditorStyle::Color_Node_Shadow)
			.Padding(FJointEditorStyle::Margin_Border)
			.Visibility(EVisibility::SelfHitTestInvisible)
			[
				SNew(SBorder)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(FJointEditorStyle::Color_Normal)
				.Padding(FJointEditorStyle::Margin_Border)
				.OnMouseDoubleClick(this, &FJointTreeItem_Manager::OnMouseDoubleClick)
				[
					SNew(SHorizontalBox)
					.Visibility(EVisibility::SelfHitTestInvisible)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SImage)
						.Image(FJointEditorStyle::Get().GetBrush("ClassThumbnail.JointManager"))
						.DesiredSizeOverride(FVector2D(32, 32))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SInlineEditableTextBlock)
						.Text(Name_Attr)
						.HighlightText(InFilterText)
						.IsReadOnly(true)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FJointEditorStyle::Margin_Border)
					[
						SNew(STextBlock)
						.TextStyle(FJointEditorStyle::Get(), "JointUI.TextBlock.Regular.h5")
						.Text(Path_Attr)
					]
				]
			]
		];
}

TSharedRef<SWidget> FJointTreeItem_Manager::GenerateWidgetForDataColumn(
	const TAttribute<FText>& InFilterText, const FName& DataColumnName, FIsSelected InIsSelected)
{
	return SNullWidget::NullWidget;
}

FName FJointTreeItem_Manager::GetRowItemName() const
{
	return JointManagerPtr ? FName(JointManagerPtr->GetPathName()) : NAME_None;
}

#include "Misc/EngineVersionComparison.h"

void FJointTreeItem_Manager::AddReferencedObjects(FReferenceCollector& Collector)
{
	
}

void FJointTreeItem_Manager::OnItemDoubleClicked()
{
	if (const UAssetEditorSubsystem* SubSystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>(); JointManagerPtr && SubSystem)
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(JointManagerPtr, true);
}

UObject* FJointTreeItem_Manager::GetObject() const
{
	return JointManagerPtr;
}

FString FJointTreeItem_Manager::GetReferencerName() const
{
	return TEXT("FJointTreeItem_Manager");
}


#undef LOCTEXT_NAMESPACE
