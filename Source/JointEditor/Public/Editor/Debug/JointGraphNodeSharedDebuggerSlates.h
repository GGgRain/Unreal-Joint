//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SharedType/JointSharedTypes.h"
#include "JointNodeDebugData.h"
#include "Widgets/SCompoundWidget.h"

class UJointEdGraphNode;
class SGraphPin;
class SGraphNode;
class SVerticalBox;
class SImage;
class SJointGraphNodeBase;

class JOINTEDITOR_API SJointBreakpointIndicator : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SJointBreakpointIndicator)
	{
	}
		SLATE_ARGUMENT(TWeakPtr<SJointGraphNodeBase>, GraphNodeSlate)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void PopulateSlate();

public:
	
	void UpdateVisual();

	const FSlateColor GetBreakpointColor();
	const FSlateBrush* GetBreakpointImageBrush();
	const FText GetBreakpointTooltipText();

	
public:

	void UpdateDebugData();

private:
	
	FJointNodeDebugData* DebugData = nullptr;

public:
	
	TWeakPtr<SJointGraphNodeBase> ParentGraphNodeSlate;

	TSharedPtr<SImage> BreakpointImage;

public:

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	
};
