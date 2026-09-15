namespace UnrealBuildTool.Rules
{
	public class StreetMapRuntime : ModuleRules
	{
        public StreetMapRuntime(ReadOnlyTargetRules Target)
			: base(Target)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] {
                    "Core",
					"CoreUObject",
					"Engine",
					"RHI",
					"RenderCore",
                    "NavigationSystem"
                }
			);

			if (Target.bBuildEditor)
			{
				// Needed by the WITH_EDITOR-only code in StreetMapComponent.cpp (PostEditChangeProperty)
				PrivateDependencyModuleNames.Add("PropertyEditor");
			}
		}
	}
}