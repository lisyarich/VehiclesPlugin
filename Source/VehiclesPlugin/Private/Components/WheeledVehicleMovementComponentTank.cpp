#include "Components/WheeledVehicleMovementComponentTank.h"
#include "PhysXIncludes.h"
#include "PhysicsPublic.h"  
#include "Components/PrimitiveComponent.h"
#include "PhysXPublic.h"
#include "Runtime/Engine/Private/PhysicsEngine/PhysXSupport.h"
#include "GameFramework/Pawn.h"

UWheeledVehicleMovementComponentTank::UWheeledVehicleMovementComponentTank()
{
	// grab default values from physx
	PxVehicleEngineData DefEngineData;
	EngineSetup.MOI = DefEngineData.mMOI;
	EngineSetup.MaxRPM = OmegaToRPM(DefEngineData.mMaxOmega);
	EngineSetup.DampingRateFullThrottle = DefEngineData.mDampingRateFullThrottle;
	EngineSetup.DampingRateZeroThrottleClutchEngaged = DefEngineData.mDampingRateZeroThrottleClutchEngaged;
	EngineSetup.DampingRateZeroThrottleClutchDisengaged = DefEngineData.mDampingRateZeroThrottleClutchDisengaged;

	// Convert from PhysX curve to Unreal Engine's
	FRichCurve* TorqueCurveData = EngineSetup.TorqueCurve.GetRichCurve();
	for (PxU32 KeyIdx = 0; KeyIdx < DefEngineData.mTorqueCurve.getNbDataPairs(); KeyIdx++)
	{
		float Input = DefEngineData.mTorqueCurve.getX(KeyIdx) * EngineSetup.MaxRPM;
		float Output = DefEngineData.mTorqueCurve.getY(KeyIdx) * DefEngineData.mPeakTorque;
		TorqueCurveData->AddKey(Input, Output);
	}

	PxVehicleClutchData DefClutchData;
	TransmissionSetup.ClutchStrength = DefClutchData.mStrength;

	PxVehicleAckermannGeometryData DefAckermannSetup;
	AckermannAccuracy = DefAckermannSetup.mAccuracy;

	PxVehicleGearsData DefGearSetup;
	TransmissionSetup.GearSwitchTime = DefGearSetup.mSwitchTime;
	TransmissionSetup.ReverseGearRatio = DefGearSetup.mRatios[PxVehicleGearsData::eREVERSE];
	TransmissionSetup.FinalRatio = DefGearSetup.mFinalRatio;

	PxVehicleAutoBoxData DefAutoBoxSetup;
	TransmissionSetup.NeutralGearUpRatio = DefAutoBoxSetup.mUpRatios[PxVehicleGearsData::eNEUTRAL];
	TransmissionSetup.GearAutoBoxLatency = DefAutoBoxSetup.getLatency();
	TransmissionSetup.bUseGearAutoBox = true;

	for (uint32 i = PxVehicleGearsData::eFIRST; i < DefGearSetup.mNbRatios; i++)
	{
		FTankGearData GearData;
		GearData.DownRatio = DefAutoBoxSetup.mDownRatios[i];
		GearData.UpRatio = DefAutoBoxSetup.mUpRatios[i];
		GearData.Ratio = DefGearSetup.mRatios[i];
		TransmissionSetup.ForwardGears.Add(GearData);
	}

	// Init steering speed curve
	FRichCurve* SteeringCurveData = SteeringCurve.GetRichCurve();
	SteeringCurveData->AddKey(0.f, 1.f);
	SteeringCurveData->AddKey(20.f, 0.9f);
	SteeringCurveData->AddKey(60.f, 0.8f);
	SteeringCurveData->AddKey(120.f, 0.7f);

	// Initialize WheelSetups array with selected number of wheels
	WheelSetups.SetNum(NumRollers);

	//Setup Default Values for Input Rate
	IdleBrakeInputLeft = 0.0f;
	IdleBrakeInputRight = 0.0f;
	BrakeLeftRate.RiseRate = 6.0f;
	BrakeLeftRate.FallRate = 10.0f;
	BrakeRightRate.RiseRate = 6.0f;
	BrakeRightRate.FallRate = 10.0f;
	SteeringLeftRate.RiseRate = 2.5f;
	SteeringLeftRate.FallRate = 5.0f;
	SteeringRightRate.RiseRate = 2.5f;
	SteeringRightRate.FallRate = 5.0f;

}

