// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System;

public class Lua : ModuleRules
{
	public Lua(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
        bEnableUndefinedIdentifierWarnings = false;
		bEnableShadowVariableWarnings = false;

        string LibBaseName = GetType().Name;
        string ArchName = "x64";
        string PlatformName = "windows";
        string CompileMode = "release";

        string LibDir = Path.Combine(ModuleDirectory, "lib");

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));

        if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicAdditionalLibraries.Add(Path.Combine(LibDir, PlatformName, ArchName, CompileMode, LibBaseName + ".lib"));    
            if (File.Exists(Path.Combine(LibDir, PlatformName, ArchName, CompileMode, LibBaseName) + ".dll"))
            {
                PublicDefinitions.Add("LUA_BUILD_AS_DLL=1");

                string RuntimeLoadBin = ModuleDirectory + "/../../../Binaries/Win64/" + LibBaseName + ".dll";
                if (File.Exists(RuntimeLoadBin) == false)
                {
                    File.Copy(Path.Combine(LibDir, PlatformName, ArchName, CompileMode, LibBaseName) + ".dll",
                        RuntimeLoadBin, true);
                }

                RuntimeDependencies.Add(RuntimeLoadBin);
            }

		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
            PlatformName = "macosx";
            ArchName = "x86_64";
            PublicAdditionalLibraries.Add(Path.Combine(LibDir, PlatformName, ArchName, CompileMode, "lib" + LibBaseName + ".a"));
		}
		else if (Target.Platform == UnrealTargetPlatform.IOS|| Target.Platform == UnrealTargetPlatform.TVOS)
		{
            PlatformName = "iphoneos";
            ArchName = "arm64";
            PublicAdditionalLibraries.Add(Path.Combine(LibDir, PlatformName, ArchName, CompileMode, "lib" + LibBaseName + ".a"));
        }
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
            PlatformName = "linux";
            ArchName = "x86_64";
            PublicAdditionalLibraries.Add(Path.Combine(LibDir, PlatformName, ArchName, CompileMode, "lib" + LibBaseName + ".a"));
		}
		else if (Target.Platform == UnrealTargetPlatform.Android)
		{
            PlatformName = "android";
            ArchName = "armv7-a";

            if (File.Exists(Path.Combine(LibDir, PlatformName, ArchName, CompileMode, "lib" + LibBaseName + ".so")))
            {
                AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, LibBaseName + "_APL.xml"));
            }
            else if(File.Exists(Path.Combine(LibDir, PlatformName, ArchName, CompileMode, "lib" + LibBaseName + ".a")))
            {

            }

            PublicLibraryPaths.Add(Path.Combine(LibDir, PlatformName, "armv7-a", CompileMode));
            PublicLibraryPaths.Add(Path.Combine(LibDir, PlatformName, "arm64-v8a", CompileMode));
            PublicAdditionalLibraries.Add(LibBaseName);
        }

    }
}
