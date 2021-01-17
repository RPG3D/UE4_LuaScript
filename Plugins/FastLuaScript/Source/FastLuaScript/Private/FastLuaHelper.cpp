// Fill out your copyright notice in the Description page of Project Settings.

#include "FastLuaHelper.h"
#include "UObject/StructOnScope.h"
#include "CoreUObject.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include  "UObject/Class.h"

#include "LuaDelegateWrapper.h"
#include "LuaObjectWrapper.h"
#include "ILuaWrapper.h"
#include "LuaStructWrapper.h"

#include "FastLuaUnrealWrapper.h"
#include "FastLuaScript.h"
#include "LuaFunctionWrapper.h"
#include "FastLuaStat.h"
#include "lua.hpp"



void FastLuaHelper::PushProperty(lua_State* InL, const FProperty* InProp, void* InContainer)
{
	SCOPE_CYCLE_COUNTER(STAT_PushToLua);

	if (InProp == nullptr || InContainer == nullptr)
	{
		return;
	}

	if (const FNumericProperty* NumProp = CastField<FNumericProperty>(InProp))
	{
		if (NumProp->IsInteger())
		{
			lua_pushinteger(InL, NumProp->GetSignedIntPropertyValue(NumProp->ContainerPtrToValuePtr<void>(InContainer)));
		}
		else
		{
			lua_pushnumber(InL, NumProp->GetFloatingPointPropertyValue(NumProp->ContainerPtrToValuePtr<void>(InContainer)));
		}
	}
	else if (const FEnumProperty * EnumProp = CastField<FEnumProperty>(InProp))
	{
		const FNumericProperty* NumEnumProp = EnumProp->GetUnderlyingProperty();
		lua_pushinteger(InL, NumEnumProp ? NumEnumProp->GetSignedIntPropertyValue(InContainer) : 0);
	}
	else if (const FBoolProperty * BoolProp = CastField<FBoolProperty>(InProp))
	{
		lua_pushboolean(InL, BoolProp->GetPropertyValue_InContainer(InContainer));
	}
	else if (const FNameProperty * NameProp = CastField<FNameProperty>(InProp))
	{
		FName name = NameProp->GetPropertyValue_InContainer(InContainer);
		lua_pushstring(InL, TCHAR_TO_UTF8(*name.ToString()));
	}
	else if (const FStrProperty * StrProp = CastField<FStrProperty>(InProp))
	{
		const FString& str = StrProp->GetPropertyValue_InContainer(InContainer);
		lua_pushstring(InL, TCHAR_TO_UTF8(*str));
	}
	else if (const FTextProperty * TextProp = CastField<FTextProperty>(InProp))
	{
		const FText& text = TextProp->GetPropertyValue_InContainer(InContainer);
		lua_pushstring(InL, TCHAR_TO_UTF8(*text.ToString()));
	}
	else if (const FClassProperty * ClassProp = CastField<FClassProperty>(InProp))
	{
		FLuaObjectWrapper::PushObject(InL, ClassProp->GetPropertyValue_InContainer(InContainer));
	}
	else if (const FStructProperty * StructProp = CastField<FStructProperty>(InProp))
	{
		if (UScriptStruct * ScriptStruct = Cast<UScriptStruct>(StructProp->Struct))
		{
			FLuaStructWrapper::PushStruct(InL, ScriptStruct, StructProp->ContainerPtrToValuePtr<void>(InContainer));
		}
	}
	else if (const FObjectProperty * ObjectProp = CastField<FObjectProperty>(InProp))
	{
		FLuaObjectWrapper::PushObject(InL, ObjectProp->GetObjectPropertyValue(ObjectProp->GetPropertyValuePtr_InContainer(InContainer)));
	}
	else if (const FDelegateProperty * DelegateProp = CastField<FDelegateProperty>(InProp))
	{
		void* ValuePtr = const_cast<TScriptDelegate<FWeakObjectPtr>*>(DelegateProp->GetPropertyValuePtr_InContainer(InContainer));
		FLuaDelegateWrapper::PushDelegate(InL, ValuePtr, false, DelegateProp->SignatureFunction);
	}
	else if (const FMulticastDelegateProperty * MultiDelegateProp = CastField<FMulticastDelegateProperty>(InProp))
	{
		void* ValuePtr = MultiDelegateProp->ContainerPtrToValuePtr<void>(InContainer);
		FLuaDelegateWrapper::PushDelegate(InL, ValuePtr, true, MultiDelegateProp->SignatureFunction);
	}
	else if (const FArrayProperty * ArrayProp = CastField<FArrayProperty>(InProp))
	{
		FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(InContainer));
		lua_newtable(InL);
		for (int32 i = 0; i < ArrayHelper.Num(); ++i)
		{
			PushProperty(InL, ArrayProp->Inner, ArrayHelper.GetRawPtr(i));
			lua_rawseti(InL, -2, i + 1);
		}
	}
	else if (const FSetProperty * SetProp = CastField<FSetProperty>(InProp))
	{
		FScriptSetHelper SetHelper(SetProp, SetProp->ContainerPtrToValuePtr<void>(InContainer));
		lua_newtable(InL);
		for (int32 i = 0; i < SetHelper.Num(); ++i)
		{
			PushProperty(InL, SetHelper.GetElementProperty(), SetHelper.GetElementPtr(i));
			lua_rawseti(InL, -2, i + 1);
		}
	}
	else if (const FMapProperty * MapProp = CastField<FMapProperty>(InProp))
	{
		FScriptMapHelper MapHelper(MapProp, MapProp->ContainerPtrToValuePtr<void>(InContainer));
		lua_newtable(InL);
		for (int32 i = 0; i < MapHelper.Num(); ++i)
		{
			uint8* PairPtr = MapHelper.GetPairPtr(i);
			PushProperty(InL, MapProp->KeyProp, PairPtr);
			PushProperty(InL, MapProp->ValueProp, PairPtr);
			lua_rawset(InL, -3);
		}
	}
}

