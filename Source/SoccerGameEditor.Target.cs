using UnrealBuildTool;

public class SoccerGameEditorTarget : TargetRules
{
    public SoccerGameEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("SoccerGame");
    }
}
