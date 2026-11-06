// Neuron Forge - ENSIM

using UnrealBuildTool;
using System.Collections.Generic;

public class ProjetNeuroEditorTarget : TargetRules
{
	public ProjetNeuroEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.Add("ProjetNeuro");
	}
}
