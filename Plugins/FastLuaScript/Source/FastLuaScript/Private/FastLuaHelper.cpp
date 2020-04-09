// Fill out your copyright notice in the Description page of Project Settings.

#include "FastLuaHelper.h"
#include "UObject/StructOnScope.h"
#include "CoreUObject.h"
#include "lua/lua.hpp"
#include "FastLuaUnrealWrapper.h"
#include "FastLuaScript.h"
#include "FastLuaDelegate.h"
#include "FastLuaStat.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "LuaDelegateWrapper.h"
#include "LuaObjectWrapper.h"
#include "ILuaWrapper.h"
#include "LuaStructWrapper.h"
//#include "LuaArrayWrapper.h"


FString FastLuaHelper::GetPropertyTypeName(const FProperty* InProp)
{
	FString TypeName;
	if (!InProp)
	{
		return TypeName;
	}

	TypeName = InProp->GetCPPType();

	if (const FClassProperty* ClassProp = CastField<FClassProperty>(InProp))
	{
		FString MetaClassName = ClassProp->MetaClass->GetName();
		FString MetaClassPrefix = ClassProp->MetaClass->GetPrefixCPP();
		TypeName = FString::Printf(TEXT("TSubclassOf<%s%s>"), *MetaClassPrefix, *MetaClassName);
	}
	else if (const FInterfaceProperty* InterfaceProp = CastField<FInterfaceProperty>(InProp))
	{
		FString MetaClassName = InterfaceProp->InterfaceClass->GetName();
		TypeName = FString::Printf(TEXT("TScriptInterface<I%s>"), *MetaClassName);
	}
	else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(InProp))
	{
		FString ElementTypeName = ArrayProp->Inner->GetCPPType();
		TypeName = FString::Printf(TEXT("TArray<%s>"), *ElementTypeName);
	}
	else if (const FSetProperty* SetProp = CastField<FSetProperty>(InProp))
	{
		FString ElementTypeName = SetProp->ElementProp->GetCPPType();
		TypeName = FString::Printf(TEXT("TSet<%s>"), *ElementTypeName);
	}
	else if (const FMapProperty* MapProp = CastField<FMapProperty>(InProp))
	{
		FString KeyTypeName = MapProp->KeyProp->GetCPPType();
		FString ValueTypeName = MapProp->ValueProp->GetCPPType();

		TypeName = FString::Printf(TEXT("TMap<%s, %s>"), *KeyTypeName, *ValueTypeName);
	}
	else if (const FWeakObjectProperty* WeakObjectProp = CastField<FWeakObjectProperty>(InProp))
	{
		FString MetaClassName = WeakObjectProp->PropertyClass->GetName();
		FString MetaClassPrefix = WeakObjectProp->PropertyClass->GetPrefixCPP();
		TypeName = FString::Printf(TEXT("TWeakObjectPtr<%s%s>"), *MetaClassPrefix, *MetaClassName);
	}
	else if (const FDelegateProperty* DelegateProp = CastField<FDelegateProperty>(InProp))
	{
		FString FuncName = DelegateProp->SignatureFunction->GetPrefixCPP() + DelegateProp->SignatureFunction->GetName();
		FString OutName = DelegateProp->SignatureFunction->GetOuter()->GetName();
		if (OutName.Len() > 3)
		{
			FuncName = OutName + FString("::") + FuncName;
		}
		TypeName = FuncName;
	}
	else if (const FMulticastDelegateProperty* MultiDelegateProp = CastField<FMulticastDelegateProperty>(InProp))
	{
		FString FuncName = MultiDelegateProp->SignatureFunction->GetPrefixCPP() + MultiDelegateProp->SignatureFunction->GetName();
		FString OutName = MultiDelegateProp->SignatureFunction->GetOuter()->GetName();
		if (OutName.Len() > 3)
		{
			FuncName = OutName + FString("::") + FuncName;
		}
		TypeName = FuncName;
	}
	else if (const FFieldPathProperty* FieldPathProp = CastField<FFieldPathProperty>(InProp))
	{
		FString MetaClassName = FieldPathProp->PropertyClass->GetName();
		//FString MetaClassPrefix = FieldPathProp->PropertyClass->GetPrefixCPP();
		TypeName = FString::Printf(TEXT("%s"), *MetaClassName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid property type? %s"), *InProp->GetNameCPP());
	}
	return TypeName;
}

