#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphPin.h"
#include "UObject/Object.h"

#include "Math/UnrealMathUtility.h"
#include "Misc/App.h"

#include "JointWiggleWireSimulator.generated.h"


/**
 * Identifier for a wire connection using start and end pins.
 * Stores raw pointers for quick comparison and handles for validity checks.
 */
struct FGraphWireId
{
	FGraphWireId(const UEdGraphPin* InStartPin, const UEdGraphPin* InEndPin)
		: StartPin(InStartPin)
		, EndPin(InEndPin)
	{
		// Ensure consistent hashing regardless of pin order (maybe unnecessary but good practice)
		const uint32 StartHash = StartPin ? GetTypeHash(StartPin->PinId) : 0;
		const uint32 EndHash = EndPin ? GetTypeHash(EndPin->PinId) : 0;
		// Combine hashes in an order-independent way if needed, though order matters for Start/End conceptually
		// For TMap usage; HashCombine is enough as long as Start/End are consistent.
		Hash = HashCombine(StartHash, EndHash);
	}

	FORCEINLINE bool operator==(const FGraphWireId& Other) const
	{
		// Compare raw pointers for equality check (fastest)
		//return StartPin == Other.StartPin && EndPin == Other.EndPin;
		return Other.Hash == Hash;
	}

	FORCEINLINE bool operator!=(const FGraphWireId& Other) const
	{
		return !(*this == Other);
	}

	friend FORCEINLINE uint32 GetTypeHash(const FGraphWireId& WireId)
	{
		return WireId.Hash;
	}

	/** Checks if this represents a preview connection (one pin is null). */
	bool IsPreviewConnector() const
	{
		return StartPin == nullptr || EndPin == nullptr;
	}

	/** Gets the non-null pin if this is a preview connector. */
	const UEdGraphPin* GetConnectedPin() const
	{
		check(IsPreviewConnector()); // Should only be called on preview connectors
		return StartPin == nullptr ? EndPin : StartPin;
	}
	
private:
	uint32 Hash;

public:
	const UEdGraphPin* StartPin;
	const UEdGraphPin* EndPin;
};

/**
 * Configuration parameters for the visual behavior of wire connections.
 * Controls how wires appear, move, and respond to interactions in the graph editor.
 */
USTRUCT(BlueprintType)
struct JOINTEDITOR_API FWiggleWireConfig
{
    GENERATED_BODY()

    /** Spring stiffness (k). Higher values result in faster movement toward the target position. */
    UPROPERTY(EditAnywhere, config, Category = "WiggleWires", meta = (ClampMin = "1.0", UIMin = "1.0", UIMax = "500.0"))
    float Stiffness = 100.0f;

    /** Damping ratio (ζ). Values < 1.0 produce oscillation, 1.0 is critically damped, > 1.0 is overdamped. */
    UPROPERTY(EditAnywhere, config, Category = "WiggleWires", meta = (ClampMin = "0.1", UIMin = "0.1", UIMax = "5.0"))
    float DampingRatio = 0.8f;

    /** Extra length factor. 1.0 = straight line, values > 1.0 add Slack (e.g., 1.15 = 15% longer). */
    UPROPERTY(EditAnywhere, config, Category = "WiggleWires", meta = (ClampMin = "1.0", UIMin = "1.0", UIMax = "2.0"))
    float SlackFactor = 1.15f;

