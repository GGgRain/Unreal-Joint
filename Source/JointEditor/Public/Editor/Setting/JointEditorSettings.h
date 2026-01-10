//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JointManagementTabs.h"
#include "Engine/DeveloperSettings.h"
#include "SharedType/JointEditorSharedTypes.h"
#include "JointGraphConnectionDrawingPolicy.h"
#include "JointEditorSettings.generated.h"

class UJointManager;
class UJointNodeBase;

namespace JointEditorDefaultSettings
{
	static const bool bUseLODRenderingForSimplePropertyDisplay(true);
	static const int LODRenderingForSimplePropertyDisplayRetainerPeriod(32);
	static const int SimplePropertyDisplayInitializationRate(150);

	//Composite Node
	static const FVector2D CompositeNodeTooltipSize(FVector2D(900.0f, 500.0f));

	//Search & Replace
	static const bool bUseAsynchronousBuildingOnSearchAndReplaceTree(true);
	
	//Graph Editor
	static const bool bUseGrid(false);
	static const FLinearColor BackgroundColor(FLinearColor(0.093963, 0.092709, 0.104167, 1));
	static const FLinearColor RegularGridColor(FColor(5, 5, 9, 255));
	static const FLinearColor RuleGridColor(FColor(5, 5, 9, 255));
	static const FLinearColor CenterGridColor(FColor(5, 5, 9, 255));
	static const float SmallestGridSize(100);
	static const float GridSnapSize(5);

	//Graph Editor - Pin / Connection
	static const FLinearColor NormalConnectionColor(FLinearColor(0.15, 0.15, 0.20, 1));
	static const FLinearColor RecursiveConnectionColor(FLinearColor(0.4, 0.25, 0.09, 1));
	static const FLinearColor HighlightedConnectionColor(FLinearColor(0.65, 0.1, 0.25, 1));
	static const FLinearColor SelfConnectionColor(HighlightedConnectionColor);
	static const FLinearColor PreviewConnectionColor(HighlightedConnectionColor);


	static const float NotHighlightedConnectionOpacity(0.2);
	static const float NotReachableRouteConnectionOpacity(0.2);


	static const float PinConnectionThickness(5.0);
	static const float HighlightedPinConnectionThickness(7.f);
	static const float ConnectionHighlightFadeBias(0.35);
	static const float ConnectionHighlightedFadeInPeriod(0.75);
	static const bool bDrawNormalConnection(true);
	static const bool bDrawRecursiveConnection(true);

	//Graph Editor - Debugger
	static const FLinearColor DebuggerPausedNodeColor(FLinearColor(1, 1, 1));
	static const FLinearColor DebuggerPlayingNodeColor(FLinearColor(0.2, 0.8, 0.2));
	static const FLinearColor DebuggerPendingNodeColor(FLinearColor(0.8, 0.5, 0.2));
	static const FLinearColor DebuggerEndedNodeColor(FLinearColor(0.2, 0.2, 0.2));

	//Graph Editor - Node
	static const FLinearColor DefaultNodeColor(FLinearColor(0.026715, 0.025900, 0.035, 1));
	static const FLinearColor NodeDepthAdditiveColor(DefaultNodeColor * 0.25);

	//Context Text Editor
	static const float ContextTextEditorFontSizeMultiplier(0.33);
	static const float ContextTextAutoTextWrapAt(500);
	static const FLinearColor ContextTextEditorBackgroundColor(FLinearColor(0.007, 0.007, 0.012, 1));


	static const float ForwardSplineHorizontalDeltaRange(1000.0f);
	static const float ForwardSplineVerticalDeltaRange(1000.0f);
	static const FVector2D ForwardSplineTangentFromHorizontalDelta(FVector2D(2.0f, 0.0f));
	static const FVector2D ForwardSplineTangentFromVerticalDelta(FVector2D(0.f, 0.0f));

