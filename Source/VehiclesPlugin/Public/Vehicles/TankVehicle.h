/*
Required for Tank Vehicle Blueprint Setup.
Only overrides movement component.
*/
#pragma once

#include "WheeledVehicle.h"
#include "TankVehicle.generated.h"

UCLASS()
class VEHICLESPLUGIN_API ATankVehicle : public AWheeledVehicle
{
	GENERATED_BODY()

public:
	ATankVehicle(const FObjectInitializer& OI);
	
};
