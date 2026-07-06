
using UnrealBuildTool;

public class InteractionSystemEditor : ModuleRules
{
	public InteractionSystemEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"KismetCompiler", 
			"UnrealEd", 
			"InteractionSystem",
			"StructUtilsEditor",
			"SlateCore",
			"Slate",
			"PropertyEditor",
			"PropertyBindingUtils",
			"PropertyBindingUtilsEditor"
		});
	}
}
