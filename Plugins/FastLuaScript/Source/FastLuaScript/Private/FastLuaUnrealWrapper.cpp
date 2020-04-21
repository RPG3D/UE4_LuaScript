﻿// Fill out your copyright notice in the Description page of Project Settings.

#include "FastLuaUnrealWrapper.h"
#include "lua/lua.hpp"
#include "Engine/GameInstance.h"
#include "UObject/UObjectIterator.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "UObject/StructOnScope.h"
#include "FastLuaHelper.h"
#include "FastLuaStat.h"

//#include "LuaGeneratedEnum.h"
//#include "LuaGeneratedStruct.h"
//#include "LuaGeneratedClass.h"

#include "LuaDelegateWrapper.h"
#include "LuaObjectWrapper.h"
#include "LuaStructWrapper.h"
#include "LuaLatentActionWrapper.h"


FastLuaUnrealWrapper::FOnLuaUnrealReset FastLuaUnrealWrapper::OnLuaUnrealReset;

TMap<UObject*, FastLuaUnrealWrapper*> FastLuaUnrealWrapper::LuaStateMap;

static int StopNewIndex(lua_State* InL)
{
	UE_LOG(LogTemp, Warning, TEXT("The lua table _G['Unreal'] is read only!"));
	return 0;
}

static int InitUnrealLib(lua_State* InL)
{
	const static luaL_Reg funcs[] =
	{

		{"LuaLoadObject", FLuaObjectWrapper::LuaLoadObject},
		{"LuaLoadClass", FLuaObjectWrapper::LuaLoadClass},
		{"LuaGetUnrealCDO", FLuaObjectWrapper::LuaGetUnrealCDO},
		{"LuaNewObject", FLuaObjectWrapper::LuaNewObject},
		{"LuaNewStruct", FLuaStructWrapper::LuaNewStruct},

		{"LuaNewDelegate", FLuaDelegateWrapper::LuaNewDelegate},

		{"LuaHookUFunction", FastLuaHelper::HookUFunction},

		/*{"LuaGenerateEnum", ULuaGeneratedEnum::GenerateEnum},
		{"LuaGenerateStruct", ULuaGeneratedStruct::GenerateStruct},
		{"LuaGenerateClass", ULuaGeneratedClass::GenerateClass},*/

		{"PrintLog", FastLuaHelper::PrintLog},

		{nullptr, nullptr}
	};

	luaL_newlib(InL, funcs);
	{
		lua_newtable(InL);
		{
			lua_pushcfunction(InL, StopNewIndex);
			lua_setfield(InL, -2, "__newindex");
		}

		lua_setmetatable(InL, -2);
	}

	return 1;
}


static int RequireFromUFS(lua_State* InL)
{
	const char* RawFilwName = lua_tostring(InL, -1);
	FString FileName = UTF8_TO_TCHAR(RawFilwName);

	FileName.ReplaceInline(*FString("."), *FString("/"));


	lua_getglobal(InL, "package");
	lua_getfield(InL, -1, "path");
	FString LuaSearchPath = UTF8_TO_TCHAR(lua_tostring(InL, -1));
	TArray<FString> PathList;
	LuaSearchPath.ParseIntoArray(PathList, TEXT(";"));

	for (int32 i = 0; i < PathList.Num(); ++i)
	{
		FString FullFilePath = PathList[i].Replace(TEXT("?"), *FileName);
		FullFilePath = FullFilePath.Replace(TEXT("\\"), TEXT("/"));

		FPaths::NormalizeFilename(FullFilePath);

		IPlatformFile* PlatformFile = nullptr;
		IFileHandle* LuaFile = nullptr;

		PlatformFile = FPlatformFileManager::Get().GetPlatformFile().GetLowerLevel();
		LuaFile = PlatformFile ? PlatformFile->OpenRead(*FullFilePath) : nullptr;

		if (PlatformFile == nullptr || LuaFile == nullptr)
		{
			PlatformFile = &(FPlatformFileManager::Get().GetPlatformFile());
			LuaFile = PlatformFile->OpenRead(*FullFilePath);
		}
		
		if (LuaFile)
		{
			TArray<uint8> FileData;
			FileData.Init(0, LuaFile->Size());
			if (FileData.Num() < 1)
			{
				continue;
			}
			LuaFile->Read(FileData.GetData(), FileData.Num());

			int32 BomLen = 0;
			if (FileData.Num() > 2 &&
				FileData[0] == 0xEF &&
				FileData[1] == 0xBB &&
				FileData[2] == 0xBF)
			{
				BomLen = 3;
			}

			FString RetPath = FString("@") + FullFilePath;
			int ret = luaL_loadbuffer(InL, (const char*)FileData.GetData() + BomLen, FileData.Num() - BomLen, TCHAR_TO_UTF8(*RetPath));
			//return full file path as 2nd value, useful for some debug tool 
			lua_pushstring(InL, TCHAR_TO_UTF8(*FullFilePath));
			if (ret != LUA_OK)
			{
				UE_LOG(LogTemp, Warning, TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(InL, -1)));
			}

			delete LuaFile;
			return 2;
		}
	}

	FString MessageToPush = FString("RequireFromUFS failed: ") + FileName;
	luaL_traceback(InL, InL, TCHAR_TO_UTF8(*MessageToPush), 1);

	UE_LOG(LogTemp, Warning, TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(InL, -1)));
	return 0;
}