void FastLuaHelper::FetchProperty(lua_State* InL, const FProperty* InProp, void* InContainer, int32 InStackIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_FetchFromLua);

	//no enough params
	if (InContainer == nullptr || lua_gettop(InL) < lua_absindex(InL, InStackIndex))
	{
		return;
	}

	if (const FNumericProperty * NumProp = CastField<FNumericProperty>(InProp))
	{
		if (NumProp->IsInteger())
		{
			int64 TmpVal = lua_tointeger(InL, InStackIndex);
			NumProp->SetIntPropertyValue(NumProp->ContainerPtrToValuePtr<void>(InContainer), TmpVal);
		}
		else
		{
			float TmpVal = lua_tonumber(InL, InStackIndex);
			NumProp->SetFloatingPointPropertyValue(NumProp->ContainerPtrToValuePtr<void>(InContainer), TmpVal);
		}
	}
	else if (const FBoolProperty * BoolProp = CastField<FBoolProperty>(InProp))
	{
		BoolProp->SetPropertyValue_InContainer(InContainer, (bool)lua_toboolean(InL, InStackIndex));
	}
	else if (const FNameProperty * NameProp = CastField<FNameProperty>(InProp))
	{
		NameProp->SetPropertyValue_InContainer(InContainer, UTF8_TO_TCHAR(lua_tostring(InL, InStackIndex)));
	}
	else if (const FStrProperty * StrProp = CastField<FStrProperty>(InProp))
	{
		StrProp->SetPropertyValue_InContainer(InContainer, UTF8_TO_TCHAR(lua_tostring(InL, InStackIndex)));
	}
	else if (const FTextProperty * TextProp = CastField<FTextProperty>(InProp))
	{
		TextProp->SetPropertyValue_InContainer(InContainer, FText::FromString(UTF8_TO_TCHAR(lua_tostring(InL, InStackIndex))));
	}
	else if (const FEnumProperty * EnumProp = CastField<FEnumProperty>(InProp))
	{
		FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
		UnderlyingProp->SetIntPropertyValue(EnumProp->ContainerPtrToValuePtr<void>(InContainer), (int64)lua_tointeger(InL, InStackIndex));
	}
	else if (const FClassProperty * ClassProp = CastField<FClassProperty>(InProp))
	{
		//TODO check property
		ClassProp->SetPropertyValue_InContainer(InContainer, FLuaObjectWrapper::FetchObject(InL, InStackIndex, true));
	}
	else if (const FStructProperty * StructProp = CastField<FStructProperty>(InProp))
	{
		void* Data = FLuaStructWrapper::FetchStruct(InL, InStackIndex, StructProp->Struct);
		if (Data)
		{
			StructProp->Struct->CopyScriptStruct(StructProp->ContainerPtrToValuePtr<void>(InContainer), Data);
		}
	}
	else if (const FObjectProperty * ObjectProp = CastField<FObjectProperty>(InProp))
	{
		//TODO, check property
		ObjectProp->SetObjectPropertyValue_InContainer(InContainer, FLuaObjectWrapper::FetchObject(InL, InStackIndex));
	}
	else if (const FDelegateProperty * DelegateProp = CastField<FDelegateProperty>(InProp))
	{
		FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, InStackIndex);
		if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Delegate && !Wrapper->IsMulti())
		{
			DelegateProp->SetPropertyValue_InContainer(InContainer, *(FScriptDelegate*)Wrapper->GetDelegateValueAddr());
		}
	}
	else if (const FMulticastDelegateProperty * MultiDelegateProp = CastField<FMulticastDelegateProperty>(InProp))
	{
		FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, InStackIndex);
		if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Delegate && Wrapper->IsMulti())
		{
			MultiDelegateProp->SetMulticastDelegate(MultiDelegateProp->ContainerPtrToValuePtr<void>(InContainer), *(FMulticastScriptDelegate*)Wrapper->GetDelegateValueAddr());
		}
	}
	else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(InProp))
	{
		FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(InContainer));
		int32 i = 0;
		if (lua_istable(InL, InStackIndex))
		{
			lua_pushnil(InL);
			if (InStackIndex < 0)
			{
				--InStackIndex;
			}
			while (lua_next(InL, InStackIndex))
			{
				if (ArrayHelper.Num() < i + 1)
				{
					ArrayHelper.AddValue();
				}

				FetchProperty(InL, ArrayProp->Inner, ArrayHelper.GetRawPtr(i), -1);
				lua_pop(InL, 1);
				++i;
			}
		}
		
	}

	else if (const FSetProperty * SetProp = CastField<FSetProperty>(InProp))
	{
		FScriptSetHelper SetHelper(SetProp, SetProp->ContainerPtrToValuePtr<void>(InContainer));
		int32 i = 0;
		if (lua_istable(InL, InStackIndex))
		{
			lua_pushnil(InL);
			if (InStackIndex < 0)
			{
				--InStackIndex;
			}
			while (lua_next(InL, InStackIndex))
			{
				if (SetHelper.Num() < i + 1)
				{
					SetHelper.AddDefaultValue_Invalid_NeedsRehash();
				}

				FetchProperty(InL, SetHelper.ElementProp, SetHelper.GetElementPtr(i), -1);
				++i;
				lua_pop(InL, 1);
			}

		}
		
		SetHelper.Rehash();
	}
	else if (const FMapProperty * MapProp = CastField<FMapProperty>(InProp))
	{
		FScriptMapHelper MapHelper(MapProp, MapProp->ContainerPtrToValuePtr<void>(InContainer));
		int32 i = 0;
		if (lua_istable(InL, InStackIndex))
		{
			lua_pushnil(InL);
			if (InStackIndex < 0)
			{
				--InStackIndex;
			}
			while (lua_next(InL, InStackIndex))
			{
				if (MapHelper.Num() < i + 1)
				{
					MapHelper.AddDefaultValue_Invalid_NeedsRehash();
				}

				FetchProperty(InL, MapProp->KeyProp, MapHelper.GetPairPtr(i), -2);
				FetchProperty(InL, MapProp->ValueProp, MapHelper.GetPairPtr(i), -1);
				++i;
				lua_pop(InL, 1);
			}
		}

		MapHelper.Rehash();
	}

	return;
}

