// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaStructWrapper.h"
#include "FastLuaUnrealWrapper.h"
#include "FastLuaHelper.h"
#include "lua/lua.hpp"

void FLuaStructWrapper::InitWrapperMetatable(lua_State* InL)
{
	int32 MetatableIndexStruct = GetMetatableIndex();
	static const luaL_Reg GlueFuncs[] =
	{
		{"__index", FLuaStructWrapper::IndexInStruct},
		{"__newindex", FLuaStructWrapper::NewIndexInStruct},
		{"__gc", FLuaStructWrapper::StructGC},
		{"__tostring", FLuaStructWrapper::StructToString},
		{nullptr, nullptr},
	};

	lua_rawgeti(InL, LUA_REGISTRYINDEX, MetatableIndexStruct);
	if (lua_istable(InL, -1))
	{
		luaL_unref(InL, LUA_REGISTRYINDEX, MetatableIndexStruct);
	}
	lua_pop(InL, 1);

	int32 tp = lua_gettop(InL);
	luaL_newlib(InL, GlueFuncs);
	lua_rawseti(InL, LUA_REGISTRYINDEX, MetatableIndexStruct);
	lua_settop(InL, tp);
}

void* FLuaStructWrapper::FetchStruct(lua_State* InL, int32 InIndex, int32 InDesiredSize)
{
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, InIndex);
	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Struct && Wrapper->StructType->GetStructureSize() >= InDesiredSize)
	{
		return Wrapper->StructInst;
	}

	return nullptr;
}

void FLuaStructWrapper::PushStruct(lua_State* InL, const UScriptStruct* InStruct, const void* InBuff)
{
	if (InStruct == nullptr)
	{
		lua_pushnil(InL);
		return;
	}

	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_newuserdata(InL, sizeof(FLuaStructWrapper));
	new(Wrapper) FLuaStructWrapper(InStruct);

	UScriptStruct::ICppStructOps* TheCppStructOps = InStruct->GetCppStructOps();
	int32 AlignSize = 0;
	if (TheCppStructOps)
	{
		AlignSize = TheCppStructOps->GetAlignment();
	}
	Wrapper->StructInst = (uint8*)FMemory::Malloc(InStruct->GetStructureSize(), AlignSize);
	InStruct->InitializeDefaultValue(Wrapper->StructInst);

	if (InBuff != nullptr)
	{
		InStruct->CopyScriptStruct(Wrapper->StructInst, InBuff);
	}
	else if (lua_istable(InL, 2))
	{
		int32 tp = lua_gettop(InL);
		for (TFieldIterator<FProperty> It(InStruct); It; ++It)
		{
			FProperty* Prop = *It;
			lua_getfield(InL, 2, TCHAR_TO_UTF8(*It->GetName()));
			FastLuaHelper::FetchProperty(InL, Prop, Wrapper->StructInst, -1, 0);
			lua_pop(InL, 1);
		}
		lua_settop(InL, tp);
	}
	//SCOPE_CYCLE_COUNTER(STAT_FindStructMetatable);

	lua_rawgeti(InL, LUA_REGISTRYINDEX, GetMetatableIndex());
	lua_setmetatable(InL, -2);
}

int32 FLuaStructWrapper::IndexInStruct(lua_State* InL)
{
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Struct)
	{
		return 0;
	}

	FString TmpName = UTF8_TO_TCHAR(lua_tostring(InL, 2));
	FProperty* TmpProp = Wrapper->StructType->FindPropertyByName(*TmpName);
	if (TmpProp == nullptr)
	{
		return 0;
	}

	FastLuaHelper::PushProperty(InL, TmpProp, Wrapper->StructInst);

	return 1;
}

int32 FLuaStructWrapper::NewIndexInStruct(lua_State* InL)
{
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Struct)
	{
		return 0;
	}

	FString TmpName = UTF8_TO_TCHAR(lua_tostring(InL, 2));
	FProperty* TmpProp = Wrapper->StructType->FindPropertyByName(*TmpName);
	if (TmpProp == nullptr)
	{
		return 0;
	}

	FastLuaHelper::FetchProperty(InL, TmpProp, Wrapper->StructInst, 3);

	return 0;
}

int FLuaStructWrapper::LuaNewStruct(lua_State* InL)
{
	FString StructName = UTF8_TO_TCHAR(lua_tostring(InL, 1));
	UScriptStruct* StructClass = FindObject<UScriptStruct>(ANY_PACKAGE, *StructName);
	PushStruct(InL, StructClass, nullptr);

	return 1;
}

int FLuaStructWrapper::StructGC(lua_State* InL)
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
