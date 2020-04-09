// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "FastLuaScript.h"
#include "FastLuaUnrealWrapper.h"

#define LOCTEXT_NAMESPACE "FFastLuaScriptModule"

void FFastLuaScriptModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	DefaultLuaWrapper = FastLuaUnrealWrapper::Create();
	DefaultLuaWrapper->RunMainFunction(FString("DefaultMain"));
}

void FFastLuaScriptModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFastLuaScriptModule, FastLuaScript)

DEFINE_LOG_CATEGORY(LogFastLuaScript);