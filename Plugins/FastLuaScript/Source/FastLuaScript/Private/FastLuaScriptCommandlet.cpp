// Fill out your copyright notice in the Description page of Project Settings.


#include "FastLuaScriptCommandlet.h"
#include "FileHelper.h"
#include "FastLuaHelper.h"
#include "FastLuaUnrealWrapper.h"

int32 UFastLuaScriptCommandlet::Main(const FString& Params)
{

	FastLuaUnrealWrapper Inst;
	Inst.InitConfig();
	return Inst.GeneratedCode();
}
