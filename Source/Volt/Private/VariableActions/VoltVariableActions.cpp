//Copyright 2022~2024 DevGrain. All Rights Reserved.


#include "VariableActions/VoltVariableActions.h"

#include "Variables/VoltVariables.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"

bool UVoltVarAction_Opacity::CheckSupportWidget(TWeakPtr<SWidget> Slate)
{
	return true;
}

void UVoltVarAction_Opacity::ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply)
{
	if (!SlateToApply.IsValid()) return;

	if (!Variable) return;

	UVoltVar_Opacity* CastedVar = Cast<UVoltVar_Opacity>(Variable);

	if (!CastedVar) return;

	SlateToApply.Pin()->SetRenderOpacity(CastedVar->Value);
}

bool UVoltVarAction_WidgetTransform::CheckSupportWidget(TWeakPtr<SWidget> Slate)
{
	return true;
}

void UVoltVarAction_WidgetTransform::ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply)
{
	if (!SlateToApply.IsValid()) return;

	if (!Variable) return;

	UVoltVar_WidgetTransform* CastedVar = Cast<UVoltVar_WidgetTransform>(Variable);

	if (!CastedVar) return;

	SlateToApply.Pin()->SetRenderTransform(CastedVar->Value.ToSlateRenderTransform());
}

bool UVoltVarAction_WidgetTransformPivot::CheckSupportWidget(TWeakPtr<SWidget> Slate)
{
	return true;
}

void UVoltVarAction_WidgetTransformPivot::ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply)
{
	if (!SlateToApply.IsValid()) return;

	if (!Variable) return;

	UVoltVar_WidgetTransformPivot* CastedVar = Cast<UVoltVar_WidgetTransformPivot>(Variable);

	if (!CastedVar) return;

	SlateToApply.Pin()->SetRenderTransformPivot(CastedVar->Value);
}

bool UVoltVarAction_ColorAndOpacity::CheckSupportWidget(TWeakPtr<SWidget> Slate)
{
	if (Slate.IsValid())
	{
		const FName SlateType = Slate.Pin()->GetType();
		if (SlateType == "SImage") return true;
		if (SlateType == "SBorder") return true;
		if (SlateType == "STextBlock") return true;

	}

	return false;
}

void UVoltVarAction_ColorAndOpacity::ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply)
{
	if (!SlateToApply.IsValid()) return;

	if (!Variable) return;

	UVoltVar_ColorAndOpacity* CastedVar = Cast<UVoltVar_ColorAndOpacity>(Variable);

	if (!CastedVar) return;
	
	if (SlateToApply.Pin()->GetType() == "SImage")
	{
		const TSharedPtr<SImage> CastedWidget = StaticCastSharedPtr<SImage>(SlateToApply.Pin());

		CastedWidget->SetColorAndOpacity(CastedVar->Value);

		return;
	}

	if (SlateToApply.Pin()->GetType() == "SBorder")
	{
		const TSharedPtr<SBorder> CastedWidget = StaticCastSharedPtr<SBorder>(SlateToApply.Pin());

		CastedWidget->SetColorAndOpacity(CastedVar->Value);

		return;
	}

	if (SlateToApply.Pin()->GetType() == "STextBlock")
	{
		const TSharedPtr<STextBlock> CastedWidget = StaticCastSharedPtr<STextBlock>(SlateToApply.Pin());

		CastedWidget->SetColorAndOpacity(CastedVar->Value);

		return;
	}
}

bool UVoltVarAction_BackgroundColor::CheckSupportWidget(TWeakPtr<SWidget> Slate)
{
	if (Slate.IsValid())
	{
		const FName SlateType = Slate.Pin()->GetType();
		if (SlateType == "SBorder") return true;
		if (SlateType == "SButton") return true;
	}

	return false;
}

void UVoltVarAction_BackgroundColor::ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply)
{
	if (!SlateToApply.IsValid()) return;

	if (!Variable) return;

	UVoltVar_BackgroundColor* CastedVar = Cast<UVoltVar_BackgroundColor>(Variable);

	if (!CastedVar) return;
	
	if (SlateToApply.Pin()->GetType() == "SBorder")
	{
		const TSharedPtr<SBorder> CastedWidget = StaticCastSharedPtr<SBorder>(SlateToApply.Pin());

		CastedWidget->SetBorderBackgroundColor(CastedVar->Value);
		
		return;
	}

	if (SlateToApply.Pin()->GetType() == "SButton")
	{
		
		const TSharedPtr<SButton> CastedWidget = StaticCastSharedPtr<SButton>(SlateToApply.Pin());

		CastedWidget->SetBorderBackgroundColor(CastedVar->Value);
		
		return;
	}
}

