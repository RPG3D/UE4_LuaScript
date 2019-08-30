// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class XQCodeGenerator : ModuleRules
{
	public XQCodeGenerator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

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

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "LevelEditor", "UnrealEd", "FastLuaScript"});

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

    }
}
