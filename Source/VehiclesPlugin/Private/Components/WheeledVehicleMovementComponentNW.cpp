#include "Components/WheeledVehicleMovementComponentNW.h"
#include "PhysXIncludes.h"
#include "PhysicsPublic.h"  
#include "Components/PrimitiveComponent.h"
#include "PhysXPublic.h"
#include "Runtime/Engine/Private/PhysicsEngine/PhysXSupport.h"

UWheeledVehicleMovementComponentNW::UWheeledVehicleMovementComponentNW()
{
	// grab default values from physx
	PxVehicleDifferentialNWData DefDifferentialSetup;

	PxVehicleEngineData DefEngineData;
	EngineSetup.MOI = DefEngineData.mMOI;
	EngineSetup.MaxRPM = OmegaToRPM(DefEngineData.mMaxOmega);
	EngineSetup.DampingRateFullThrottle = DefEngineData.mDampingRateFullThrottle;
	EngineSetup.DampingRateZeroThrottleClutchEngaged = DefEngineData.mDampingRateZeroThrottleClutchEngaged;
	EngineSetup.DampingRateZeroThrottleClutchDisengaged = DefEngineData.mDampingRateZeroThrottleClutchDisengaged;

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
	// Convert from PhysX curve to Unreal Engine's
	for (uint32 i = PxVehicleGearsData::eFIRST; i < DefGearSetup.mNbRatios; i++)
	{
		FVehicleGearData GearData;
		GearData.DownRatio = DefAutoBoxSetup.mDownRatios[i];
		GearData.UpRatio = DefAutoBoxSetup.mUpRatios[i];
		GearData.Ratio = DefGearSetup.mRatios[i];
		TransmissionSetup.ForwardGears.Add(GearData);
	}

	WheelNum = EWheelNum::WN_Six;
	switch (WheelNum)
	{
	case EWheelNum::WN_Six:
		ChoosenWheelNum = 6;
		break;
	case EWheelNum::WN_Eight:
		ChoosenWheelNum = 8;
		break;
	case EWheelNum::WN_Ten:
		ChoosenWheelNum = 10;
		break;
	case EWheelNum::WN_Twelve:
		ChoosenWheelNum = 12;
		break;
	case EWheelNum::WN_Fourteen:
		ChoosenWheelNum = 14;
		break;
	case EWheelNum::WN_Sixteen:
		ChoosenWheelNum = 16;
		break;
	default:
		ChoosenWheelNum = 6;
		break;
	}

	WheelSetups.SetNum(ChoosenWheelNum);

	// Init steering speed curve
	FRichCurve* SteeringCurveData = SteeringCurve.GetRichCurve();
	SteeringCurveData->AddKey(0.f, 1.f);
	SteeringCurveData->AddKey(20.f, 0.9f);
	SteeringCurveData->AddKey(60.f, 0.8f);
	SteeringCurveData->AddKey(120.f, 0.7f);
	// Initialize WheelSetups array with 16 wheels
	
}

#if WITH_EDITOR
void UWheeledVehicleMovementComponentNW::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == TEXT("DownRatio"))
	{
		for (int32 GearIdx = 0; GearIdx < TransmissionSetup.ForwardGears.Num(); ++GearIdx)
		{
			FVehicleGearData & GearData = TransmissionSetup.ForwardGears[GearIdx];
			GearData.DownRatio = FMath::Min(GearData.DownRatio, GearData.UpRatio);
		}
	}
	else if (PropertyName == TEXT("UpRatio"))
	{
		for (int32 GearIdx = 0; GearIdx < TransmissionSetup.ForwardGears.Num(); ++GearIdx)
		{
			FVehicleGearData & GearData = TransmissionSetup.ForwardGears[GearIdx];
			GearData.UpRatio = FMath::Max(GearData.DownRatio, GearData.UpRatio);
		}
	}
	else if (PropertyName == TEXT("SteeringCurve"))
	{
		//make sure values are capped between 0 and 1
		TArray<FRichCurveKey> SteerKeys = SteeringCurve.GetRichCurve()->GetCopyOfKeys();
		for (int32 KeyIdx = 0; KeyIdx < SteerKeys.Num(); ++KeyIdx)
		{
			float NewValue = FMath::Clamp(SteerKeys[KeyIdx].Value, 0.f, 1.f);
			SteeringCurve.GetRichCurve()->UpdateOrAddKey(SteerKeys[KeyIdx].Time, NewValue);
		}
	}
	else if (PropertyName == TEXT("WheelNum"))
	{
		switch (WheelNum)
		{
		case EWheelNum::WN_Six:
			ChoosenWheelNum = 6;
			break;
		case EWheelNum::WN_Eight:
			ChoosenWheelNum = 8;
			break;
		case EWheelNum::WN_Ten:
			ChoosenWheelNum = 10;
			break;
		case EWheelNum::WN_Twelve:
			ChoosenWheelNum = 12;
			break;
		case EWheelNum::WN_Fourteen:
			ChoosenWheelNum = 14;
			break;
		case EWheelNum::WN_Sixteen:
			ChoosenWheelNum = 16;
			break;
		default:
			ChoosenWheelNum = 6;
			break;
		}

		WheelSetups.SetNum(ChoosenWheelNum);
	}
}

