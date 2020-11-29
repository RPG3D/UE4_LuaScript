// Fill out your copyright notice in the Description page of Project Settings.

#include "LuaFunctionWrapper.h"
#include "FastLuaScript.h"
#include "lua.hpp"
#include "FastLuaHelper.h"
#include "FastLuaUnrealWrapper.h"
#include "FastLuaStat.h"
#include <LuaObjectWrapper.h>

FDelegateHandle ULuaFunctionWrapper::OnLuaResetHandle;


bool ULuaFunctionWrapper::BindLuaFunction(lua_State* InL, uint32 InStackIndex)
{
	if (!OnLuaResetHandle.IsValid())
	{
		OnLuaResetHandle = FastLuaUnrealWrapper::OnLuaUnrealReset.AddUObject(this, &ULuaFunctionWrapper::HandleLuaUnrealReset);
	}

	LuaState = InL;
	//ref function first
	if (lua_isfunction(InL, InStackIndex))
	{
		lua_pushvalue(InL, InStackIndex);
		LuaFunctionID = luaL_ref(InL, LUA_REGISTRYINDEX);

		//ref self
		if (lua_istable(InL, InStackIndex + 1))
		{
			lua_pushvalue(InL, InStackIndex + 1);
			LuaSelfID = luaL_ref(InL, LUA_REGISTRYINDEX);
		}

		return true;
	}
	else
	{
		return false;
	}
}

int32 ULuaFunctionWrapper::Unbind()
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

	LuaState = nullptr;

	return 0;
}


void ULuaFunctionWrapper::BeginDestroy()
{
	Super::BeginDestroy();

	Unbind();
}

void ULuaFunctionWrapper::ProcessEvent(UFunction* InFunction, void* Parms)
{
	SCOPE_CYCLE_COUNTER(STAT_DelegateCallLua);
	if (LuaState == nullptr || LuaFunctionID < 1)
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

	FProperty* ReturnParam = nullptr;
	if (FunctionSignature)
	{
		for (TFieldIterator<FProperty> It(FunctionSignature); It; ++It)
		{
			//get function return Param
			FProperty* CurrentParam = *It;
			if (CurrentParam->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				ReturnParam = CurrentParam;
			}
			else
			{
				//set params for lua function
				FastLuaHelper::PushProperty(LuaState, CurrentParam, Parms, 0);
				++ParamsNum;
			}
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
		FastLuaHelper::FetchProperty(LuaState, ReturnParam, Parms, -1);
	}
}