void UWheeledVehicleMovementComponentTank::UpdateTankState(float DeltaTime)
{
	// update input values
	APawn* MyOwner = UpdatedComponent ? Cast<APawn>(UpdatedComponent->GetOwner()) : NULL;
	if (MyOwner && MyOwner->IsLocallyControlled())
	{
		//Manual shifting between reverse and first gear
		if (FMath::Abs(GetForwardSpeed()) < WrongDirectionThreshold) //we only shift between reverse and first if the car is slow enough. This isn't 100% correct since we really only care about engine speed, but good enough
		{
			if ((RawThrottleInput < 0.f && GetCurrentGear() >= 0 && GetTargetGear() >= 0) || (RawSteeringLeftInput < 0.f && GetCurrentGear() >= 0 && GetTargetGear() >= 0) || (RawSteeringRightInput < 0.f && GetCurrentGear() >= 0 && GetTargetGear() >= 0))
			{
				SetTargetGear(-1, true);
			}
			else if ((RawThrottleInput > 0.f && GetCurrentGear() <= 0 && GetTargetGear() <= 0) || (RawSteeringLeftInput > 0.f && GetCurrentGear() <= 0 && GetTargetGear() <= 0) || (RawSteeringRightInput > 0.f && GetCurrentGear() <= 0 && GetTargetGear() <= 0))
			{
				SetTargetGear(1, true);
			}
		}

		if (bUseRVOAvoidance)
		{
			CalculateAvoidanceVelocity(DeltaTime);
			UpdateAvoidance(DeltaTime);
		}

		SteeringLeftInput = SteeringLeftRate.InterpInputValue(DeltaTime, SteeringLeftInput, CalcLeftSteeringInput());
		SteeringRightInput = SteeringRightRate.InterpInputValue(DeltaTime, SteeringRightInput, CalcRightSteeringInput());
		BrakeLeftInput = BrakeLeftRate.InterpInputValue(DeltaTime, BrakeLeftInput, CalcLeftBrakeInput());
		BrakeRightInput = BrakeRightRate.InterpInputValue(DeltaTime, BrakeRightInput, CalcRightBrakeInput());
		// and send to server
		ServerUpdateState(SteeringInput, ThrottleInput, BrakeInput, HandbrakeInput, GetCurrentGear());
		ServerUpdateTankState(SteeringLeftInput, SteeringRightInput, BrakeLeftInput, BrakeRightInput);
	}
	else
	{
		// use replicated values for remote pawns
		SteeringLeftInput = RepTankState.SteeringInputLeft;
		SteeringRightInput = RepTankState.SteeringInputRight;
		ThrottleInput = ReplicatedState.ThrottleInput;
		BrakeLeftInput = RepTankState.BrakeInputLeft;
		BrakeRightInput = RepTankState.BrakeInputRight;
		SetTargetGear(ReplicatedState.CurrentGear, true);
	}
}

bool UWheeledVehicleMovementComponentTank::ServerUpdateTankState_Validate(float InSteeringInputLeft, float InSteeringInputRight, float InBrakeInputLeft, float InBrakeInputRight)
{
	return true;
}

// Update movement variables on server side.
void UWheeledVehicleMovementComponentTank::ServerUpdateTankState_Implementation(float InSteeringInputLeft, float InSteeringInputRight, float InBrakeInputLeft, float InBrakeInputRight)
{
	SteeringLeftInput = InSteeringInputLeft;
	SteeringRightInput = InSteeringInputRight;
	BrakeLeftInput = InBrakeInputLeft;
	BrakeRightInput = InBrakeInputRight;

	// update state of inputs
	RepTankState.SteeringInputLeft = InSteeringInputLeft;
	RepTankState.SteeringInputRight = InSteeringInputRight;
	RepTankState.BrakeInputLeft = InBrakeInputLeft;
	RepTankState.BrakeInputRight = InBrakeInputRight;
}