FastLuaUnrealWrapper::FastLuaUnrealWrapper()
{
	static int32 TmpIdx = 0;
	InstanceIndex = TmpIdx++;
}

FastLuaUnrealWrapper::~FastLuaUnrealWrapper()
{
	Reset();
}

TSharedPtr<FastLuaUnrealWrapper> FastLuaUnrealWrapper::Create(class UGameInstance* InGameInstance, const FString& InName)
{
	TSharedPtr<FastLuaUnrealWrapper> Inst(new FastLuaUnrealWrapper());
	if (InName.Len())
	{
		Inst->LuaStateName = InName;
	}

	Inst->Init(InGameInstance);

	if (InGameInstance)
	{
		Inst->CachedGameInstance = InGameInstance;
		LuaStateMap.Add(InGameInstance, Inst.Get());
	}

	return Inst;
}

void FastLuaUnrealWrapper::Init(UGameInstance* InGameInstance)
{
	if (L != nullptr)
	{
		return;
	}

	L = luaL_newstate();
	luaL_openlibs(L);

	FLuaObjectWrapper::PushObject(L, InGameInstance);
	lua_setglobal(L, "GameInstance");

	luaL_requiref(L, "Unreal", InitUnrealLib, 1);

	FLuaObjectWrapper::InitWrapperMetatable(L);
	FLuaStructWrapper::InitWrapperMetatable(L);
	FLuaDelegateWrapper::InitWrapperMetatable(L);

	//set package path
	{
		FString LuaEnvPath;

		LuaEnvPath += FPaths::ProjectDir() / FString("LuaScript/?.lua") + FString(";") + FPaths::ProjectDir() / FString("LuaScript/?/Init.lua;");
		LuaEnvPath += FPaths::ProjectContentDir() / FString("LuaScript/?.lua") + FString(";") + FPaths::ProjectContentDir() / FString("LuaScript/?/Init.lua;");
		
		lua_getglobal(L, "package");
		FString RetString = UTF8_TO_TCHAR(lua_pushstring(L, TCHAR_TO_UTF8(*LuaEnvPath)));
		lua_setfield(L, -2, "path");
		lua_pop(L, 1);
	}
	//set searcher
	{
		int32 tp = lua_gettop(L);
		int32 ret = lua_getglobal(L, "package");
		lua_getfield(L, -1, "searchers");
		int32 FunctionNum = luaL_len(L, -1);
		lua_pushcfunction(L, RequireFromUFS);
		lua_rawseti(L, -2, FunctionNum + 1);
		lua_settop(L, tp);
	}
}

void FastLuaUnrealWrapper::Reset()
{
	OnLuaUnrealReset.Broadcast(L);

	LuaStateMap.Remove(CachedGameInstance.Get());

	if (LuaTickerHandle.IsValid())
	{
		FTicker::GetCoreTicker().RemoveTicker(LuaTickerHandle);
		LuaTickerHandle.Reset();
	}

	if (L)
	{
		lua_close(L);
		L = nullptr;
	}

	LuaMemory = 0;
}

bool FastLuaUnrealWrapper::HandleLuaTick(float InDeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_LuaTick);
	if (!bTickError && L)
	{
		int32 tp = lua_gettop(L);
		int32 ret = lua_getglobal(L, "LuaTick");
		if (lua_isfunction(L, -1))
		{
			lua_pushnumber(L, InDeltaTime);
			int32 Ret = lua_pcall(L, 1, 0, 0);
			if (Ret)
			{
				bTickError = true;
				UE_LOG(LogTemp, Warning, TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(L, -1)));
				UE_LOG(LogTemp, Warning, TEXT("Lua stopped tick!"));
			}
				
		}

		lua_settop(L, tp);
	}

	return true;
}

int32 FastLuaUnrealWrapper::RunMainFunction(const FString& InMainFile /*=FString("ApplicationMain")*/)
{
	//load entry lua
	FString LoadMainScript = FString::Printf(TEXT("require('%s')"), *InMainFile);
	int32 Ret = luaL_dostring(GetLuaSate(), TCHAR_TO_UTF8(*LoadMainScript));
	if (Ret > 0)
	{
		FString RetString = UTF8_TO_TCHAR(lua_tostring(L, -1));
		UE_LOG(LogTemp, Warning, TEXT("%s"), *RetString);
	}
	//run Main
	Ret = lua_getglobal(L, "Main");
	Ret = lua_pcall(L, 0, 0, 0);
	if (Ret > 0)
	{
		FString RetString = UTF8_TO_TCHAR(lua_tostring(L, -1));
		UE_LOG(LogTemp, Warning, TEXT("%s"), *RetString);
	}

	if (LuaTickerHandle.IsValid() == false)
	{
		LuaTickerHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FastLuaUnrealWrapper::HandleLuaTick));
	}

	return Ret;
}

FString FastLuaUnrealWrapper::DoLuaCode(const FString& InCode)
{
	luaL_dostring(GetLuaSate(), TCHAR_TO_UTF8(*InCode));
	return UTF8_TO_TCHAR(lua_tostring(L, -1));
}
