// Fill out your copyright notice in the Description page of Project Settings.


#include "GameClassLuaBind.h"
#include "lua/lua.hpp"
#include "FastLuaUnrealWrapper.h"
#include "FastLuaHelper.h"

GameClassLuaBind::GameClassLuaBind()
{
}

GameClassLuaBind::~GameClassLuaBind()
{
	ReleaseLuaBind();
}

int32 GameClassLuaBind::InitLuaBind(UObject* InObject)
{
	if (LuaWrapper == nullptr || LuaWrapper->GetLuaSate() == nullptr)
	{
		return 0;
	}

	lua_State* InL = LuaWrapper->GetLuaSate();

	int32 tp = lua_gettop(LuaWrapper->GetLuaSate());

	FString LoadLua = FString("require('") + FString(LuaTableName) + FString("')");
	FString Ret = LuaWrapper->DoLuaCode(LoadLua);

	lua_getglobal(InL, TCHAR_TO_UTF8(*LuaTableName));
	if (lua_istable(InL, -1))
	{
		lua_getfield(InL, -1, "new");
		FastLuaHelper::PushObject(InL, InObject);
		if (lua_pcall(InL, 1, 1, 0) == LUA_OK && lua_istable(InL, -1))
		{
			LuaSelf = luaL_ref(InL, LUA_REGISTRYINDEX);
		}
		else
		{
			FString ErrorString = UTF8_TO_TCHAR(lua_tostring(InL, -1));
			UE_LOG(LogTemp, Warning, TEXT("%s"), *ErrorString);
			lua_settop(InL, tp);
			return 0;
		}

		for (auto It : FunctionNameToAddress)
		{
			lua_getfield(InL, -1, TCHAR_TO_UTF8(*It.Key));
			if (It.Value != nullptr && lua_isfunction(InL, -1))
			{
				int32 FuncIndex = luaL_ref(InL, LUA_REGISTRYINDEX);
				FunctionAddressToIndex.Emplace(It.Value, FuncIndex);
			}
			else
			{
				lua_pop(InL, 1);
			}
		}
	}

	lua_settop(InL, tp);
	return 0;
}

int32 GameClassLuaBind::ReleaseLuaBind()
{
	int32 Ret = 0;

	if (LuaWrapper == nullptr || LuaWrapper->GetLuaSate() == nullptr)
	{
		return 0;
	}

	lua_State* InL = LuaWrapper->GetLuaSate();

	for (auto It : FunctionNameToAddress)
	{
		if (FunctionAddressToIndex.Find(It.Value))
		{
			luaL_unref(InL, LUA_REGISTRYINDEX, *FunctionAddressToIndex.Find(It.Value));
			It.Value = 0;
		}
	}

	if (LuaSelf > 0)
	{
		luaL_unref(InL, LUA_REGISTRYINDEX, LuaSelf);
	}

	LuaSelf = 0;
	return Ret;
}

struct lua_State* GameClassLuaBind::GetLua()
{
	if (LuaWrapper)
	{
		return LuaWrapper->GetLuaSate();
	}

	return nullptr;
}
