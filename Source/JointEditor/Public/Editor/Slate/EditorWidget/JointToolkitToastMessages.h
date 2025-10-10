//Copyright 2022~2024 DevGrain. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoltAnimationTrack.h"
#include "Widgets/SCompoundWidget.h"

class SBox;
class SHorizontalBox;
class UVoltAnimationManager;
class SJointToolkitToastMessage;
class SJointToolkitToastMessageHub;

class JOINTEDITOR_API SJointToolkitToastMessageHub : public SCompoundWidget
{
	
public:

	SLATE_BEGIN_ARGS(SJointToolkitToastMessageHub) {}
	SLATE_END_ARGS()

public:
	
	void ReleaseVoltAnimationManager();
	
	void ImplementVoltAnimationManager();

public:

	void Construct(const FArguments& InArgs);
	
	void PopulateSlate();

public:
	
	/**
	 * Add a Toaster Message.
	 * @param Widget 
	 * @return GUID for the created toaster message.
	 */
	FGuid AddToasterMessage(const TSharedPtr<SJointToolkitToastMessage>& Widget);
	
	/**
	 * Remove Toaster Message. 
	 * @param ToasterGuid 
	 * @param bInstant 
	 */
	void RemoveToasterMessage(const FGuid& ToasterGuid, const bool bInstant = false);

	/**
	 * Find Toaster Message. 
	 * @param ToasterGuid 
	 */
	TWeakPtr<SJointToolkitToastMessage> FindToasterMessage(const FGuid& ToasterGuid);

	const bool HasToasterMessage(const FGuid& ToasterGuid);


public:
	
	UVoltAnimationManager* GetAnimationManager();

	void OnAnimationEnded(UVoltAnimationManager* VoltAnimationManager, const FVoltAnimationTrack& VoltAnimationTrack, const UVoltAnimation* VoltAnimation);

public:

	TWeakPtr<SHorizontalBox> ToasterDisplayBox;
	
public:
	
	TMap<FGuid, TWeakPtr<SJointToolkitToastMessage>> Messages;

private:

	//Only for the toast messages.
	
	UVoltAnimationManager* ToastMessageAnimationManager = nullptr;
	
};



class JOINTEDITOR_API SJointToolkitToastMessage : public SCompoundWidget
{
	
public:

	SLATE_BEGIN_ARGS(SJointToolkitToastMessage) :
		_Duration(-1),
		_SizeIncreaseInterpolationSpeed(8.5f),
		_SizeDecreaseInterpolationSpeed(5.5f),
		_AppearAnimationDuration(0.5),
		_RemoveAnimationDuration(0.25),
		_AppearAnimationExp(5),
		_RemoveAnimationExp(5)
		{}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_ARGUMENT(float, Duration)

		SLATE_ARGUMENT(float, SizeIncreaseInterpolationSpeed)
		SLATE_ARGUMENT(float, SizeDecreaseInterpolationSpeed)
		
		SLATE_ARGUMENT(float, AppearAnimationDuration)
		SLATE_ARGUMENT(float, RemoveAnimationDuration)
		
		SLATE_ARGUMENT(float, AppearAnimationExp)
		SLATE_ARGUMENT(float, RemoveAnimationExp)

	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

public:

	void SetMessageHub(const TSharedPtr<SJointToolkitToastMessageHub>& Hub);

public:

	void PlayAppearAnimation();
	
	void PlayRemoveAnimation();

	EActiveTimerReturnType OnExpired(double X, float Arg);

public:
    
	FGuid MessageGuid;

	TWeakPtr<SJointToolkitToastMessageHub> MessageHub;

public:

	TWeakPtr<SBox> SizeBox;

	TWeakPtr<SWidget> SlotWidget;

public:

	float SizeIncreaseInterpolationSpeed = 8.5f;
	float SizeDecreaseInterpolationSpeed = 5.5f;

	FVector2D SizeOverride;

public:

	//Animations

	float AppearAnimationDuration = 0.3;
	float RemoveAnimationDuration = 0.3;
	
	float AppearAnimationExp = 6;
	float RemoveAnimationExp = 6;

private:

	float Duration = -1;
	
public:

	FVoltAnimationTrack AppearTrack;

	FVoltAnimationTrack RemoveTrack;

public:

	bool bMarkedKilled = false;
	
};
