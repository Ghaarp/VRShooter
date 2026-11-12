using UnrealBuildTool;

public class VRShooterXRBridge : ModuleRules
{
	public VRShooterXRBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.Add("Core");
		PrivateDependencyModuleNames.AddRange(new[] { "Engine", "HeadMountedDisplay", "OpenXRHMD", "AugmentedReality" });
	}
}
