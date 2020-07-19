#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Net/UnrealNetwork.h"
#include "WheeledVehicleMovementComponent4W.h"
#include "WheeledVehicleMovementComponentTank.generated.h"

/*
Struct of variables that replicates movement.
*/
USTRUCT()
struct FRepTankState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float SteeringInputLeft;

	UPROPERTY()
	float SteeringInputRight;

	UPROPERTY()
	float BrakeInputLeft;

	UPROPERTY()
	float BrakeInputRight;
};

/*
Some Parts of code are duplicating UWheeledVehicleMovementComponent4W.
This was made because UWheeledVehicleMovementComponentTank is not a children of UWheeledVehicleMovementComponent4W.
*/
USTRUCT()
struct FTankEngineData
{
	GENERATED_USTRUCT_BODY()

	/** Torque (Nm) at a given RPM*/
	UPROPERTY(EditAnywhere, Category = Setup)
	FRuntimeFloatCurve TorqueCurve;

	/** Maximum revolutions per minute of the engine */
	UPROPERTY(EditAnywhere, Category = Setup, meta = (ClampMin = "0.01", UIMin = "0.01"))
	float MaxRPM;

	/** Moment of inertia of the engine around the axis of rotation (Kgm^2). */
	UPROPERTY(EditAnywhere, Category = Setup, meta = (ClampMin = "0.01", UIMin = "0.01"))
	float MOI;

