/*
Required for Vehicle 16W Blueprint Setup.
Only overrides movement component.
*/
#pragma once

#include "WheeledVehicle.h"
#include "WheeledVehicle16W.generated.h"

UCLASS()
class VEHICLESPLUGIN_API AWheeledVehicle16W : public AWheeledVehicle
{
	GENERATED_BODY()

public:
	AWheeledVehicle16W(const FObjectInitializer& OI);
};