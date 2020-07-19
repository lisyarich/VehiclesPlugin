#include "Vehicles/WheeledVehicle10W.h"
#include "Components/WheeledVehicleMovementComponentNW.h"

AWheeledVehicle10W::AWheeledVehicle10W(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWheeledVehicleMovementComponentNW>(AWheeledVehicle::VehicleMovementComponentName))
{
	UWheeledVehicleMovementComponentNW* WheeledMC = Cast<UWheeledVehicleMovementComponentNW>(GetVehicleMovementComponent());
	if (WheeledMC)
	{
		WheeledMC->WheelNum = EWheelNum::WN_Ten;
	}
}