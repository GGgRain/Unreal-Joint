//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Notifications/INotificationWidget.h"

class JOINTEDITOR_API SJointNotificationWidget : public SCompoundWidget, public INotificationWidget
{
public:

	SLATE_BEGIN_ARGS(SJointNotificationWidget) {}
		SLATE_DEFAULT_SLOT( FArguments, Content )
	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

public:

	virtual void OnSetCompletionState(SNotificationItem::ECompletionState State) override;
	
	virtual TSharedRef< SWidget > AsWidget() override;
	
	virtual bool UseNotificationBackground() const override;

public:
	
	/** Pointer to the notification item that owns this widget (this is a deliberate reference cycle as we need this object alive until we choose to expire it, at which point we release our reference to allow everything to be destroyed) */
	TSharedPtr<SNotificationItem> OwningNotification;

	
};
