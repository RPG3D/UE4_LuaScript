// Fill out your copyright notice in the Description page of Project Settings.

#include "FastLuaDelegate.h"
#include "FastLuaScript.h"
#include "lua/lua.hpp"
#include "FastLuaHelper.h"
#include "FastLuaUnrealWrapper.h"
#include "FastLuaStat.h"

bool UFastLuaDelegate::BindLuaFunction(lua_State* InL, uint32 InStackIndex)
{
	FastLuaUnrealWrapper::OnLuaUnrealReset.Remove(OnLuaResetHandle);
	OnLuaResetHandle = FastLuaUnrealWrapper::OnLuaUnrealReset.AddUObject(this, &UFastLuaDelegate::HandleLuaUnrealReset);

	LuaState = InL;
	//ref function first
	if (lua_isfunction(InL, InStackIndex))
	{
		lua_pushvalue(InL, InStackIndex);
		LuaFunctionID = luaL_ref(InL, LUA_REGISTRYINDEX);

		if (bIsMulti)
		{
			FMulticastScriptDelegate* MultiDelegate = (FMulticastScriptDelegate*)DelegateAddr;
			if (MultiDelegate)
			{
				FScriptDelegate ScriptDelegate;
				ScriptDelegate.BindUFunction(this, GetWrapperFunctionName());
				MultiDelegate->Add(ScriptDelegate);
				DelegateAddr = MultiDelegate;
			}
		}
		else
		{
			FScriptDelegate* SingleDelegate = (FScriptDelegate*)DelegateAddr;
			if (SingleDelegate)
			{
				SingleDelegate->BindUFunction(this, UFastLuaDelegate::GetWrapperFunctionName());
				DelegateAddr = SingleDelegate;
			}
		}

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

int32 UFastLuaDelegate::Unbind()
{
	if (bIsMulti && DelegateAddr)
	{
		FMulticastScriptDelegate* MultiDelegate = (FMulticastScriptDelegate*)DelegateAddr;
		MultiDelegate->Remove(this, GetWrapperFunctionName());
	}

	if (!bIsMulti && DelegateAddr)
	{
		FScriptDelegate* SingleDelegate = (FScriptDelegate*)DelegateAddr;
		SingleDelegate->Clear();
	}

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

void UFastLuaDelegate::InitFromUFunction(UFunction* InFunction)
{
	if (bIsMulti)
	{
		DelegateAddr = (uint8*)FMemory::Malloc(sizeof(FMulticastScriptDelegate));
		new(DelegateAddr) FMulticastScriptDelegate();
	}
	else
	{
		DelegateAddr = (uint8*)FMemory::Malloc(sizeof(FScriptDelegate));
		new(DelegateAddr) FScriptDelegate();
	}

	bIsUserDefined = true;
}

void UFastLuaDelegate::BeginDestroy()
{
	Super::BeginDestroy();

	Unbind();

	if (!bIsUserDefined)
	{
		return;
	}

	if (bIsMulti && DelegateAddr)
	{
		FMulticastScriptDelegate* MultiDelegate = (FMulticastScriptDelegate*)DelegateAddr;
		if (MultiDelegate)
		{
			MultiDelegate->~FMulticastScriptDelegate();
		}
	}

	if (!bIsMulti && DelegateAddr)
	{
		FScriptDelegate* SingleDelegate = (FScriptDelegate*)DelegateAddr;
		if (SingleDelegate)
		{
			SingleDelegate->~FScriptDelegate();
		}
	}

	FMemory::Free(DelegateAddr);
	DelegateAddr = nullptr;
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

	FProperty* ReturnParam = nullptr;
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
