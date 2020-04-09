// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "LuaObjectWrapper.h"
#include "FastLuaUnrealWrapper.h"
#include "FastLuaHelper.h"
#include "lua/lua.hpp"

void FLuaObjectWrapper::InitWrapperMetatable(lua_State* InL)
{
	int32 MetatableIndexClass = GetMetatableIndex();

	static const luaL_Reg GlueFuncs[] =
	{
		{"__index", FLuaObjectWrapper::IndexInClass},
		{"__newindex", FLuaObjectWrapper::NewIndexInClass},
		{"__gc", FLuaObjectWrapper::ObjectGC},
		{"__tostring", FLuaObjectWrapper::ObjectToString},
		{nullptr, nullptr},
	};

	lua_rawgeti(InL, LUA_REGISTRYINDEX, MetatableIndexClass);
	if (lua_istable(InL, -1))
	{
		luaL_unref(InL, LUA_REGISTRYINDEX, MetatableIndexClass);
	}
	lua_pop(InL, 1);

	int32 tp = lua_gettop(InL);
	luaL_newlib(InL, GlueFuncs);
	lua_rawseti(InL, LUA_REGISTRYINDEX, MetatableIndexClass);
	lua_settop(InL, tp);
}

UObject* FLuaObjectWrapper::FetchObject(lua_State* InL, int32 InIndex, bool IsUClass)
{
	UObject* RetObject = nullptr;
	UClass* RetClass = nullptr;
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, InIndex);
	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Object)
	{
		RetObject = Wrapper->ObjInst.Get();
	}

	if (IsUClass && RetObject)
	{
		RetClass = Cast<UClass>(RetObject);
		if (!RetClass)
		{
			if (UBlueprintCore* BPC = Cast<UBlueprintCore>(RetObject))
			{
				RetClass = BPC->GeneratedClass;
			}
		}
	}

	return IsUClass ? RetClass : RetObject;
}

void FLuaObjectWrapper::PushObject(lua_State* InL, UObject* InObj)
{
	if (InObj == nullptr)
	{
		lua_pushnil(InL);
		return;
	}

	{
		FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_newuserdata(InL, sizeof(FLuaObjectWrapper));
		new(Wrapper) FLuaObjectWrapper(InObj);

		//SCOPE_CYCLE_COUNTER(STAT_FindClassMetatable);

		const UClass* Class = InObj->GetClass();

		lua_rawgetp(InL, LUA_REGISTRYINDEX, Class);
		if (lua_istable(InL, -1))
		{
			lua_setmetatable(InL, -2);
		}
		else
		{
			lua_pop(InL, 1);

			lua_rawgeti(InL, LUA_REGISTRYINDEX, GetMetatableIndex());
			lua_setmetatable(InL, -2);
		}
	}
}

int32 FLuaObjectWrapper::IndexInClass(lua_State* InL)
{
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Object)
	{
		return 0;
	}

	FString TmpName = UTF8_TO_TCHAR(lua_tostring(InL, 2));
	UFunction* TmpFunc = Wrapper->ObjInst->GetClass()->FindFunctionByName(*TmpName);
	FProperty* TmpProp = TmpFunc ? nullptr : Wrapper->ObjInst->GetClass()->FindPropertyByName(*TmpName);

	if (TmpFunc)
	{
		lua_pushlightuserdata(InL, TmpFunc);
		lua_pushcclosure(InL, FastLuaHelper::CallUnrealFunction, 1);
		return 1;
	}

	if (TmpProp)
	{
		int32 TmpIndex = lua_tointeger(InL, 3);
		FastLuaHelper::PushProperty(InL, TmpProp, Wrapper->ObjInst.Get(), TmpIndex);
		return 1;
	}

	return 0;
}

int32 FLuaObjectWrapper::NewIndexInClass(lua_State* InL)
{
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Object)
	{
		return 0;
	}

	FString TmpName = UTF8_TO_TCHAR(lua_tostring(InL, 2));
	FProperty* TmpProp = Wrapper->ObjInst->GetClass()->FindPropertyByName(*TmpName);
	if (TmpProp == nullptr)
	{
		return 0;
	}

	FastLuaHelper::FetchProperty(InL, TmpProp, Wrapper->ObjInst.Get(), 3);

	return 0;
}

int32 FLuaObjectWrapper::ObjectToString(lua_State* InL)
{
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Object || !Wrapper->ObjInst.IsValid())
	{
		lua_pushnil(InL);
		return 1;
	}

	FString ObjName = Wrapper->ObjInst->GetName();
	lua_pushstring(InL, TCHAR_TO_UTF8(*ObjName));
	return 1;
}

int FLuaObjectWrapper::LuaGetUnrealCDO(lua_State* InL)
{
	int32 tp = lua_gettop(InL);
	if (tp < 1)
	{
		return 0;
	}

	FString ClassName = UTF8_TO_TCHAR(lua_tostring(InL, 1));

	UClass* Class = FindObject<UClass>(ANY_PACKAGE, *ClassName);
	if (Class)
	{
		FLuaObjectWrapper::PushObject(InL, Class->GetDefaultObject());
		return 1;
	}

	return 0;
}

int FLuaObjectWrapper::LuaNewObject(lua_State* InL)
{
	UObject* ObjOuter = nullptr;
	FLuaObjectWrapper* OwnerWrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	if (OwnerWrapper && OwnerWrapper->WrapperType == ELuaWrapperType::Object)
	{
		ObjOuter = OwnerWrapper->ObjInst.Get();
	}

	UClass* ObjClass = nullptr;
	FLuaObjectWrapper* ClassWrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 2);

	if (ClassWrapper && ClassWrapper->WrapperType == ELuaWrapperType::Object)
	{
		ObjClass = Cast<UClass>(ClassWrapper->ObjInst.Get());
	}

	if (ObjClass == nullptr)
	{
		return 0;
	}

	FString ObjName = UTF8_TO_TCHAR(lua_tostring(InL, 3));
	UObject* NewObj = NewObject<UObject>(ObjOuter, ObjClass, FName(*ObjName));

	FLuaObjectWrapper::PushObject(InL, NewObj);
	return 1;
}

int FLuaObjectWrapper::ObjectGC(lua_State* InL)
{
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, -1);
	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Object)
	{
		Wrapper->~FLuaObjectWrapper();
	}

	return 0;
}