// Capture throttle/steering input for left caterpillar.
void UWheeledVehicleMovementComponentTank::SetSteeringLeftInput(float SteeringLeft)
{
	RawSteeringLeftInput = FMath::Clamp(SteeringLeft, -1.0f, 1.0f);
}

// Capture throttle/steering input for right caterpillar.
void UWheeledVehicleMovementComponentTank::SetSteeringRightInput(float SteeringRight)
{
	RawSteeringRightInput = FMath::Clamp(SteeringRight, -1.0f, 1.0f);
}

// Capture brake input for left caterpillar.
void UWheeledVehicleMovementComponentTank::SetBrakeLeftInput(float bNewBreakLeft)
{
	bRawBrakeLeftInput = bNewBreakLeft;
}

// Capture brake input for right caterpillar.
void UWheeledVehicleMovementComponentTank::SetBrakeRightInput(float bNewBreakRight)
{
	bRawBrakeRightInput = bNewBreakRight;
}

/* This Calculates Brake Input For Left Caterpillar */
float UWheeledVehicleMovementComponentTank::CalcLeftBrakeInput()
{
	const float ForwardSpeed = GetForwardSpeed();

	float NewBrakeInput = 0.0f;
	// if player wants to move forwards...
	if (RawThrottleInput > 0.f)
	{
		// if vehicle is moving backwards, then press brake
		if (ForwardSpeed < -WrongDirectionThreshold)
		{
			NewBrakeInput = 1.0f;
		}
	}

	// if player wants to move backwards...
	else if (RawThrottleInput < 0.f)
	{
		// if vehicle is moving forwards, then press brake
		if (ForwardSpeed > WrongDirectionThreshold)
		{
			NewBrakeInput = 1.0f;	// Seems a bit severe to have 0 or 1 braking. Better control can be had by allowing continuous brake input values
		}
	}

	// if player isn't pressing forward or backwards...
	else
	{
		if (ForwardSpeed < StopThreshold && ForwardSpeed > -StopThreshold) //auto break 
		{
			NewBrakeInput = 1.f;
		}
		else
		{
			NewBrakeInput = IdleBrakeInputLeft;
		}
	}

	return FMath::Clamp<float>(NewBrakeInput, 0.0, 1.0);
}

/* This Calculates Brake Input For Right Caterpillar */
float UWheeledVehicleMovementComponentTank::CalcRightBrakeInput()
{
	const float ForwardSpeed = GetForwardSpeed();

	float NewBrakeInput = 0.0f;

	// if player wants to move forwards...
	if (RawThrottleInput > 0.f)
	{
		// if vehicle is moving backwards, then press brake
		if (ForwardSpeed < -WrongDirectionThreshold)
		{
			NewBrakeInput = 1.0f;
		}
	}

	// if player wants to move backwards...
	else if (RawThrottleInput < 0.f)
	{
		// if vehicle is moving forwards, then press brake
		if (ForwardSpeed > WrongDirectionThreshold)
		{
			// Seems a bit severe to have 0 or 1 braking. Better control can be had by allowing continuous brake input values
			NewBrakeInput = 1.0f;
		}
	}

	// if player isn't pressing forward or backwards...
	else
	{
		if (ForwardSpeed < StopThreshold && ForwardSpeed > -StopThreshold) //auto break 
		{
			NewBrakeInput = 1.f;
		}
		else
		{
			NewBrakeInput = IdleBrakeInputRight;
		}
	}

	return FMath::Clamp<float>(NewBrakeInput, 0.0, 1.0);
}

