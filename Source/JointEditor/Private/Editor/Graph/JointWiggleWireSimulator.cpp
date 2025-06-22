// Copyright Epic Games, Inc. All Rights Reserved.

#include "JointWiggleWireSimulator.h"


FWiggleWireSimulator::FWiggleWireSimulator()
    : CurrentVisualOffset(FVector2D::ZeroVector)
    , Velocity(FVector2D::ZeroVector)
    , TargetVisualOffset(FVector2D::ZeroVector)
    , LastInteractionTime(0.0)
    , LastStartPoint(FVector2D::ZeroVector)
    , LastEndPoint(FVector2D::ZeroVector)
    , LastEndpointVelocity(FVector2D::ZeroVector)
    , bIsActive(false)
{
}

FVector2D FWiggleWireSimulator::Update(const FVector2D& StartPoint, const FVector2D& EndPoint, const FWiggleWireConfig& Config, float DeltaTime)
{
    // Clamp delta time to prevent simulation instability
    DeltaTime = FMath::Min(DeltaTime, MAX_DELTA_TIME);

    // Calculate endpoint movement
    const FVector2D CurrentEndpointDelta = ((StartPoint - LastStartPoint) + (EndPoint - LastEndPoint)) * 0.5f;
    const FVector2D CurrentEndpointVelocity = CurrentEndpointDelta / FMath::Max(DeltaTime, KINDA_SMALL_NUMBER_2D);
    
    // Detect significant movement
    if (!StartPoint.Equals(LastStartPoint, POSITION_CHANGE_THRESHOLD) || !EndPoint.Equals(LastEndPoint, POSITION_CHANGE_THRESHOLD))
    {
        // Add impulse to velocity based on movement speed
        const float MovementMagnitude = CurrentEndpointDelta.Size();
        if (MovementMagnitude > POSITION_CHANGE_THRESHOLD)
        {
            const FVector2D Direction = (EndPoint - StartPoint).GetSafeNormal();
            const FVector2D PerpDirection = FVector2D(-Direction.Y, Direction.X);
            
            // Apply impulse proportional to movement speed
            Velocity += PerpDirection * MovementMagnitude * MOVEMENT_IMPULSE_SCALE * Config.MovementResponseFactor;
        }
        
        Activate();
    }

    // Calculate target offset
    TargetVisualOffset = CalculateTargetOffset(StartPoint, EndPoint, Config);
    
    // Get time since last interaction for adaptive damping
    const double CurrentTime = FApp::GetCurrentTime();
    const double TimeSinceInteraction = CurrentTime - LastInteractionTime;
    
    // Calculate adaptive damping factor
    const float AdaptiveDamping = CalculateAdaptiveDamping(Config, TimeSinceInteraction);
    
    // Use the preferred direction to ensure the wire has downward curvature
    const FVector2D PreferredDirection(0.0f, 1.0f);
    
    // Apply spring physics
    ApplySpringPhysics(CurrentVisualOffset, Velocity, TargetVisualOffset, DeltaTime,
        Config.Stiffness,AdaptiveDamping,PreferredDirection);
    
    // Ensure wires don't curve upward (Y is negative in Slate coordinates)
    if (CurrentVisualOffset.Y < 0.0f)
    {
        CurrentVisualOffset.Y = 0.0f;
        
        // Adjust X component to maintain a natural curve
        if (!TargetVisualOffset.IsNearlyZero())
        {
            const float TargetMagnitude = TargetVisualOffset.Size();
            CurrentVisualOffset.X = FMath::Sign(CurrentVisualOffset.X) * TargetMagnitude * DIRECTIONAL_ADJUSTMENT_SCALE;
        }
    }
    
    // Determine if simulation should be active
    if (TimeSinceInteraction > Config.InactivityThreshold)
    {
        const bool bCloseEnough = CurrentVisualOffset.Equals(TargetVisualOffset, 1.0f);
        const bool bVelocityLow = Velocity.IsNearlyZero(0.5f);
        if (bCloseEnough && bVelocityLow)
        {
            bIsActive = false;
            Velocity = FVector2D::ZeroVector;
        }
    }
    
    // Store state for the next frame
    LastEndpointVelocity = CurrentEndpointVelocity;
    LastStartPoint = StartPoint;
    LastEndPoint = EndPoint;
    
    return CurrentVisualOffset;
}

void FWiggleWireSimulator::Activate()
{
    bIsActive = true;
    LastInteractionTime = FApp::GetCurrentTime();
}

bool FWiggleWireSimulator::IsActive() const
{
    return bIsActive;
}

TSharedPtr<FWiggleWireSimulator> FWiggleWireSimulator::MakeInstance()
{
    //return MakeShareable(new FWiggleWireSimulator());
    return MakeShared<FWiggleWireSimulator>();
}

