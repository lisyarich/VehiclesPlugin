/*
Required for Vehicle 6W Blueprint Setup.
Only overrides movement component.
*/

#pragma once

#include "WheeledVehicle.h"
#include "WheeledVehicle6W.generated.h"

UCLASS()
class VEHICLESPLUGIN_API AWheeledVehicle6W : public AWheeledVehicle
{
	GENERATED_BODY()

public:
	AWheeledVehicle6W(const FObjectInitializer& OI);
};