/* This Calculates Throttle/Steering Input For Right Caterpillar */
float UWheeledVehicleMovementComponentTank::CalcRightSteeringInput()
{
	if (bUseRVOAvoidance)
	{
		const float AvoidanceSpeedSq = AvoidanceVelocity.SizeSquared();
		const float DesiredSpeedSq = GetVelocityForRVOConsideration().SizeSquared();

		if (AvoidanceSpeedSq > DesiredSpeedSq)
		{
			RawSteeringRightInput = FMath::Clamp(RawSteeringRightInput + RVOThrottleStep, -1.0f, 1.0f);
		}
		else if (AvoidanceSpeedSq < DesiredSpeedSq)
		{
			RawSteeringRightInput = FMath::Clamp(RawSteeringRightInput - RVOThrottleStep, -1.0f, 1.0f);
		}
	}

	//If the user is changing direction we should really be braking first and not applying any gas, so wait until they've changed gears
	if ((RawSteeringRightInput > 0.f && GetTargetGear() < 0) || (RawSteeringRightInput < 0.f && GetTargetGear() > 0))
	{
		return 0.f;
	}

	return FMath::Abs(RawSteeringRightInput);
}

/* This Calculates Throttle/Steering Input For Left Caterpillar */
float UWheeledVehicleMovementComponentTank::CalcLeftSteeringInput()
{
	if (bUseRVOAvoidance)
	{
		const float AvoidanceSpeedSq = AvoidanceVelocity.SizeSquared();
		const float DesiredSpeedSq = GetVelocityForRVOConsideration().SizeSquared();

		if (AvoidanceSpeedSq > DesiredSpeedSq)
		{
			RawSteeringLeftInput = FMath::Clamp(RawSteeringLeftInput + RVOThrottleStep, -1.0f, 1.0f);
		}
		else if (AvoidanceSpeedSq < DesiredSpeedSq)
		{
			RawSteeringLeftInput = FMath::Clamp(RawSteeringLeftInput - RVOThrottleStep, -1.0f, 1.0f);
		}
	}

	//If the user is changing direction we should really be braking first and not applying any gas, so wait until they've changed gears
	if ((RawSteeringLeftInput > 0.f && GetTargetGear() < 0) || (RawSteeringLeftInput < 0.f && GetTargetGear() > 0))
	{
		return 0.f;
	}

	return FMath::Abs(RawSteeringLeftInput);
}

void UWheeledVehicleMovementComponentTank::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//	DOREPLIFETIME(UWheeledVehicleMovementComponentTank, RepTankState);
}

#if WITH_EDITOR
void UWheeledVehicleMovementComponentTank::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == TEXT("DownRatio"))
	{
		for (int32 GearIdx = 0; GearIdx < TransmissionSetup.ForwardGears.Num(); ++GearIdx)
		{
			FTankGearData & GearData = TransmissionSetup.ForwardGears[GearIdx];
			GearData.DownRatio = FMath::Min(GearData.DownRatio, GearData.UpRatio);
		}
	}
	else if (PropertyName == TEXT("UpRatio"))
	{
		for (int32 GearIdx = 0; GearIdx < TransmissionSetup.ForwardGears.Num(); ++GearIdx)
		{
			FTankGearData & GearData = TransmissionSetup.ForwardGears[GearIdx];
			GearData.UpRatio = FMath::Max(GearData.DownRatio, GearData.UpRatio);
		}
	}
	else if (PropertyName == TEXT("SteeringCurve"))
	{
		TArray<FRichCurveKey> SteerKeys = SteeringCurve.GetRichCurve()->GetCopyOfKeys();
		for (int32 KeyIdx = 0; KeyIdx < SteerKeys.Num(); ++KeyIdx)
		{
			float NewValue = FMath::Clamp(SteerKeys[KeyIdx].Value, 0.f, 1.f);
			SteeringCurve.GetRichCurve()->UpdateOrAddKey(SteerKeys[KeyIdx].Time, NewValue);
		}
	}
}

#endif

float FTankEngineData::FindPeakTorque() const
{
	// Find max torque
	float PeakTorque = 0.f;
	TArray<FRichCurveKey> TorqueKeys = TorqueCurve.GetRichCurveConst()->GetCopyOfKeys();
	for (int32 KeyIdx = 0; KeyIdx < TorqueKeys.Num(); KeyIdx++)
	{
		FRichCurveKey& Key = TorqueKeys[KeyIdx];
		PeakTorque = FMath::Max(PeakTorque, Key.Value);
	}
	return PeakTorque;
}