#endif

static void GetVehicleNWEngineSetup(const FVehicleEngineData& Setup, PxVehicleEngineData& PxSetup)
{
	PxSetup.mMOI = M2ToCm2(Setup.MOI);
	PxSetup.mMaxOmega = RPMToOmega(Setup.MaxRPM);
	PxSetup.mDampingRateFullThrottle = M2ToCm2(Setup.DampingRateFullThrottle);
	PxSetup.mDampingRateZeroThrottleClutchEngaged = M2ToCm2(Setup.DampingRateZeroThrottleClutchEngaged);
	PxSetup.mDampingRateZeroThrottleClutchDisengaged = M2ToCm2(Setup.DampingRateZeroThrottleClutchDisengaged);
	// Find max torque
	float PeakTorque = 0.f;
	TArray<FRichCurveKey> TorqueKeys = Setup.TorqueCurve.GetRichCurveConst()->GetCopyOfKeys();
	for (int32 KeyIdx = 0; KeyIdx < TorqueKeys.Num(); KeyIdx++)
	{
		FRichCurveKey& Key = TorqueKeys[KeyIdx];
		PeakTorque = FMath::Max(PeakTorque, Key.Value);
	}

	PxSetup.mPeakTorque = M2ToCm2(PeakTorque);// convert Nm to (kg cm^2/s^2)

	PxSetup.mTorqueCurve.clear();
	int32 NumTorqueCurveKeys = FMath::Min<int32>(TorqueKeys.Num(), PxVehicleEngineData::eMAX_NB_ENGINE_TORQUE_CURVE_ENTRIES);
	for (int32 KeyIdx = 0; KeyIdx < NumTorqueCurveKeys; KeyIdx++)
	{
		FRichCurveKey& Key = TorqueKeys[KeyIdx];
		PxSetup.mTorqueCurve.addPair(FMath::Clamp(Key.Time / Setup.MaxRPM, 0.f, 1.f), Key.Value / PeakTorque);
	}
}

static void GetVehicleNWGearSetup(const FVehicleTransmissionData& Setup, PxVehicleGearsData& PxSetup)
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

static void GetVehicleNWAutoBoxSetup(const FVehicleTransmissionData& Setup, PxVehicleAutoBoxData& PxSetup)
{
	for (int32 i = 0; i < Setup.ForwardGears.Num(); i++)
	{
		const FVehicleGearData& GearData = Setup.ForwardGears[i];
		PxSetup.mUpRatios[i] = GearData.UpRatio;
		PxSetup.mDownRatios[i] = GearData.DownRatio;
	}
	PxSetup.mUpRatios[PxVehicleGearsData::eNEUTRAL] = Setup.NeutralGearUpRatio;
	PxSetup.setLatency(Setup.GearAutoBoxLatency);
}
void SetupDriveHelper(const UWheeledVehicleMovementComponentNW* VehicleData, const PxVehicleWheelsSimData* PWheelsSimData, PxVehicleDriveSimDataNW& DriveData)
{

	PxVehicleEngineData EngineSetup;
	GetVehicleNWEngineSetup(VehicleData->EngineSetup, EngineSetup);
	DriveData.setEngineData(EngineSetup);

	PxVehicleClutchData ClutchSetup;
	ClutchSetup.mStrength = M2ToCm2(VehicleData->TransmissionSetup.ClutchStrength);
	DriveData.setClutchData(ClutchSetup);

	PxVehicleGearsData GearSetup;
	GetVehicleNWGearSetup(VehicleData->TransmissionSetup, GearSetup);
	DriveData.setGearsData(GearSetup);

	PxVehicleAutoBoxData AutoBoxSetup;
	GetVehicleNWAutoBoxSetup(VehicleData->TransmissionSetup, AutoBoxSetup);
	DriveData.setAutoBoxData(AutoBoxSetup);

}

