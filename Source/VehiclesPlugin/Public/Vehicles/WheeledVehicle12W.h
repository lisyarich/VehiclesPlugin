/*
Required for Vehicle 12W Blueprint Setup.
Only overrides movement component.
*/
#pragma once

#include "WheeledVehicle.h"
#include "WheeledVehicle12W.generated.h"

UCLASS()
class VEHICLESPLUGIN_API AWheeledVehicle12W : public AWheeledVehicle
{
	GENERATED_BODY()

public:
	AWheeledVehicle12W(const FObjectInitializer& OI);
};