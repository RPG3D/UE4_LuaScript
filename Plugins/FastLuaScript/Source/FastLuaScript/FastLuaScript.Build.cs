// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class FastLuaScript : ModuleRules
{
	public FastLuaScript(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        string LuaAPIFilePath = Path.Combine(ModuleDirectory, "Generated/FastLuaAPI.h");
        int LuaCodeGenerated = File.Exists(LuaAPIFilePath) ? 1 : 0;
        string MacroDef = string.Format("LUA_CODE_GENERATED={0}", LuaCodeGenerated);
        PublicDefinitions.Add(MacroDef);

        Console.WriteLine(MacroDef);

        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
            }
            ); ;

        if(LuaCodeGenerated == 1)
        {
            PrivateIncludePaths.Add("Generated");
        }

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "Engine",
                "UMG",
                "InputCore",
                "ClothingSystemRuntimeInterface",
				// ... add other public dependencies that you statically link with here ...
                "Lua"
            }
			);

        //project/plugin module to export:
        string ConfigFilePath = Path.Combine(ModuleDirectory, "../../Config/ModuleToExport.txt");
        if(File.Exists(ConfigFilePath))
        {
            StreamReader Stream = File.OpenText(ConfigFilePath);
            string ModuleToExport = Stream.ReadToEnd();
            string[] ModuleList = ModuleToExport.Split(new char[2] {'\n','\r'}, StringSplitOptions.RemoveEmptyEntries);
            for(int i = 0; i < ModuleList.Length; ++i)
            {
                PublicDependencyModuleNames.Add(ModuleList[i]);
            }
        }
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...
            }
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