static void CreateVehicleSimulationNW(PxVehicleWheelsSimData& VehNWSimData, int32 NumWheels, PxVehicleDriveSimDataNW& DriveSimData16W)
{
	PxVehicleWheelsSimData* PWheelsSimData = PxVehicleWheelsSimData::allocate(4);
	PxVehicleDifferentialNWData DifData;

	//Copy front wheels bones to apply throttle and make rest of the wheels physical
	VehNWSimData.copy(*PWheelsSimData, 0, 0);
	VehNWSimData.copy(*PWheelsSimData, 1, 1);
	VehNWSimData.copy(*PWheelsSimData, 2, 2);
	VehNWSimData.copy(*PWheelsSimData, 3, 3);

	// Add wheels to diferretial
	DifData.setDrivenWheel(0, true);
	DifData.setDrivenWheel(1, true);
	DifData.setDrivenWheel(2, true);
	DifData.setDrivenWheel(3, true);

	if (NumWheels >= 6)
	{
		VehNWSimData.copy(*PWheelsSimData, 0, 4);
		VehNWSimData.copy(*PWheelsSimData, 1, 5);
		DifData.setDrivenWheel(4, true);
		DifData.setDrivenWheel(5, true);
	}
	else if (NumWheels >= 8)
	{
		VehNWSimData.copy(*PWheelsSimData, 0, 6);
		VehNWSimData.copy(*PWheelsSimData, 1, 7);
		DifData.setDrivenWheel(6, true);
		DifData.setDrivenWheel(7, true);
	}
	else if (NumWheels >= 10)
	{
		VehNWSimData.copy(*PWheelsSimData, 0, 8);
		VehNWSimData.copy(*PWheelsSimData, 1, 9);
		DifData.setDrivenWheel(8, true);
		DifData.setDrivenWheel(9, true);
	}
	else if (NumWheels >= 12)
	{
		VehNWSimData.copy(*PWheelsSimData, 0, 10);
		VehNWSimData.copy(*PWheelsSimData, 1, 11);
		DifData.setDrivenWheel(10, true);
		DifData.setDrivenWheel(11, true);
	}
	else if (NumWheels >= 14)
	{
		VehNWSimData.copy(*PWheelsSimData, 0, 12);
		VehNWSimData.copy(*PWheelsSimData, 1, 13);
		DifData.setDrivenWheel(12, true);
		DifData.setDrivenWheel(13, true);
	}
	else if (NumWheels >= 16)
	{
		VehNWSimData.copy(*PWheelsSimData, 0, 14);
		VehNWSimData.copy(*PWheelsSimData, 1, 15);
		DifData.setDrivenWheel(14, true);
		DifData.setDrivenWheel(15, true);
	}

	DriveSimData16W.setDiffData(DifData);
	VehNWSimData.setTireLoadFilterData(PWheelsSimData->getTireLoadFilterData());
}

void UWheeledVehicleMovementComponentNW::SetupVehicle()
{
	for (int32 WheelIdx = 0; WheelIdx < WheelSetups.Num(); ++WheelIdx)
	{
		const FWheelSetup& WheelSetup = WheelSetups[WheelIdx];
		if (WheelSetup.BoneName == NAME_None)
		{
			return;
		}
	}
	// Setup the chassis and wheel shapes
	SetupVehicleShapes();

	// Setup mass properties
	SetupVehicleMass();

	// Setup the wheels
	PxVehicleWheelsSimData* PWheelsSimData = PxVehicleWheelsSimData::allocate(ChoosenWheelNum);

	// Setup drive data
	PxVehicleDriveSimDataNW DriveData;
	CreateVehicleSimulationNW(*PWheelsSimData, ChoosenWheelNum, DriveData);

	SetupWheels(PWheelsSimData);
	SetupDriveHelper(this, PWheelsSimData, DriveData);

	PxVehicleDriveNW* PVehicleDriveNW = PxVehicleDriveNW::allocate(ChoosenWheelNum);
	check(PVehicleDriveNW);

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
				PVehicleDriveNW->setup(GPhysXSDK, PRigidDynamic, *PWheelsSimData, DriveData, 0);
				PVehicleDriveNW->setToRestState();

				//Cleanup
				PWheelsSimData->free();
			}
		}
	});
	// cache values
	PVehicle = PVehicleDriveNW;
	PVehicleDrive = PVehicleDriveNW;

	SetUseAutoGears(TransmissionSetup.bUseGearAutoBox);
}