void FastLuaHelper::PushProperty(lua_State* InL, const FProperty* InProp, void* InBuff, int32 InArrayElementIndex)
{
	if (InProp == nullptr || InBuff == nullptr)
	{
		return;
	}

	if (const FNumericProperty* NumProp = CastField<FNumericProperty>(InProp))
	{
		if (NumProp->IsInteger())
		{
			lua_pushinteger(InL, NumProp->GetSignedIntPropertyValue(NumProp->ContainerPtrToValuePtr<void>(InBuff, InArrayElementIndex)));
		}
		else
		{
			lua_pushnumber(InL, NumProp->GetFloatingPointPropertyValue(NumProp->ContainerPtrToValuePtr<void>(InBuff, InArrayElementIndex)));
		}
	}
	else if (const FEnumProperty * EnumProp = CastField<FEnumProperty>(InProp))
	{
		const FNumericProperty* NumEnumProp = EnumProp->GetUnderlyingProperty();
		lua_pushinteger(InL, NumEnumProp ? NumEnumProp->GetSignedIntPropertyValue(InBuff) : 0);
	}
	else if (const FBoolProperty * BoolProp = CastField<FBoolProperty>(InProp))
	{
		lua_pushboolean(InL, BoolProp->GetPropertyValue_InContainer(InBuff));
	}
	else if (const FNameProperty * NameProp = CastField<FNameProperty>(InProp))
	{
		FName name = NameProp->GetPropertyValue_InContainer(InBuff);
		lua_pushstring(InL, TCHAR_TO_UTF8(*name.ToString()));
	}
	else if (const FStrProperty * StrProp = CastField<FStrProperty>(InProp))
	{
		const FString& str = StrProp->GetPropertyValue_InContainer(InBuff);
		lua_pushstring(InL, TCHAR_TO_UTF8(*str));
	}
	else if (const FTextProperty * TextProp = CastField<FTextProperty>(InProp))
	{
		const FText& text = TextProp->GetPropertyValue_InContainer(InBuff);
		lua_pushstring(InL, TCHAR_TO_UTF8(*text.ToString()));
	}
	else if (const FClassProperty * ClassProp = CastField<FClassProperty>(InProp))
	{
		FLuaObjectWrapper::PushObject(InL, ClassProp->GetPropertyValue_InContainer(InBuff));
	}
	else if (const FStructProperty * StructProp = CastField<FStructProperty>(InProp))
	{
		if (const UScriptStruct * ScriptStruct = Cast<UScriptStruct>(StructProp->Struct))
		{
			FLuaStructWrapper::PushStruct(InL, ScriptStruct, StructProp->ContainerPtrToValuePtr<void>(InBuff, InArrayElementIndex));
		}
	}
	else if (const FObjectProperty * ObjectProp = CastField<FObjectProperty>(InProp))
	{
		FLuaObjectWrapper::PushObject(InL, ObjectProp->GetObjectPropertyValue(ObjectProp->GetPropertyValuePtr_InContainer(InBuff, InArrayElementIndex)));
	}
	else if (const FDelegateProperty * DelegateProp = CastField<FDelegateProperty>(InProp))
	{
		void* ValuePtr = const_cast<TScriptDelegate<FWeakObjectPtr>*>(DelegateProp->GetPropertyValuePtr_InContainer(InBuff, InArrayElementIndex));
		FLuaDelegateWrapper::PushDelegate(InL, ValuePtr, false, DelegateProp->SignatureFunction);
	}
	else if (const FMulticastDelegateProperty * MultiDelegateProp = CastField<FMulticastDelegateProperty>(InProp))
	{
		void* ValuePtr = MultiDelegateProp->ContainerPtrToValuePtr<void>(InBuff, InArrayElementIndex);
		FLuaDelegateWrapper::PushDelegate(InL, ValuePtr, true, MultiDelegateProp->SignatureFunction);
	}
	else if (const FArrayProperty * ArrayProp = CastField<FArrayProperty>(InProp))
	{
		FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(InBuff));
		lua_newtable(InL);
		for (int32 i = 0; i < ArrayHelper.Num(); ++i)
		{
			PushProperty(InL, ArrayProp->Inner, ArrayHelper.GetRawPtr(i));
			lua_rawseti(InL, -2, i + 1);
		}
		//FLuaArrayWrapper::PushScriptArray(InL, ArrayProp->Inner, (FScriptArray*)ArrayProp->ContainerPtrToValuePtr<void>(InBuff));
	}
	else if (const FSetProperty * SetProp = CastField<FSetProperty>(InProp))
	{
		FScriptSetHelper SetHelper(SetProp, SetProp->ContainerPtrToValuePtr<void>(InBuff));
		lua_newtable(InL);
		for (int32 i = 0; i < SetHelper.Num(); ++i)
		{
			PushProperty(InL, SetHelper.GetElementProperty(), SetHelper.GetElementPtr(i));
			lua_rawseti(InL, -2, i + 1);
		}
	}
	else if (const FMapProperty * MapProp = CastField<FMapProperty>(InProp))
	{
		FScriptMapHelper MapHelper(MapProp, MapProp->ContainerPtrToValuePtr<void>(InBuff));
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

void FastLuaHelper::FetchProperty(lua_State* InL, const FProperty* InProp, void* InBuff, int32 InStackIndex, int32 InArrayElementIndex)
{
	//no enough params
	if (InBuff == nullptr || lua_gettop(InL) < lua_absindex(InL, InStackIndex))
	{
		return;
	}

	FString PropName = InProp->GetName();

	if (const FNumericProperty * NumProp = CastField<FNumericProperty>(InProp))
	{
		if (NumProp->IsInteger())
		{
			int64 TmpVal = lua_tointeger(InL, InStackIndex);
			NumProp->SetIntPropertyValue(NumProp->ContainerPtrToValuePtr<void>(InBuff), TmpVal);
		}
		else
		{
			float TmpVal = lua_tonumber(InL, InStackIndex);
			NumProp->SetFloatingPointPropertyValue(NumProp->ContainerPtrToValuePtr<void>(InBuff), TmpVal);
		}
	}
	else if (const FBoolProperty * BoolProp = CastField<FBoolProperty>(InProp))
	{
		BoolProp->SetPropertyValue_InContainer(InBuff, (bool)lua_toboolean(InL, InStackIndex));
	}
	else if (const FNameProperty * NameProp = CastField<FNameProperty>(InProp))
	{
		NameProp->SetPropertyValue_InContainer(InBuff, UTF8_TO_TCHAR(lua_tostring(InL, InStackIndex)));
	}
	else if (const FStrProperty * StrProp = CastField<FStrProperty>(InProp))
	{
		StrProp->SetPropertyValue_InContainer(InBuff, UTF8_TO_TCHAR(lua_tostring(InL, InStackIndex)));
	}
	else if (const FTextProperty * TextProp = CastField<FTextProperty>(InProp))
	{
		TextProp->SetPropertyValue_InContainer(InBuff, FText::FromString(UTF8_TO_TCHAR(lua_tostring(InL, InStackIndex))));
	}
	else if (const FEnumProperty * EnumProp = CastField<FEnumProperty>(InProp))
	{
		FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
		UnderlyingProp->SetIntPropertyValue(EnumProp->ContainerPtrToValuePtr<void>(InBuff), lua_tointeger(InL, InStackIndex));
	}
	else if (const FClassProperty * ClassProp = CastField<FClassProperty>(InProp))
	{
		//TODO check property
		ClassProp->SetPropertyValue_InContainer(InBuff, FLuaObjectWrapper::FetchObject(InL, InStackIndex, true));
	}
	else if (const FStructProperty * StructProp = CastField<FStructProperty>(InProp))
	{
		void* Data = FLuaStructWrapper::FetchStruct(InL, InStackIndex, StructProp->Struct->GetStructureSize());
		if (Data)
		{
			StructProp->Struct->CopyScriptStruct(StructProp->ContainerPtrToValuePtr<void>(InBuff), Data);
		}
	}
	else if (const FObjectProperty * ObjectProp = CastField<FObjectProperty>(InProp))
	{
		//TODO, check property
		ObjectProp->SetObjectPropertyValue_InContainer(InBuff, FLuaObjectWrapper::FetchObject(InL, InStackIndex));
	}
	else if (const FDelegateProperty * DelegateProp = CastField<FDelegateProperty>(InProp))
	{
		FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, InStackIndex);
		if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Delegate && !Wrapper->IsMulti())
		{
			DelegateProp->SetPropertyValue_InContainer(InBuff, *(FScriptDelegate*)Wrapper->GetDelegateValueAddr());
		}
	}
	else if (const FMulticastDelegateProperty * MultiDelegateProp = CastField<FMulticastDelegateProperty>(InProp))
	{
		FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, InStackIndex);
		if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Delegate && Wrapper->IsMulti())
		{
			MultiDelegateProp->SetMulticastDelegate(MultiDelegateProp->ContainerPtrToValuePtr<void>(InBuff), *(FMulticastScriptDelegate*)Wrapper->GetDelegateValueAddr());
		}
	}
	else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(InProp))
	{
		FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(InBuff));
		/*{
			FLuaArrayWrapper* Wrapper = FLuaArrayWrapper::FetchArrayWrapper(InL, InStackIndex);
			ArrayHelper.Resize(Wrapper->GetElementNum());
			for (int32 Idx = 0; Idx < Wrapper->GetElementNum(); ++Idx)
			{
				ArrayProp->Inner->CopySingleValue(ArrayHelper.GetRawPtr(Idx), Wrapper->GetElementAddr(Idx));
			}
		}*/

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
		FScriptSetHelper SetHelper(SetProp, SetProp->ContainerPtrToValuePtr<void>(InBuff));
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
		FScriptMapHelper MapHelper(MapProp, MapProp->ContainerPtrToValuePtr<void>(InBuff));
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
	//SCOPE_CYCLE_COUNTER(STAT_LuaCallBP);
	UFunction* Func = (UFunction*)lua_touserdata(InL, lua_upvalueindex(1));
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	UObject* Obj = nullptr;
	//TODO
	//if (Wrapper && Wrapper->GetWrapperType() == ELuaWrapperType::Object)
	{
		Obj = Wrapper->ObjInst.Get();
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


void FastLuaHelper::FixClassMetatable(lua_State* InL, TArray<const UClass*> InRegistedClassList)
{
	for (int32 i = 0; i < InRegistedClassList.Num(); ++i)
	{
		//fix metatable.__index = metatable
		lua_rawgetp(InL, LUA_REGISTRYINDEX, (const void*)InRegistedClassList[i]);
		if (lua_istable(InL, -1))
		{
			lua_pushvalue(InL, -1);
			lua_setfield(InL, -2, "__index");
		}
		else
		{
			//ERROR!!!
			lua_pop(InL, 1);
			continue;
		}

		UClass* SuperClass = InRegistedClassList[i]->GetSuperClass();
		if (SuperClass)
		{
			lua_rawgetp(InL, LUA_REGISTRYINDEX, (const void*)SuperClass);
			if (lua_istable(InL, -1))
			{
				lua_setmetatable(InL, -2);
			}
			else
			{
				lua_pop(InL, 1);
				//当前类的父类没有被导出
				lua_rawgeti(InL, LUA_REGISTRYINDEX, FLuaObjectWrapper::GetMetatableIndex());
				lua_setmetatable(InL, -2);
			}
		}
		else
		{
			//当前类没有父类，理论上只有UObject
			lua_rawgeti(InL, LUA_REGISTRYINDEX, FLuaObjectWrapper::GetMetatableIndex());
			lua_setmetatable(InL, -2);
		}

		lua_pop(InL, 1);
	}
}

int FastLuaHelper::LuaGetGameInstance(lua_State* InL)
{
	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
	lua_pop(InL, 1);
	UGameInstance* TmpGameInstance = LuaWrapper->GetGameInstance();
	if (TmpGameInstance == nullptr)
	{
		TmpGameInstance = GEngine->GetWorld()->GetGameInstance();
	}
	FLuaObjectWrapper::PushObject(InL, (UObject*)TmpGameInstance);
	return 1;
}

//local obj = Unreal.LuaLoadObject(Owner, ObjectPath)
int FastLuaHelper::LuaLoadObject(lua_State* InL)
{
	FString ObjectPath = UTF8_TO_TCHAR(lua_tostring(InL, 2));

	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	UObject* Owner = (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Object) ? Wrapper->GetObject() : nullptr;
	UObject* LoadedObj = FindObject<UObject>(ANY_PACKAGE, *ObjectPath);
	if (LoadedObj == nullptr)
	{
		LoadedObj = LoadObject<UObject>(Owner, *ObjectPath);
	}
	
	FLuaObjectWrapper::PushObject(InL, LoadedObj);

	return 1;
}

//local cls = Unreal.LuaLoadClass(Owner, ClassPath)
int FastLuaHelper::LuaLoadClass(lua_State* InL)
{
	UObject* ObjOuter = nullptr;
	FLuaObjectWrapper* OwnerWrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	if (OwnerWrapper && OwnerWrapper->WrapperType == ELuaWrapperType::Object)
	{
		ObjOuter = OwnerWrapper->ObjInst.Get();
	}

	FString ClassName = UTF8_TO_TCHAR(lua_tostring(InL, 2));

	UClass* ObjClass = FindObject<UClass>(ANY_PACKAGE, *ClassName);
	if (ObjClass == nullptr)
	{
		int32 Pos = ClassName.Find(FString("'/"));
		if (Pos > 1)
		{
			ClassName = ClassName.Mid(Pos);
			ClassName.ReplaceInline(*FString("'"), *FString(""));
			if (!ClassName.EndsWith(FString("_C"), ESearchCase::CaseSensitive))
			{
				ClassName += FString("_C");
			}
		}

		LoadObject<UClass>(ObjOuter, *ClassName);
	}
	
	FLuaObjectWrapper::PushObject(InL, ObjClass);
	return 1;

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

	UE_LOG(LogTemp, Warning, TEXT("%s"), *StringToPrint);
	return 0;
}


int FastLuaHelper::RegisterTickFunction(lua_State* InL)
{

	if (InL && lua_isfunction(InL, 1))
	{
		lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
		FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
		lua_pop(InL, 1);

		int32 RefIndex = luaL_ref(InL, LUA_REGISTRYINDEX);
		if (RefIndex && LuaWrapper)
		{
			LuaWrapper->SetLuaTickFunction(RefIndex);
		}

	}
	else
	{
		lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
		FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
		lua_pop(InL, 1);
		if (LuaWrapper->GetLuaTickFunction() > 0)
		{
			luaL_unref(InL, LUA_REGISTRYINDEX, LuaWrapper->GetLuaTickFunction());
		}
		LuaWrapper->SetLuaTickFunction(0);
	}

	lua_pushboolean(InL, true);
	return 1;
}
