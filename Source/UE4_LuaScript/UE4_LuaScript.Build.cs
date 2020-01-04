// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;

public class UE4_LuaScript : ModuleRules
{
	public UE4_LuaScript(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        if (Directory.Exists(ModuleDirectory + "/../../Binaries/Win64") == false)
        {
            Directory.CreateDirectory(ModuleDirectory + "/../../Binaries/Win64");
        }

        string LuaAPIFilePath = Path.Combine(ModuleDirectory, "GeneratedLua/FastLuaAPI.h");
        int LuaCodeGenerated = File.Exists(LuaAPIFilePath) ? 1 : 0;
        string MacroDef = string.Format("LUA_CODE_GENERATED={0}", LuaCodeGenerated);
        PublicDefinitions.Add(MacroDef);
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", 
			"InputCore", "Lua", "FastLuaScript",
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"ClothingSystemRuntimeInterface",
			"UMG"
		});

		// Uncomment if you are using Slate UI
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
