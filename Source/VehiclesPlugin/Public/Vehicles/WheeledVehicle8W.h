/*
Required for Vehicle 8W Blueprint Setup.
Only overrides movement component.
*/

#pragma once

#include "WheeledVehicle.h"
#include "WheeledVehicle8W.generated.h"

UCLASS()
class VEHICLESPLUGIN_API AWheeledVehicle8W : public AWheeledVehicle
{
	GENERATED_BODY()

public:
	AWheeledVehicle8W(const FObjectInitializer& OI);
};
