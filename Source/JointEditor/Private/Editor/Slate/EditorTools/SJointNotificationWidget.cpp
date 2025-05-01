//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Editor/Slate/EditorTools/SJointNotificationWidget.h"

void SJointNotificationWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		InArgs._Content.Widget
	];
}

void SJointNotificationWidget::OnSetCompletionState(SNotificationItem::ECompletionState State)
{

	// If we completed and we aren't keeping the notification open (which will show the Close button), then expire the notification immediately
	if (State == SNotificationItem::CS_Success || State == SNotificationItem::CS_Fail)
	{
		if (OwningNotification)
		{
			// Perform the normal automatic fadeout
			OwningNotification->ExpireAndFadeout();

			// Release our reference to our owner so that everything can be destroyed
			OwningNotification.Reset();
		}
	}
}

TSharedRef<SWidget> SJointNotificationWidget::AsWidget()
{
	return AsShared();
}

bool SJointNotificationWidget::UseNotificationBackground() const
{
	return false;
}
