// Fill out your copyright notice in the Description page of Project Settings.

#include "FastLuaUnrealWrapper.h"

#include "Engine/GameInstance.h"
#include "UObject/UObjectIterator.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "UObject/StructOnScope.h"
#include "FastLuaHelper.h"
#include "FastLuaStat.h"
#include "FastLuaScript.h"

#include "LuaDelegateWrapper.h"
#include "LuaObjectWrapper.h"


#include "lua.hpp"


FastLuaUnrealWrapper::FOnLuaEventNoParam FastLuaUnrealWrapper::OnLuaUnrealReset;
FastLuaUnrealWrapper::FOnLuaEventNoParam FastLuaUnrealWrapper::OnLuaLoadThirdPartyLib;

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
		{"LuaGetUnrealCDO", FLuaObjectWrapper::LuaGetUnrealCDO},

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

	TArray<FString> PathList;
	{
		FString LuaPath = FPaths::ProjectDir() / FString("Content/LuaScript/?.lua");
		PathList.Add(LuaPath);
#if PLATFORM_WINDOWS
		FString LuaPath2 = FPaths::ProjectDir() / FString("Content/LuaScript/?.dll");
		PathList.Add(LuaPath2);
#else
		FString LuaPath2 = FPaths::ProjectDir() / FString("Content/LuaScript/?.so");
		PathList.Add(LuaPath2);
#endif
	}

	IPlatformFile& PhysicalPlatformFile = IPlatformFile::GetPlatformPhysical();

	for (int32 i = 0; i < PathList.Num(); ++i)
	{
		FString FullFilePath = PathList[i].Replace(TEXT("?"), *FileName);
		FullFilePath = FullFilePath.Replace(TEXT("\\"), TEXT("/"));

		FPaths::NormalizeFilename(FullFilePath);

		IFileHandle* LuaFile = PhysicalPlatformFile.OpenRead(*FullFilePath, false);

		if (LuaFile)
		{
			UE_LOG(LogFastLuaScript, Log, TEXT("RequireFromUFS|Physical:%s"), *FileName);
		}
		else
		{
			LuaFile = FPlatformFileManager::Get().GetPlatformFile().OpenRead(*FullFilePath, false);
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
	UE_LOG(LogTemp, Warning, TEXT("%s"), *MessageToPush);

	//luaL_traceback(InL, InL, TCHAR_TO_UTF8(*MessageToPush), 1);
	//UE_LOG(LogTemp, Warning, TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(InL, -1)));
	return 0;
}


FastLuaUnrealWrapper::FastLuaUnrealWrapper()
{

}

FastLuaUnrealWrapper::~FastLuaUnrealWrapper()
{
	Reset();
}

TSharedPtr<FastLuaUnrealWrapper> FastLuaUnrealWrapper::GetDefault(const UGameInstance* InGameInstnce)
{
	static TSharedPtr<FastLuaUnrealWrapper> Inst = nullptr;
	if (Inst.IsValid() == false)
	{
		Inst = MakeShareable((new FastLuaUnrealWrapper()));
	}

	if (Inst->GetLuaState() == nullptr)
	{
		Inst->Init();
	}

	return Inst;
}

void FastLuaUnrealWrapper::Init()
{
	if (L != nullptr)
	{
		return;
	}


	L = luaL_newstate();
	luaL_openlibs(L);

	luaL_requiref(L, "Unreal", InitUnrealLib, 1);

	FLuaDelegateWrapper::InitWrapperMetatable(L);

	//add searcher
	{
		int32 tp = lua_gettop(L);
		int32 ret = lua_getglobal(L, "package");
		lua_getfield(L, -1, "searchers");
		int32 FunctionNum = luaL_len(L, -1);
		lua_pushcfunction(L, RequireFromUFS);
		lua_rawseti(L, -2, FunctionNum + 1);
		lua_settop(L, tp);
	}

	OnLuaLoadThirdPartyLib.Broadcast(L);
}

void FastLuaUnrealWrapper::Reset()
{
	OnLuaUnrealReset.Broadcast(L);

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
		int32 ret = lua_getglobal(L, ProgramTableName);
		ret = ret == LUA_TTABLE ? lua_getfield(L, -1, "LuaTick") : LUA_TNIL;
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

int32 FastLuaUnrealWrapper::RunMainFunction(UGameInstance* InGameInstance)
{
	//load entry lua
	lua_getglobal(L, "require");
	lua_pushstring(L, ProgramTableName);
	int32 Ret = lua_pcall(L, 1, 1, 0);
	if (Ret != LUA_OK || !lua_istable(L, -1))
	{
		FString RetString = UTF8_TO_TCHAR(lua_tostring(L, -1));
		UE_LOG(LogTemp, Warning, TEXT("%s"), *RetString);
		return Ret;
	}
	//run Program.Main
	Ret = lua_getfield(L, -1, "Main");
	if (lua_isfunction(L, -1))
	{
		FLuaObjectWrapper::PushObject(L, InGameInstance);
		Ret = lua_pcall(L, 1, 0, 0);
	}
	else
	{
		Ret = LUA_ERRRUN;
	}

	if (Ret != LUA_OK)
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
	luaL_dostring(GetLuaState(), TCHAR_TO_UTF8(*InCode));
	return UTF8_TO_TCHAR(lua_tostring(L, -1));
}
