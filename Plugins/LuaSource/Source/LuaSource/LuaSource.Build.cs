// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class LuaSource : ModuleRules
{
	public LuaSource(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnableUndefinedIdentifierWarnings = false;

        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
            );

        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
            }
            );

        PublicDependencyModuleNames.AddRange(new string[] { "Core" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PrivateDefinitions.Add("LUA_BUILD_AS_DLL");
        }
        else
        {
            PrivateDefinitions.Add("LUA_USE_POSIX");
        }
    }
}
