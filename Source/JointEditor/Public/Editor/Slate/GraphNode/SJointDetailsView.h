#pragma once

#include "CoreMinimal.h"
#include "JointEdGraphNode.h"
#include "Node/JointNodeBase.h"
#include "Widgets/SCompoundWidget.h"


struct FPropertyAndParent;
class IDetailsView;
class SJointRetainerWidget;

class JOINTEDITOR_API SJointDetailsView : public SCompoundWidget
{

public:

	SLATE_BEGIN_ARGS(SJointDetailsView) :
		_OwnerGraphNode(nullptr),
		_Object(nullptr),
		_EditorNodeObject(nullptr)
	{} 
	SLATE_ARGUMENT(TSharedPtr<SJointGraphNodeBase>, OwnerGraphNode)
	SLATE_ARGUMENT(UJointNodeBase*, Object)
	SLATE_ARGUMENT(UJointEdGraphNode*, EditorNodeObject)
	SLATE_ARGUMENT(TArray<FJointGraphNodePropertyData>, PropertyData)

	SLATE_END_ARGS()
	
public:

	void Construct(const FArguments& InArgs);

	SJointDetailsView();

	~SJointDetailsView();

public:

	void PopulateSlate();

public:
	
	bool GetIsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	
	bool GetIsRowVisible(FName InRowName, FName InParentName) const;

public:
	
	void CachePropertiesToShow();

public:
	
	TWeakObjectPtr<UJointNodeBase> Object;
	TWeakObjectPtr<UJointEdGraphNode> EdNode;
	TArray<FJointGraphNodePropertyData> PropertyData;

public:

	void UpdatePropertyData(const TArray<FJointGraphNodePropertyData>& PropertyData);
	
	void SetOwnerGraphNode(const TWeakPtr<SJointGraphNodeBase>& InOwnerGraphNode);

public:
	
	EActiveTimerReturnType InitializationTimer(double InCurrentTime, float InDeltaTime);

private:
	
	TArray<FName> PropertiesToShow;

public:
	
	TWeakPtr<SJointGraphNodeBase> OwnerGraphNode;
	
private:

	TWeakPtr<SJointRetainerWidget> JointRetainerWidget;
	
	TSharedPtr<IDetailsView> DetailViewWidget;

	float InitializationDelay = 5;

private:
	
	TWeakPtr<FActiveTimerHandle> TimerHandle;

public:

	//LOD
	bool UseLowDetailedRendering() const;

private:

	bool bInitializing = false;
	bool bInitialized = false;
	bool bAbandonBuild = false;


	bool bUseLOD = false;

private:

	// for the async initialization
	FCriticalSection InitializationCriticalSection;
	
};
