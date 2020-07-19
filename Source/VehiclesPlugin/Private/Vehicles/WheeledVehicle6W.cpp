#include "Vehicles/WheeledVehicle6W.h"
#include "Components/WheeledVehicleMovementComponentNW.h"

AWheeledVehicle6W::AWheeledVehicle6W(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWheeledVehicleMovementComponentNW>(AWheeledVehicle::VehicleMovementComponentName))
{
	UWheeledVehicleMovementComponentNW* WheeledMC = Cast<UWheeledVehicleMovementComponentNW>(GetVehicleMovementComponent());
	if (WheeledMC)
	{
		WheeledMC->WheelNum = EWheelNum::WN_Eight;
	}
}