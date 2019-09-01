// Fill out your copyright notice in the Description page of Project Settings.

#include "LuaUnrealWrapper.h"
#include "Package.h"
#include "LuaScript.h"
#include "lua.hpp"
#include "UnrealMisc.h"
#include "Engine/GameInstance.h"
#include "UObjectIterator.h"
#include "FileHelper.h"
#include "Paths.h"
#include "PlatformFilemanager.h"
#include "UObject/StructOnScope.h"
#include "LuaStat.h"
#include "DelegateCallLua.h"
#include "ObjectLuaReference.h"


static int InitUnrealLib(lua_State* L);
static int RegisterRawAPI(lua_State* InL);
static int RequireFromUFS(lua_State* InL);


FLuaUnrealWrapper::FLuaUnrealWrapper()
{
}

FLuaUnrealWrapper::~FLuaUnrealWrapper()
{
	Reset();
}

TSharedPtr<FLuaUnrealWrapper> FLuaUnrealWrapper::Create(class UGameInstance* InGameInstance, const FString& InName)
{
	TSharedPtr<FLuaUnrealWrapper> Inst(new FLuaUnrealWrapper());
	if (InName.Len())
	{
		Inst->LuaStateName = InName;
	}
	Inst->Init(InGameInstance);

	return Inst;
}

void FLuaUnrealWrapper::Init(UGameInstance* InGameInstance)
{
	if (L == nullptr)
	{
		ObjectRef = NewObject<UObjectLuaReference>();
		ObjectRef->AddToRoot();

		L = lua_newstate(FUnrealMisc::LuaAlloc, this);
		luaL_openlibs(L);

		lua_pushlightuserdata(L, this);
		lua_rawsetp(L, LUA_REGISTRYINDEX, L);

		RegisterUnreal(InGameInstance);
	}
}

void FLuaUnrealWrapper::Reset()
{
	if (L)
	{
		lua_close(L);
		L = nullptr;
	}

	if (ObjectRef)
	{
		ObjectRef->ObjectRefMap.Empty();
		ObjectRef->RemoveFromRoot();
		ObjectRef = nullptr;
	}

	LuaMemory = 0;

	if (LuaTickerHandle.IsValid())
	{
		FTicker::GetCoreTicker().RemoveTicker(LuaTickerHandle);
		LuaTickerHandle.Reset();
	}

	if (DelegateCallLuaList.Num())
	{
		for (TObjectIterator<UDelegateCallLua>It; It; ++It)
		{
			if ((*It)->bIsMulti && (*It)->DelegateInst)
			{
				FMulticastScriptDelegate* MultiDelegateInst = (FMulticastScriptDelegate*)((*It)->DelegateInst);
				if (MultiDelegateInst && MultiDelegateInst->IsBound())
				{
					MultiDelegateInst->Remove((*It), UDelegateCallLua::GetWrapperFunctionName());
				}
			}
			else if ((*It)->bIsMulti == false && (*It)->DelegateInst)
			{
				FScriptDelegate* SingleDelegateInst = (FScriptDelegate*)((*It)->DelegateInst);
				if (SingleDelegateInst && SingleDelegateInst->IsBound())
				{
					SingleDelegateInst->Clear();
				}
			}

			(*It)->RemoveFromRoot();
			DelegateCallLuaList.Remove((*It));
		}
	}

}

