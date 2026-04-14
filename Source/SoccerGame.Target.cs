using UnrealBuildTool;

public class SoccerGameTarget : TargetRules
{
    public SoccerGameTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("SoccerGame");
    }
}