void UWheeledVehicleMovementComponentNW::UpdateSimulation(float DeltaTime)
{
	if (PVehicleDrive == NULL)
		return;

	PxVehicleDriveNWRawInputData RawInputData;
	RawInputData.setAnalogAccel(ThrottleInput);
	RawInputData.setAnalogSteer(SteeringInput);
	RawInputData.setAnalogBrake(BrakeInput);
	RawInputData.setAnalogHandbrake(HandbrakeInput);

	if (!PVehicleDrive->mDriveDynData.getUseAutoGears())
	{
		RawInputData.setGearUp(bRawGearUpInput);
		RawInputData.setGearDown(bRawGearDownInput);
	}
	// Convert from our curve to PxFixedSizeLookupTable
	PxFixedSizeLookupTable<8> SpeedSteerLookup;
	TArray<FRichCurveKey> SteerKeys = SteeringCurve.GetRichCurve()->GetCopyOfKeys();
	const int32 MaxSteeringSamples = FMath::Min(ChoosenWheelNum, SteerKeys.Num());
	for (int32 KeyIdx = 0; KeyIdx < MaxSteeringSamples; KeyIdx++)
	{
		FRichCurveKey& Key = SteerKeys[KeyIdx];
		SpeedSteerLookup.addPair(KmHToCmS(Key.Time), FMath::Clamp(Key.Value, 0.f, 1.0f));
	}

	PxVehiclePadSmoothingData SmoothData = {
		{ ThrottleInputRate.RiseRate, BrakeInputRate.RiseRate, HandbrakeInputRate.RiseRate, SteeringInputRate.RiseRate, SteeringInputRate.RiseRate },
	{ ThrottleInputRate.FallRate, BrakeInputRate.FallRate, HandbrakeInputRate.FallRate, SteeringInputRate.FallRate, SteeringInputRate.FallRate }
	};

	PxVehicleDriveNW* PVehicleDriveNW = (PxVehicleDriveNW*)PVehicleDrive;
	PxVehicleDriveNWSmoothAnalogRawInputsAndSetAnalogInputs(SmoothData, SpeedSteerLookup, RawInputData, DeltaTime, false, *PVehicleDriveNW);
}

// Updates engine setup on spawn.
void UWheeledVehicleMovementComponentNW::UpdateEngineSetup(const FVehicleEngineData& NewEngineSetup)
{
	if (PVehicleDrive)
	{
		PxVehicleEngineData EngineData;
		GetVehicleNWEngineSetup(NewEngineSetup, EngineData);

		PxVehicleDriveNW* PVehicleDriveNW = (PxVehicleDriveNW*)PVehicleDrive;
		PVehicleDriveNW->mDriveSimData.setEngineData(EngineData);
	}
}

// Updates transmisson setup on spawn.
void UWheeledVehicleMovementComponentNW::UpdateTransmissionSetup(const FVehicleTransmissionData& NewTransmissionSetup)
{
	if (PVehicleDrive)
	{
		PxVehicleGearsData GearData;
		GetVehicleNWGearSetup(NewTransmissionSetup, GearData);

		PxVehicleAutoBoxData AutoBoxData;
		GetVehicleNWAutoBoxSetup(NewTransmissionSetup, AutoBoxData);

		PxVehicleDriveNW* PVehicleDriveNW = (PxVehicleDriveNW*)PVehicleDrive;
		PVehicleDriveNW->mDriveSimData.setGearsData(GearData);
		PVehicleDriveNW->mDriveSimData.setAutoBoxData(AutoBoxData);
	}
}