	/** Damping rate of engine when full throttle is applied (Kgm^2/s) */
	UPROPERTY(EditAnywhere, Category = Setup, AdvancedDisplay, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DampingRateFullThrottle;

	/** Damping rate of engine in at zero throttle when the clutch is engaged (Kgm^2/s)*/
	UPROPERTY(EditAnywhere, Category = Setup, AdvancedDisplay, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DampingRateZeroThrottleClutchEngaged;

	/** Damping rate of engine in at zero throttle when the clutch is disengaged (in neutral gear) (Kgm^2/s)*/
	UPROPERTY(EditAnywhere, Category = Setup, AdvancedDisplay, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DampingRateZeroThrottleClutchDisengaged;

	/** Find the peak torque produced by the TorqueCurve */
	float FindPeakTorque() const;
};

USTRUCT()
struct FTankGearData
{
	GENERATED_USTRUCT_BODY()
	/** Determines the amount of torque multiplication*/
	UPROPERTY(EditAnywhere, Category = Setup)
	float Ratio;
	/** Value of engineRevs/maxEngineRevs that is low enough to gear down*/
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"), Category = Setup)
	float DownRatio;
	/** Value of engineRevs/maxEngineRevs that is high enough to gear up*/
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"), Category = Setup)
	float UpRatio;
};

USTRUCT()
struct FTankTransmissionData
{
	GENERATED_USTRUCT_BODY()
	/** Whether to use automatic transmission */
	UPROPERTY(EditAnywhere, Category = TankSetup, meta = (DisplayName = "Authomatic Transmission"))
	bool bUseGearAutoBox;
	/** Time it takes to switch gears (seconds) */
	UPROPERTY(EditAnywhere, Category = Setup, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float GearSwitchTime;
	/** Minimum time it takes the automatic transmission to initiate a gear change (seconds)*/
	UPROPERTY(EditAnywhere, Category = Setup, meta = (editcondition = "bUseGearAutoBox", ClampMin = "0.0", UIMin = "0.0"))
	float GearAutoBoxLatency;
	/** The final gear ratio multiplies the transmission gear ratios.*/
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Setup)
	float FinalRatio;
	/** Forward gear ratios (up to 30) */
	UPROPERTY(EditAnywhere, Category = Setup, AdvancedDisplay)
	TArray<FTankGearData> ForwardGears;
	/** Reverse gear ratio */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Setup)
	float ReverseGearRatio;
	/** Value of engineRevs/maxEngineRevs that is high enough to increment gear*/
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Setup, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float NeutralGearUpRatio;
	/** Strength of clutch (Kgm^2/s)*/
	UPROPERTY(EditAnywhere, Category = Setup, AdvancedDisplay, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ClutchStrength;
};

UCLASS()
class VEHICLESPLUGIN_API UWheeledVehicleMovementComponentTank : public UWheeledVehicleMovementComponent
{
	GENERATED_BODY()

public:
	UWheeledVehicleMovementComponentTank();

	/** Read current state for simulation */
	void UpdateTankState(float DeltaTime);

	/** Engine */
	UPROPERTY(EditAnywhere, Category = MechanicalSetup)
	FTankEngineData EngineSetup;

	/** Differential */
	UPROPERTY(EditAnywhere, Category = MechanicalSetup)
	FTankTransmissionData TransmissionSetup;

	/** Transmission data */
	UPROPERTY(EditAnywhere, Category = SteeringSetup)
	FRuntimeFloatCurve SteeringCurve;

	UFUNCTION(reliable, server, WithValidation)
	void ServerUpdateTankState(float InSteeringInputLeft, float InSteeringInputRight, float InBrakeInputLeft, float InBrakeInputRight);

	/** Calls Torque On Left Caterpillar*/
	UFUNCTION(BlueprintCallable, Category = "Game|Components|TankMovement")
	void SetSteeringLeftInput(float SteeringLeft);

	/** Calls Torque On Right Caterpillar*/
	UFUNCTION(BlueprintCallable, Category = "Game|Components|TankMovement")
	void SetSteeringRightInput(float SteeringRight);

	/** Calls Break On Left Caterpillar*/
	UFUNCTION(BlueprintCallable, Category = "Game|Components|TankMovement")
	void SetBrakeRightInput(float bNewBrakeRight);

	/** Calls Break On Right Caterpillar*/
	UFUNCTION(BlueprintCallable, Category = "Game|Components|TankMovement")
	void SetBrakeLeftInput(float bNewBrakeLeft);

	/** Accuracy of Ackermann steer calculation (range: 0..1) */
	UPROPERTY(EditAnywhere, Category = SteeringSetup, AdvancedDisplay, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float AckermannAccuracy;

	virtual void Serialize(FArchive & Ar) override;
	virtual void ComputeConstants() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:

	//Setup All vehicle Components.
	virtual void SetupVehicle() override;
	//Tick Tank Physics.
	virtual void UpdateSimulation(float DeltaTime) override;

	virtual void PreTick(float DeltaTime) override;

	//Setup engine properties on spawn.
	void UpdateEngineSetup(const FTankEngineData& NewEngineSetup);
	//Setup transmission propersties on spawn.
	void UpdateTransmissionSetup(const FTankTransmissionData& NewGearSetup);

protected:
	//Left throttle caterpillar input.
	UPROPERTY(Transient)
	float SteeringLeftInput;

	//Right throttle caterpillar input.
	UPROPERTY(Transient)
	float SteeringRightInput;

	//Left brake caterpillar input.
	UPROPERTY(Transient)
	float BrakeLeftInput;

	//Right brake caterpillar input.
	UPROPERTY(Transient)
	float BrakeRightInput;

	// What the player has the steering set to. Range -1...1
	UPROPERTY(Transient)
	float RawSteeringLeftInput;

	// What the player has the steering set to. Range -1...1
	UPROPERTY(Transient)
	float RawSteeringRightInput;

	// True if the player is holding the brake.
	UPROPERTY(Transient)
	float bRawBrakeRightInput;

	// True if the player is holding the brake.
	UPROPERTY(Transient)
	float bRawBrakeLeftInput;

	// How much to press the brake when the player has release throttle
	UPROPERTY(EditAnywhere, Category = VehicleInput)
	float IdleBrakeInputLeft;

	// How much to press the brake when the player has release throttle
	UPROPERTY(EditAnywhere, Category = VehicleInput)
	float IdleBrakeInputRight;

	/** Compute steering input */
	float CalcLeftBrakeInput();

	/** Compute steering input */
	float CalcRightBrakeInput();

	/** Compute brake input */
	float CalcLeftSteeringInput();

	/** Compute brake input */
	float CalcRightSteeringInput();

	// Rate at which input for left caterpillar can rise and fall
	UPROPERTY(EditAnywhere, Category = VehicleInput, AdvancedDisplay)
	FVehicleInputRate SteeringLeftRate;

	// Rate at which input for right caterpillar can rise and fall
	UPROPERTY(EditAnywhere, Category = VehicleInput, AdvancedDisplay)
	FVehicleInputRate SteeringRightRate;

	// Rate at which input brake for left caterpillar can rise and fall
	UPROPERTY(EditAnywhere, Category = VehicleInput, AdvancedDisplay)
	FVehicleInputRate BrakeLeftRate;

	// Rate at which input brake can for right caterpillar rise and fall
	UPROPERTY(EditAnywhere, Category = VehicleInput, AdvancedDisplay)
	FVehicleInputRate BrakeRightRate;

	/** Replicates state of tank input */
	UPROPERTY(Transient, Replicated)
	FRepTankState RepTankState;

public:

	/** Used For Number Of Wheel Bones **/
	UPROPERTY(EditAnywhere, Category = TankSetup)
	int32 NumRollers;

};