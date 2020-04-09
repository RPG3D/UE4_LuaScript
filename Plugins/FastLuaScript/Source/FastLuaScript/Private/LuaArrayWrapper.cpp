// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaArrayWrapper.h"
#include "Containers/ScriptArray.h"

#include "lua/lua.hpp"
#include "FastLuaHelper.h"

FProperty* CreatePropertyFromGenFlag(UE4CodeGen_Private::EPropertyGenFlags InGenFlag);

FLuaArrayWrapper::FLuaArrayWrapper(const FProperty* InElementProp, FScriptArray* InArrayData)
{
	ElementProp = InElementProp;
	ElementSize = ElementProp->GetSize();

	ArrayData = new FScriptArray();

	FScriptArrayHelper ArrayHelper = FScriptArrayHelper::CreateHelperFormInnerProperty(ElementProp, ArrayData);
	if (InArrayData)
	{
		ArrayHelper.Resize(InArrayData->Num());
		FScriptArrayHelper SrcArrayHelper = FScriptArrayHelper::CreateHelperFormInnerProperty(ElementProp, InArrayData);

		for (int32 Idx = 0; InArrayData && Idx < InArrayData->Num(); ++Idx)
		{
			ElementProp->CopySingleValue(ArrayHelper.GetRawPtr(Idx), SrcArrayHelper.GetRawPtr(Idx));
		}
	}
}

FLuaArrayWrapper::~FLuaArrayWrapper()
{
	for (int32 Idx = 0; Idx < ArrayData->Num(); ++Idx)
	{
		ElementProp->DestroyValue(GetElementAddr(Idx));
	}

	if (bRefElementProp)
	{
		delete ElementProp;
	}
	delete ArrayData;
}

void FLuaArrayWrapper::InitMetatableWrapper(lua_State* InL)
{
	int32 MetatableIndexArray = GetMetatableIndex();

	static const luaL_Reg GlueFuncs[] =
	{
		{"__len", FLuaArrayWrapper::LuaArrayLen},
		{"__index", FLuaArrayWrapper::LuaArrayIndex},
		{"__newindex", FLuaArrayWrapper::LuaArrayNewIndex},
		{"__gc", FLuaArrayWrapper::LuaArrayGC},

		{nullptr, nullptr},
	};

	lua_rawgeti(InL, LUA_REGISTRYINDEX, MetatableIndexArray);
	if (lua_istable(InL, -1))
	{
		luaL_unref(InL, LUA_REGISTRYINDEX, MetatableIndexArray);
	}
	lua_pop(InL, 1);

	int32 tp = lua_gettop(InL);
	luaL_newlib(InL, GlueFuncs);

	lua_rawseti(InL, LUA_REGISTRYINDEX, MetatableIndexArray);
	lua_settop(InL, tp);
}

int32 FLuaArrayWrapper::PushScriptArray(lua_State* InL, const FProperty* InElementProp, FScriptArray* InScriptArray)
{
	FLuaArrayWrapper* Wrapper = (FLuaArrayWrapper*)lua_newuserdata(InL, sizeof(FLuaArrayWrapper));
	new(Wrapper) FLuaArrayWrapper(InElementProp, InScriptArray);
	lua_rawgeti(InL, LUA_REGISTRYINDEX, GetMetatableIndex());
	lua_setmetatable(InL, -2);
	return 1;
}

FLuaArrayWrapper* FLuaArrayWrapper::FetchArrayWrapper(lua_State* InL, int32 InIndex)
{
	FLuaArrayWrapper* Wrapper = (FLuaArrayWrapper*)lua_touserdata(InL, InIndex);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Array)
	{
		return nullptr;
	}

	return Wrapper;
}

int32 FLuaArrayWrapper::LuaArrayLen(lua_State* InL)
{
	FLuaArrayWrapper* Wrapper = (FLuaArrayWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Array)
	{
		return 0;
	}

	lua_pushinteger(InL, Wrapper->GetElementNum());
	return 1;
}

