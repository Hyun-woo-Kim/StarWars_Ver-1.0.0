using UnrealBuildTool;
using System.IO;

public class meshy : ModuleRules
{
	public meshy(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Projects",
				"Json",
				"Sockets",
				"Networking",
				"HTTP",
				"PakFile",
				"SlateCore",
				"Slate",
				"UMG"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"AssetTools",
				"AssetRegistry",
				"LevelEditor",
				"EditorStyle",
				"ToolMenus",
				"ApplicationCore"
			}
		);

		// zlib is available in UE 5.1 at 1.2.12 but minizip/unzip.h is not included.
		// ZIP extraction is disabled; the fallback path in ExtractZipFile() will log an error.
		PublicDefinitions.Add("MESHY_MINIZIP_SUPPORTED=0");

		string ZlibIncludePath = Path.Combine(EngineDirectory, "Source", "ThirdParty", "zlib", "1.2.12", "include");
		if (Directory.Exists(ZlibIncludePath))
		{
			PublicSystemIncludePaths.Add(ZlibIncludePath);
			PublicDefinitions.Add("HAVE_ZLIB=1");
		}
	}
}