int32 FastLuaHelper::CallUnrealFunction(lua_State* InL)
{
	SCOPE_CYCLE_COUNTER(STAT_CallUnrealFunction);
	UFunction* Func = (UFunction*)lua_touserdata(InL, lua_upvalueindex(1));
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	UObject* Obj = nullptr;

	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Object)
	{
		Obj = Wrapper->GetObject();
	}
	int32 StackTop = 2;
	if (Obj == nullptr)
	{
		lua_pushnil(InL);
		return 1;
	}

	if (Func->NumParms < 1)
	{
		Obj->ProcessEvent(Func, nullptr);
		return 0;
	}
	else
	{
		FStructOnScope FuncParam(Func);

		FProperty* ReturnProp = nullptr;

		for (TFieldIterator<FProperty> It(Func); It; ++It)
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

		Obj->ProcessEvent(Func, FuncParam.GetStructMemory());

		int32 ReturnNum = 0;
		if (ReturnProp)
		{
			FastLuaHelper::PushProperty(InL, ReturnProp, FuncParam.GetStructMemory());
			++ReturnNum;
		}

		if (Func->HasAnyFunctionFlags(FUNC_HasOutParms))
		{
			for (TFieldIterator<FProperty> It(Func); It; ++It)
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

}

int FastLuaHelper::PrintLog(lua_State* L)
{
	FString StringToPrint;
	int Num = lua_gettop(L);
	for (int i = 1; i <= Num; ++i)
	{
		if (lua_isstring(L, i))
		{
			StringToPrint.Append(UTF8_TO_TCHAR(lua_tostring(L, i)));
		}
		else if (lua_isinteger(L, i) || lua_isboolean(L, i))
		{
			StringToPrint.Append(FString::Printf(TEXT("%d"), lua_tointeger(L, i)));
		}
		else if (lua_isnumber(L, i))
		{
			StringToPrint.Append(FString::Printf(TEXT("%f"), lua_tonumber(L, i)));
		}
		else if (lua_isuserdata(L, i) || lua_islightuserdata(L, i))
		{
			StringToPrint.Append(FString::Printf(TEXT("UserData:%d"), lua_touserdata(L, i)));
		}

		if (Num > 1 && i < Num)
		{
			StringToPrint.Append(FString(","));
		}
	}

	UE_LOG(LogFastLuaScript, Warning, TEXT("%s"), *StringToPrint);
	return 0;
}


