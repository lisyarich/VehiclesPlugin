// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class VehiclesPlugin : ModuleRules
	{
		public VehiclesPlugin(ReadOnlyTargetRules Target) : base(Target)
		{
            PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

            SetupModulePhysicsSupport(Target);

            PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
				}
				);

            PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
                    "Engine"
					// ... add other public dependencies that you statically link with here ...
				}
				);

            if (Target.bCompilePhysX)
            {
                // Not ideal but as this module publicly exposes PhysX types
                // to other modules when PhysX is enabled it requires that its
                // public files have access to PhysX includes
                PublicDependencyModuleNames.Add("PhysX");
                PublicDependencyModuleNames.Add("PhysXVehicles");
                PublicDependencyModuleNames.Add("PhysXVehicleLib");
            }

            if (Target.Platform != UnrealTargetPlatform.Android && Target.Platform != UnrealTargetPlatform.IOS && Target.Platform != UnrealTargetPlatform.Mac)
            {
                PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                     "APEX"
                }
                );
            }

            PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add private dependencies that you statically link with here ...
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);
		}
	}
}