static void GetVehicleEngineSetup(const FTankEngineData& Setup, PxVehicleEngineData& PxSetup)
{
	PxSetup.mMOI = M2ToCm2(Setup.MOI);
	PxSetup.mMaxOmega = RPMToOmega(Setup.MaxRPM);
	PxSetup.mDampingRateFullThrottle = M2ToCm2(Setup.DampingRateFullThrottle);
	PxSetup.mDampingRateZeroThrottleClutchEngaged = M2ToCm2(Setup.DampingRateZeroThrottleClutchEngaged);
	PxSetup.mDampingRateZeroThrottleClutchDisengaged = M2ToCm2(Setup.DampingRateZeroThrottleClutchDisengaged);

	float PeakTorque = Setup.FindPeakTorque(); // In Nm
	PxSetup.mPeakTorque = M2ToCm2(PeakTorque); // convert Nm to (kg cm^2/s^2)

											   // Convert from our curve to PhysX
	PxSetup.mTorqueCurve.clear();
	TArray<FRichCurveKey> TorqueKeys = Setup.TorqueCurve.GetRichCurveConst()->GetCopyOfKeys();
	int32 NumTorqueCurveKeys = FMath::Min<int32>(TorqueKeys.Num(), PxVehicleEngineData::eMAX_NB_ENGINE_TORQUE_CURVE_ENTRIES);
	for (int32 KeyIdx = 0; KeyIdx < NumTorqueCurveKeys; KeyIdx++)
	{
		FRichCurveKey& Key = TorqueKeys[KeyIdx];
		PxSetup.mTorqueCurve.addPair(FMath::Clamp(Key.Time / Setup.MaxRPM, 0.f, 1.f), Key.Value / PeakTorque); // Normalize torque to 0-1 range
	}
}

//Setups vehicle gear.
static void GetVehicleGearSetup(const FTankTransmissionData& Setup, PxVehicleGearsData& PxSetup)
{
	PxSetup.mSwitchTime = Setup.GearSwitchTime;
	PxSetup.mRatios[PxVehicleGearsData::eREVERSE] = Setup.ReverseGearRatio;
	for (int32 i = 0; i < Setup.ForwardGears.Num(); i++)
	{
		PxSetup.mRatios[i + PxVehicleGearsData::eFIRST] = Setup.ForwardGears[i].Ratio;
	}
	PxSetup.mFinalRatio = Setup.FinalRatio;
	PxSetup.mNbRatios = Setup.ForwardGears.Num() + PxVehicleGearsData::eFIRST;
}

static void GetVehicleAutoBoxSetup(const FTankTransmissionData& Setup, PxVehicleAutoBoxData& PxSetup)
{
	for (int32 i = 0; i < Setup.ForwardGears.Num(); i++)
	{
		const FTankGearData& GearData = Setup.ForwardGears[i];
		PxSetup.mUpRatios[i] = GearData.UpRatio;
		PxSetup.mDownRatios[i] = GearData.DownRatio;
	}
	PxSetup.mUpRatios[PxVehicleGearsData::eNEUTRAL] = Setup.NeutralGearUpRatio;
	PxSetup.setLatency(Setup.GearAutoBoxLatency);
}

void SetupDriveHelper(const UWheeledVehicleMovementComponentTank* VehicleData, const PxVehicleWheelsSimData* PWheelsSimData, PxVehicleDriveSimData& DriveData)
{
	PxVehicleEngineData EngineSetup;
	GetVehicleEngineSetup(VehicleData->EngineSetup, EngineSetup);
	DriveData.setEngineData(EngineSetup);

	PxVehicleClutchData ClutchSetup;
	ClutchSetup.mStrength = M2ToCm2(VehicleData->TransmissionSetup.ClutchStrength);
	DriveData.setClutchData(ClutchSetup);


	PxVehicleGearsData GearSetup;
	GetVehicleGearSetup(VehicleData->TransmissionSetup, GearSetup);
	DriveData.setGearsData(GearSetup);

	PxVehicleAutoBoxData AutoBoxSetup;
	GetVehicleAutoBoxSetup(VehicleData->TransmissionSetup, AutoBoxSetup);
	DriveData.setAutoBoxData(AutoBoxSetup);

}