int32 FLuaArrayWrapper::LuaArrayIndex(lua_State* InL)
{
	FLuaArrayWrapper* Wrapper = (FLuaArrayWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Array)
	{
		return 0;
	}

	int32 TmpIndex = lua_tointeger(InL, 2);
	FScriptArrayHelper ArrayHelper = FScriptArrayHelper::CreateHelperFormInnerProperty(Wrapper->ElementProp, Wrapper->ArrayData);
	FastLuaHelper::PushProperty(InL, Wrapper->ElementProp, ArrayHelper.GetRawPtr(TmpIndex));

	return 1;
}

int32 FLuaArrayWrapper::LuaArrayNewIndex(lua_State* InL)
{
	FLuaArrayWrapper* Wrapper = (FLuaArrayWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Array)
	{
		return 0;
	}

	int32 TmpIndex = lua_tointeger(InL, 2);

	FScriptArrayHelper ArrayHelper = FScriptArrayHelper::CreateHelperFormInnerProperty(Wrapper->ElementProp, Wrapper->ArrayData);

	if (ArrayHelper.Num() <= TmpIndex)
	{
		ArrayHelper.AddValue();
	}

	FastLuaHelper::FetchProperty(InL, Wrapper->ElementProp, ArrayHelper.GetRawPtr(TmpIndex), 3);

	return 1;
}

int32 FLuaArrayWrapper::LuaArrayGC(lua_State* InL)
{
	FLuaArrayWrapper* Wrapper = (FLuaArrayWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Array)
	{
		return 0;
	}

	Wrapper->~FLuaArrayWrapper();

	return 0;
}

int32 FLuaArrayWrapper::LuaArrayNew(lua_State* InL)
{
	using namespace UE4CodeGen_Private;
	EPropertyGenFlags ElementType = (EPropertyGenFlags)lua_tointeger(InL, 1);
	FLuaArrayWrapper* Wrapper = (FLuaArrayWrapper*)lua_newuserdata(InL, sizeof(FLuaArrayWrapper));

	Wrapper->ElementProp = CreatePropertyFromGenFlag(ElementType);

	Wrapper->ElementProp = const_cast<FProperty*>(Wrapper->ElementProp);
	new(Wrapper) FLuaArrayWrapper(Wrapper->ElementProp, nullptr);
	Wrapper->bRefElementProp = true;

	lua_rawgeti(InL, LUA_REGISTRYINDEX, GetMetatableIndex());
	lua_setmetatable(InL, -2);

	return 1;
}


//暂时定为全局函数，方便后续其他模块(Set, Map)复用
FProperty* CreatePropertyFromGenFlag(UE4CodeGen_Private::EPropertyGenFlags InGenFlag)
{
	FProperty* RetProp = nullptr;

	switch (InGenFlag)
	{
	case UE4CodeGen_Private::EPropertyGenFlags::Byte:
		RetProp = new FByteProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;
	case UE4CodeGen_Private::EPropertyGenFlags::Int8:
		RetProp = new FInt8Property(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::Int16:
		RetProp = new FInt16Property(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::Int:
		RetProp = new FIntProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::Int64:
		RetProp = new FInt64Property(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::UInt16:
		RetProp = new FUInt16Property(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::UInt32:
		RetProp = new FUInt32Property(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::UInt64:
		RetProp = new FUInt64Property(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::Float:
		RetProp = new FFloatProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::Double:
		RetProp = new FDoubleProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::Bool:
		RetProp = new FBoolProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::SoftClass:
		RetProp = new FSoftClassProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::WeakObject:
		RetProp = new FWeakObjectProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::SoftObject:
		RetProp = new FSoftObjectProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::Class:
		RetProp = new FClassProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::Object:
		RetProp = new FObjectProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::Name:
		RetProp = new FNameProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::Str:
		RetProp = new FStrProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::Struct:
		RetProp = new FStructProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::Delegate:
		RetProp = new FDelegateProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::Text:
		RetProp = new FTextProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	case UE4CodeGen_Private::EPropertyGenFlags::Enum:
		RetProp = new FEnumProperty(FFieldVariant(), FName("LuaArrayElement"), EObjectFlags::RF_Public | RF_Standalone | RF_Transient);
		break;

	}

	return RetProp;
}