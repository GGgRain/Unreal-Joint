//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Module/Volt_ASM_InterpBoxProperties.h"

#include "VoltInterface.h"
#include "VoltVariableCollection.h"
#include "Variables/VoltVariables.h"


void UVolt_ASM_InterpBoxProperties::Construct(const FArguments& InArgs)
{
	InterpolationMode = InArgs._InterpolationMode;

	StartHeightOverride = InArgs._StartHeightOverride;
	StartWidthOverride = InArgs._StartWidthOverride;
	StartMinDesiredWidth = InArgs._StartMinDesiredWidth;
	StartMinDesiredHeight = InArgs._StartMinDesiredHeight;
	StartMaxDesiredWidth = InArgs._StartMaxDesiredWidth;
	StartMaxDesiredHeight = InArgs._StartMaxDesiredHeight;
	StartMinAspectRatio = InArgs._StartMinAspectRatio;
	StartMaxAspectRatio = InArgs._StartMaxAspectRatio;
		
	TargetHeightOverride = InArgs._TargetHeightOverride;
	TargetWidthOverride = InArgs._TargetWidthOverride;
	TargetMinDesiredWidth = InArgs._TargetMinDesiredWidth;
	TargetMinDesiredHeight = InArgs._TargetMinDesiredHeight;
	TargetMaxDesiredWidth = InArgs._TargetMaxDesiredWidth;
	TargetMaxDesiredHeight = InArgs._TargetMaxDesiredHeight;
	TargetMinAspectRatio = InArgs._TargetMinAspectRatio;
	TargetMaxAspectRatio = InArgs._TargetMaxAspectRatio;
	
	bUseStartHeightOverride = InArgs._bUseStartHeightOverride;
	bUseStartWidthOverride = InArgs._bUseStartWidthOverride;
	bUseStartMinDesiredWidth = InArgs._bUseStartMinDesiredWidth;
	bUseStartMinDesiredHeight = InArgs._bUseStartMinDesiredHeight;
	bUseStartMaxDesiredWidth = InArgs._bUseStartMaxDesiredWidth;
	bUseStartMaxDesiredHeight = InArgs._bUseStartMaxDesiredHeight;
	bUseStartMinAspectRatio = InArgs._bUseStartMinAspectRatio;
	bUseStartMaxAspectRatio = InArgs._bUseStartMaxAspectRatio;
				
	bOverride_WidthOverride = InArgs._bOverride_WidthOverride;
	bOverride_HeightOverride = InArgs._bOverride_HeightOverride;
	bOverride_MinDesiredWidth = InArgs._bOverride_MinDesiredWidth;
	bOverride_MinDesiredHeight = InArgs._bOverride_MinDesiredHeight;
	bOverride_MaxDesiredWidth = InArgs._bOverride_MaxDesiredWidth;
	bOverride_MaxDesiredHeight = InArgs._bOverride_MaxDesiredHeight;
	bOverride_MinAspectRatio = InArgs._bOverride_MinAspectRatio;
	bOverride_MaxAspectRatio = InArgs._bOverride_MaxAspectRatio;
	
	
	RateBasedInterpSpeed = InArgs._RateBasedInterpSpeed;
	bRateBasedUseConstant = InArgs._bRateBasedUseConstant;
	bRateBasedNeverFinish = InArgs._bRateBasedNeverFinish;

	AlphaBasedEasingFunction = InArgs._AlphaBasedEasingFunction;
	AlphaBasedDuration = InArgs._AlphaBasedDuration;
	AlphaBasedBlendExp = InArgs._AlphaBasedBlendExp;
	AlphaBasedSteps = InArgs._AlphaBasedSteps;
}

