//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "ItemTag/JointTreeItemTag.h"

#define LOCTEXT_NAMESPACE "FJointTreeItemTag"

FJointTreeItemTag::FJointTreeItemTag()
{
}

TSharedRef<SWidget> FJointTreeItemTag::MakeTagWidget()
{
	return SNullWidget::NullWidget;
}

FText FJointTreeItemTag::GetFilterText()
{
	return FText::GetEmpty();
}

TSharedPtr<IJointTreeFilterItem> FJointTreeItemTag::GetFilterItem()
{
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