bool UVoltVarAction_ForegroundColor::CheckSupportWidget(TWeakPtr<SWidget> Slate)
{
	if (Slate.IsValid())
	{
		const FName SlateType = Slate.Pin()->GetType();
		if (SlateType == "SBorder") return true;
		if (SlateType == "SButton") return true;
	}

	return false;
}

void UVoltVarAction_ForegroundColor::ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply)
{
	if (!SlateToApply.IsValid()) return;

	if (!Variable) return;

	UVoltVar_BackgroundColor* CastedVar = Cast<UVoltVar_BackgroundColor>(Variable);

	if (!CastedVar) return;
	
	if (SlateToApply.Pin()->GetType() == "SBorder")
	{
		const TSharedPtr<SBorder> CastedWidget = StaticCastSharedPtr<SBorder>(SlateToApply.Pin());

		CastedWidget->SetForegroundColor(CastedVar->Value);
		
		return;
	}

	if (SlateToApply.Pin()->GetType() == "SButton")
	{
		
		const TSharedPtr<SButton> CastedWidget = StaticCastSharedPtr<SButton>(SlateToApply.Pin());

		CastedWidget->SetForegroundColor(CastedVar->Value);
		
		return;
	}

	
}

bool UVoltVarAction_ChildSlotPadding::CheckSupportWidget(TWeakPtr<SWidget> Slate)
{
	if (Slate.IsValid())
	{
		const FName SlateType = Slate.Pin()->GetType();
		if (SlateType == "SBorder") return true;
		if (SlateType == "SHorizontalBox") return true;
		if (SlateType == "SVerticalBox") return true;
	}

	return false;
}

void UVoltVarAction_ChildSlotPadding::ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply)
{
	if (!SlateToApply.IsValid()) return;

	if (!Variable) return;

	UVoltVar_ChildSlotPadding* CastedVar = Cast<UVoltVar_ChildSlotPadding>(Variable);

	if (!CastedVar) return;
	
	if (SlateToApply.Pin()->GetType() == "SBorder")
	{
		const TSharedPtr<SBorder> CastedWidget = StaticCastSharedPtr<SBorder>(SlateToApply.Pin());

		CastedWidget->SetPadding(CastedVar->Value);
		
		return;
	}

	if (SlateToApply.Pin()->GetType() == "SHorizontalBox")
	{
		const TSharedPtr<SHorizontalBox> CastedWidget = StaticCastSharedPtr<SHorizontalBox>(SlateToApply.Pin());

		const int SlotNum = CastedWidget->NumSlots();
		
		for(int i = 0; i < SlotNum; i++)
		{
			SHorizontalBox::FSlot* Slot = &CastedWidget->GetSlot(i);
			
			Slot->SetPadding(CastedVar->Value);
		}
		
		return;
	}

	if (SlateToApply.Pin()->GetType() == "SVerticalBox")
	{
		const TSharedPtr<SVerticalBox> CastedWidget = StaticCastSharedPtr<SVerticalBox>(SlateToApply.Pin());

		const int SlotNum = CastedWidget->NumSlots();
		
		for(int i = 0; i < SlotNum; i++)
		{
			SVerticalBox::FSlot* Slot = &CastedWidget->GetSlot(i);
			
			Slot->SetPadding(CastedVar->Value);
		}
		
		return;
	}
}

bool UVoltVarAction_ParentSlotPadding::CheckSupportWidget(TWeakPtr<SWidget> Slate)
{
	if (Slate.IsValid())
	{
		const FName SlateType = Slate.Pin()->GetType();
		if (SlateType == "SBorder") return true;
		if (SlateType == "SHorizontalBox") return true;
		if (SlateType == "SVerticalBox") return true;
		if (SlateType == "SScrollBox") return true;
		if (SlateType == "SWrapBox") return true;
	}

	return false;
}