    /** Vertical sag multiplier. Controls how many wires hang downward under gravity. */
    UPROPERTY(EditAnywhere, config, Category = "WiggleWires", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "5.0"))
    float HangFactor = 1.0f;

    /** Maximum allowed sag amount in Slate units, prevents excessive drooping for long connections. */
    UPROPERTY(EditAnywhere, config, Category = "WiggleWires", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1000.0"))
    float MaxSag = 200.0f;

    /** Bezier tangent multiplier. Controls curve sharpness near connection points. */
    UPROPERTY(EditAnywhere, config, Category = "WiggleWires", meta = (ClampMin = "0.5", UIMin = "0.5", UIMax = "3.0"))
    float TangentFactor = 1.4f;

    /** Time in seconds of inactivity before adaptive damping reaches the maximum. */
    UPROPERTY(EditAnywhere, config, Category = "WiggleWires", meta = (ClampMin = "0.1", UIMin = "0.1", UIMax = "5.0"))
    float InactivityThreshold = 0.75f;

    /** Distance threshold below which wires remain relaxed. */
    UPROPERTY(EditAnywhere, config, Category = "WiggleWires", meta = (ClampMin = "50.0", UIMin = "50.0", UIMax = "1000.0"))
    float RelaxedDistanceThreshold = 300.0f;

    /** Distance threshold above which wires become fully tensioned. */
    UPROPERTY(EditAnywhere, config, Category = "WiggleWires", meta = (ClampMin = "100.0", UIMin = "100.0", UIMax = "2000.0"))
    float TensionDistanceThreshold = 600.0f;

    /** Responsiveness to movement. Higher values make wires more reactive to node movement. */
    UPROPERTY(EditAnywhere, config, Category = "WiggleWires", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "5.0"))
    float MovementResponseFactor = 1.0f;

    /** Default constructor */
    FWiggleWireConfig() = default;

	FWiggleWireConfig(
		const float InStiffness,
		const float InDampingRatio,
		const float InSlackFactor,
		const float InHangFactor,
		const float InMaxSag,
		const float InTangentFactor,
		const float InInactivityThreshold,
		const float InRelaxedDistanceThreshold,
		const float InTensionDistanceThreshold,
		const float InMovementResponseFactor)
		: Stiffness(InStiffness)
		, DampingRatio(InDampingRatio)
		, SlackFactor(InSlackFactor)
		, HangFactor(InHangFactor)
		, MaxSag(InMaxSag)
		, TangentFactor(InTangentFactor)
		, InactivityThreshold(InInactivityThreshold)
		, RelaxedDistanceThreshold(InRelaxedDistanceThreshold)
		, TensionDistanceThreshold(InTensionDistanceThreshold)
		, MovementResponseFactor(InMovementResponseFactor)
	{
	}
};

/**
 * Simulates the visual behavior of wires connecting nodes in the graph editor.
 * Provides physically based movement with spring dynamics, gravity effects, and 
 * tension simulation for a more natural appearance.
 * @author David (hero2xyz), Huge thanks to him for the original implementation.
 */
class JOINTEDITOR_API FWiggleWireSimulator: public TSharedFromThis<FWiggleWireSimulator>
{
public:
    /** Constructor with default initialization. */
    FWiggleWireSimulator();

    /**
     * Updates the simulation state for the current frame.
     * 
     * @param StartPoint     Current position of the connection's start point.
     * @param EndPoint       Current position of the connection's end point.
     * @param Config         Configuration parameters for the simulation.
     * @param DeltaTime      Time elapsed since the last update.
     * @return               The current visual offset to apply to the wire.
     */
    FVector2D Update(const FVector2D& StartPoint, const FVector2D& EndPoint, const FWiggleWireConfig& Config, float DeltaTime);
	
    /**
     * Activates the simulation, resetting inactivity timers.
     * Call when user interaction occurs that should affect the wire.
     */
    void Activate();

    /**
     * Gets the visual center point of the wire with the current offset applied.
     * 
     * @param StartPoint     Current position of the connection's start point.
     * @param EndPoint       Current position of the connection's end point.
     * @return               The calculated visual center position.
     */
    FVector2D GetVisualCenter(const FVector2D& StartPoint, const FVector2D& EndPoint) const;

    /** Checks if the simulation is actively updating. */
    bool IsActive() const;

public:

	static TSharedPtr<FWiggleWireSimulator> MakeInstance();

private:
    /**
     * Calculates the target visual offset based on the wire configuration
     * and current endpoint positions.
     */
    static FVector2D CalculateTargetOffset(const FVector2D& StartPoint, const FVector2D& EndPoint,  const FWiggleWireConfig& Config);

    /**
     * Applies spring-based physics to move the current position toward the target.
     * Handles various damping regimes (underdamped, critically damped, overdamped).
     */
    static void ApplySpringPhysics(FVector2D& Current, FVector2D& InVelocity, const FVector2D& Target,
    	const float DeltaTime, const float Stiffness, const float DampingRatio, const FVector2D& PreferredDirection);

    /**
     * Calculates adaptive damping based on time since the last interaction.
     * Creates more stable resting positions without hard state transitions.
     */
    static float CalculateAdaptiveDamping(const FWiggleWireConfig& Config, const double TimeSinceInteraction);

    /** Current visual offset from the straight-line center. */
    FVector2D CurrentVisualOffset;

    /** Current velocity of the visual offset. */
    FVector2D Velocity;

    /** Target offset the simulation is moving toward. */
    FVector2D TargetVisualOffset;

    /** Time when the last interaction occurred. */
    double LastInteractionTime;

    /** Last known start point, used to detect movement. */
    FVector2D LastStartPoint;

    /** Last known end point, used to detect movement. */
    FVector2D LastEndPoint;

    /** Last measured movement velocity of endpoints. */
    FVector2D LastEndpointVelocity;

    /** Whether the simulation is actively updating. */
    bool bIsActive;

    /** Threshold for position change detection. */
    static constexpr float POSITION_CHANGE_THRESHOLD = 1.0f;

