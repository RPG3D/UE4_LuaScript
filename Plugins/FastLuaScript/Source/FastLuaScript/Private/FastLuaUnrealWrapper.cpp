// Fill out your copyright notice in the Description page of Project Settings.

#include "FastLuaUnrealWrapper.h"
#include "Package.h"
#include "lua/lua.hpp"
#include "Engine/GameInstance.h"
#include "UObjectIterator.h"
#include "Paths.h"
#include "PlatformFilemanager.h"
#include "UObject/StructOnScope.h"
#include "FastLuaHelper.h"
#include "FastLuaStat.h"
#include "FastLuaDelegate.h"

static int StopNewIndex(lua_State* InL)
{
	UE_LOG(LogTemp, Warning, TEXT("The lua table _G['Unreal'] is read only!"));
	return 0;
}

static int InitUnrealLib(lua_State* InL)
{
	const static luaL_Reg funcs[] =
	{
		{"LuaGetGameInstance", FastLuaHelper::LuaGetGameInstance},
		{"LuaLoadObject", FastLuaHelper::LuaLoadObject},
		{"LuaLoadClass", FastLuaHelper::LuaLoadClass},
		{"LuaGetUnrealCDO", FastLuaHelper::LuaGetUnrealCDO},
		{"LuaNewObject", FastLuaHelper::LuaNewObject},
		{"LuaNewStruct", FastLuaHelper::LuaNewStruct},
		{"LuaNewDelegate", FastLuaHelper::LuaNewDelegate},
		{"RegisterTickFunction", FastLuaHelper::RegisterTickFunction},
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

static int RegisterRawAPI(lua_State* InL)
{
	lua_getglobal(InL, "_G");
	const static luaL_Reg funcs[] =
	{
		{ "print", FastLuaHelper::PrintLog },
		{ nullptr, nullptr }
	};

	luaL_setfuncs(InL, funcs, 0);

	lua_pop(InL, 1);

	return 0;
}


static int RequireFromUFS(lua_State* InL)
{
	const char* RawFilwName = lua_tostring(InL, -1);
	FString FileName = UTF8_TO_TCHAR(RawFilwName);

	FileName.ReplaceInline(*FString("."), *FString("/"));

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
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

		IFileHandle* LuaFile = PlatformFile.OpenRead(*FullFilePath);
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
				lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
				FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
				lua_pop(InL, 1);
				FastLuaHelper::LuaLog(FString::Printf(TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(InL, -1))), 1, LuaWrapper);
			}

			delete LuaFile;
			return 2;
		}
	}

	FString MessageToPush = FString("RequireFromUFS failed: ") + FileName;
	luaL_traceback(InL, InL, TCHAR_TO_UTF8(*MessageToPush), 1);

	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
	lua_pop(InL, 1);
	FastLuaHelper::LuaLog(FString::Printf(TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(InL, -1))), 1, LuaWrapper);
	return 0;
}


void FastLuaUnrealWrapper::InitDelegateMetatable()
{
	static const luaL_Reg DelegateFuncs[] =
	{
		{"Bind", FastLuaHelper::LuaBindDelegate},
		{"Unbind", FastLuaHelper::LuaUnbindDelegate},
		{"Call", FastLuaHelper::LuaCallUnrealDelegate},
		{nullptr, nullptr},
	};

	luaL_newlib(GetLuaSate(), DelegateFuncs);
	lua_pushvalue(GetLuaSate(), -1);
	lua_setfield(GetLuaSate(), -2, "__index");

	lua_pushcfunction(GetLuaSate(), FastLuaHelper::UserDelegateGC);
	lua_setfield(GetLuaSate(), -2, "__gc");

	DelegateMetatableIndex = luaL_ref(GetLuaSate(), LUA_REGISTRYINDEX);
}


FastLuaUnrealWrapper::FastLuaUnrealWrapper()
{
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

	return Inst;
}

void FastLuaUnrealWrapper::Init(UGameInstance* InGameInstance)
{
	if (L != nullptr)
	{
		return;
	}

	L = lua_newstate(FastLuaHelper::LuaAlloc, this);
	luaL_openlibs(L);

	lua_pushlightuserdata(L, this);
	lua_rawsetp(L, LUA_REGISTRYINDEX, L);

	if (InGameInstance)
	{
		CachedGameInstance = InGameInstance;
	}

	luaL_requiref(L, "Unreal", InitUnrealLib, 1);

	RegisterRawAPI(L);

	InitDelegateMetatable();

	//set package path
	{
		FString LuaEnvPath = FPaths::ProjectDir() / FString("LuaScript/?.lua") + FString(";") + FPaths::ProjectDir() / FString("LuaScript/?/Init.lua");
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
	for (int32 i = DelegateCallLuaList.Num() - 1; i >= 0; --i)
	{
		if (DelegateCallLuaList[i])
		{
			DelegateCallLuaList[i]->Unbind();
		}
	}

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
	static float Count = 0.f;
	SCOPE_CYCLE_COUNTER(STAT_LuaTick);
	if (L && LuaTickFunctionIndex)
	{
		int32 tp = lua_gettop(L);
		lua_rawgeti(L, LUA_REGISTRYINDEX, LuaTickFunctionIndex);
		if (lua_isfunction(L, -1))
		{
			lua_pushnumber(L, InDeltaTime);
			int32 Ret = lua_pcall(L, 1, 0, 0);
			if (Ret)
			{
				Count += InDeltaTime;
				if (Count > 3.f)
				{
					Count = 0.f;
					FastLuaHelper::LuaLog(FString::Printf(TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(L, -1))), 1, this);
				}

			}
				
		}

		lua_settop(L, tp);
	}

	return true;
}

//Lua usage: LoadMapEndedEvent:Bind("LuaBindLoadMapEnded", LuaObj, LuaFunctionName)
int32 FastLuaUnrealWrapper::RunMainFunction(const FString& InMainFile /*=FString("ApplicationMain")*/)
{
	//load entry lua
	FString LoadMainScript = FString::Printf(TEXT("require('%s')"), *InMainFile);
	int32 Ret = luaL_dostring(GetLuaSate(), TCHAR_TO_UTF8(*LoadMainScript));
	if (Ret > 0)
	{
		FString RetString = UTF8_TO_TCHAR(lua_tostring(L, -1));
		FastLuaHelper::LuaLog(FString::Printf(TEXT("%s"), *RetString), 1, this);
	}
	//run Main
	Ret = lua_getglobal(L, "Main");
	Ret = lua_pcall(L, 0, 0, 0);
	if (Ret > 0)
	{
		FString RetString = UTF8_TO_TCHAR(lua_tostring(L, -1));
		FastLuaHelper::LuaLog(FString::Printf(TEXT("%s"), *RetString), 1, this);
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
