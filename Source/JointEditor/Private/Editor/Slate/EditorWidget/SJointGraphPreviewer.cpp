#include "EditorWidget/SJointGraphPreviewer.h"

#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorStyleSet.h"
#include "SGraphPanel.h"
#include "EditorWidget/SJointGraphPanel.h"

#define LOCTEXT_NAMESPACE "SJointGraphPreviewer"

EActiveTimerReturnType SJointGraphPreviewer::RefreshGraphTimer(const double InCurrentTime, const float InDeltaTime)
{
	if (NeedsRefreshCounter > 0)
	{
		if (GraphPanel.IsValid())
		{
			GraphPanel.Pin()->ZoomToFit(false);
			GraphPanel.Pin()->Update();
		}
		NeedsRefreshCounter--;
		
		return EActiveTimerReturnType::Continue;
	}
	else
	{
		return EActiveTimerReturnType::Stop;
	}
}

void SJointGraphPreviewer::Construct(const FArguments& InArgs, UEdGraph* InGraphObj)
{
	EdGraphObj = InGraphObj;
	NeedsRefreshCounter = 20;

	if (EdGraphObj)
	{
		this->ChildSlot
		[
			SAssignNew(GraphPanel, SJointGraphPanel,
				SGraphPanel::FArguments()
				.GraphObj( EdGraphObj )
				.InitialZoomToFit(true)
				.DisplayAsReadOnly(true)
				.ShowGraphStateOverlay(false)
				.Clipping(EWidgetClipping::Inherit)
			)
			.Clipping(EWidgetClipping::Inherit)
		];
		
		if (GraphPanel.IsValid()){
			GraphPanel.Pin()->Update();
		}

		RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SJointGraphPreviewer::RefreshGraphTimer));
	}else
	{

		this->ChildSlot
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoGraph", "No Graph to show or graph is invalid") )
			.Justification(ETextJustify::Center)
		];
	}
}

TWeakPtr<SJointGraphPanel> SJointGraphPreviewer::GetGraphPanel() const
{
	return GraphPanel;
}

#undef LOCTEXT_NAMESPACE
