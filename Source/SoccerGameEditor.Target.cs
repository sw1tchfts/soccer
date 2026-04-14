using UnrealBuildTool;

public class SoccerGameEditorTarget : TargetRules
{
    public SoccerGameEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
        ExtraModuleNames.Add("SoccerGame");
    }
}
