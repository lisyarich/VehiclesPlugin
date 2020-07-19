/*
Required for Vehicle 10W Blueprint Setup.
Only overrides movement component.
*/
#pragma once

#include "WheeledVehicle.h"
#include "WheeledVehicle10W.generated.h"

UCLASS()
class VEHICLESPLUGIN_API AWheeledVehicle10W : public AWheeledVehicle
{
	GENERATED_BODY()

public:
	AWheeledVehicle10W(const FObjectInitializer& OI);
};