FVector2D FWiggleWireSimulator::GetVisualCenter(const FVector2D& StartPoint, const FVector2D& EndPoint) const
{
    // Calculate the visual center with the current offset applied
    FVector2D Center = (StartPoint + EndPoint) * 0.5f + CurrentVisualOffset;
    
    // Final check to ensure the center doesn't go above the straight line
    const FVector2D StraightCenter = (StartPoint + EndPoint) * 0.5f;
    if (Center.Y < StraightCenter.Y)
    {
        Center.Y = StraightCenter.Y;
    }
    
    return Center;
}

FVector2D FWiggleWireSimulator::CalculateTargetOffset(const FVector2D& StartPoint, const FVector2D& EndPoint,  const FWiggleWireConfig& Config)
{
    const FVector2D Delta = EndPoint - StartPoint;
    const float DistanceSqr = Delta.SizeSquared();

    // Early return for very close points
    static constexpr float MinDistanceSqr = 0.01f;
    if (DistanceSqr < MinDistanceSqr)
    {
        return FVector2D::ZeroVector;
    }

    const float Distance = FMath::Sqrt(DistanceSqr);
    
    // Scale sag for short segments
    static constexpr float MinWiggleDistance = 50.0f;
    const float DistanceScale = FMath::Clamp(Distance / MinWiggleDistance, 0.0f, 1.0f);
    
    // Calculate the tension factor based on distance
    float TensionFactor = 0.0f;
    if (Distance > Config.RelaxedDistanceThreshold)
    {
        TensionFactor = FMath::Clamp((Distance - Config.RelaxedDistanceThreshold) / 
            (Config.TensionDistanceThreshold - Config.RelaxedDistanceThreshold), 0.0f, 1.0f
        );
    }
    
    // Apply tension effect on Slack and hang factors
    const float EffectiveSlackFactor = FMath::Lerp(Config.SlackFactor, 1.01f, TensionFactor);
    const float EffectiveHangFactor = FMath::Lerp(Config.HangFactor, 0.1f, TensionFactor);
    
    // Calculate sag magnitude
    float SagMagnitude = Distance * (EffectiveSlackFactor - 1.0f) * EffectiveHangFactor;
    
    // Apply the maximum sag limit with tension adjustment
    const float EffectiveMaxSag = Config.MaxSag * (1.0f - TensionFactor * 0.8f);
    SagMagnitude = FMath::Min(SagMagnitude, EffectiveMaxSag);
    if (SagMagnitude < KINDA_SMALL_NUMBER_2D)
    {
        return FVector2D::ZeroVector;
    }
    
    // Calculate sag direction
    const FVector2D Direction = Delta / Distance;
    
    // Get perpendicular direction
    FVector2D SagDirection(-Direction.Y, Direction.X);
    
    // Ensure the sag direction points downward (positive Y in Slate coordinates)
    if (SagDirection.Y < 0.0f)
    {
        SagDirection = -SagDirection;
    }
    
    // For tensioned wires, adjust the direction to be more perpendicular
    if (TensionFactor > 0.0f)
    {
        // Direction perpendicular to wire
        FVector2D PerpDirection = FVector2D(-Direction.Y, Direction.X);
        if (PerpDirection.Y < 0.0f)
        {
            PerpDirection = -PerpDirection;
        }
        
        // Interpolate based on tension
        SagDirection = FMath::Lerp(SagDirection, PerpDirection, TensionFactor * 0.8f);
        SagDirection.Normalize();
    }
    
    // Apply distance scaling to make very short segments more rigid
    return SagDirection * SagMagnitude * DistanceScale;
}

void FWiggleWireSimulator::ApplySpringPhysics(FVector2D& Current, FVector2D& InVelocity, const FVector2D& Target, 
    const float DeltaTime,  const float Stiffness, const float DampingRatio, const FVector2D& PreferredDirection)
{
    JointGraphDrawPolicyEditorUtils::Vector2DSpringInterp(
        Current,InVelocity, Target, DeltaTime,
        Stiffness, DampingRatio, PreferredDirection);
}

float FWiggleWireSimulator::CalculateAdaptiveDamping(const FWiggleWireConfig& Config, const double TimeSinceInteraction)
{
    // Start with base damping
    float AdaptiveDamping = Config.DampingRatio;
    
    // Gradually increase damping with inactivity time
    if (TimeSinceInteraction > 0.0f)
    {
        const float InactivityProgress = FMath::Min(
            static_cast<float>(TimeSinceInteraction / Config.InactivityThreshold), 
            1.0f
        );
        
        // Quadratic ramp-up of damping (starts slowly, then becomes more aggressive)
        AdaptiveDamping += InactivityProgress * InactivityProgress * 2.0f;
    }
    
    return AdaptiveDamping;
}

