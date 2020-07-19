// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "PhysXSupportCore.h"
#if WITH_PHYSX
#include "PhysXPublicCore.h"
#endif

#ifdef WITH_APEX
#undef WITH_APEX
#endif
#define WITH_APEX 0


/**
* The public interface to this module
*/
class IVehiclesPlugin : public IModuleInterface
{

public:

	/**
	* Singleton-like access to this module's interface.  This is just for convenience!
	* Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	*
	* @return Returns singleton instance, loading the module on demand if needed
	*/
	static inline IVehiclesPlugin& Get()
	{
		return FModuleManager::LoadModuleChecked< IVehiclesPlugin >("VehiclesPlugin");
	}

	/**
	* Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	*
	* @return True if the module is loaded and ready to use
	*/
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("VehiclesPlugin");
	}
};

