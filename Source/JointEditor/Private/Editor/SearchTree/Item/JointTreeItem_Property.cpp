//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Item/JointTreeItem_Property.h"

#include "BlueprintEditor.h"
#include "ISinglePropertyView.h"
#include "JointEditorSettings.h"
#include "JointEditorStyle.h"
#include "JointEdUtils.h"
#include "Filter/JointTreeFilter.h"
#include "ItemTag/JointTreeItemTag_Type.h"
#include "UObject/TextProperty.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

#define LOCTEXT_NAMESPACE "FJointTreeItem_Property"

FJointTreeItem_Property::FJointTreeItem_Property(FProperty* InProperty, TWeakObjectPtr<UObject> InObject,
                                                       const TSharedRef<SJointTree>& InTree)
	: FJointTreeItem(InTree),
	  Property(InProperty),
	  PropertyOuter(InObject)
{
	CacheAdditionalRowSearchString();
	AllocateItemTags();
}

void FJointTreeItem_Property::GenerateWidgetForNameColumn(TSharedPtr<SHorizontalBox> Box,
                                                             const TAttribute<FText>& InFilterText,
                                                             FIsSelected InIsSelected)
{
	if (PropertyOuter == nullptr || Property == nullptr) return;

	Box->AddSlot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.OnMouseDoubleClick(this, &FJointTreeItem_Property::OnMouseDoubleClick)
			.BorderBackgroundColor(FLinearColor::Transparent)
			[
				SNew(STextBlock)
				.HighlightText(InFilterText)
				.Text(FText::FromString(Property->GetName()))
			]
		];

	Box->AddSlot()
		.AutoWidth()
		.Padding(FMargin(6, 0))
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			MakeItemTagContainerWidget()
		];
}

TSharedRef<SWidget> FJointTreeItem_Property::GenerateWidgetForDataColumn(const TAttribute<FText>& InFilterText,
                                                                            const FName& DataColumnName,
                                                                            FIsSelected InIsSelected)
{

	if (PropertyOuter.Get() == nullptr || Property == nullptr) return SNullWidget::NullWidget;

	
	if (DataColumnName == SJointTree::Columns::Value)
	{
		if (CastField<FStructProperty>(Property) || CastField<FArrayProperty>(Property) || CastField<
				FMapProperty>(Property)
			|| CastField<FSetProperty>(Property))
		{
			FString ExportedStringValue;

#if UE_VERSION_OLDER_THAN(5, 1, 0)
			Property->ExportTextItem(ExportedStringValue, Property->ContainerPtrToValuePtr<uint8>(PropertyOuter.Get()), NULL,
								 NULL, PPF_PropertyWindow, NULL);
#else
			Property->ExportTextItem_Direct(ExportedStringValue, Property->ContainerPtrToValuePtr<uint8>(PropertyOuter.Get()),
													NULL,
													NULL, PPF_PropertyWindow, NULL);		
#endif
			

			AdditionalRowSearchString = FJointTreeFilter::ReplaceInqueryableCharacters(ExportedStringValue);

			TSharedPtr<SBorder> WrapperBorder = SNew(SBorder)
				.Padding(FJointEditorStyle::Margin_Normal)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Empty"))
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center);

			WrapperBorder->SetContent(
				SNew(SBorder)
				.BorderImage(FJointEditorStyle::Get().GetBrush("JointUI.Border.Round"))
				.BorderBackgroundColor(FJointEditorStyle::Color_Node_Inactive)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(ExportedStringValue))
					.HighlightText(InFilterText)
				]);

			return WrapperBorder.ToSharedRef();
		}

		if (CastField<FTextProperty>(Property) || CastField<FNameProperty>(Property) || CastField<
			FStrProperty>(Property))
		{
			const TAttribute<FText> ValueText_Attr = TAttribute<FText>::CreateLambda([this]
				{
					if (Property && PropertyOuter.Get())
					{
						FString ExportedStringValue;

#if UE_VERSION_OLDER_THAN(5, 1, 0)
						Property->ExportTextItem(ExportedStringValue,
					 Property->ContainerPtrToValuePtr<uint8>(PropertyOuter.Get()), NULL,
					 NULL, PPF_PropertyWindow, NULL);
#else
						Property->ExportTextItem_Direct(ExportedStringValue,
																			Property->ContainerPtrToValuePtr<uint8>(PropertyOuter.Get()), NULL,
																			NULL, PPF_PropertyWindow, NULL);				
#endif

						
						AdditionalRowSearchString = FJointTreeFilter::ReplaceInqueryableCharacters(ExportedStringValue);

						return FText::FromString(ExportedStringValue);
					}

					return FText::GetEmpty();
				});

			return SNew(SInlineEditableTextBlock)
					//.Style(FJointEditorStyle::GetUEEditorSlateStyleSet(), "DataprepAction.TitleInlineEditableText")
					.MultiLine(true)
					.HighlightText(InFilterText)
					.Text(ValueText_Attr)
					.WrapTextAt(UJointEditorSettings::Get()->ContextTextAutoTextWrapAt)
					.OnTextCommitted(this, &FJointTreeItem_Property::OnTextCommitted);
		}


		CacheAdditionalRowSearchString();

		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(
			"PropertyEditor");

		FSinglePropertyParams Args;
		Args.NamePlacement = EPropertyNamePlacement::Hidden;

		TSharedPtr<ISinglePropertyView> SinglePropertyViewWidget = PropertyEditorModule.CreateSingleProperty(
			PropertyOuter.Get(),
			Property->GetFName(),
			Args);

		return SinglePropertyViewWidget.ToSharedRef();
	}

	return SNullWidget::NullWidget;
}

