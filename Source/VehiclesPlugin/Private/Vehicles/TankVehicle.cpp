#include "Vehicles/TankVehicle.h"
#include "Components/WheeledVehicleMovementComponentTank.h"

ATankVehicle::ATankVehicle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWheeledVehicleMovementComponentTank>(AWheeledVehicle::VehicleMovementComponentName))
{

}