// Fill out your copyright notice in the Description page of Project Settings.

#include "FastLuaDelegate.h"
#include "FastLuaScript.h"
#include "lua.hpp"
#include "FastLuaHelper.h"
#include "FastLuaUnrealWrapper.h"
#include "FastLuaStat.h"

int32 UFastLuaDelegate::Unbind()
{
	if (LuaState == nullptr)
	{
		return -1;
	}

	if (LuaFunctionID)
	{
		luaL_unref(LuaState, LUA_REGISTRYINDEX, LuaFunctionID);
		LuaFunctionID = 0;
	}

	if (LuaSelfID)
	{
		luaL_unref(LuaState, LUA_REGISTRYINDEX, LuaSelfID);
		LuaSelfID = 0;
	}

	lua_rawgetp(LuaState, LUA_REGISTRYINDEX, LuaState);
	FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(LuaState, -1);
	lua_pop(LuaState, 1);
	LuaState = nullptr;

	if (bIsMulti && DelegateInst)
	{
		FMulticastScriptDelegate* MultiDelegate = (FMulticastScriptDelegate*)DelegateInst;
		MultiDelegate->Remove(this, UFastLuaDelegate::GetWrapperFunctionName());
	}

	if (!bIsMulti && DelegateInst)
	{
		FScriptDelegate* SingleDelegate = (FScriptDelegate*)DelegateInst;
		SingleDelegate->Clear();
	}

	LuaWrapper->DelegateCallLuaList.Remove(this);

	this->RemoveFromRoot();

	return 0;
}

void UFastLuaDelegate::ProcessEvent(UFunction* InFunction, void* Parms)
{
	SCOPE_CYCLE_COUNTER(STAT_DelegateCallLua);
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
			FastLuaHelper::PushProperty(LuaState, CurrentParam, Parms, CurrentParam->HasAnyPropertyFlags(CPF_OutParm));
			++ParamsNum;
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
		FastLuaHelper::FetchProperty(LuaState, ReturnParam, Parms);
	}
}
