// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaDelegateWrapper.h"
#include "LuaFunctionWrapper.h"
#include "FastLuaHelper.h"
#include "LuaObjectWrapper.h"
#include "UObject/ScriptDelegates.h"
#include <UObject/WeakObjectPtr.h>
#include <UObject/StructOnScope.h>

#include "lua.hpp"



FLuaDelegateWrapper::FLuaDelegateWrapper(class UFunction* InFunction, void* InDelegateAddr, bool InMulti)
{
	FunctionSignature = InFunction;
	DelegateAddr = InDelegateAddr;
	bIsMulti = InMulti;

	bIsUserCreated = (DelegateAddr == nullptr);

	if (bIsUserCreated)
	{
		if (bIsMulti)
		{
			DelegateAddr = new FMulticastScriptDelegate();
		}
		else
		{
			DelegateAddr = new FScriptDelegate();
		}
	}
}

FLuaDelegateWrapper::~FLuaDelegateWrapper()
{
	if (bIsUserCreated == false)
	{
		return;
	}

	if (bIsMulti && DelegateAddr)
	{
		((FMulticastScriptDelegate*)DelegateAddr)->~FMulticastScriptDelegate();
	}

	if (!bIsMulti && DelegateAddr)
	{
		((FScriptDelegate*)DelegateAddr)->~FScriptDelegate();
	}

	DelegateAddr = nullptr;

}

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

	int32 ValueType = lua_getfield(InL, LUA_REGISTRYINDEX, GetMetatableName());
	if (ValueType != LUA_TTABLE)
	{
		lua_pop(InL, 1);

		lua_newtable(InL);

		lua_pushvalue(InL, -1);
		lua_setfield(InL, -2, "__index");

		luaL_setfuncs(InL, GlueFuncs, 0);

		lua_setfield(InL, LUA_REGISTRYINDEX, GetMetatableName());
	}

	lua_settop(InL, tp);
}

void* FLuaDelegateWrapper::GetDelegateValueAddr()
{
	return DelegateAddr;
}

bool FLuaDelegateWrapper::IsMulti() const
{
	return bIsMulti;
}

void FLuaDelegateWrapper::PushDelegate(lua_State* InL, void* InValueAddr, bool InMulti, UFunction* InFunction)
{
	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_newuserdata(InL, sizeof(FLuaDelegateWrapper));
	new(Wrapper) FLuaDelegateWrapper(InFunction, InValueAddr, InMulti);


	if (lua_getfield(InL, LUA_REGISTRYINDEX, GetMetatableName()) == LUA_TTABLE)
	{
		lua_setmetatable(InL, -2);
	}
	else
	{
		lua_pop(InL, 1);
	}
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

int32 FLuaDelegateWrapper::LuaNewDelegate(lua_State* InL)
{
	UFunction* Func = Cast<UFunction>(FLuaObjectWrapper::FetchObject(InL, 1));
	bool bIsMulti = (bool)lua_toboolean(InL, 2);
	if (Func == nullptr)
	{
		lua_pushnil(InL);
	}
	else
	{
		PushDelegate(InL, nullptr, bIsMulti, Func);
	}

	return 1;
}

int32 FLuaDelegateWrapper::LuaBindDelegate(lua_State* InL)
{
	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, 1);

	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Delegate)
	{
		return 0;
	}

	ULuaFunctionWrapper* LuaFunc = Cast<ULuaFunctionWrapper>(FLuaObjectWrapper::FetchObject(InL, 2, false));
	if (LuaFunc == nullptr)
	{
		return 0;
	}

	LuaFunc->FunctionSignature = Wrapper->FunctionSignature;

	if (Wrapper->bIsMulti)
	{
		FScriptDelegate TempDelegate;
		TempDelegate.BindUFunction(LuaFunc, LuaFunc->GetWrapperFunctionFName());
		if (((FMulticastScriptDelegate*)Wrapper->DelegateAddr)->Contains(TempDelegate) == false)
		{
			((FMulticastScriptDelegate*)Wrapper->DelegateAddr)->Add(TempDelegate);
		}
	}
	else
	{
		if (((FScriptDelegate*)Wrapper->DelegateAddr)->GetUObject() != LuaFunc)
		{
			((FScriptDelegate*)Wrapper->DelegateAddr)->BindUFunction(LuaFunc, LuaFunc->GetWrapperFunctionFName());
		}
	}

	lua_pushinteger(InL, 1);

	return 1;
}

int32 FLuaDelegateWrapper::LuaUnbindDelegate(lua_State* InL)
{
	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, 1);

	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Delegate)
	{
		return 0;
	}

	ULuaFunctionWrapper* LuaFunc = Cast<ULuaFunctionWrapper>(FLuaObjectWrapper::FetchObject(InL, 2, false));

	if (Wrapper->bIsMulti)
	{
		FScriptDelegate TempDelegate;
		TempDelegate.BindUFunction(LuaFunc, ULuaFunctionWrapper::GetWrapperFunctionFName());
		((FMulticastScriptDelegate*)Wrapper->DelegateAddr)->Remove(TempDelegate);
	}
	else
	{
		((FScriptDelegate*)Wrapper->DelegateAddr)->Clear();
	}

	lua_pushinteger(InL, 1);

	return 1;
}

int32 FLuaDelegateWrapper::LuaCallUnrealDelegate(lua_State* InL)
{
	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Delegate || Wrapper->GetDelegateValueAddr() == nullptr)
	{
		return 0;
	}

	const UFunction* SignatureFunction = Wrapper->FunctionSignature;

	int32 StackTop = 2;

	int32 ReturnNum = 0;
	//Fill parameters
	FStructOnScope FuncParam(SignatureFunction);

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
			FastLuaHelper::FetchProperty(InL, Prop, FuncParam.GetStructMemory(), StackTop++);
		}
	}

	if (Wrapper->IsMulti())
	{
		FMulticastScriptDelegate* MultiDelegate = (FMulticastScriptDelegate*)Wrapper->GetDelegateValueAddr();
		MultiDelegate->ProcessMulticastDelegate<UObject>(FuncParam.GetStructMemory());
	}
	else
	{
		FScriptDelegate* SingleDelegate = (FScriptDelegate*)Wrapper->GetDelegateValueAddr();
		SingleDelegate->ProcessDelegate<UObject>(FuncParam.GetStructMemory());
	}

	if (ReturnProp)
	{
		FastLuaHelper::PushProperty(InL, ReturnProp, FuncParam.GetStructMemory());
		++ReturnNum;
	}

	if (SignatureFunction->HasAnyFunctionFlags(FUNC_HasOutParms))
	{
		for (TFieldIterator<FProperty> It(SignatureFunction); It; ++It)
		{
			FProperty* Prop = *It;
			if (Prop->HasAnyPropertyFlags(CPF_OutParm) && !Prop->HasAnyPropertyFlags(CPF_ConstParm))
			{
				FastLuaHelper::PushProperty(InL, *It, FuncParam.GetStructMemory());
				++ReturnNum;
			}
		}
	}

	return ReturnNum;
}

int32 FLuaDelegateWrapper::UserDelegateGC(lua_State* InL)
{
	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, -1);

	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Delegate)
	{
		Wrapper->~FLuaDelegateWrapper();
	}

	return 0;
}
