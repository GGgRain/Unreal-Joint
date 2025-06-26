//Copyright 2022~2024 DevGrain. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Logging/TokenizedMessage.h"
#include "Widgets/SCompoundWidget.h"

class SJointGraphNodeBase;

class JOINTEDITOR_API SJointGraphNodeCompileResult : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SJointGraphNodeCompileResult) {}
		SLATE_ARGUMENT(TWeakPtr<class SJointGraphNodeBase>, GraphNodeBase)
	SLATE_END_ARGS()
	
public:
	
	void Construct(const FArguments& InArgs);

	void RebuildWidget();

	void UpdateWidgetWithCompileResult(const TArray<TSharedPtr<FTokenizedMessage>>& CompileResult);

public:

	TSharedPtr<SWidget> GetErrorToolTip();
	TSharedPtr<SWidget> GetWarningToolTip();
	TSharedPtr<SWidget> GetInfoToolTip();


public:

	TSharedPtr<class SJointGraphNodeBase> GraphNodeBase;

	TSharedPtr<class STextBlock> ErrorCountTextBlock;
	TSharedPtr<class STextBlock> WarningCountTextBlock;
	TSharedPtr<class STextBlock> InfoCountTextBlock;
	
	TSharedPtr<class SButton> ErrorCountButton;
	TSharedPtr<class SButton> WarningCountButton;
	TSharedPtr<class SButton> InfoCountButton;
	
	TSharedPtr<class SToolTip> ErrorCountToolTip;
	TSharedPtr<class SToolTip> WarningCountToolTip;
	TSharedPtr<class SToolTip> InfoCountToolTip;

public:

	TArray<TWeakPtr<FTokenizedMessage>> ErrorMessages;
	TArray<TWeakPtr<FTokenizedMessage>> WarningMessages;
	TArray<TWeakPtr<FTokenizedMessage>> InfoMessages;

};
