// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaLatentActionWrapper.h"
#include "lua/lua.hpp"
#include "FastLuaUnrealWrapper.h"

void ULuaLatentActionWrapper::TestFunction(int32 InParam)
{
	int32 nres = 0;
	int32 Result = lua_resume(WorkerThread, MainThread, 0, &nres);
	if (Result == LUA_OK)
	{
		luaL_unref(MainThread, LUA_REGISTRYINDEX, InParam);
	}

	this->RemoveFromRoot();
	this->MarkPendingKill();
}
