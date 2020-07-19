#include "Vehicles/WheeledVehicle8W.h"
#include "Components/WheeledVehicleMovementComponentNW.h"

AWheeledVehicle8W::AWheeledVehicle8W(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWheeledVehicleMovementComponentNW>(AWheeledVehicle::VehicleMovementComponentName))
{
	UWheeledVehicleMovementComponentNW* WheeledMC = Cast<UWheeledVehicleMovementComponentNW>(GetVehicleMovementComponent());
	if (WheeledMC)
	{
		WheeledMC->WheelNum = EWheelNum::WN_Eight;
	}
}