#include "Vehicles/WheeledVehicleNW.h"
#include "Components/WheeledVehicleMovementComponentNW.h"

AWheeledVehicleNW::AWheeledVehicleNW(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWheeledVehicleMovementComponentNW>(AWheeledVehicle::VehicleMovementComponentName))
{

}