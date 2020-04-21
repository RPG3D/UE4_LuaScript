// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "LuaDelegateWrapper.h"
#include "FastLuaDelegate.h"
#include "FastLuaHelper.h"
#include "lua/lua.hpp"
#include "LuaObjectWrapper.h"

void FLuaDelegateWrapper::InitWrapperMetatable(lua_State* InL)
{
	static const luaL_Reg GlueFuncs[] =
	{
		{"Bind", FLuaDelegateWrapper::LuaBindDelegate},
		{"Unbind", FLuaDelegateWrapper::LuaUnbindDelegate},
		{"Call", FLuaDelegateWrapper::LuaCallUnrealDelegate},
		{"__gc", FLuaDelegateWrapper::UserDelegateGC},
		{nullptr, nullptr},
	};

	int32 tp = lua_gettop(InL);

	if (!luaL_newmetatable(InL, GetMetatableName()))
	{
		return;
	}

	luaL_setfuncs(InL, GlueFuncs, 0);
	lua_settop(InL, tp);
}

void FLuaDelegateWrapper::PushDelegate(lua_State* InL, void* InValueAddr, bool InMulti, UFunction* InFunction)
{
	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_newuserdata(InL, sizeof(FLuaDelegateWrapper));
	new(Wrapper) FLuaDelegateWrapper(InFunction, InValueAddr, InMulti);

	luaL_setmetatable(InL, GetMetatableName());
}

void* FLuaDelegateWrapper::FetchDelegate(lua_State* InL, int32 InIndex, bool InIsMulti /*= true*/)
{
	FLuaDelegateWrapper* DelegateWrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, InIndex);
	if (DelegateWrapper && DelegateWrapper->WrapperType == ELuaWrapperType::Delegate && DelegateWrapper->IsMulti() == InIsMulti)
	{
		return DelegateWrapper->GetDelegateValueAddr();
	}

	return nullptr;
}

int FLuaDelegateWrapper::LuaNewDelegate(lua_State* InL)
{
	FString FuncName = UTF8_TO_TCHAR(lua_tostring(InL, 1)) + FString("__DelegateSignature");
	FString ScopeName = UTF8_TO_TCHAR(lua_tostring(InL, 2));
	bool bIsMulti = (bool)lua_toboolean(InL, 3);
	UFunction* Func = FindObject<UFunction>(ANY_PACKAGE, *FuncName);
	if (Func == nullptr || (ScopeName.Len() > 1 && ScopeName != Func->GetOuter()->GetName()))
	{
		lua_pushnil(InL);
	}
	else
	{
		PushDelegate(InL, nullptr, bIsMulti, Func);
	}

	return 1;
}

int FLuaDelegateWrapper::LuaBindDelegate(lua_State* InL)
{
	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, 1);

	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Delegate)
	{
		Wrapper->DelegateObjectProxy->BindLuaFunction(InL, 2);
	}

	return 0;
}

int FLuaDelegateWrapper::LuaUnbindDelegate(lua_State* InL)
{
	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, 1);
	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Delegate)
	{
		Wrapper->DelegateObjectProxy->Unbind();
	}

	return 0;
}

int FLuaDelegateWrapper::LuaCallUnrealDelegate(lua_State* InL)
{
	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Delegate || Wrapper->GetDelegateValueAddr() == nullptr)
	{
		return 0;
	}

	const UFunction* SignatureFunction = Wrapper->DelegateObjectProxy->FunctionSignature;

	int32 StackTop = 2;

	int32 ReturnNum = 0;
	//Fill parameters
	static const int32 FunctionParamDataSize = 1024;
	uint8 FuncParam[FunctionParamDataSize];
	FMemory::Memzero(FuncParam, FunctionParamDataSize);
	if (SignatureFunction->GetPropertiesSize() > FunctionParamDataSize)
	{
		UE_LOG(LogTemp, Warning, TEXT("UFunction parameter data size more than 512!"));
		return 0;
	}
	FProperty* ReturnProp = nullptr;

	for (TFieldIterator<FProperty> It(SignatureFunction); It; ++It)
	{
		FProperty* Prop = *It;
		if (Prop->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			ReturnProp = Prop;
		}
		else
		{
			FastLuaHelper::FetchProperty(InL, Prop, FuncParam, StackTop++);
		}
	}

	if (Wrapper->IsMulti())
	{
		FMulticastScriptDelegate* MultiDelegate = (FMulticastScriptDelegate*)Wrapper->GetDelegateValueAddr();
		MultiDelegate->ProcessMulticastDelegate<UObject>(FuncParam);
	}
	else
	{
		FScriptDelegate* SingleDelegate = (FScriptDelegate*)Wrapper->GetDelegateValueAddr();
		SingleDelegate->ProcessDelegate<UObject>(FuncParam);
	}

	if (ReturnProp)
	{
		FastLuaHelper::PushProperty(InL, ReturnProp, FuncParam);
		++ReturnNum;
	}

	if (SignatureFunction->HasAnyFunctionFlags(FUNC_HasOutParms))
	{
		for (TFieldIterator<FProperty> It(SignatureFunction); It; ++It)
		{
			FProperty* Prop = *It;
			if (Prop->HasAnyPropertyFlags(CPF_OutParm) && !Prop->HasAnyPropertyFlags(CPF_ConstParm))
			{
				FastLuaHelper::PushProperty(InL, *It, FuncParam);
				++ReturnNum;
			}
		}
	}

	return ReturnNum;
}

int FLuaDelegateWrapper::UserDelegateGC(lua_State* InL)
{
	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, -1);

	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Delegate)
	{
		Wrapper->~FLuaDelegateWrapper();
	}

	return 0;
}
