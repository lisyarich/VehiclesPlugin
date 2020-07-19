#include "Vehicles/WheeledVehicle14W.h"
#include "Components/WheeledVehicleMovementComponentNW.h"

AWheeledVehicle14W::AWheeledVehicle14W(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWheeledVehicleMovementComponentNW>(AWheeledVehicle::VehicleMovementComponentName))
{
	UWheeledVehicleMovementComponentNW* WheeledMC = Cast<UWheeledVehicleMovementComponentNW>(GetVehicleMovementComponent());
	if (WheeledMC)
	{
		WheeledMC->WheelNum = EWheelNum::WN_Fourteen;
	}
}