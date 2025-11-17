//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "Slate/SJointTreeRow.h"

#include "Preferences/PersonaOptions.h"
#include "SearchTree/Item/IJointTreeItem.h"
#include "Slate/SJointTree.h"

//////////////////////////////////////////////////////////////////////////
// SJointList

#define LOCTEXT_NAMESPACE "JointPropertyTreeRow"


void SJointTreeRow::Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
{
	Item = InArgs._Item;
	OnDraggingItem = InArgs._OnDraggingItem;
	FilterText = InArgs._FilterText;

	check( Item.IsValid() );

	SMultiColumnTableRow< TSharedPtr<IJointTreeItem> >::Construct( FSuperRowType::FArguments(), InOwnerTableView );
}

void SJointTreeRow::ConstructChildren(ETableViewMode::Type InOwnerTableMode, const TAttribute<FMargin>& InPadding, const TSharedRef<SWidget>& InContent)
{
	STableRow<TSharedPtr<IJointTreeItem>>::Content = InContent;
	
	// MultiColumnRows let the user decide which column should contain the expander/indenter item.
	this->ChildSlot
		.Padding(InPadding)
		[
			InContent
		];
}

TSharedRef< SWidget > SJointTreeRow::GenerateWidgetForColumn( const FName& ColumnName )
{
	if ( ColumnName == SJointTree::Columns::Name )
	{
		TSharedPtr< SHorizontalBox > RowBox;

		SAssignNew( RowBox, SHorizontalBox );
		RowBox->AddSlot()
			.AutoWidth()
			[
				SNew( SExpanderArrow, SharedThis(this) )
				.ShouldDrawWires(true)
				.IndentAmount(20)
			];

		Item.Pin()->GenerateWidgetForNameColumn( RowBox, FilterText, FIsSelected::CreateSP(this, &STableRow::IsSelectedExclusively ) );

		return RowBox.ToSharedRef();
	}
	else
	{
		return Item.Pin()->GenerateWidgetForDataColumn(FilterText, ColumnName, FIsSelected::CreateSP(this, &STableRow::IsSelectedExclusively ));
	}
}

void SJointTreeRow::OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	Item.Pin()->HandleDragEnter(DragDropEvent);
}

void SJointTreeRow::OnDragLeave( const FDragDropEvent& DragDropEvent )
{
	Item.Pin()->HandleDragLeave(DragDropEvent);
}

FReply SJointTreeRow::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	return Item.Pin()->HandleDrop(DragDropEvent);
}

FReply SJointTreeRow::OnDragDetected( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if ( OnDraggingItem.IsBound() )
	{
		return OnDraggingItem.Execute( MyGeometry, MouseEvent );
	}
	else
	{
		return FReply::Unhandled();
	}
}

int32 SJointTreeRow::DoesItemHaveChildren() const
{
	if(Item.Pin()->HasInlineEditor())
	{
		return 1;
	}

	return SMultiColumnTableRow<TSharedPtr<IJointTreeItem>>::DoesItemHaveChildren();
}

bool SJointTreeRow::IsItemExpanded() const
{
	return SMultiColumnTableRow<TSharedPtr<IJointTreeItem>>::IsItemExpanded() || Item.Pin()->IsInlineEditorExpanded();
}

void SJointTreeRow::ToggleExpansion()
{
	SMultiColumnTableRow<TSharedPtr<IJointTreeItem>>::ToggleExpansion();
	
	if (Item.Pin()->HasInlineEditor())
	{
		Item.Pin()->ToggleInlineEditorExpansion();
		OwnerTablePtr.Pin()->Private_SetItemExpansion(Item.Pin(), Item.Pin()->IsInlineEditorExpanded());
	}
}


#undef LOCTEXT_NAMESPACE
