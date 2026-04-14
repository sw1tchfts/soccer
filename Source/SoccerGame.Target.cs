using UnrealBuildTool;

public class SoccerGameTarget : TargetRules
{
    public SoccerGameTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
        ExtraModuleNames.Add("SoccerGame");
    }
}