void UWheeledVehicleMovementComponentTank::PreTick(float DeltaTime)
{
	Super::PreTick(DeltaTime);

	if (PVehicle && UpdatedComponent)
	{
		APawn* MyOwner = Cast<APawn>(UpdatedComponent->GetOwner());
		if (MyOwner)
		{
			UpdateTankState(DeltaTime);
		}
	}
}

void UWheeledVehicleMovementComponentTank::SetupVehicle()
{
	// Setup the chassis and wheel shapes
	SetupVehicleShapes();

	// Setup mass properties
	SetupVehicleMass();

	// Setup the wheels
	PxVehicleWheelsSimData* PWheelsSimData = PxVehicleWheelsSimData::allocate(NumRollers);
	SetupWheels(PWheelsSimData);

	// Setup drive data
	PxVehicleDriveSimData DriveData;
	SetupDriveHelper(this, PWheelsSimData, DriveData);

	PxVehicleDriveTank* PVehicleDriveTank = PxVehicleDriveTank::allocate(NumRollers);
	check(PVehicleDriveTank);

	FPhysicsCommand::ExecuteWrite(UpdatedPrimitive->GetBodyInstance()->ActorHandle, [&](const FPhysicsActorHandle& Actor)
	{
#if WITH_CHAOS || WITH_IMMEDIATE_PHYSX
		PxRigidActor* PRigidActor = nullptr;
#else
		PxRigidActor* PRigidActor = Actor.SyncActor;
#endif
		if (PRigidActor)
		{
			if (PxRigidDynamic* PRigidDynamic = PRigidActor->is<PxRigidDynamic>())
			{
				PVehicleDriveTank->setup(GPhysXSDK, PRigidDynamic, *PWheelsSimData, DriveData, 0);
				PVehicleDriveTank->setToRestState();

				//Cleanup
				PWheelsSimData->free();
			}
		}
	});
	// cache values
	PVehicle = PVehicleDriveTank;
	PVehicleDrive = PVehicleDriveTank;
}

void UWheeledVehicleMovementComponentTank::UpdateSimulation(float DeltaTime)
{
	if (PVehicleDrive == NULL)
		return;

	PxVehicleDriveTankRawInputData RawInputData(PxVehicleDriveTankControlModel::eSTANDARD);
	RawInputData.setAnalogAccel(ThrottleInput);
	RawInputData.setAnalogLeftThrust(SteeringLeftInput);
	RawInputData.setAnalogRightThrust(SteeringRightInput);
	RawInputData.setAnalogLeftBrake(BrakeLeftInput);
	RawInputData.setAnalogRightBrake(BrakeRightInput);


	if (!PVehicleDrive->mDriveDynData.getUseAutoGears())
	{
		RawInputData.setGearUp(bRawGearUpInput);
		RawInputData.setGearDown(bRawGearDownInput);
	}

	// Convert from our curve to PxFixedSizeLookupTable
	PxFixedSizeLookupTable<8> SpeedSteerLookup;
	TArray<FRichCurveKey> SteerKeys = SteeringCurve.GetRichCurve()->GetCopyOfKeys();
	const int32 MaxSteeringSamples = FMath::Min(8, SteerKeys.Num());
	for (int32 KeyIdx = 0; KeyIdx < MaxSteeringSamples; KeyIdx++)
	{
		FRichCurveKey& Key = SteerKeys[KeyIdx];
		SpeedSteerLookup.addPair(KmHToCmS(Key.Time), FMath::Clamp(Key.Value, 0.f, 1.f));
	}

	PxVehiclePadSmoothingData SmoothData = {
		{ ThrottleInputRate.RiseRate, BrakeInputRate.RiseRate, HandbrakeInputRate.RiseRate, SteeringInputRate.RiseRate, SteeringInputRate.RiseRate, SteeringLeftRate.RiseRate, SteeringRightRate.RiseRate, BrakeLeftRate.RiseRate, BrakeRightRate.RiseRate },
		{ ThrottleInputRate.FallRate, BrakeInputRate.FallRate, HandbrakeInputRate.FallRate, SteeringInputRate.FallRate, SteeringInputRate.FallRate, SteeringLeftRate.FallRate, SteeringRightRate.FallRate, BrakeLeftRate.FallRate, BrakeRightRate.FallRate }
	};

	PxVehicleDriveTank* PVehicleDriveTank = (PxVehicleDriveTank*)PVehicleDrive;
	PxVehicleDriveTankSmoothAnalogRawInputsAndSetAnalogInputs(SmoothData, RawInputData, DeltaTime, *PVehicleDriveTank);

}