    /** Small epsilon value for floating-point comparisons. */
    static constexpr float KINDA_SMALL_NUMBER_2D = 1.0e-4f;

    /** Maximum time step to prevent simulation instability. */
    static constexpr float MAX_DELTA_TIME = 1.0f / 15.0f;

    /** Movement response scaling factor. */
    static constexpr float MOVEMENT_IMPULSE_SCALE = 0.5f;

    /** Directional adjustment scaling factor. */
    static constexpr float DIRECTIONAL_ADJUSTMENT_SCALE = 0.7f;
};

namespace JointGraphDrawPolicyEditorUtils
{
    /**
     * Optimized spring-damper system for 2D vector interpolation.
     */
    inline void Vector2DSpringInterp(FVector2D& Current, FVector2D& Velocity, const FVector2D& Target, const float DeltaTime, 
                                     const float Stiffness, float DampingRatio, const FVector2D& PreferredDirection)
    {
        // Early return checks
        if (DeltaTime < KINDA_SMALL_NUMBER || Stiffness <= 0.0f)
        { 
            return; 
        }

        // Ensure non-negative damping
        DampingRatio = FMath::Max(0.0f, DampingRatio);

        // Core spring parameters
        const float Omega = FMath::Sqrt(Stiffness);
        const float Exp = FMath::Exp(-DampingRatio * Omega * DeltaTime);
        
        auto ComputeSpringValues = [&](const float Pos, const float Vel, const float TargetPos) -> TPair<float, float>
        {
            const float Delta = Pos - TargetPos;
            
            // Handle different damping regimes
            if (DampingRatio < 1.0f)
            {
                // Underdamped case (oscillates)
                const float OmegaZeta = Omega * DampingRatio;
                const float OmegaD = Omega * FMath::Sqrt(1.0f - DampingRatio * DampingRatio);
                const float Cos = FMath::Cos(OmegaD * DeltaTime);
                const float Sin = FMath::Sin(OmegaD * DeltaTime);
                const float InvOmegaD = 1.0f / OmegaD;

                const float C1 = Delta;
                const float C2 = (Vel + OmegaZeta * Delta) * InvOmegaD;

                const float NewPos = TargetPos + Exp * (C1 * Cos + C2 * Sin);
                const float NewVel = Exp * (-C1 * OmegaZeta * Cos - C2 * OmegaZeta * Sin - C1 * OmegaD * Sin + C2 * OmegaD * Cos);
                return { NewPos, NewVel };
            }
            if (DampingRatio == 1.0f)
            {
                // Critically damped
                const float C1 = Delta;
                const float C2 = Vel + Omega * Delta;

                const float NewPos = TargetPos + (C1 + C2 * DeltaTime) * Exp;
                const float NewVel = (Vel - Omega * (C1 + C2 * DeltaTime)) * Exp;
                return { NewPos, NewVel };
            }
            
            // Overdamped case
            const float R1 = -Omega * (DampingRatio - FMath::Sqrt(DampingRatio * DampingRatio - 1.0f));
            const float R2 = -Omega * (DampingRatio + FMath::Sqrt(DampingRatio * DampingRatio - 1.0f));

            const float C2 = (Vel - R1 * Delta) / (R2 - R1);
            const float C1 = Delta - C2;

            const float NewPos = TargetPos + C1 * FMath::Exp(R1 * DeltaTime) + C2 * FMath::Exp(R2 * DeltaTime);
            const float NewVel = C1 * R1 * FMath::Exp(R1 * DeltaTime) + C2 * R2 * FMath::Exp(R2 * DeltaTime);
            return { NewPos, NewVel };
        };

        // Apply spring physics to both components
        const auto XOut = ComputeSpringValues(Current.X, Velocity.X, Target.X);
        const auto YOut = ComputeSpringValues(Current.Y, Velocity.Y, Target.Y);
        Current = FVector2D(XOut.Key, YOut.Key);
        Velocity = FVector2D(XOut.Value, YOut.Value);

        // Apply directional preference if specified
        if (!PreferredDirection.IsNearlyZero())
        {
            // Check if movement is against the preferred direction
            const FVector2D Movement = Current - Target;
            const float DotProduct = FVector2D::DotProduct(Movement, PreferredDirection);
            
            // If movement is against the preferred direction, adjust it
            if (DotProduct < 0.0f)
            {
                FVector2D AdjustmentDir = PreferredDirection;
                AdjustmentDir.Normalize();
                
                // Apply directional adjustment
                const float AdjustmentAmount = -DotProduct * 0.5f;
                Current += AdjustmentDir * AdjustmentAmount;
            }
        }
    }
}