void UVoltVarAction_ParentSlotPadding::ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply)
{
		
	if (!SlateToApply.IsValid()) return;

	if (!Variable) return;

	UVoltVar_ParentSlotPadding* CastedVar = Cast<UVoltVar_ParentSlotPadding>(Variable);
	
	if (!CastedVar) return;
	
	TSharedPtr<SWidget> ParentSlate = SlateToApply.Pin()->GetParentWidget();

	if (SlateToApply.Pin()->GetParentWidget() == nullptr) return;
	
	if (ParentSlate->GetType() == "SBorder")
	{
		const TSharedPtr<SBorder> CastedParentWidget = StaticCastSharedPtr<SBorder>(ParentSlate);
		
		CastedParentWidget->SetPadding(CastedVar->Value);

		return;
	}

	if (ParentSlate->GetType() == "SHorizontalBox")
	{
		const TSharedPtr<SHorizontalBox> CastedParentWidget = StaticCastSharedPtr<SHorizontalBox>(ParentSlate);

		const int NumSlot = CastedParentWidget->NumSlots();

		for (int Index = 0; Index < NumSlot; ++Index)
		{
			SHorizontalBox::FSlot* Slot = &CastedParentWidget->GetSlot(Index);

			if (Slot->GetWidget() != SlateToApply) continue;

			Slot->SetPadding(CastedVar->Value);

			return;
		}

		return;
	}

	if (ParentSlate->GetType() == "SVerticalBox")
	{
		const TSharedPtr<SVerticalBox> CastedParentWidget = StaticCastSharedPtr<SVerticalBox>(ParentSlate);

		const int NumSlot = CastedParentWidget->NumSlots();

		for (int Index = 0; Index < NumSlot; ++Index)
		{
			SVerticalBox::FSlot* Slot = &CastedParentWidget->GetSlot(Index);

			if (Slot->GetWidget() != SlateToApply) continue;

			Slot->SetPadding(CastedVar->Value);

			return;
		}

		return;
	}

	if (ParentSlate->GetType() == "SScrollBox")
	{
		const TSharedPtr<SScrollBox> CastedParentWidget = StaticCastSharedPtr<SScrollBox>(ParentSlate);

		TPanelChildren<SScrollBox::FSlot>* Slots = static_cast<TPanelChildren<SScrollBox::FSlot>*>(CastedParentWidget->
			GetChildren());

		const int NumSlot = Slots->Num();

		for (int Index = 0; Index < NumSlot; ++Index)
		{
			SScrollBox::FSlot* Slot = &(*Slots)[Index];

			if (Slot->GetWidget() != SlateToApply) continue;

			Slot->SetPadding(CastedVar->Value);

			return;
		}

		return;
	}


	if (ParentSlate->GetType() == "SWrapBox")
	{
		const TSharedPtr<SWrapBox> CastedParentWidget = StaticCastSharedPtr<SWrapBox>(ParentSlate);

		TPanelChildren<SWrapBox::FSlot>* Slots = static_cast<TPanelChildren<SWrapBox::FSlot>*>(CastedParentWidget->
			GetChildren());

		const int NumSlot = Slots->Num();

		for (int Index = 0; Index < NumSlot; ++Index)
		{
			SWrapBox::FSlot* Slot = &(*Slots)[Index];

			if (Slot->GetWidget() != SlateToApply) continue;

			Slot->SetPadding(CastedVar->Value);

			return;
		}

		return;
	}
}


bool UVoltVarAction_Box::CheckSupportWidget(TWeakPtr<SWidget> Slate)
{
	if (Slate.IsValid())
	{
		const FName SlateType = Slate.Pin()->GetType();
		if (SlateType == "SBox") return true;
	}

	return false;
}

void UVoltVarAction_Box::ApplyVariable(UVoltVariableBase* Variable, TWeakPtr<SWidget> SlateToApply)
{
	if (!SlateToApply.IsValid()) return;

	if (!Variable) return;

	UVoltVar_Box* CastedVar = Cast<UVoltVar_Box>(Variable);

	if (!CastedVar) return;
	
	if (SlateToApply.Pin()->GetType() == "SBox")
	{
		const TSharedPtr<SBox> CastedParentWidget = StaticCastSharedPtr<SBox>(SlateToApply.Pin());

		if(CastedVar->bOverride_HeightOverride) CastedParentWidget->SetHeightOverride(CastedVar->HeightOverride);
		if(CastedVar->bOverride_WidthOverride) CastedParentWidget->SetWidthOverride(CastedVar->WidthOverride);

		if(CastedVar->bOverride_MinDesiredHeight) CastedParentWidget->SetMinDesiredHeight(CastedVar->MinDesiredHeight);
		if(CastedVar->bOverride_MinDesiredWidth) CastedParentWidget->SetMinDesiredWidth(CastedVar->MinDesiredWidth);

		if(CastedVar->bOverride_MaxDesiredHeight) CastedParentWidget->SetMaxDesiredHeight(CastedVar->MaxDesiredHeight);
		if(CastedVar->bOverride_MaxDesiredWidth) CastedParentWidget->SetMaxDesiredWidth(CastedVar->MaxDesiredWidth);
		
		if(CastedVar->bOverride_MinAspectRatio) CastedParentWidget->SetMinAspectRatio(CastedVar->MinAspectRatio);
		if(CastedVar->bOverride_MaxAspectRatio) CastedParentWidget->SetMaxAspectRatio(CastedVar->MaxAspectRatio);

		return;
	}

}