// Updates engine setup on spawn.
void UWheeledVehicleMovementComponentTank::UpdateEngineSetup(const FTankEngineData& NewEngineSetup)
{
	if (PVehicleDrive)
	{
		PxVehicleEngineData EngineData;
		GetVehicleEngineSetup(NewEngineSetup, EngineData);

		PxVehicleDriveTank* PVehicleDriveTank = (PxVehicleDriveTank*)PVehicleDrive;
		PVehicleDriveTank->mDriveSimData.setEngineData(EngineData);
	}
}

// Updates transmisson setup on spawn.
void UWheeledVehicleMovementComponentTank::UpdateTransmissionSetup(const FTankTransmissionData& NewTransmissionSetup)
{
	if (PVehicleDrive)
	{
		PxVehicleGearsData GearData;
		GetVehicleGearSetup(NewTransmissionSetup, GearData);

		PxVehicleAutoBoxData AutoBoxData;
		GetVehicleAutoBoxSetup(NewTransmissionSetup, AutoBoxData);

		PxVehicleDriveTank* PVehicleDriveTank = (PxVehicleDriveTank*)PVehicleDrive;
		PVehicleDriveTank->mDriveSimData.setGearsData(GearData);
		PVehicleDriveTank->mDriveSimData.setAutoBoxData(AutoBoxData);
	}
}

void BackwardsConvertCm2ToM2Tank(float& val, float defaultValue)
{
	if (val != defaultValue)
	{
		val = Cm2ToM2(val);
	}
}

void UWheeledVehicleMovementComponentTank::Serialize(FArchive & Ar)
{
	Super::Serialize(Ar);
#if WITH_PHYSX
	if (Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_VEHICLES_UNIT_CHANGE)
	{
		PxVehicleEngineData DefEngineData;
		float DefaultRPM = OmegaToRPM(DefEngineData.mMaxOmega);

		EngineSetup.MaxRPM = EngineSetup.MaxRPM != DefaultRPM ? OmegaToRPM(EngineSetup.MaxRPM) : DefaultRPM;
	}

	if (Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_VEHICLES_UNIT_CHANGE2)
	{
		PxVehicleEngineData DefEngineData;
		PxVehicleClutchData DefClutchData;

		BackwardsConvertCm2ToM2Tank(EngineSetup.DampingRateFullThrottle, DefEngineData.mDampingRateFullThrottle);
		BackwardsConvertCm2ToM2Tank(EngineSetup.DampingRateZeroThrottleClutchDisengaged, DefEngineData.mDampingRateZeroThrottleClutchDisengaged);
		BackwardsConvertCm2ToM2Tank(EngineSetup.DampingRateZeroThrottleClutchEngaged, DefEngineData.mDampingRateZeroThrottleClutchEngaged);
		BackwardsConvertCm2ToM2Tank(EngineSetup.MOI, DefEngineData.mMOI);
		BackwardsConvertCm2ToM2Tank(TransmissionSetup.ClutchStrength, DefClutchData.mStrength);
	}
#endif
}

void UWheeledVehicleMovementComponentTank::ComputeConstants()
{
	Super::ComputeConstants();
	MaxEngineRPM = EngineSetup.MaxRPM;
}