	static const float BackwardSplineHorizontalDeltaRange(200.0f);
	static const float BackwardSplineVerticalDeltaRange(200.0f);
	static const FVector2D BackwardSplineTangentFromHorizontalDelta(FVector2D(2.0f, 0.0f));
	static const FVector2D BackwardSplineTangentFromVerticalDelta(FVector2D(0.f, 0.0f));

	static const float SelfSplineHorizontalDeltaRange(200.0f);
	static const float SelfSplineVerticalDeltaRange(200.0f);
	static const FVector2D SelfSplineTangentFromHorizontalDelta(FVector2D(4.0f, 4.0f));
	static const FVector2D SelfSplineTangentFromVerticalDelta(FVector2D(0.f, 4.0f));


	static const bool bUseWiggleWireForNormalConnection(false);
	static const bool bUseWiggleWireForRecursiveConnection(false);
	static const bool bUseWiggleWireForSelfConnection(false);
	static const bool bUseWiggleWireForPreviewConnection(true);

	static const FWiggleWireConfig WiggleWireConfig(
		100,
		0.1,
		1.5,
		1.5,
		1000,
		1.4,
		0.75,
		50,
		1500,
		3
	);

	//Detail Panel
	static const float DescriptionCardBoxMaxDesiredHeight(400.0f);
};


UCLASS(config = JointEditorSettings, defaultconfig)
class JOINTEDITOR_API UJointEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UJointEditorSettings();

public:
	/** 
	 * Enables or disables the LOD rendering of the Simple Property Display section.
	 * This option is still in BETA, if your editor crashes for no reason, try to turn it off.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Experimental",meta = (DisplayName = "Use LOD Rendering For Simple Property Display (BETA)"))
	bool bUseLODRenderingForSimplePropertyDisplay = JointEditorDefaultSettings::bUseLODRenderingForSimplePropertyDisplay;

	/** 
	 * LOD retainer Period for the SimplePropertyDisplay.
	 * Higher value means higher frequency of the update.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Experimental", meta = (DisplayName = "LOD Rendering For Simple Property Display Retainer Period (BETA)"))
	int LODRenderingForSimplePropertyDisplayRetainerPeriod = JointEditorDefaultSettings::LODRenderingForSimplePropertyDisplayRetainerPeriod;

	/** 
	 * The count of the nodes that will initialize the Simple Property Display section per second.
	 * Graph's node count / this value = the time in seconds it takes to initialize the Simple Property Display section.
	 * bigger value means faster initialization, but more performance cost - can lead to stuttering or freezing of the editor.
	 * we recommend to use the default value of 150.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Experimental", meta = (DisplayName = "Simple Property Display Initialization Delay Standard (BETA)"))
	int SimplePropertyDisplayInitializationRate = JointEditorDefaultSettings::SimplePropertyDisplayInitializationRate;


public:

	/**
	 * Use asynchronous building on the search & replace tree.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Search & Replace",meta = (DisplayName = "Use Asynchronous Building On Search & Replace Tree (BETA)"))
	bool bUseAsynchronousBuildingOnSearchAndReplaceTree = JointEditorDefaultSettings::bUseAsynchronousBuildingOnSearchAndReplaceTree;

public:
	
	UPROPERTY(EditAnywhere, config, Category = "Sub Graph", meta = (DisplayName = "Composite Node Tooltip Size"))
	FVector2D CompositeNodeTooltipSize = JointEditorDefaultSettings::CompositeNodeTooltipSize;

public:
	/** Enables or disables the display of a background grid in the material and blueprint editors. */
	UPROPERTY(EditAnywhere, config, Category = "Joint Graph Editor", meta = (DisplayName = "Use Grid in Graph"))
	bool bUseGrid = JointEditorDefaultSettings::bUseGrid;

	/** Defines the background color of the graph editor. */
	UPROPERTY(EditAnywhere, config, Category = "Joint Graph Editor", meta = (DisplayName = "Background Color"))
	FLinearColor BackgroundColor = JointEditorDefaultSettings::BackgroundColor;

	/** Specifies the color for the regular grid lines in the graph editor. */
	UPROPERTY(EditAnywhere, config, Category = "Joint Graph Editor", meta = (DisplayName = "Regular Grid Color"))
	FLinearColor RegularGridColor = JointEditorDefaultSettings::RegularGridColor;

	/** Determines the color of ruler lines in the graph grid, used for measurement and alignment. */
	UPROPERTY(EditAnywhere, config, Category = "Joint Graph Editor", meta = (DisplayName = "Ruler Grid Color"))
	FLinearColor RuleGridColor = JointEditorDefaultSettings::RuleGridColor;

	/** Specifies the color for center lines in the graph grid, aiding in spatial orientation. */
	UPROPERTY(EditAnywhere, config, Category = "Joint Graph Editor", meta = (DisplayName = "Center Grid Color"))
	FLinearColor CenterGridColor = JointEditorDefaultSettings::CenterGridColor;

	/** The smallest allowable grid size in pixels, defining the minimum unit of spacing. */
	UPROPERTY(EditAnywhere, config, Category = "Joint Graph Editor", meta = (DisplayName = "Smallest Grid Size"))
	float SmallestGridSize = JointEditorDefaultSettings::SmallestGridSize;

	/** The increment size for snapping objects to the grid, ensuring consistent placement. */
	UPROPERTY(EditAnywhere, config, Category = "Joint Graph Editor", meta = (DisplayName = "Grid Snap Size"))
	float GridSnapSize = JointEditorDefaultSettings::GridSnapSize;