FName FJointTreeItem_Property::GetRowItemName() const
{
	return Property != nullptr
		       ? FName(Property->GetName().Replace(TEXT(" "), TEXT("_")))
		       : FName("Invalid_Property");
}

UObject* FJointTreeItem_Property::GetObject() const
{
	return PropertyOuter.Get();
}

void FJointTreeItem_Property::AllocateItemTags()
{

	if(Property == nullptr || PropertyOuter == nullptr) return;
	
	FSlateColor BaseColor;
	FSlateColor SecondaryColor;
	FSlateBrush const* SecondaryIcon;
	FSlateBrush const* Icon = FBlueprintEditor::GetVarIconAndColorFromProperty(
		Property, BaseColor, SecondaryIcon, SecondaryColor);

	ItemTags.Add(
		MakeShareable(new FJointTreeItemTag_Type(
				BaseColor,
				Icon,
				Property ? FText::FromString(Property->GetCPPType()) : FText::GetEmpty()
				, GetJointPropertyTree()->Filter)
		)
	);
}

TSet<TSharedPtr<IJointTreeItemTag>> FJointTreeItem_Property::GetItemTags()
{
	return ItemTags;
}

#include "Misc/EngineVersionComparison.h"

void FJointTreeItem_Property::AddReferencedObjects(FReferenceCollector& Collector)
{
	
}

FString FJointTreeItem_Property::GetReferencerName() const
{
	return TEXT("FJointTreeItem_Property");
}

void FJointTreeItem_Property::OnTextCommitted(const FText& Text, ETextCommit::Type Arg)
{
	if(Property == nullptr || PropertyOuter == nullptr) return;
	
	if (CastField<FTextProperty>(Property))
	{
		FText CurText = CastField<FTextProperty>(Property)->GetPropertyValue(
			Property->ContainerPtrToValuePtr<void>(PropertyOuter.Get()));

		FString OutKey, OutNamespace;

		const FString Namespace = FTextInspector::GetNamespace(CurText).Get(FString());
		const FString Key = FTextInspector::GetKey(CurText).Get(FString());

		FJointEdUtils::JointText_StaticStableTextIdWithObj(
			PropertyOuter.Get(),
			IEditableTextProperty::ETextPropertyEditAction::EditedSource,
			Text.ToString(),
			Namespace,
			Key,
			OutNamespace,
			OutKey);

		CurText = FText::ChangeKey(FTextKey(OutNamespace), FTextKey(OutKey), Text);

		CastField<FTextProperty>(Property)->SetPropertyValue(Property->ContainerPtrToValuePtr<void>(PropertyOuter.Get()),
		                                                     CurText);

		AdditionalRowSearchString = Text.ToString();
	}
	else if (CastField<FNameProperty>(Property) || CastField<FStrProperty>(Property))
	{
#if UE_VERSION_OLDER_THAN(5, 1, 0)
		Property->ImportText(*Text.ToString(), Property->ContainerPtrToValuePtr<uint8>(PropertyOuter.Get()), PPF_None,
							 PropertyOuter.Get());

#else
		
		Property->ImportText_Direct(*Text.ToString(), Property->ContainerPtrToValuePtr<uint8>(PropertyOuter.Get()), PropertyOuter.Get(), PPF_None);


#endif

		AdditionalRowSearchString = Text.ToString();
	}
}

FReply FJointTreeItem_Property::OnMouseDoubleClick(const FGeometry& Geometry, const FPointerEvent& PointerEvent)
{
	OnItemDoubleClicked();

	return FReply::Handled();
}

void FJointTreeItem_Property::OnItemDoubleClicked()
{
	if(GetJointPropertyTree() && PropertyOuter.Get()) GetJointPropertyTree()->JumpToHyperlink(PropertyOuter.Get());
}


void FJointTreeItem_Property::CacheAdditionalRowSearchString()
{
	if (Property && PropertyOuter.Get())
	{
		FString ExportedStringValue;


#if UE_VERSION_OLDER_THAN(5, 1, 0)
		Property->ExportTextItem(ExportedStringValue, Property->ContainerPtrToValuePtr<uint8>(PropertyOuter.Get()), NULL,
								 NULL, PPF_PropertyWindow, NULL);
#else
		
		Property->ExportTextItem_Direct(ExportedStringValue,
										Property->ContainerPtrToValuePtr<uint8>(PropertyOuter.Get()), NULL,
										NULL, PPF_PropertyWindow, NULL);
#endif

		AdditionalRowSearchString = FJointTreeFilter::ReplaceInqueryableCharacters(ExportedStringValue);
	}
}

const FString FJointTreeItem_Property::GetFilterString()
{
	FString FilterString;

	for (TSharedPtr<IJointTreeItemTag> JointTreeItemTag : GetItemTags())
	{
		FilterString += " ";
		FilterString += JointTreeItemTag->GetFilterText().ToString();
	}

	return "Name=" + GetRowItemName().ToString() + ",Value=" + AdditionalRowSearchString.Replace(TEXT(" "), TEXT("_")) + FilterString;
}

#undef LOCTEXT_NAMESPACE
