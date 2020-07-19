#include "Vehicles/WheeledVehicle12W.h"
#include "Components/WheeledVehicleMovementComponentNW.h"

AWheeledVehicle12W::AWheeledVehicle12W(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWheeledVehicleMovementComponentNW>(AWheeledVehicle::VehicleMovementComponentName))
{
	UWheeledVehicleMovementComponentNW* WheeledMC = Cast<UWheeledVehicleMovementComponentNW>(GetVehicleMovementComponent());
	if (WheeledMC)
	{
		WheeledMC->WheelNum = EWheelNum::WN_Twelve;
	}
}