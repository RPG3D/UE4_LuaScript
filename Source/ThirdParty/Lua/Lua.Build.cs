// Fill out your copyright notice in the Description page of Project Settings.

using System;
using System.IO;
using UnrealBuildTool;
using System.Diagnostics;

public class Lua : ModuleRules
{
    public Lua(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        //bUseRTTI = true;

        ShadowVariableWarningLevel = WarningLevel.Warning;
        bEnableUndefinedIdentifierWarnings = false;

        bEnforceIWYU = false;
        bEnableExceptions = false;

        
        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core", "CoreUObject", "Engine",
        });


        PrivateDependencyModuleNames.AddRange(new string[] { });


        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
			PublicDefinitions.Add("LUA_BUILD_AS_DLL=1");
        }

        
        if(Target.Type == TargetType.Editor)
        {

        }

        if (Target.Platform == UnrealTargetPlatform.Android)
        {

        }
    }

}
