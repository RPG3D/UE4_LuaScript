// Fill out your copyright notice in the Description page of Project Settings.

#include "FastLuaDelegate.h"
#include "FastLuaScript.h"
#include "lua.hpp"
#include "FastLuaHelper.h"


void UFastLuaDelegate::ProcessEvent(UFunction* InFunction, void* Parms)
{
	//SCOPE_CYCLE_COUNTER(STAT_DelegateCallLua);
	if (LuaState == nullptr || FunctionSignature == nullptr )
	{
		return;
	}
	//get lua function in global registry
	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaFunctionID);
	int32 ParamsNum = 0;
	if (LuaSelfID)
	{
		lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaSelfID);
		++ParamsNum;
	}

	UProperty* ReturnParam = nullptr;
	for (TFieldIterator<UProperty> It(FunctionSignature); It; ++It)
	{
		//get function return Param
		UProperty* CurrentParam = *It;
		if (CurrentParam->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			ReturnParam = CurrentParam;
		}
		else
		{
			//set params for lua function
			//FastLuaHelper::PushProperty(LuaState, CurrentParam, Parms, CurrentParam->HasAnyPropertyFlags(CPF_OutParm));
			//++ParamsNum;
		}
	}

	//call lua function
	int32 CallRet = lua_pcall(LuaState, ParamsNum, ReturnParam ? 1 : 0, 0);
	if (CallRet)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(LuaState, -1)));
	}
	
	if (ReturnParam)
	{
		//get function return Value, in common
		//FUnrealMisc::FetchProperty(LuaState, ReturnParam, Parms);
	}
}

bool UFastLuaDelegate::HotfixLuaFunction()
{
	if (LuaState == nullptr)
	{
		return false;
	}

	//unref old
	if (LuaState && LuaFunctionID)
	{
		luaL_unref(LuaState, LUA_REGISTRYINDEX, LuaFunctionID);
	}

	//ref function
	lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaSelfID);
	lua_getfield(LuaState, -1, TCHAR_TO_UTF8(*LuaFunctionName));
	if (lua_isfunction(LuaState, -1))
	{
		LuaFunctionID = luaL_ref(LuaState, LUA_REGISTRYINDEX);
	}
	else
	{
		lua_pop(LuaState, 1);
		return false;
	}

	//remove self
	lua_pop(LuaState, 1);

	return true;
}