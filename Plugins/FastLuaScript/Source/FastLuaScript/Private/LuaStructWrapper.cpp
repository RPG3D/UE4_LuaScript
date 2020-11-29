// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaStructWrapper.h"
#include "FastLuaUnrealWrapper.h"
#include "FastLuaHelper.h"
#include <LuaObjectWrapper.h>

#include "lua.hpp"
#include "FastLuaStat.h"


void* FLuaStructWrapper::FetchStruct(lua_State* InL, int32 InIndex, const UScriptStruct* InStruct)
{
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, InIndex);
	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Struct && Wrapper->StructType == InStruct)
	{
		uint8* ValuePtr = ((uint8*)Wrapper) + sizeof(FLuaStructWrapper);
		return ValuePtr;
	}

	return nullptr;
}

void FLuaStructWrapper::PushStruct(lua_State* InL, UScriptStruct* InStruct, const void* InBuff)
{
	if (InStruct == nullptr)
	{
		lua_pushnil(InL);
		return;
	}


	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_newuserdata(InL, sizeof(FLuaStructWrapper) + InStruct->GetStructureSize());
	new(Wrapper) FLuaStructWrapper(InStruct);

	uint8* ValuePtr = ((uint8*)Wrapper) + sizeof(FLuaStructWrapper);
	if (InBuff != nullptr)
	{
		InStruct->CopyScriptStruct(ValuePtr, InBuff);
	}
	else 
	{
		InStruct->InitializeDefaultValue(ValuePtr);
	}

	if (lua_rawgetp(InL, LUA_REGISTRYINDEX, InStruct) == LUA_TTABLE)
	{
		lua_setmetatable(InL, -2);
	}
	else
	{
		lua_pop(InL, 1);
		RegisterStruct(InL, InStruct);
		if (lua_rawgetp(InL, LUA_REGISTRYINDEX, InStruct) == LUA_TTABLE)
		{
			lua_setmetatable(InL, -2);
		}
		else
		{
			lua_pop(InL, 1);
		}
	}
}

int32 FLuaStructWrapper::StructNew(lua_State* InL)
{
	UScriptStruct* StructClass = (UScriptStruct*)lua_touserdata(InL, lua_upvalueindex(1));
	if (StructClass == nullptr)
	{
		return 0;
	}
	PushStruct(InL, StructClass, nullptr);

	return 1;
}

int32 FLuaStructWrapper::StructGC(lua_State* InL)
{
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, -1);

	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Struct)
	{
		Wrapper->~FLuaStructWrapper();
	}

	return 0;
}

int32 FLuaStructWrapper::StructToString(lua_State* InL)
{
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Struct)
	{
		lua_pushnil(InL);
		return 1;
	}

	FString ObjName = Wrapper->StructType->GetName();
	lua_pushstring(InL, TCHAR_TO_UTF8(*ObjName));
	return 1;
}

bool FLuaStructWrapper::RegisterStruct(lua_State* InL, UScriptStruct* InStruct)
{
	bool bResult = false;
	if (InL == nullptr || InStruct == nullptr)
	{
		return false;
	}

	int32 ValueType = lua_rawgetp(InL, LUA_REGISTRYINDEX, InStruct);
	lua_pop(InL, 1);
	if (ValueType == LUA_TTABLE)
	{

		return true;
	}

	int32 tp = lua_gettop(InL);

	UScriptStruct* SuperClass = (UScriptStruct*)InStruct->GetSuperStruct();
	if (SuperClass)
	{
		RegisterStruct(InL, SuperClass);
	}

	lua_newtable(InL);
	{
		lua_pushvalue(InL, -1);
		lua_rawsetp(InL, LUA_REGISTRYINDEX, InStruct);

		lua_pushvalue(InL, -1);
		lua_setfield(InL, -2, "__index");

		lua_pushcfunction(InL, FLuaStructWrapper::StructToString);
		lua_setfield(InL, -2, "__tostring");

		lua_pushlightuserdata(InL, InStruct);
		lua_pushcclosure(InL, FLuaStructWrapper::StructNew, 1);
		lua_setfield(InL, -2, "__call");

		if ((InStruct->StructFlags & (EStructFlags::STRUCT_IsPlainOldData | EStructFlags::STRUCT_NoDestructor)) == 0)
		{
			lua_pushcfunction(InL, FLuaStructWrapper::StructGC);
			lua_setfield(InL, -2, "__gc");
		}

		if (lua_rawgetp(InL, LUA_REGISTRYINDEX, SuperClass) == LUA_TTABLE)
		{
			lua_setmetatable(InL, -2);
		}
		else
		{
			lua_pop(InL, 1);
		}
	}

	for (TFieldIterator<FProperty>It(InStruct, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		FString PropName = It->GetName();

		FString GetPropName = FString("Get") + PropName;
		{
			lua_pushlightuserdata(InL, *It);
			lua_pushcclosure(InL, FLuaStructWrapper::StructIndex, 1);
			lua_setfield(InL, -2, TCHAR_TO_UTF8(*GetPropName));
		}

		FString SetPropName = FString("Set") + PropName;
		{
			lua_pushlightuserdata(InL, *It);
			lua_pushcclosure(InL, FLuaStructWrapper::StructNewIndex, 1);
			lua_setfield(InL, -2, TCHAR_TO_UTF8(*SetPropName));
		}
	}

	lua_settop(InL, tp);

	return bResult;
}


int FLuaStructWrapper::StructIndex(lua_State* InL)
{
	SCOPE_CYCLE_COUNTER(STAT_PushToLua);
	FProperty* Prop = (FProperty*)lua_touserdata(InL, lua_upvalueindex(1));
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, 1);

	void* StructAddr = nullptr;
	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Struct)
	{
		uint8* ValuePtr = ((uint8*)Wrapper) + sizeof(FLuaStructWrapper);
		StructAddr = ValuePtr;
	}

	if (StructAddr)
	{
		FastLuaHelper::PushProperty(InL, Prop, StructAddr);
	}
	else
	{
		lua_pushnil(InL);
	}

	return 1;
}

int FLuaStructWrapper::StructNewIndex(lua_State* InL)
{
	SCOPE_CYCLE_COUNTER(STAT_FetchFromLua);
	FProperty* Prop = (FProperty*)lua_touserdata(InL, lua_upvalueindex(1));
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, 1);

	void* StructAddr = nullptr;
	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Struct)
	{
		uint8* ValuePtr = ((uint8*)Wrapper) + sizeof(FLuaStructWrapper);
		StructAddr = ValuePtr;
	}

	if (StructAddr)
	{
		FastLuaHelper::FetchProperty(InL, Prop, StructAddr, 2);
	}

	return 0;
}