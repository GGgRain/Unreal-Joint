#include "Editor/Slate/GraphNode/SJointDetailsView.h"

#include "JointEdGraphNodesCustomization.h"
#include "JointEditorSettings.h"
#include "JointEditorStyle.h"
#include "SGraphPanel.h"
#include "Async/Async.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GraphNode/SJointGraphNodeBase.h"
#include "GraphNode/SJointRetainerWidget.h"
#include "Slate/WidgetRenderer.h"


SJointDetailsView::SJointDetailsView()
	: Object(nullptr),
	  EdNode(nullptr)
{
	// use custom prepass to ensure the widget is ready for rendering
	bHasCustomPrepass = false;
}

SJointDetailsView::~SJointDetailsView()
{
	JointRetainerWidget.Reset();

	OwnerGraphNode.Reset();
	DetailViewWidget.Reset();
	
	Object = nullptr;
	EdNode = nullptr;

	PropertyData.Empty();
	PropertiesToShow.Empty();

	bAbandonBuild = true;
	
}

void SJointDetailsView::Construct(const FArguments& InArgs)
{
	this->Object = InArgs._Object;
	this->PropertyData = InArgs._PropertyData;
	this->EdNode = InArgs._EditorNodeObject;
	this->OwnerGraphNode = InArgs._OwnerGraphNode;

	if (!Object.Get()) return;
	if (this->PropertyData.IsEmpty()) return;
	if (!EdNode.Get() || !EdNode->GetCastedGraph()) return;

	InitializationDelay = static_cast<float>(EdNode->GetCastedGraph()->GetCachedJointGraphNodes().Num()) / (static_cast<float>(UJointEditorSettings::Get()->SimplePropertyDisplayInitializationRate) + 1);
	bUseLOD = UJointEditorSettings::Get()->bUseLODRenderingForSimplePropertyDisplay;
	
	SetVisibility(EVisibility::SelfHitTestInvisible);
	CachePropertiesToShow();
	PopulateSlate();
	
	const float DelaySeconds = FMath::FRand() * InitializationDelay; // Set your delay

	if (TimerHandle.IsValid())
	{
		UnRegisterActiveTimer(TimerHandle.Pin().ToSharedRef());

		TimerHandle.Reset();
	}

	TimerHandle = RegisterActiveTimer(DelaySeconds,FWidgetActiveTimerDelegate::CreateSP(this, &SJointDetailsView::InitializationTimer));
	
}

void SJointDetailsView::PopulateSlate()
{
	this->ChildSlot.DetachWidget();

	
	this->ChildSlot.AttachWidget(
		SAssignNew(JointRetainerWidget, SJointRetainerWidget )
		.DisplayRetainerRendering(this, &SJointDetailsView::UseLowDetailedRendering)
		);
	
	
	bInitialized = false;
	bInitializing = false;
	bAbandonBuild = false;
}

EActiveTimerReturnType SJointDetailsView::InitializationTimer(double InCurrentTime, float InDeltaTime)
{
	// check if this widget is still valid
	if (!this) return EActiveTimerReturnType::Stop;

	// we have to make a new shared ptr for this slate to prevent it getting deleted while we are building it - which can happen if the user closes the tab while we are building it.
	// otherwise it will crash.
	
	TSharedPtr<SJointDetailsView> ThisPtr = SharedThis(this);

	if (ThisPtr->bInitialized) return EActiveTimerReturnType::Stop;
	if (ThisPtr->bInitializing) return EActiveTimerReturnType::Stop;
	
	ThisPtr->bInitializing = true;
	
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(
				"PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ENameAreaSettings::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bAllowSearch = false;
	//DetailsViewArgs.ViewIdentifier = FName(FGuid::NewGuid().ToString());

	ThisPtr->DetailViewWidget = nullptr;
			
	ThisPtr->DetailViewWidget = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [ThisPtr]()
	{
		//there are some weird issues with the detail view not being valid right away sometimes - so we wait here on a background thread.
		//This is not ideal, but it works TODO: find a better way.

		do
		{
			FPlatformProcess::Sleep(0.02f);
		}
		while (!ThisPtr->DetailViewWidget.IsValid() || ThisPtr->bAbandonBuild);
		
		if (ThisPtr->bAbandonBuild) return;
		
		AsyncTask(ENamedThreads::GameThread, [ThisPtr]()
		{
			// check if this widget is still valid
			if (!ThisPtr) return;
			
			if (ThisPtr->DetailViewWidget.IsValid())
			{
				const TSharedRef<IDetailsView> View = ThisPtr->DetailViewWidget.ToSharedRef();
				
				View->SetIsCustomRowVisibleDelegate(
					FIsCustomRowVisible::CreateSP(ThisPtr.ToSharedRef(), &SJointDetailsView::GetIsRowVisible));
				View->SetIsPropertyVisibleDelegate(
					FIsPropertyVisible::CreateSP(ThisPtr.ToSharedRef(), &SJointDetailsView::GetIsPropertyVisible));

				//For the better control over expanding.
				View->SetGenericLayoutDetailsDelegate(
					FOnGetDetailCustomizationInstance::CreateStatic(
						&FJointNodeInstanceSimpleDisplayCustomizationBase::MakeInstance));

				View->SetObject(ThisPtr->Object.Get(), false);
				
				ThisPtr->JointRetainerWidget.Pin()->GetChildSlot().AttachWidget(View);

				ThisPtr->bInitializing = false;
				ThisPtr->bInitialized = true;
			}
		});
	});


	if (TimerHandle.IsValid())
	{
		UnRegisterActiveTimer(TimerHandle.Pin().ToSharedRef());

		TimerHandle.Reset();
	}


	return EActiveTimerReturnType::Stop;
}


bool SJointDetailsView::GetIsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	if (PropertyData.IsEmpty()) return false;

	//Grab the parent-most one or itself.
	const FProperty* Property = PropertyAndParent.ParentProperties.Num() > 0
		                            ? PropertyAndParent.ParentProperties.Last()
		                            : &PropertyAndParent.Property;

	// Check against the parent property name
	return PropertiesToShow.Contains(Property->GetFName());
}

bool SJointDetailsView::GetIsRowVisible(FName InRowName, FName InParentName) const
{
	if (PropertyData.IsEmpty()) return false;

	//Watch out for the later usage. I didn't have time for this...
	if (InParentName == "Description") return false;

	return true;
}

void SJointDetailsView::CachePropertiesToShow()
{
	if (PropertyData.IsEmpty()) return;

	PropertiesToShow.Empty();
	PropertiesToShow.Reserve(PropertyData.Num());
	for (const FJointGraphNodePropertyData& Data : PropertyData)
	{
		PropertiesToShow.Emplace(Data.PropertyName);
	}
}


void SJointDetailsView::UpdatePropertyData(const TArray<FJointGraphNodePropertyData>& InPropertyData)
{
	PropertyData = InPropertyData;
}

void SJointDetailsView::SetOwnerGraphNode(const TWeakPtr<SJointGraphNodeBase>& InOwnerGraphNode)
{
	OwnerGraphNode = InOwnerGraphNode;
}

bool SJointDetailsView::UseLowDetailedRendering() const
{
	if (!bUseLOD) return false;
	
	if (OwnerGraphNode.IsValid())
	{
		if (const TSharedPtr<SGraphPanel>& MyOwnerPanel = OwnerGraphNode.Pin()->GetOwnerPanel())
		{
			if (MyOwnerPanel.IsValid())
			{
				return MyOwnerPanel->GetCurrentLOD() <= EGraphRenderingLOD::LowDetail;
			}
		}
	}

	return false;
}
