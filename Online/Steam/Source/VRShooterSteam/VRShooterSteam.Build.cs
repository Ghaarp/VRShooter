using UnrealBuildTool;

public class VRShooterSteam : ModuleRules
{
    public VRShooterSteam(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "OnlineSubsystem" });
        PrivateDependencyModuleNames.Add("OnlineSubsystemUtils");
    }
}