void UVolt_ASM_InterpBoxProperties::ModifySlateVariable(const float DeltaTime,
                                                const TScriptInterface<IVoltInterface>& Volt)
{
	if(Volt == nullptr) return;
	if(Volt->GetVoltVariableCollection() == nullptr) return;

	UVoltVariableBase* Var = Volt->GetVoltVariableCollection()->FindOrAddVariable(UVoltVar_Box::StaticClass());

	UVoltVar_Box* CastedVar = Cast<UVoltVar_Box>(Var);

	//interp
	switch (InterpolationMode)
	{
	case EVoltInterpMode::RateBased:

		if(RateBasedInterpSpeed > 0)
		{
			if(bRateBasedUseConstant)
			{
				if(bOverride_WidthOverride) CastedVar->WidthOverride = FMath::FInterpConstantTo(CastedVar->WidthOverride , TargetWidthOverride, DeltaTime ,RateBasedInterpSpeed);
				if(bOverride_HeightOverride) CastedVar->HeightOverride = FMath::FInterpConstantTo(CastedVar->HeightOverride , TargetHeightOverride, DeltaTime ,RateBasedInterpSpeed);
				if(bOverride_MinDesiredWidth) CastedVar->MinDesiredWidth = FMath::FInterpConstantTo(CastedVar->MinDesiredWidth , TargetMinDesiredWidth, DeltaTime ,RateBasedInterpSpeed);
				if(bOverride_MinDesiredHeight) CastedVar->MinDesiredHeight = FMath::FInterpConstantTo(CastedVar->MinDesiredHeight , TargetMinDesiredHeight, DeltaTime ,RateBasedInterpSpeed);
				if(bOverride_MaxDesiredWidth) CastedVar->MaxDesiredWidth = FMath::FInterpConstantTo(CastedVar->MaxDesiredWidth , TargetMaxDesiredWidth, DeltaTime ,RateBasedInterpSpeed);
				if(bOverride_MaxDesiredHeight) CastedVar->MaxDesiredHeight = FMath::FInterpConstantTo(CastedVar->MaxDesiredHeight , TargetMaxDesiredHeight, DeltaTime ,RateBasedInterpSpeed);
				if(bOverride_MinAspectRatio) CastedVar->MinAspectRatio = FMath::FInterpConstantTo(CastedVar->MinAspectRatio , TargetMinAspectRatio, DeltaTime ,RateBasedInterpSpeed);
				if(bOverride_MaxAspectRatio) CastedVar->MaxAspectRatio = FMath::FInterpConstantTo(CastedVar->MaxAspectRatio , TargetMaxAspectRatio, DeltaTime ,RateBasedInterpSpeed);

			}else
			{
				if(bOverride_WidthOverride) CastedVar->WidthOverride = FMath::FInterpTo(CastedVar->WidthOverride , TargetWidthOverride, DeltaTime ,RateBasedInterpSpeed);
				if(bOverride_HeightOverride) CastedVar->HeightOverride = FMath::FInterpTo(CastedVar->HeightOverride , TargetHeightOverride, DeltaTime ,RateBasedInterpSpeed);
				if(bOverride_MinDesiredWidth) CastedVar->MinDesiredWidth = FMath::FInterpTo(CastedVar->MinDesiredWidth , TargetMinDesiredWidth, DeltaTime ,RateBasedInterpSpeed);
				if(bOverride_MinDesiredHeight) CastedVar->MinDesiredHeight = FMath::FInterpTo(CastedVar->MinDesiredHeight , TargetMinDesiredHeight, DeltaTime ,RateBasedInterpSpeed);
				if(bOverride_MaxDesiredWidth) CastedVar->MaxDesiredWidth = FMath::FInterpTo(CastedVar->MaxDesiredWidth , TargetMaxDesiredWidth, DeltaTime ,RateBasedInterpSpeed);
				if(bOverride_MaxDesiredHeight) CastedVar->MaxDesiredHeight = FMath::FInterpTo(CastedVar->MaxDesiredHeight , TargetMaxDesiredHeight, DeltaTime ,RateBasedInterpSpeed);
				if(bOverride_MinAspectRatio) CastedVar->MinAspectRatio = FMath::FInterpTo(CastedVar->MinAspectRatio , TargetMinAspectRatio, DeltaTime ,RateBasedInterpSpeed);
				if(bOverride_MaxAspectRatio) CastedVar->MaxAspectRatio = FMath::FInterpTo(CastedVar->MaxAspectRatio , TargetMaxAspectRatio, DeltaTime ,RateBasedInterpSpeed);
			}
			
		}else
		{
			if(bOverride_WidthOverride) CastedVar->WidthOverride = TargetWidthOverride;
			if(bOverride_HeightOverride) CastedVar->HeightOverride = TargetHeightOverride;
			if(bOverride_MinDesiredWidth) CastedVar->MinDesiredWidth = TargetMinDesiredWidth;
			if(bOverride_MinDesiredHeight) CastedVar->MinDesiredHeight = TargetMinDesiredHeight;
			if(bOverride_MaxDesiredWidth) CastedVar->MaxDesiredWidth = TargetMaxDesiredWidth;
			if(bOverride_MaxDesiredHeight) CastedVar->MaxDesiredHeight = TargetMaxDesiredHeight;
			if(bOverride_MinAspectRatio) CastedVar->MinAspectRatio = TargetMinAspectRatio;
			if(bOverride_MaxAspectRatio) CastedVar->MaxAspectRatio = TargetMaxAspectRatio;
		}
		
		break;
	case EVoltInterpMode::AlphaBased:
		
		if(AlphaBasedDuration > 0)
		{
			AccumulatedTime += DeltaTime;

			AlphaBasedEaseAlpha = FMath::Clamp<double>(AccumulatedTime / AlphaBasedDuration, 0.f, 1.f);
		}

		if(bOverride_WidthOverride) CastedVar->WidthOverride = UKismetMathLibrary::Ease(StartWidthOverride, TargetWidthOverride, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		if(bOverride_HeightOverride) CastedVar->HeightOverride = UKismetMathLibrary::Ease(StartHeightOverride, TargetHeightOverride, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		if(bOverride_MinDesiredWidth) CastedVar->MinDesiredWidth = UKismetMathLibrary::Ease(StartMinDesiredWidth, TargetMinDesiredWidth, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		if(bOverride_MinDesiredHeight) CastedVar->MinDesiredHeight = UKismetMathLibrary::Ease(StartMinDesiredHeight, TargetMinDesiredHeight, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		if(bOverride_MaxDesiredWidth) CastedVar->MaxDesiredWidth = UKismetMathLibrary::Ease(StartMaxDesiredWidth, TargetMaxDesiredWidth, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		if(bOverride_MaxDesiredHeight) CastedVar->MaxDesiredHeight = UKismetMathLibrary::Ease(StartMaxDesiredHeight, TargetMaxDesiredHeight, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		if(bOverride_MinAspectRatio) CastedVar->MinAspectRatio = UKismetMathLibrary::Ease(StartMinAspectRatio, TargetMinAspectRatio, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		if(bOverride_MaxAspectRatio) CastedVar->MaxAspectRatio = UKismetMathLibrary::Ease(StartMaxAspectRatio, TargetMaxAspectRatio, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		
		break;
	}
	
}

void UVolt_ASM_InterpBoxProperties::OnModuleBeginPlay_Implementation()
{
	if(GetVoltSlate() == nullptr) return;
	if(GetVoltSlate()->GetVoltVariableCollection() == nullptr) return;
	
	UVoltVariableBase* Var = GetVoltSlate()->GetVoltVariableCollection()->FindOrAddVariable(UVoltVar_Box::StaticClass());

	if(!Var) return;

	UVoltVar_Box* CastedVar = Cast<UVoltVar_Box>(Var);

	CastedVar->bOverride_WidthOverride = bOverride_WidthOverride;
	CastedVar->bOverride_HeightOverride = bOverride_HeightOverride;
	CastedVar->bOverride_MinDesiredWidth = bOverride_MinDesiredWidth;
	CastedVar->bOverride_MinDesiredHeight = bOverride_MinDesiredHeight;
	CastedVar->bOverride_MaxDesiredWidth = bOverride_MaxDesiredWidth;
	CastedVar->bOverride_MaxDesiredHeight = bOverride_MaxDesiredHeight;
	CastedVar->bOverride_MaxAspectRatio = bOverride_MaxAspectRatio;
	CastedVar->bOverride_MinAspectRatio = bOverride_MinAspectRatio;
	
	if(InterpolationMode == EVoltInterpMode::AlphaBased)
	{
		if(!bUseStartHeightOverride)
		{
			StartHeightOverride = CastedVar->HeightOverride;
			StartWidthOverride = CastedVar->WidthOverride;
			StartMinDesiredWidth = CastedVar->MinDesiredWidth;
			StartMinDesiredHeight = CastedVar->MinDesiredHeight;
			StartMaxDesiredWidth = CastedVar->MaxDesiredWidth;
			StartMaxDesiredHeight = CastedVar->MaxDesiredHeight;
			StartMinAspectRatio = CastedVar->MinAspectRatio;
			StartMaxAspectRatio = CastedVar->MaxAspectRatio;
		}
	}else
	{
		if(bUseStartHeightOverride) CastedVar->HeightOverride = StartHeightOverride;
		if(bUseStartWidthOverride) CastedVar->WidthOverride = StartWidthOverride;
		if(bUseStartMinDesiredWidth) CastedVar->MinDesiredWidth = StartMinDesiredWidth;
		if(bUseStartMinDesiredHeight) CastedVar->MinDesiredHeight = StartMinDesiredHeight;
		if(bUseStartMaxDesiredWidth) CastedVar->MaxDesiredWidth = StartMaxDesiredWidth;
		if(bUseStartMaxDesiredHeight) CastedVar->MaxDesiredHeight = StartMaxDesiredHeight;
		if(bUseStartMinAspectRatio) CastedVar->MinAspectRatio = StartMinAspectRatio;
		if(bUseStartMaxAspectRatio) CastedVar->MaxAspectRatio = StartMaxAspectRatio;
	}

	
	
	
	AccumulatedTime = 0;
}

void UVolt_ASM_InterpBoxProperties::OnModuleEndPlay_Implementation()
{
	Super::OnModuleEndPlay_Implementation();
}

bool UVolt_ASM_InterpBoxProperties::IsActive()
{
	const TScriptInterface<IVoltInterface>& SlateInterface = GetVoltSlate();
	if(SlateInterface == nullptr) return false;
	if(SlateInterface->GetVoltVariableCollection() == nullptr) return false;
	
	UVoltVariableBase* Var = SlateInterface->GetVoltVariableCollection()->FindOrAddVariable(UVoltVar_Box::StaticClass());

	UVoltVar_Box* CastedVar = Cast<UVoltVar_Box>(Var);
	
	switch (InterpolationMode)
	{
	case EVoltInterpMode::RateBased:

		if(bRateBasedNeverFinish) return true;
	
		if(CastedVar)
		{
			return (bOverride_HeightOverride ? CastedVar->HeightOverride != TargetHeightOverride : false)
			|| (bOverride_WidthOverride ? CastedVar->WidthOverride != TargetWidthOverride : false)
			|| (bOverride_MinDesiredWidth ? CastedVar->MinDesiredWidth != TargetMinDesiredWidth : false) 
			|| (bOverride_MinDesiredHeight ? CastedVar->MinDesiredHeight != TargetMinDesiredHeight : false)
			|| (bOverride_MaxDesiredWidth ? CastedVar->MaxDesiredWidth != TargetMaxDesiredWidth : false)
			|| (bOverride_MaxDesiredHeight ? CastedVar->MaxDesiredHeight != TargetMaxDesiredHeight : false)
			|| (bOverride_MinAspectRatio ? CastedVar->MinAspectRatio != TargetMinAspectRatio : false)
			|| (bOverride_MaxAspectRatio ? CastedVar->MaxAspectRatio != TargetMaxAspectRatio : false);
		}
		
		break;
		
	case EVoltInterpMode::AlphaBased:
		
		return AlphaBasedDuration > 0 ? AccumulatedTime < AlphaBasedDuration : true;
		
		break;
	}
	
	return false;
}