void FLuaUnrealWrapper::RegisterUnreal(UGameInstance* InGameInst)
{
	CachedGameInstance = InGameInst;

	//create a table "Unreal" in _G
	luaL_requiref(L, "Unreal", InitUnrealLib, 1);

	int ret = 0;// lua_getglobal(L, "Unreal");

	RegisterRawAPI(L);

	//create a table for UClass
	lua_newtable(L);
	{
		lua_pushcfunction(L, FUnrealMisc::ObjectIndex);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, FUnrealMisc::ObjectNewIndex);
		lua_setfield(L, -2, "__newindex");

		ClassMetatableIdx = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	//create a table for Struct
	lua_newtable(L);
	{
		lua_pushcfunction(L, FUnrealMisc::StructIndex);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, FUnrealMisc::StructNewIndex);
		lua_setfield(L, -2, "__newindex");

		StructMetatableIdx = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	lua_newtable(L);
	{
		RegisterDelegate(L);
		DelegateMetatableIndex = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	//set package path
	{
		FString LuaEnvPath = FPaths::ProjectDir() / FString("LuaScript/?.lua") + FString(";") + FPaths::ProjectDir() / FString("LuaScript/?/Init.lua");
		ret = lua_getglobal(L, "package");
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

bool FLuaUnrealWrapper::HandleLuaTick(float InDeltaTime)
{
	static float Count = 0.f;

	if (L && LuaTickFunctionIndex)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, LuaTickFunctionIndex);
		if (lua_isfunction(L, -1))
		{
			lua_pushnumber(L, InDeltaTime);
			lua_pcall(L, 1, 0, 0);
		}
		else
		{
			lua_pop(L, 1);
		}
	}

	return true;
}

void FLuaUnrealWrapper::RunMainFunction(const FString& InMainFile /*=FString("ApplicationMain")*/)
{
	//load entry lua
	FString LoadMainScript = FString::Printf(TEXT("require('%s')"), *InMainFile);
	int32 Ret = luaL_dostring(GetLuaSate(), TCHAR_TO_UTF8(*LoadMainScript));
	if (Ret > 0)
	{
		FString RetString = UTF8_TO_TCHAR(lua_tostring(L, -1));
		FUnrealMisc::LuaLog(FString::Printf(TEXT("%s"), *RetString), 1, this);
	}
	//run Main
	Ret = lua_getglobal(L, "Main");
	Ret = lua_pcall(L, 0, 0, 0);
	if (Ret > 0)
	{
		FString RetString = UTF8_TO_TCHAR(lua_tostring(L, -1));
		FUnrealMisc::LuaLog(FString::Printf(TEXT("%s"), *RetString), 1, this);
	}

	if (LuaTickerHandle.IsValid() == false)
	{
		LuaTickerHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FLuaUnrealWrapper::HandleLuaTick));
	}

}


bool FLuaUnrealWrapper::RegisterDelegate(lua_State* InL)
{
	if (lua_istable(InL, -1) == false)
	{
		return false;
	}

	lua_pushvalue(InL, -1);
	lua_setfield(InL, -2, "__index");

	lua_pushcfunction(InL, FUnrealMisc::LuaBindDelegate);
	lua_setfield(InL, -2, "Bind");

	lua_pushcfunction(InL, FUnrealMisc::LuaBindDelegate);
	lua_setfield(InL, -2, "Add");


	lua_pushcfunction(InL, FUnrealMisc::LuaUnbindDelegate);
	lua_setfield(InL, -2, "Unbind");

	lua_pushcfunction(InL, FUnrealMisc::LuaUnbindDelegate);
	lua_setfield(InL, -2, "Remove");

	lua_pushcfunction(InL, FUnrealMisc::LuaCallUnrealDelegate);
	lua_setfield(InL, -2, "Call");

	return true;
}

FString FLuaUnrealWrapper::DoLuaCode(const FString& InCode)
{
	luaL_dostring(GetLuaSate(), TCHAR_TO_UTF8(*InCode));
	return UTF8_TO_TCHAR(lua_tostring(L, -1));
}

int InitUnrealLib(lua_State* L)
{
	const static luaL_Reg funcs[] =
	{
		{"LuaGetGameInstance", FUnrealMisc::LuaGetGameInstance},
		{"LuaLoadObject", FUnrealMisc::LuaLoadObject},
		{"LuaLoadClass", FUnrealMisc::LuaLoadClass},
		{"LuaGetUnrealCDO", FUnrealMisc::LuaGetUnrealCDO},
		{"LuaNewObject", FUnrealMisc::LuaNewObject},
		{"LuaNewStruct", FUnrealMisc::LuaNewStruct},
		{"LuaDumpObject", FUnrealMisc::LuaDumpObject},
		{"LuaAddObjectRef", FUnrealMisc::LuaAddObjectRef},
		{"RegisterTickFunction", FUnrealMisc::RegisterTickFunction},
		{nullptr, nullptr}
	};

	luaL_newlib(L, funcs);

	return 1;
}

int RegisterRawAPI(lua_State* InL)
{
	lua_getglobal(InL, "_G");
	const static luaL_Reg funcs[] =
	{
		{ "print", FUnrealMisc::PrintLog },
		{ nullptr, nullptr }
	};

	luaL_setfuncs(InL, funcs, 0);

	lua_pop(InL, 1);

	return 0;
}


int RequireFromUFS(lua_State* InL)
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
				FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(InL, -1);
				lua_pop(InL, 1);
				FUnrealMisc::LuaLog(FString::Printf(TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(InL, -1))), 1, LuaWrapper);
			}

			delete LuaFile;
			return 2;
		}
	}

	FString MessageToPush = FString("RequireFromUFS failed: ") + FileName;
	luaL_traceback(InL, InL, TCHAR_TO_UTF8(*MessageToPush), 1);

	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(InL, -1);
	lua_pop(InL, 1);
	FUnrealMisc::LuaLog(FString::Printf(TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(InL, -1))), 1, LuaWrapper);
	return 0;
}
