using UnrealBuildTool;

public class SoccerGame : ModuleRules
{
    public SoccerGame(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "UMG",
            "Slate",
            "SlateCore",
            "PhysicsCore",
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "Niagara",
            "AIModule",
            "NavigationSystem",
            "GameplayTasks"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "OnlineSubsystemSteam"
        });
    }
}
