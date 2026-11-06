// Neuron Forge - ENSIM

using UnrealBuildTool;

public class ProjetNeuro : ModuleRules
{
	public ProjetNeuro(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput"
		});

		PublicIncludePaths.Add(ModuleDirectory);
	}
}
