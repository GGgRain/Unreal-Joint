//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Volt_ASM_InterpBackgroundColor.h"
#include "VoltInterface.h"
#include "VoltVariableCollection.h"
#include "Variables/VoltVariables.h"
#include "Kismet/KismetMathLibrary.h"


void UVolt_ASM_InterpBackgroundColor::Construct(const FArguments& InArgs)
{

	InterpolationMode = InArgs._InterpolationMode;
	
	StartColor = InArgs._StartColor;
	bUseStartColor = InArgs._bUseStartColor;
	TargetColor = InArgs._TargetColor;

	RateBasedInterpSpeed = InArgs._RateBasedInterpSpeed;
	bRateBasedUseConstant = InArgs._bRateBasedUseConstant;
	bRateBasedNeverFinish = InArgs._bRateBasedNeverFinish;

	AlphaBasedEasingFunction = InArgs._AlphaBasedEasingFunction;
	AlphaBasedDuration = InArgs._AlphaBasedDuration;
	AlphaBasedBlendExp = InArgs._AlphaBasedBlendExp;
	AlphaBasedSteps = InArgs._AlphaBasedSteps;
}


void UVolt_ASM_InterpBackgroundColor::ModifySlateVariable(const float DeltaTime,
                                                const TScriptInterface<IVoltInterface>& Volt)
{
	if(Volt == nullptr) return;
	if(Volt->GetVoltVariableCollection() == nullptr) return;

	UVoltVariableBase* Var = Volt->GetVoltVariableCollection()->FindOrAddVariable(UVoltVar_BackgroundColor::StaticClass());

	UVoltVar_BackgroundColor* CastedVar = Cast<UVoltVar_BackgroundColor>(Var);
	
	//interp
	switch (InterpolationMode)
	{
	case EVoltInterpMode::RateBased:

		if(RateBasedInterpSpeed > 0)
		{
			if(bRateBasedUseConstant)
			{
				CastedVar->Value.R = FMath::FInterpConstantTo(CastedVar->Value.R, TargetColor.R, DeltaTime ,RateBasedInterpSpeed);
				CastedVar->Value.G = FMath::FInterpConstantTo(CastedVar->Value.G, TargetColor.G, DeltaTime ,RateBasedInterpSpeed);
				CastedVar->Value.B = FMath::FInterpConstantTo(CastedVar->Value.B, TargetColor.B, DeltaTime ,RateBasedInterpSpeed);
				CastedVar->Value.A = FMath::FInterpConstantTo(CastedVar->Value.A, TargetColor.A, DeltaTime ,RateBasedInterpSpeed);
			}else
			{
				CastedVar->Value.R = FMath::FInterpTo(CastedVar->Value.R, TargetColor.R, DeltaTime ,RateBasedInterpSpeed);
				CastedVar->Value.G = FMath::FInterpTo(CastedVar->Value.G, TargetColor.G, DeltaTime ,RateBasedInterpSpeed);
				CastedVar->Value.B = FMath::FInterpTo(CastedVar->Value.B, TargetColor.B, DeltaTime ,RateBasedInterpSpeed);
				CastedVar->Value.A = FMath::FInterpTo(CastedVar->Value.A, TargetColor.A, DeltaTime ,RateBasedInterpSpeed);
			}
			
		}else
		{
			CastedVar->Value = TargetColor;
		}
		
		break;
	case EVoltInterpMode::AlphaBased:

		if(AlphaBasedDuration > 0)
		{
			AccumulatedTime += DeltaTime;

			AlphaBasedEaseAlpha = FMath::Clamp<double>(AccumulatedTime / AlphaBasedDuration, 0.f, 1.f);
		}

		CastedVar->Value.R = UKismetMathLibrary::Ease(StartColor.R, TargetColor.R, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		CastedVar->Value.G = UKismetMathLibrary::Ease(StartColor.G, TargetColor.G, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		CastedVar->Value.B = UKismetMathLibrary::Ease(StartColor.B, TargetColor.B, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		CastedVar->Value.A = UKismetMathLibrary::Ease( StartColor.A, TargetColor.A, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		
		break;
	}
	
}

void UVolt_ASM_InterpBackgroundColor::OnModuleBeginPlay_Implementation()
{
	if(GetVoltSlate() == nullptr) return;
	if(GetVoltSlate()->GetVoltVariableCollection() == nullptr) return;

	UVoltVariableBase* Var = GetVoltSlate()->GetVoltVariableCollection()->FindOrAddVariable(UVoltVar_BackgroundColor::StaticClass());

	UVoltVar_BackgroundColor* CastedVar = Cast<UVoltVar_BackgroundColor>(Var);

	if(InterpolationMode == EVoltInterpMode::AlphaBased)
	{
		if(!bUseStartColor) StartColor = CastedVar->Value;
	}else
	{
		if(bUseStartColor) CastedVar->Value = StartColor;
	}

		
	AccumulatedTime = 0;
	
}

void UVolt_ASM_InterpBackgroundColor::OnModuleEndPlay_Implementation()
{
	Super::OnModuleEndPlay_Implementation();
}

bool UVolt_ASM_InterpBackgroundColor::IsActive()
{

	const TScriptInterface<IVoltInterface>& SlateInterface = GetVoltSlate();
	if(SlateInterface == nullptr) return false;
	if(SlateInterface->GetVoltVariableCollection() == nullptr) return false;
	
	UVoltVariableBase* Var = SlateInterface->GetVoltVariableCollection()->FindOrAddVariable(UVoltVar_BackgroundColor::StaticClass());

	UVoltVar_BackgroundColor* CastedVar = Cast<UVoltVar_BackgroundColor>(Var);
	
	switch (InterpolationMode)
	{
	case EVoltInterpMode::RateBased:

		if(bRateBasedNeverFinish) return true;

		if(CastedVar) return FLinearColor::Dist(CastedVar->Value, TargetColor) > 0.001 ;

		break;
		
	case EVoltInterpMode::AlphaBased:
		
		return (AlphaBasedDuration > 0) ? AccumulatedTime < AlphaBasedDuration : true;
		
		break;
	}
	
	return false;
}
