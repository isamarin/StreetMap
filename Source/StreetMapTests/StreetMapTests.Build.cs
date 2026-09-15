namespace UnrealBuildTool.Rules
{
	public class StreetMapTests : ModuleRules
	{
		public StreetMapTests(ReadOnlyTargetRules Target)
			: base(Target)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"Engine",
					"StreetMapRuntime",
					"StreetMapImporting",
					"XmlParser"
				}
			);

			// StreetMapImporting doesn't use a Public/Private folder split, so its headers (e.g. OSMFile.h)
			// aren't exposed to dependent modules by default; reach into it directly for test-only access.
			PrivateIncludePaths.Add( System.IO.Path.Combine( ModuleDirectory, "../StreetMapImporting" ) );
		}
	}
}