public:
	/** Defines a scaling factor for font size in the context text editor. */
	UPROPERTY(Config, EditAnywhere, Category = "Context Text Editor",meta = (DisplayName = "Context Text Editor Font Size Multiplier"))
	float ContextTextEditorFontSizeMultiplier = JointEditorDefaultSettings::ContextTextEditorFontSizeMultiplier;

	/** Specifies the number of characters per line before text automatically wraps within a node context box. */
	UPROPERTY(Config, EditAnywhere, Category = "Context Text Editor",meta = (DisplayName = "Context Text Editor Auto-Wrap Texts Amount"))
	float ContextTextAutoTextWrapAt = JointEditorDefaultSettings::ContextTextAutoTextWrapAt;

	/** Determines the background color used in the context text editor for improved readability. */
	UPROPERTY(Config, EditAnywhere, Category = "Context Text Editor",meta = (DisplayName = "Context Text Editor Background Color"))
	FLinearColor ContextTextEditorBackgroundColor = JointEditorDefaultSettings::ContextTextEditorBackgroundColor;

public:
	/** The color used for rendering normal (default) pin connections between nodes. */
	UPROPERTY(Config, EditAnywhere, Category = "Pin / Pin Connection", meta = (DisplayName = "Normal Connection Color"))
	FLinearColor NormalConnectionColor = JointEditorDefaultSettings::NormalConnectionColor;

	/** Specifies the color of recursive connections, often used for loops or self-referencing structures. */
	UPROPERTY(Config, EditAnywhere, Category = "Pin / Pin Connection", meta = (DisplayName = "Recursive Connection Color"))
	FLinearColor RecursiveConnectionColor = JointEditorDefaultSettings::RecursiveConnectionColor;

	/** The color applied to highlighted connections, making them more visually distinct. */
	UPROPERTY(Config, EditAnywhere, Category = "Pin / Pin Connection", meta = (DisplayName = "Highlighted Connection Color"))
	FLinearColor HighlightedConnectionColor = JointEditorDefaultSettings::HighlightedConnectionColor;

	/** The color used to represent self-referential connections within a single node. */
	UPROPERTY(Config, EditAnywhere, Category = "Pin / Pin Connection", meta = (DisplayName = "Self Connection Color"))
	FLinearColor SelfConnectionColor = JointEditorDefaultSettings::SelfConnectionColor;

	/** The color applied to connections in preview mode, before finalizing a link. */
	UPROPERTY(Config, EditAnywhere, Category = "Pin / Pin Connection", meta = (DisplayName = "Preview Connection Color"))
	FLinearColor PreviewConnectionColor = JointEditorDefaultSettings::PreviewConnectionColor;

	/** The default line thickness for pin connections. */
	UPROPERTY(Config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Pin Connection Thickness"))
	float PinConnectionThickness = JointEditorDefaultSettings::PinConnectionThickness;

	/** The thickness applied to highlighted pin connections for better visibility. */
	UPROPERTY(Config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Highlighted Pin Connection Thickness"))
	float HighlightedPinConnectionThickness = JointEditorDefaultSettings::HighlightedPinConnectionThickness;

	/** Adjusts the fade bias when highlighting a connection, influencing its transition effect. */
	UPROPERTY(Config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Connection Highlight Fade Bias"))
	float ConnectionHighlightFadeBias = JointEditorDefaultSettings::ConnectionHighlightFadeBias;

	/** Specifies the duration for fade-in effects when a connection is highlighted. */
	UPROPERTY(Config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Connection Highlight Fade Duration"))
	float ConnectionHighlightedFadeInPeriod = JointEditorDefaultSettings::ConnectionHighlightedFadeInPeriod;

	/** Determines whether normal (non-recursive) connections should be drawn. */
	UPROPERTY(Config, EditAnywhere, Category = "Pin / Pin Connection", meta = (DisplayName = "Draw Normal Connections"))
	bool bDrawNormalConnection = JointEditorDefaultSettings::bDrawNormalConnection;

	/** Enables or disables the visualization of recursive connections. */
	UPROPERTY(Config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Draw Recursive Connections"))
	bool bDrawRecursiveConnection = JointEditorDefaultSettings::bDrawRecursiveConnection;


	/** Opacity for connections that are not highlighted */
	UPROPERTY(Config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Not Highlighted Connection Opacity"))
	float NotHighlightedConnectionOpacity = JointEditorDefaultSettings::NotHighlightedConnectionOpacity;

	/** Opacity for connections representing routes that are not currently on active route (not reachable) */
	UPROPERTY(Config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Not Reachable Route Connection Opacity"))
	float NotReachableRouteConnectionOpacity = JointEditorDefaultSettings::NotReachableRouteConnectionOpacity;


	/** Defines the maximum horizontal distance that can be used to calculate the spline tangent when a connection moves forward. */
	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Forward Spline Horizontal Delta Range"))
	float ForwardSplineHorizontalDeltaRange = JointEditorDefaultSettings::ForwardSplineHorizontalDeltaRange;

	/** Defines the maximum vertical distance that can be used to calculate the spline tangent when a connection moves forward. */
	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Forward Spline Vertical Delta Range"))
	float ForwardSplineVerticalDeltaRange = JointEditorDefaultSettings::ForwardSplineVerticalDeltaRange;

	/** Determines how much the horizontal distance affects the tangent calculation for forward-moving splines. */
	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Forward Spline Tangent From Horizontal Delta"))
	FVector2D ForwardSplineTangentFromHorizontalDelta =
		JointEditorDefaultSettings::ForwardSplineTangentFromHorizontalDelta;

	/** Determines how much the vertical distance affects the tangent calculation for forward-moving splines. */
	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Forward Spline Tangent From Vertical Delta"))
	FVector2D ForwardSplineTangentFromVerticalDelta = JointEditorDefaultSettings::ForwardSplineTangentFromVerticalDelta;


	/** Defines the maximum horizontal distance that can be used to calculate the spline tangent when a connection moves backward. */
	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Backward Spline Horizontal Delta Range"))
	float BackwardSplineHorizontalDeltaRange = JointEditorDefaultSettings::BackwardSplineHorizontalDeltaRange;

	/** Defines the maximum vertical distance that can be used to calculate the spline tangent when a connection moves backward. */
	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Backward Spline Vertical Delta Range"))
	float BackwardSplineVerticalDeltaRange = JointEditorDefaultSettings::BackwardSplineVerticalDeltaRange;

	/** Determines how much the horizontal distance affects the tangent calculation for backward-moving splines. */
	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Backward Spline Tangent From Horizontal Delta"))
	FVector2D BackwardSplineTangentFromHorizontalDelta = JointEditorDefaultSettings::BackwardSplineTangentFromHorizontalDelta;

	/** Determines how much the vertical distance affects the tangent calculation for backward-moving splines. */
	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Backward Spline Tangent From Vertical Delta"))
	FVector2D BackwardSplineTangentFromVerticalDelta =JointEditorDefaultSettings::BackwardSplineTangentFromVerticalDelta;


	/** Defines the maximum horizontal distance that can be used to calculate the spline tangent for self-connections. */
	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Self Spline Horizontal Delta Range"))
	float SelfSplineHorizontalDeltaRange = JointEditorDefaultSettings::SelfSplineHorizontalDeltaRange;

	/** Defines the maximum vertical distance that can be used to calculate the spline tangent for self-connections. */
	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Self Spline Vertical Delta Range"))
	float SelfSplineVerticalDeltaRange = JointEditorDefaultSettings::SelfSplineVerticalDeltaRange;

	/** Determines how much the horizontal distance affects the tangent calculation for self-referential splines. */
	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Self Spline Tangent From Horizontal Delta"))
	FVector2D SelfSplineTangentFromHorizontalDelta = JointEditorDefaultSettings::SelfSplineTangentFromHorizontalDelta;

	/** Determines how much the vertical distance affects the tangent calculation for self-referential splines. */
	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection",meta = (DisplayName = "Self Spline Tangent From Vertical Delta"))
	FVector2D SelfSplineTangentFromVerticalDelta = JointEditorDefaultSettings::SelfSplineTangentFromVerticalDelta;

public:
	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection - Physics-Based Wiggle Wire Rendering (Beta)",meta = (DisplayName = "Use Wiggle Wire For Normal Connection"))
	bool bUseWiggleWireForNormalConnection = JointEditorDefaultSettings::bUseWiggleWireForNormalConnection;

	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection - Physics-Based Wiggle Wire Rendering (Beta)",meta = (DisplayName = "Use Wiggle Wire For Recursive Connection"))
	bool bUseWiggleWireForRecursiveConnection = JointEditorDefaultSettings::bUseWiggleWireForRecursiveConnection;

	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection - Physics-Based Wiggle Wire Rendering (Beta)",meta = (DisplayName = "Use Wiggle Wire For Self Connection"))
	bool bUseWiggleWireForSelfConnection = JointEditorDefaultSettings::bUseWiggleWireForSelfConnection;

	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection - Physics-Based Wiggle Wire Rendering (Beta)",meta = (DisplayName = "Use Wiggle Wire For Preview Connection"))
	bool bUseWiggleWireForPreviewConnection = JointEditorDefaultSettings::bUseWiggleWireForPreviewConnection;

	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection - Physics-Based Wiggle Wire Rendering (Beta)",meta = (DisplayName = "Normal Connection Wiggle Wire Renderer Config"))
	FWiggleWireConfig NormalConnectionWiggleWireConfig = JointEditorDefaultSettings::WiggleWireConfig;

	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection - Physics-Based Wiggle Wire Rendering (Beta)",meta = (DisplayName = "Recursive Connection Wiggle Wire Renderer Config"))
	FWiggleWireConfig RecursiveConnectionWiggleWireConfig = JointEditorDefaultSettings::WiggleWireConfig;

	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection - Physics-Based Wiggle Wire Rendering (Beta)",meta = (DisplayName = "Self Connection Wiggle Wire Renderer Config"))
	FWiggleWireConfig SelfConnectionWiggleWireConfig = JointEditorDefaultSettings::WiggleWireConfig;

	UPROPERTY(config, EditAnywhere, Category = "Pin / Pin Connection - Physics-Based Wiggle Wire Rendering (Beta)",meta = (DisplayName = "Preview Connection Wiggle Wire Renderer Config"))
	FWiggleWireConfig PreviewConnectionWiggleWireConfig = JointEditorDefaultSettings::WiggleWireConfig;

public:
	
	/** Specifies the color applied to nodes that are currently active and executing in the debugger. */
	UPROPERTY(Config, EditAnywhere, Category = "Debugger Color", meta = (DisplayName = "Debugger Playing Node Color"))
	FLinearColor DebuggerPausedNodeColor = JointEditorDefaultSettings::DebuggerPausedNodeColor;
	
	/** Specifies the color applied to nodes that are currently active and executing in the debugger. */
	UPROPERTY(Config, EditAnywhere, Category = "Debugger Color", meta = (DisplayName = "Debugger Playing Node Color"))
	FLinearColor DebuggerPlayingNodeColor = JointEditorDefaultSettings::DebuggerPlayingNodeColor;

	/** Specifies the color assigned to nodes that are pending execution in the debugger. */
	UPROPERTY(Config, EditAnywhere, Category = "Debugger Color", meta = (DisplayName = "Debugger Pending Node Color"))
	FLinearColor DebuggerPendingNodeColor = JointEditorDefaultSettings::DebuggerPendingNodeColor;

	/** Specifies the color used for nodes that have finished execution in the debugger. */
	UPROPERTY(Config, EditAnywhere, Category = "Debugger Color", meta = (DisplayName = "Debugger Ended Node Color"))
	FLinearColor DebuggerEndedNodeColor = JointEditorDefaultSettings::DebuggerEndedNodeColor;

public:
	/** Default color assigned to nodes when no specific color is set. */
	UPROPERTY(Config, EditAnywhere, Category = "Graph Node", meta = (DisplayName = "Default Node Color"))
	FLinearColor DefaultNodeColor = JointEditorDefaultSettings::DefaultNodeColor;

	/** Additional color applied to nodes based on their depth in the hierarchy to enhance visualization. */
	UPROPERTY(Config, EditAnywhere, Category = "Graph Node", meta = (DisplayName = "Node Depth Additive Color"))
	FLinearColor NodeDepthAdditiveColor = JointEditorDefaultSettings::NodeDepthAdditiveColor;

public:
	/** Default color assigned to nodes when no specific color is set. */
	UPROPERTY(Config, EditAnywhere, Category = "Detail Panel",meta = (DisplayName = "Description Card Box Max Desired Height"))
	float DescriptionCardBoxMaxDesiredHeight = JointEditorDefaultSettings::DescriptionCardBoxMaxDesiredHeight;

public:
	/**
	 * A set of classes to hide on the editor, especially new node context menu and new fragment context menu.
	 * This is useful when you have your own implementation of any specific native node, and want to hide the old one.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Editing", meta = (DisplayName = "Node Classes To Hide"))
	TSet<TSubclassOf<UJointNodeBase>> NodeClassesToHide;

public:
	/**
	 * Enables or disables the developer mode.
	 * It will show some additional information about the nodes, and graph.
	 * This is useful for debugging and development purposes, especially good to see how internal data is working.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Debugging", meta = (DisplayName = "Enable Developer Mode"))
	bool bEnableDeveloperMode = false;

public:
	//Internal Properties

	UPROPERTY(config)
	TMap<TSoftObjectPtr<UJointManager>, bool> BulkSearchAndReplaceCheckedJoints;

	UPROPERTY(Config)
	bool bDebuggerEnabled = true;

public:
	/** 
	 * A set of class name redirects for the editor.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Joint Redirects", meta = (DisplayName = "Joint Class Name Redirects"))
	TArray<FJointCoreRedirect> JointCoreRedirects;

public:
	void AddCoreRedirect(const FJointCoreRedirect& Redirect);

	void RemoveCoreRedirect(const FJointCoreRedirect& Redirect);

public:
	static const float GetJointGridSnapSize();

public:
	//Instance Obtain Related

	static UJointEditorSettings* Get();

	static void Save();

public:
	virtual FName GetCategoryName() const override final { return TEXT("Joint"); }
	virtual FText GetSectionText() const override final { return FText::FromString("Joint Editor Preferences"); }
};
