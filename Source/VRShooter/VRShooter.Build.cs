// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VRShooter : ModuleRules
{
	public VRShooter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore", "AnimGraphRuntime", "BlueprintGraph", "Niagara", "RigVM", "ControlRig", "AnimGraph", "XRBase" });

        PrivateDependencyModuleNames.AddRange(new string[] { "HeadMountedDisplay" });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
        }

        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "GraphEditor", "PropertyEditor", "EditorStyle", "AnimGraph" });

        PrivateIncludePaths.AddRange(
           new string[] {
              "VRShooter",
             }
        );

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
