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
        int Exist = File.Exists(LuaAPIFilePath) ? 1 : 0;
        string MacroDef = string.Format("LUA_CODE_GENERATED={0}", Exist);
        PublicDefinitions.Add(MacroDef);

        Console.WriteLine(MacroDef);

        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
            }
            ); ;

		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
                "Generated"
            }
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "Engine",
                "UMG",
                "InputCore",
				// ... add other public dependencies that you statically link with here ...
                "LuaSource",

                //project/plugin module to export:
                "UE4_LuaScript"
            }
			);
			
		
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
