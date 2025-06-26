//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Volt_ASM_InterpChildSlotPadding.h"

#include "VoltInterface.h"
#include "VoltVariableCollection.h"
#include "Variables/VoltVariables.h"


void UVolt_ASM_InterpChildSlotPadding::Construct(const FArguments& InArgs)
{
	InterpolationMode = InArgs._InterpolationMode;

	StartPadding = InArgs._StartPadding;
	bUseStartPadding = InArgs._bUseStartPadding;
	TargetPadding = InArgs._TargetPadding;

	RateBasedInterpSpeed = InArgs._RateBasedInterpSpeed;
	bRateBasedUseConstant = InArgs._bRateBasedUseConstant;
	bRateBasedNeverFinish = InArgs._bRateBasedNeverFinish;

	AlphaBasedEasingFunction = InArgs._AlphaBasedEasingFunction;
	AlphaBasedDuration = InArgs._AlphaBasedDuration;
	AlphaBasedBlendExp = InArgs._AlphaBasedBlendExp;
	AlphaBasedSteps = InArgs._AlphaBasedSteps;
}

void UVolt_ASM_InterpChildSlotPadding::ModifySlateVariable(const float DeltaTime,
                                                           const TScriptInterface<IVoltInterface>& Volt)
{
	if (Volt == nullptr) return;
	if (Volt->GetVoltVariableCollection() == nullptr) return;

	UVoltVariableBase* Var = Volt->GetVoltVariableCollection()->FindOrAddVariable(
		UVoltVar_ChildSlotPadding::StaticClass());

	UVoltVar_ChildSlotPadding* CastedVar = Cast<UVoltVar_ChildSlotPadding>(Var);

	//interp
	switch (InterpolationMode)
	{
	case EVoltInterpMode::RateBased:

		if (RateBasedInterpSpeed > 0)
		{
			if (bRateBasedUseConstant)
			{
				CastedVar->Value.Bottom = FMath::FInterpConstantTo(CastedVar->Value.Bottom, TargetPadding.Bottom,
				                                                   DeltaTime, RateBasedInterpSpeed);
				CastedVar->Value.Top = FMath::FInterpConstantTo(CastedVar->Value.Top, TargetPadding.Top, DeltaTime,
				                                                RateBasedInterpSpeed);
				CastedVar->Value.Right = FMath::FInterpConstantTo(CastedVar->Value.Right, TargetPadding.Right,
				                                                  DeltaTime, RateBasedInterpSpeed);
				CastedVar->Value.Left = FMath::FInterpConstantTo(CastedVar->Value.Left, TargetPadding.Left, DeltaTime,
				                                                 RateBasedInterpSpeed);
			}
			else
			{
				CastedVar->Value.Bottom = FMath::FInterpTo(CastedVar->Value.Bottom, TargetPadding.Bottom, DeltaTime,
				                                           RateBasedInterpSpeed);
				CastedVar->Value.Top = FMath::FInterpTo(CastedVar->Value.Top, TargetPadding.Top, DeltaTime,
				                                        RateBasedInterpSpeed);
				CastedVar->Value.Right = FMath::FInterpTo(CastedVar->Value.Right, TargetPadding.Right, DeltaTime,
				                                          RateBasedInterpSpeed);
				CastedVar->Value.Left = FMath::FInterpTo(CastedVar->Value.Left, TargetPadding.Left, DeltaTime,
				                                         RateBasedInterpSpeed);
			}
		}
		else
		{
			CastedVar->Value = TargetPadding;
		}

		break;
	case EVoltInterpMode::AlphaBased:

		if(AlphaBasedDuration > 0)
		{
			AccumulatedTime += DeltaTime;

			AlphaBasedEaseAlpha = FMath::Clamp<double>(AccumulatedTime / AlphaBasedDuration, 0.f, 1.f);
		}

		CastedVar->Value.Bottom = UKismetMathLibrary::Ease(StartPadding.Bottom, TargetPadding.Bottom,
		                                                   AlphaBasedEaseAlpha, AlphaBasedEasingFunction,
		                                                   AlphaBasedBlendExp, AlphaBasedSteps);
		CastedVar->Value.Top = UKismetMathLibrary::Ease(StartPadding.Top, TargetPadding.Top, AlphaBasedEaseAlpha,
		                                                AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		CastedVar->Value.Right = UKismetMathLibrary::Ease(StartPadding.Right, TargetPadding.Right, AlphaBasedEaseAlpha,
		                                                  AlphaBasedEasingFunction, AlphaBasedBlendExp,
		                                                  AlphaBasedSteps);
		CastedVar->Value.Left = UKismetMathLibrary::Ease(StartPadding.Left, TargetPadding.Left, AlphaBasedEaseAlpha,
		                                                 AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		
		break;
	}
}

void UVolt_ASM_InterpChildSlotPadding::OnModuleBeginPlay_Implementation()
{
	if (GetVoltSlate() == nullptr) return;
	if (GetVoltSlate()->GetVoltVariableCollection() == nullptr) return;

	UVoltVariableBase* Var = GetVoltSlate()->GetVoltVariableCollection()->FindOrAddVariable(
		UVoltVar_ChildSlotPadding::StaticClass());

	UVoltVar_ChildSlotPadding* CastedVar = Cast<UVoltVar_ChildSlotPadding>(Var);

	if (InterpolationMode == EVoltInterpMode::AlphaBased)
	{
		if (!bUseStartPadding) StartPadding = CastedVar->Value;
	}else
	{
		if (bUseStartPadding) CastedVar->Value = StartPadding;
	}
	
	AccumulatedTime = 0;

}

void UVolt_ASM_InterpChildSlotPadding::OnModuleEndPlay_Implementation()
{
	Super::OnModuleEndPlay_Implementation();
}

bool UVolt_ASM_InterpChildSlotPadding::IsActive()
{
	const TScriptInterface<IVoltInterface>& SlateInterface = GetVoltSlate();
	if (SlateInterface == nullptr) return false;
	if (SlateInterface->GetVoltVariableCollection() == nullptr) return false;

	UVoltVariableBase* Var = SlateInterface->GetVoltVariableCollection()->FindOrAddVariable(
		UVoltVar_ChildSlotPadding::StaticClass());

	UVoltVar_ChildSlotPadding* CastedVar = Cast<UVoltVar_ChildSlotPadding>(Var);

	switch (InterpolationMode)
	{
	case EVoltInterpMode::RateBased:

		if (bRateBasedNeverFinish) return true;

		if (CastedVar) return CastedVar->Value != TargetPadding;

		break;

	case EVoltInterpMode::AlphaBased:

		return (AlphaBasedDuration > 0) ? AccumulatedTime < AlphaBasedDuration : true;

		break;
	}

	return false;
}
