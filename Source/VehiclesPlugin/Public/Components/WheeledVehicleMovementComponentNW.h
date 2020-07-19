#pragma once

#include "CoreMinimal.h"
#include "WheeledVehicleMovementComponent4W.h"
#include "WheeledVehicleMovementComponentNW.generated.h"

UENUM(BlueprintType)
enum class EWheelNum : uint8
{
	WN_Six		UMETA(DisplayName = "6"),
	WN_Eight	UMETA(DisplayName = "8"),
	WN_Ten		UMETA(DisplayName = "10"),
	WN_Twelve	UMETA(DisplayName = "12"),
	WN_Fourteen	UMETA(DisplayName = "14"),
	WN_Sixteen	UMETA(DisplayName = "16"),
};

UCLASS()
class VEHICLESPLUGIN_API UWheeledVehicleMovementComponentNW : public UWheeledVehicleMovementComponent4W
{
	GENERATED_BODY()

#if WITH_EDITOR
		virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyCangedEvent) override;
#endif

public:

	UPROPERTY(EditDefaultsOnly, Category = Setup)
	EWheelNum WheelNum;

	UWheeledVehicleMovementComponentNW();

protected:

	int32 ChoosenWheelNum;

	//Setup All vehicle Components.
	virtual void SetupVehicle() override;

	//Tick Vehicle Physics.
	virtual void UpdateSimulation(float DeltaTime) override;

	//Setup engine properties on spawn.
	void UpdateEngineSetup(const FVehicleEngineData& NewEngineSetup);
	//Setup transmission propersties on spawn.
	void UpdateTransmissionSetup(const FVehicleTransmissionData& NewGearSetup);

};