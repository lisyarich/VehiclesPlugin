/*
Required for Vehicle 14W Blueprint Setup.
Only overrides movement component.
*/
#pragma once

#include "WheeledVehicle.h"
#include "WheeledVehicle14W.generated.h"

UCLASS()
class VEHICLESPLUGIN_API AWheeledVehicle14W : public AWheeledVehicle
{
	GENERATED_BODY()

public:
	AWheeledVehicle14W(const FObjectInitializer& OI);
};