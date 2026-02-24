#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SJointGraphPanel;
class SGraphPanel;
class UEdGraph;

// This widget provides a fully-zoomed-out preview of a specified graph
class JOINTEDITOR_API SJointGraphPreviewer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SJointGraphPreviewer) {}
		
	SLATE_END_ARGS()

public:
	
	void Construct(const FArguments& InArgs, UEdGraph* InGraphObj);

public:

	/** 
	 * Get the graph panel of this previewer. This is the underlying graph panel that is used to display the graph.
	 * @return The graph panel of this previewer. nullptr if the graph panel is not valid.
	 */
	TWeakPtr<SJointGraphPanel> GetGraphPanel() const;

	/**
	 * Set the graph to be previewed. This will refresh the previewer and zoom out to fit the whole graph.
	 * @param InGraphObj Graph to be previewed
	 */
	void SetGraph(UEdGraph* InGraphObj);

private:
	
	void BuildGraphWidget();
	
	/** Helper function used to refresh the graph */
	EActiveTimerReturnType RefreshGraphTimer(const double InCurrentTime, const float InDeltaTime);

protected:
	
	/// The Graph we are currently viewing
	UEdGraph* EdGraphObj = nullptr;

	// As node's bounds dont get updated immediately, to truly zoom out to fit we need to tick a few times
	int32 NeedsRefreshCounter = 0;

	// The underlying graph panel
	TWeakPtr<SJointGraphPanel> GraphPanel;
};
