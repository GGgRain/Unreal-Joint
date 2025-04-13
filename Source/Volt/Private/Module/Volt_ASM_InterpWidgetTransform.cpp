//Copyright 2022~2024 DevGrain. All Rights Reserved.

#include "Module/Volt_ASM_InterpWidgetTransform.h"

#include "VoltInterface.h"
#include "VoltVariableCollection.h"
#include "Variables/VoltVariables.h"


void UVolt_ASM_InterpWidgetTransform::Construct(const FArguments& InArgs)
{
	InterpolationMode = InArgs._InterpolationMode;

	TargetWidgetTransform = InArgs._TargetWidgetTransform;
	bUseStartWidgetTransform = InArgs._bUseStartWidgetTransform;
	StartWidgetTransform = InArgs._StartWidgetTransform;
	
	RateBasedInterpSpeed = InArgs._RateBasedInterpSpeed;
	bRateBasedUseConstant = InArgs._bRateBasedUseConstant;
	bRateBasedNeverFinish = InArgs._bRateBasedNeverFinish;

	AlphaBasedEasingFunction = InArgs._AlphaBasedEasingFunction;
	AlphaBasedDuration = InArgs._AlphaBasedDuration;
	AlphaBasedBlendExp = InArgs._AlphaBasedBlendExp;
	AlphaBasedSteps = InArgs._AlphaBasedSteps;
}

void UVolt_ASM_InterpWidgetTransform::ModifySlateVariable(const float DeltaTime,
                                                const TScriptInterface<IVoltInterface>& Volt)
{
	if(Volt == nullptr) return;
	if(Volt->GetVoltVariableCollection() == nullptr) return;

	UVoltVariableBase* Var = Volt->GetVoltVariableCollection()->FindOrAddVariable(UVoltVar_WidgetTransform::StaticClass());

	UVoltVar_WidgetTransform* CastedVar = Cast<UVoltVar_WidgetTransform>(Var);

	//interp
	switch (InterpolationMode)
	{
	case EVoltInterpMode::RateBased:

		if(RateBasedInterpSpeed > 0)
		{
			if(bRateBasedUseConstant)
			{
				CastedVar->Value.Angle = FMath::FInterpConstantTo(CastedVar->Value.Angle, TargetWidgetTransform.Angle, DeltaTime ,RateBasedInterpSpeed);
				CastedVar->Value.Scale = FMath::Vector2DInterpConstantTo(CastedVar->Value.Scale, TargetWidgetTransform.Scale, DeltaTime ,RateBasedInterpSpeed);
				CastedVar->Value.Shear = FMath::Vector2DInterpConstantTo(CastedVar->Value.Shear, TargetWidgetTransform.Shear, DeltaTime ,RateBasedInterpSpeed);
				CastedVar->Value.Translation = FMath::Vector2DInterpConstantTo(CastedVar->Value.Translation, TargetWidgetTransform.Translation, DeltaTime,RateBasedInterpSpeed);
			}else
			{
				CastedVar->Value.Angle = FMath::FInterpTo(CastedVar->Value.Angle,TargetWidgetTransform.Angle,DeltaTime,RateBasedInterpSpeed);
				CastedVar->Value.Scale = FMath::Vector2DInterpTo(CastedVar->Value.Scale,TargetWidgetTransform.Scale,DeltaTime,RateBasedInterpSpeed);
				CastedVar->Value.Shear = FMath::Vector2DInterpTo(CastedVar->Value.Shear,TargetWidgetTransform.Shear,DeltaTime,RateBasedInterpSpeed);
				CastedVar->Value.Translation = FMath::Vector2DInterpTo(CastedVar->Value.Translation,TargetWidgetTransform.Translation,DeltaTime,RateBasedInterpSpeed);
			}
			
		}else
		{
			CastedVar->Value = TargetWidgetTransform;
		}
		
		break;
	case EVoltInterpMode::AlphaBased:
		
		if(AlphaBasedDuration > 0)
		{
			AccumulatedTime += DeltaTime;

			AlphaBasedEaseAlpha = FMath::Clamp<double>(AccumulatedTime / AlphaBasedDuration, 0.f, 1.f);
		}

		CastedVar->Value.Angle = UKismetMathLibrary::Ease(StartWidgetTransform.Angle, TargetWidgetTransform.Angle, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		CastedVar->Value.Scale.X = UKismetMathLibrary::Ease(StartWidgetTransform.Scale.X, TargetWidgetTransform.Scale.X, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		CastedVar->Value.Scale.Y = UKismetMathLibrary::Ease(StartWidgetTransform.Scale.Y, TargetWidgetTransform.Scale.Y, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);

		CastedVar->Value.Shear.X = UKismetMathLibrary::Ease(StartWidgetTransform.Shear.X, TargetWidgetTransform.Shear.X, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		CastedVar->Value.Shear.Y = UKismetMathLibrary::Ease(StartWidgetTransform.Shear.Y, TargetWidgetTransform.Shear.Y, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);

		CastedVar->Value.Translation.X = UKismetMathLibrary::Ease(StartWidgetTransform.Translation.X, TargetWidgetTransform.Translation.X, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		CastedVar->Value.Translation.Y = UKismetMathLibrary::Ease(StartWidgetTransform.Translation.Y, TargetWidgetTransform.Translation.Y, AlphaBasedEaseAlpha, AlphaBasedEasingFunction, AlphaBasedBlendExp, AlphaBasedSteps);
		
		break;
	}
	
}

void UVolt_ASM_InterpWidgetTransform::OnModuleBeginPlay_Implementation()
{
	if(GetVoltSlate() == nullptr) return;
	if(GetVoltSlate()->GetVoltVariableCollection() == nullptr) return;
	
	UVoltVariableBase* Var = GetVoltSlate()->GetVoltVariableCollection()->FindOrAddVariable(UVoltVar_WidgetTransform::StaticClass());

	UVoltVar_WidgetTransform* CastedVar = Cast<UVoltVar_WidgetTransform>(Var);
	
	if(InterpolationMode == EVoltInterpMode::AlphaBased)
	{
		if(!bUseStartWidgetTransform) StartWidgetTransform = CastedVar->Value;
	}else
	{
		if(bUseStartWidgetTransform) CastedVar->Value = StartWidgetTransform;
	}

	
	AccumulatedTime = 0;
}

void UVolt_ASM_InterpWidgetTransform::OnModuleEndPlay_Implementation()
{
	Super::OnModuleEndPlay_Implementation();
}

bool UVolt_ASM_InterpWidgetTransform::IsActive()
{
	const TScriptInterface<IVoltInterface>& SlateInterface = GetVoltSlate();
	if(SlateInterface == nullptr) return false;
	if(SlateInterface->GetVoltVariableCollection() == nullptr) return false;
	
	UVoltVariableBase* Var = SlateInterface->GetVoltVariableCollection()->FindOrAddVariable(UVoltVar_WidgetTransform::StaticClass());

	UVoltVar_WidgetTransform* CastedVar = Cast<UVoltVar_WidgetTransform>(Var);
	
	switch (InterpolationMode)
	{
	case EVoltInterpMode::RateBased:

		if(bRateBasedNeverFinish) return true;
	
		if(CastedVar) return CastedVar->Value != TargetWidgetTransform;

		break;
		
	case EVoltInterpMode::AlphaBased:
		
		return AlphaBasedDuration > 0 ? AccumulatedTime < AlphaBasedDuration : true;
		
		break;
	}
	
	return false;
}
