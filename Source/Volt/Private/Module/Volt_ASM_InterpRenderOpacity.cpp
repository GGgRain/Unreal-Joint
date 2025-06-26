//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Volt_ASM_InterpRenderOpacity.h"

#include "VoltInterface.h"
#include "VoltVariableCollection.h"
#include "Variables/VoltVariables.h"


void UVolt_ASM_InterpRenderOpacity::Construct(const FArguments& InArgs)
{

	InterpolationMode = InArgs._InterpolationMode;
	
	StartOpacity = InArgs._StartOpacity;
	bUseStartOpacity = InArgs._bUseStartOpacity;
	TargetOpacity = InArgs._TargetOpacity;

	RateBasedInterpSpeed = InArgs._RateBasedInterpSpeed;
	bRateBasedUseConstant = InArgs._bRateBasedUseConstant;
	bRateBasedNeverFinish = InArgs._bRateBasedNeverFinish;

	AlphaBasedEasingFunction = InArgs._AlphaBasedEasingFunction;
	AlphaBasedDuration = InArgs._AlphaBasedDuration;
	AlphaBasedBlendExp = InArgs._AlphaBasedBlendExp;
	AlphaBasedSteps = InArgs._AlphaBasedSteps;
}

void UVolt_ASM_InterpRenderOpacity::ModifySlateVariable(const float DeltaTime,
                                                const TScriptInterface<IVoltInterface>& Volt)
{
	if(Volt == nullptr) return;
	if(Volt->GetVoltVariableCollection() == nullptr) return;

	UVoltVariableBase* Var = Volt->GetVoltVariableCollection()->FindOrAddVariable(UVoltVar_Opacity::StaticClass());

	UVoltVar_Opacity* CastedVar = Cast<UVoltVar_Opacity>(Var);

	//interp
	switch (InterpolationMode)
	{
	case EVoltInterpMode::RateBased:

		if(RateBasedInterpSpeed > 0)
		{
			if(bRateBasedUseConstant == true)
			{
				CastedVar->Value = FMath::FInterpConstantTo(CastedVar->Value, TargetOpacity, DeltaTime ,RateBasedInterpSpeed);
			}else
			{
				CastedVar->Value = FMath::FInterpTo(CastedVar->Value, TargetOpacity, DeltaTime ,RateBasedInterpSpeed);
			}
			
		}else
		{
			CastedVar->Value = TargetOpacity;
		}
		
		break;
	case EVoltInterpMode::AlphaBased:

		if(AlphaBasedDuration > 0)
		{
			AccumulatedTime += DeltaTime;

			AlphaBasedEaseAlpha = FMath::Clamp<double>(AccumulatedTime / AlphaBasedDuration, 0.f, 1.f);
		}

		CastedVar->Value = UKismetMathLibrary::Ease(StartOpacity, TargetOpacity, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		
		break;
	}
	
}

void UVolt_ASM_InterpRenderOpacity::OnModuleBeginPlay_Implementation()
{

	if(GetVoltSlate() == nullptr) return;
	if(GetVoltSlate()->GetVoltVariableCollection() == nullptr) return;

	UVoltVariableBase* Var = GetVoltSlate()->GetVoltVariableCollection()->FindOrAddVariable(UVoltVar_Opacity::StaticClass());

	UVoltVar_Opacity* CastedVar = Cast<UVoltVar_Opacity>(Var);
	
	if(InterpolationMode == EVoltInterpMode::AlphaBased)
	{
		if(!bUseStartOpacity) StartOpacity = CastedVar->Value;

	}else
	{
		if(bUseStartOpacity) CastedVar->Value = StartOpacity;
	}

	
	AccumulatedTime = 0;
}

void UVolt_ASM_InterpRenderOpacity::OnModuleEndPlay_Implementation()
{
	Super::OnModuleEndPlay_Implementation();
}

bool UVolt_ASM_InterpRenderOpacity::IsActive()
{
	const TScriptInterface<IVoltInterface>& SlateInterface = GetVoltSlate();
	if(SlateInterface == nullptr) return false;
	if(SlateInterface->GetVoltVariableCollection() == nullptr) return false;
	
	UVoltVariableBase* Var = SlateInterface->GetVoltVariableCollection()->FindOrAddVariable(UVoltVar_Opacity::StaticClass());

	UVoltVar_Opacity* CastedVar = Cast<UVoltVar_Opacity>(Var);
	
	switch (InterpolationMode)
	{
	case EVoltInterpMode::RateBased:

		if(bRateBasedNeverFinish) return true;

		if(CastedVar) return CastedVar->Value != TargetOpacity;

		break;
		
	case EVoltInterpMode::AlphaBased:
		
		return (AlphaBasedDuration > 0) ? AccumulatedTime < AlphaBasedDuration : true;
		
		break;
	}
	
	return false;
}
