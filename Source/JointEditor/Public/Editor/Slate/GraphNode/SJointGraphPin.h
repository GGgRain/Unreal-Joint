//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SGraphPin.h"
#include "VoltAnimationTrack.h"

struct FJointEdPinData;
class FRichTextLayoutMarshaller;
class UDataTable;
class UJointEdGraphNode;
class UJointNodeBase;

class JOINTEDITOR_API SJointGraphPinBase : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SJointGraphPinBase) {}
	SLATE_END_ARGS()

public:
	
	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);

public:
	
	void PopulateSlate();
	
public:

	void OnHovered();

	void OnUnHovered();

public:
	
	bool bShouldUseParentNodeColorAsOwnColor = true;

public:

	TSharedPtr<STextBlock> TextBlock;
	
public:
	
	//~ Begin SGraphPin Interface
	
	virtual const FSlateBrush* GetPinIcon() const override;
	
	virtual FSlateColor GetPinColor() const override;

	virtual TSharedRef<SWidget> GetLabelWidget(const FName& InLabelStyle) override;
	
	//~ End SGraphPin Interface

public:
	
	void CachePinData();
	
	FORCEINLINE FJointEdPinData* GetCachedPinData() const;
	
public:


	FJointEdPinData* GetPinDataForCurrentPin() const;
	
	UJointEdGraphNode* GetPinRealParentEdNode() const;

	UJointEdGraphNode* GetPinParentEdNode() const;
	
	UJointNodeBase* GetPinParentNode() const;

public:

	bool ShouldAlwaysDisplayNameText();

public:

	FVoltAnimationTrack TextBlockAnimationTrack;

private:

	FJointEdPinData* CachedPinData = nullptr;
	
};
