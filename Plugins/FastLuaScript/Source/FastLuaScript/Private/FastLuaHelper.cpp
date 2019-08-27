// Fill out your copyright notice in the Description page of Project Settings.

#include "FastLuaHelper.h"
#include "StructOnScope.h"
#include "CoreUObject.h"
#include "lua.hpp"
#include "FastLuaUnrealWrapper.h"
#include "FastLuaScript.h"

bool FastLuaHelper::HasScriptAccessibleField(const UStruct* InStruct)
{
	for (TFieldIterator<const UField> It(InStruct); It; ++It)
	{
		if (const UFunction* Func = Cast<UFunction>(*It))
		{
			if (IsScriptCallableFunction(Func))
			{
				return true;
			}
		}
		else if (const UProperty* Prop = Cast<UProperty>(*It))
		{
			if (IsScriptReadableProperty(Prop))
			{
				return true;
			}
		}
	}

	return false;
}

bool FastLuaHelper::IsScriptCallableFunction(const UFunction* InFunction)
{
	return InFunction && InFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable);
}

bool FastLuaHelper::IsScriptReadableProperty(const UProperty* InProperty)
{
	const uint64 ReadableFlags = CPF_BlueprintAssignable | CPF_BlueprintVisible | CPF_InstancedReference;
	return InProperty && InProperty->HasAnyPropertyFlags(ReadableFlags);
}

FString FastLuaHelper::GetPropertyTypeName(const UProperty* InProp)
{
	FString TypeName = FString("int32");

	if (const UFloatProperty* FloatProp = Cast<UFloatProperty>(InProp))
	{
		TypeName = FString("float");
	}
	else if (const UIntProperty* IntProp = Cast<UIntProperty>(InProp))
	{
		TypeName = FString("int32");
	}
	else if (const UInt64Property * IntProp = Cast<UInt64Property>(InProp))
	{
		TypeName = FString("int64");
	}
	else if (const UBoolProperty* BoolProp = Cast<UBoolProperty>(InProp))
	{
		TypeName = FString("bool");
	}
	else if (const UEnumProperty* EnumProp = Cast<UEnumProperty>(InProp))
	{
		TypeName = FString("uint8");
	}
	else if (const UNameProperty* NameProp = Cast<UNameProperty>(InProp))
	{
		TypeName = FString("FName");
	}
	else if (const UStrProperty* StrProp = Cast<UStrProperty>(InProp))
	{
		TypeName = FString("FString");
	}
	else if (const UTextProperty* TextProp = Cast<UTextProperty>(InProp))
	{
		TypeName = FString("FText");
	}
	else if (const UClassProperty* ClassProp = Cast<UClassProperty>(InProp))
	{
		FString MetaClassName = ClassProp->MetaClass->GetName();
		FString MetaClassPrefix = ClassProp->MetaClass->GetPrefixCPP();
		TypeName = FString::Printf(TEXT("TSubclassOf<%s%s>"), *MetaClassPrefix, *MetaClassName);
	}
	else if (const UObjectProperty* ObjectProp = Cast<UObjectProperty>(InProp))
	{
		FString MetaClassName = ObjectProp->PropertyClass->GetName();
		FString MetaClassPrefix = ObjectProp->PropertyClass->GetPrefixCPP();
		TypeName = FString::Printf(TEXT("%s%s"), *MetaClassPrefix, *MetaClassName);
	}
	else if (const UStructProperty* StructProp = Cast<UStructProperty>(InProp))
	{
		FString MetaClassName = StructProp->Struct->GetName();
		FString MetaClassPrefix = StructProp->Struct->GetPrefixCPP();
		TypeName = FString::Printf(TEXT("%s%s"), *MetaClassPrefix, *MetaClassName);
	}

	return TypeName;
}

void FastLuaHelper::PushProperty(lua_State* InL, UProperty* InProp, void* InBuff, bool bRef /*= true*/)
{
	if (InProp == nullptr || InProp->GetClass() == nullptr)
	{
		LuaLog(FString("Property is nil"));
		return;
	}
	if (const UIntProperty* IntProp = Cast<UIntProperty>(InProp))
	{
		lua_pushinteger(InL, IntProp->GetPropertyValue_InContainer(InBuff));
	}
	else if (const UFloatProperty* FloatProp = Cast<UFloatProperty>(InProp))
	{
		lua_pushnumber(InL, FloatProp->GetPropertyValue_InContainer(InBuff));
	}
	else if (const UEnumProperty* EnumProp = Cast<UEnumProperty>(InProp))
	{
		const UNumericProperty* NumProp = EnumProp->GetUnderlyingProperty();
		lua_pushinteger(InL, NumProp ? NumProp->GetSignedIntPropertyValue(InBuff) : 0);
	}
	else if (const UBoolProperty* BoolProp = Cast<UBoolProperty>(InProp))
	{
		lua_pushboolean(InL, BoolProp->GetPropertyValue_InContainer(InBuff));
	}
	else if (const UNameProperty* NameProp = Cast<UNameProperty>(InProp))
	{
		FName name = NameProp->GetPropertyValue_InContainer(InBuff);
		lua_pushstring(InL, TCHAR_TO_UTF8(*name.ToString()));
	}
	else if (const UStrProperty* StrProp = Cast<UStrProperty>(InProp))
	{
		const FString& str = StrProp->GetPropertyValue_InContainer(InBuff);
		lua_pushstring(InL, TCHAR_TO_UTF8(*str));
	}
	else if (const UTextProperty* TextProp = Cast<UTextProperty>(InProp))
	{
		const FText& text = TextProp->GetPropertyValue_InContainer(InBuff);
		lua_pushstring(InL, TCHAR_TO_UTF8(*text.ToString()));
	}
	else if (const UClassProperty* ClassProp = Cast<UClassProperty>(InProp))
	{
		PushObject(InL, ClassProp->GetPropertyValue_InContainer(InBuff));
	}
	else if (const UStructProperty* StructProp = Cast<UStructProperty>(InProp))
	{
		if (const UScriptStruct* ScriptStruct = Cast<UScriptStruct>(StructProp->Struct))
		{
			PushStruct(InL, ScriptStruct, StructProp->ContainerPtrToValuePtr<void>(InBuff));
		}
	}
	else if (const UObjectProperty* ObjectProp = Cast<UObjectProperty>(InProp))
	{
		PushObject(InL, ObjectProp->GetObjectPropertyValue(ObjectProp->GetPropertyValuePtr_InContainer(InBuff)));
	}
	else if (UDelegateProperty* DelegateProp = Cast<UDelegateProperty>(InProp))
	{
		PushDelegate(InL, DelegateProp, InBuff, false);
	}
	else if (UMulticastDelegateProperty* MultiDelegateProp = Cast<UMulticastDelegateProperty>(InProp))
	{
		PushDelegate(InL, MultiDelegateProp, InBuff, true);
	}
	else if (const UArrayProperty* ArrayProp = Cast<UArrayProperty>(InProp))
	{
		FScriptArrayHelper ArrayHelper(ArrayProp, InBuff);
		lua_newtable(InL);
		for (int32 i = 0; i < ArrayHelper.Num(); ++i)
		{
			PushProperty(InL, ArrayProp->Inner, ArrayHelper.GetRawPtr(i), true);
			lua_rawseti(InL, -2, i);
		}
	}
	else if (const USetProperty* SetProp = Cast<USetProperty>(InProp))
	{
		FScriptSetHelper SetHelper(SetProp, InBuff);
		lua_newtable(InL);
		for (int32 i = 0; i < SetHelper.Num(); ++i)
		{
			PushProperty(InL, SetHelper.GetElementProperty(), SetHelper.GetElementPtr(i), true);
			lua_rawseti(InL, -2, i);
		}
	}
	else if (const UMapProperty* MapProp = Cast<UMapProperty>(InProp))
	{
		FScriptMapHelper MapHelper(MapProp, InBuff);
		lua_newtable(InL);
		for (int32  i = 0; i < MapHelper.Num(); ++i)
		{
			uint8* PairPtr = MapHelper.GetPairPtr(i);
			PushProperty(InL, MapProp->KeyProp, PairPtr, true);
			PushProperty(InL, MapProp->ValueProp, PairPtr, true);
			lua_rawset(InL, -3);
		}
	}
}

FString FastLuaHelper::GetFetchPropertyStr(const UProperty* InProp, const FString& InParamName, int32 InStackIndex /*= -1*/)
{
	//SCOPE_CYCLE_COUNTER(STAT_FetchFromLua);
	//no enough params

	FString BodyStr;

	FString PropName = InProp->GetName();

	if (const UIntProperty* IntProp = Cast<UIntProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("int32 %s = lua_tointeger(InL, %d);"), *InParamName, InStackIndex);
	}
	else if (const UInt64Property * Int64Prop = Cast<UInt64Property>(InProp))
	{
		BodyStr = FString::Printf(TEXT("int32 %s = lua_tointeger(InL, %d);"), *InParamName, InStackIndex);
	}
	else if (const UFloatProperty* FloatProp = Cast<UFloatProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("float %s = lua_tonumber(InL, %d);"), *InParamName, InStackIndex);
	}
	else if (const UBoolProperty* BoolProp = Cast<UBoolProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("int32 %s = lua_tointeger(InL, %d);"), *InParamName, InStackIndex);
	}
	else if (const UNameProperty* NameProp = Cast<UNameProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FName %s = FName(UTF8_TO_TCHAR(lua_tostring(InL, %d)));\n"), *InParamName, InStackIndex);
	}
	else if (const UStrProperty* StrProp = Cast<UStrProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FString %s = UTF8_TO_TCHAR(lua_tostring(InL, %d));\n"), *InParamName, InStackIndex);
	}
	else if (const UTextProperty* TextProp = Cast<UTextProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FText %s = FText::FromString(UTF8_TO_TCHAR(lua_tostring(InL, %d)));\n"), *InParamName, InStackIndex);
	}
	else if (const UByteProperty* ByteProp = Cast<UByteProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("uint8 %s = lua_tointeger(InL, %d);"), *InParamName, InStackIndex);
	}
	else if (const UEnumProperty* EnumProp = Cast<UEnumProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("uint8 %s = lua_tointeger(InL, %d);"), *InParamName, InStackIndex);
	}
	else if (const UClassProperty* ClassProp = Cast<UClassProperty>(InProp))
	{
		FString MetaClassName = ClassProp->MetaClass->GetName();
		FString MetaClassPrefix = ClassProp->MetaClass->GetPrefixCPP();
		BodyStr = FString::Printf(TEXT("FLuaObjectWrapper* %s_Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, %d);\tTSubclassOf<%s%s> %s = (UClass*)%s_Wrapper->ObjInst.Get();"), *InParamName, InStackIndex, *MetaClassPrefix, *MetaClassName, *InParamName, *InParamName);
	}
	else if (const UStructProperty* StructProp = Cast<UStructProperty>(InProp))
	{
		FString MetaClassName = StructProp->Struct->GetName();
		FString MetaClassPrefix = StructProp->Struct->GetPrefixCPP();
		
		BodyStr = FString::Printf(TEXT("FLuaStructWrapper* %s_Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, %d); \t%s%s %s; \t%s_Wrapper->StructType->CopyScriptStruct(&%s, &(%s_Wrapper->StructInst));"), *InParamName, InStackIndex, *MetaClassPrefix, *MetaClassName, *InParamName, *InParamName, *InParamName, *InParamName);
	}
	else if (const UObjectProperty* ObjectProp = Cast<UObjectProperty>(InProp))
	{
		FString MetaClassName = ObjectProp->PropertyClass->GetName();
		FString MetaClassPrefix = ObjectProp->PropertyClass->GetPrefixCPP();
		BodyStr = FString::Printf(TEXT("FLuaObjectWrapper* %s_Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, %d);\t%s%s* %s = Cast<%s%s>(%s_Wrapper->ObjInst.Get());"), *InParamName, InStackIndex, *MetaClassPrefix, *MetaClassName, *InParamName, *MetaClassPrefix, *MetaClassName, *InParamName);
	}
	/*else if (const UDelegateProperty* DelegateProp = Cast<UDelegateProperty>(InProp))
	{
		FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, InStackIndex);
		if (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Delegate && Wrapper->DelegateInst && Wrapper->bIsMulti == false)
		{
			DelegateProp->SetPropertyValue_InContainer(InBuff, *(FScriptDelegate*)Wrapper->DelegateInst);
		}
	}
	else if (const UMulticastDelegateProperty* MultiDelegateProp = Cast<UMulticastDelegateProperty>(InProp))
	{
		FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, InStackIndex);
		if (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Delegate && Wrapper->DelegateInst && Wrapper->bIsMulti)
		{
			MultiDelegateProp->SetMulticastDelegate(InBuff, *(FMulticastScriptDelegate*)Wrapper->DelegateInst);
		}
	}*/
	else if (const UArrayProperty* ArrayProp = Cast<UArrayProperty>(InProp))
	{
		FString ElementTypeName = GetPropertyTypeName(ArrayProp->Inner);
		FString NewElement = FString::Printf(TEXT("%s"), *FastLuaHelper::GetFetchPropertyStr(ArrayProp->Inner, FString::Printf(TEXT("Temp_NewElement")), -1));

		BodyStr = FString::Printf(TEXT("TArray<%s> %s;\n\t{\n\tif(lua_istable(InL, %d))\n\t{\n\t\tlua_pushnil(InL);\n\twhile(lua_next(InL, -2))\n\t{\n\t %s \n\t%s.Add(Temp_NewElement);\n\tlua_pop(InL, 1); \n\t}\n\t}  \n\t}"), *ElementTypeName, *InParamName, InStackIndex, *NewElement, *InParamName);
	}
	//else if (const USetProperty* SetProp = Cast<USetProperty>(InProp))
	//{
	//	FScriptSetHelper SetHelper(SetProp, InBuff);
	//	int32 i = 0;
	//	
	//	lua_pushnil(InL);
	//	while (lua_next(InL, -2))
	//	{
	//		if (SetHelper.Num() < i + 1)
	//		{
	//			SetHelper.AddDefaultValue_Invalid_NeedsRehash();
	//		}

	//		//GetFetchPropertyStr(SetHelper.ElementProp, TODO, -1);
	//		++i;
	//		lua_pop(InL, 1);
	//	}

	//	SetHelper.Rehash();
	//}
	//else if (const UMapProperty* MapProp = Cast<UMapProperty>(InProp))
	//{
	//	FScriptMapHelper MapHelper(MapProp, InBuff);
	//	int32 i = 0;
	//	
	//	lua_pushnil(InL);
	//	while (lua_next(InL, -2))
	//	{
	//		if (MapHelper.Num() < i + 1)
	//		{
	//			MapHelper.AddDefaultValue_Invalid_NeedsRehash();
	//		}

	//		//GetFetchPropertyStr(MapProp->KeyProp, TODO, -2);
	//		//GetFetchPropertyStr(MapProp->ValueProp, TODO, -1);
	//		++i;
	//		lua_pop(InL, 1);
	//	}

	//	MapHelper.Rehash();
	//}

	return BodyStr;

}

UObject* FastLuaHelper::FetchObject(lua_State* InL, int32 InIndex)
{
	InIndex = lua_absindex(InL, InIndex);

	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, InIndex);
	if (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Object)
	{
		return Wrapper->ObjInst.Get();
	}

	return nullptr;
}

void FastLuaHelper::PushObject(lua_State* InL, UObject* InObj)
{
	if (InObj == nullptr)
	{
		lua_pushnil(InL);
		return;
	}

	{
		FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_newuserdata(InL, sizeof(FLuaObjectWrapper));
		Wrapper->WrapperType = ELuaUnrealWrapperType::Object;
		Wrapper->ObjInst = InObj;

		const UClass* Class = InObj->GetClass();

		//SCOPE_CYCLE_COUNTER(STAT_FindClassMetatable);
		lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
		FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
		lua_pop(InL, 1);
		lua_rawgeti(InL, LUA_REGISTRYINDEX, LuaWrapper->ClassMetatableIdx);
		lua_rawgetp(InL, -1, Class);
		if (lua_istable(InL, -1))
		{
			lua_setmetatable(InL, -3);
		}
		else
		{
			lua_pop(InL, 1);
		}

		lua_pop(InL, 1);
	}
}

void* FastLuaHelper::FetchStruct(lua_State* InL, int32 InIndex)
{
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, InIndex);
	if (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Struct)
	{
		return &(Wrapper->StructInst);
	}

	return nullptr;
}

void FastLuaHelper::PushStruct(lua_State* InL, const UScriptStruct* InStruct, const void* InBuff)
{
	const int32 StructSize = FMath::Max(InStruct->GetStructureSize(), 1);
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_newuserdata(InL, sizeof(FLuaStructWrapper) + StructSize);
	Wrapper->WrapperType = ELuaUnrealWrapperType::Struct;
	Wrapper->StructType = InStruct;

	InStruct->CopyScriptStruct(&(Wrapper->StructInst), InBuff);

	//SCOPE_CYCLE_COUNTER(STAT_FindStructMetatable);
	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
	lua_pop(InL, 1);
	lua_rawgeti(InL, LUA_REGISTRYINDEX, LuaWrapper->StructMetatableIdx);
	lua_rawgetp(InL,-1, InStruct);
	if (lua_istable(InL, -1))
	{
		lua_setmetatable(InL, -3);
		lua_pop(InL, 1);
	}
	else
	{
		lua_pop(InL, 2);
	}
}

void FastLuaHelper::PushDelegate(lua_State* InL, void* InDelegateProperty, void* InBuff, bool InMulti)
{
	bool bValid = false;
	if (InMulti == false)
	{
		FScriptDelegate* Delegate = ((UDelegateProperty*)InDelegateProperty)->GetPropertyValuePtr_InContainer(InBuff);
		if (Delegate)
		{
			FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_newuserdata(InL, sizeof(FLuaDelegateWrapper));
			Wrapper->WrapperType = ELuaUnrealWrapperType::Delegate;
			Wrapper->bIsMulti = false;
			Wrapper->DelegateInst = Delegate;
			Wrapper->FunctionSignature = ((UDelegateProperty*)InDelegateProperty)->SignatureFunction;
			bValid = true;
		}
	}
	else
	{
		FMulticastScriptDelegate* Delegate = ((UMulticastDelegateProperty*)InDelegateProperty)->ContainerPtrToValuePtr<FMulticastScriptDelegate>(InBuff);
		if (Delegate)
		{
			FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_newuserdata(InL, sizeof(FLuaDelegateWrapper));
			Wrapper->WrapperType = ELuaUnrealWrapperType::Delegate;
			Wrapper->bIsMulti = true;
			Wrapper->DelegateInst = Delegate;
			Wrapper->FunctionSignature = ((UMulticastDelegateProperty*)InDelegateProperty)->SignatureFunction;
			bValid = true;
		}
	}

	if (bValid)
	{
		lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
		FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
		lua_pop(InL, 1);
		lua_rawgeti(InL, LUA_REGISTRYINDEX, LuaWrapper->DelegateMetatableIndex);
		lua_setmetatable(InL, -2);
	}
}

int32 FastLuaHelper::GetObjectProperty(lua_State* L)
{
	UProperty* Prop = (UProperty*)lua_touserdata(L, lua_upvalueindex(1));
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(L, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaUnrealWrapperType::Object)
	{
		return 0;
	}

	PushProperty(L, Prop, Wrapper->ObjInst.Get(), !Prop->HasAnyPropertyFlags(CPF_BlueprintReadOnly));

	return 1;
}

int32 FastLuaHelper::SetObjectProperty(lua_State* L)
{
	UProperty* Prop = (UProperty*)lua_touserdata(L, lua_upvalueindex(1));
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(L, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaUnrealWrapperType::Object)
	{
		return 0;
	}

	
	//GetFetchPropertyStr(Prop, TODO, 2);

	return 0;
}

int32 FastLuaHelper::GetStructProperty(lua_State* InL)
{
	UProperty* Prop = (UProperty*)lua_touserdata(InL, lua_upvalueindex(1));
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaUnrealWrapperType::Struct)
	{
		return 0;
	}

	PushProperty(InL, Prop, &(Wrapper->StructInst), !Prop->HasAnyPropertyFlags(CPF_BlueprintReadOnly));
	return 1;
}

int32 FastLuaHelper::SetStructProperty(lua_State* InL)
{
	UProperty* Prop = (UProperty*)lua_touserdata(InL, lua_upvalueindex(1));
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaUnrealWrapperType::Struct)
	{
		return 0;
	}

	//GetFetchPropertyStr(Prop, TODO, 2);

	return 0;
}


int32 FastLuaHelper::CallFunction(lua_State* L)
{
	//SCOPE_CYCLE_COUNTER(STAT_LuaCallBP);
	UFunction* Func = (UFunction*)lua_touserdata(L, lua_upvalueindex(1));
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(L, 1);
	UObject* Obj = nullptr;
	if (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Object)
	{
		Obj = Wrapper->ObjInst.Get();
	}
	int32 StackTop = 2;
	if (Obj == nullptr)
	{
		lua_pushnil(L);
		return 1;
	}

	if (Func->Children == nullptr)
	{
		Obj->ProcessEvent(Func, nullptr);
		return 0;
	}
	else
	{
		FStructOnScope FuncParam(Func);
		UProperty* ReturnProp = nullptr;

		for (TFieldIterator<UProperty> It(Func); It; ++It)
		{
			UProperty* Prop = *It;
			if (Prop->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				ReturnProp = Prop;
			}
			else
			{
				//FastLuaHelper::GetFetchPropertyStr(Prop, TODO, StackTop++);
			}
		}

		Obj->ProcessEvent(Func, FuncParam.GetStructMemory());

		int32 ReturnNum = 0;
		if (ReturnProp)
		{
			FastLuaHelper::PushProperty(L, ReturnProp, FuncParam.GetStructMemory(), false);
			++ReturnNum;
		}

		if (Func->HasAnyFunctionFlags(FUNC_HasOutParms))
		{
			for (TFieldIterator<UProperty> It(Func); It; ++It)
			{
				UProperty* Prop = *It;
				if (Prop->HasAnyPropertyFlags(CPF_OutParm) && !Prop->HasAnyPropertyFlags(CPF_ConstParm))
				{
					FastLuaHelper::PushProperty(L, *It, FuncParam.GetStructMemory(), false);
					++ReturnNum;
				}
			}
		}

		return ReturnNum;
	}

}

void* FastLuaHelper::LuaAlloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	FastLuaUnrealWrapper* Inst = (FastLuaUnrealWrapper*)ud;
	if (nsize == 0)
	{
		FMemory::Free(ptr);
		return nullptr;
	}
	else
	{
		ptr = FMemory::Realloc(ptr, nsize);
		if (Inst && Inst->bStatMemory && Inst->GetLuaSate())
		{
			int k = lua_gc(Inst->GetLuaSate(), LUA_GCCOUNT);
			int b = lua_gc(Inst->GetLuaSate(), LUA_GCCOUNTB);
			Inst->LuaMemory = (k << 10) + b;
			//SET_MEMORY_STAT(STAT_LuaMemory, Inst->LuaMemory);
		}
		return ptr;
	}
}

void FastLuaHelper::LuaLog(const FString& InLog, int32 InLevel, FastLuaUnrealWrapper* InLuaWrapper)
{
	FString Str = InLog;
	if (InLuaWrapper)
	{
		Str = FString("[") + InLuaWrapper->LuaStateName + FString("]") + Str;
	}
	switch (InLevel)
	{
	case 0 :
		UE_LOG(LogFastLuaScript, Log, TEXT("%s"), *Str);
		break;
	case 1:
		UE_LOG(LogFastLuaScript, Warning, TEXT("%s"), *Str);
		break;
	case 2:
		UE_LOG(LogFastLuaScript, Error, TEXT("%s"), *Str);
		break;
	default:
		break;
	}

}


int FastLuaHelper::LuaGetGameInstance(lua_State* InL)
{
	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
	lua_pop(InL, 1);
	FastLuaHelper::PushObject(InL, LuaWrapper->CachedGameInstance);
	return 1;
}

//LuaLoadObject(Owner, ObjectPath)
int FastLuaHelper::LuaLoadObject(lua_State* InL)
{
	int32 tp = lua_gettop(InL);
	if (tp < 2)
	{
		return 0;
	}

	FString ObjectPath = UTF8_TO_TCHAR(lua_tostring(InL, 2));

	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	UObject* Owner = (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Object) ? Wrapper->ObjInst.Get() : nullptr;
	UObject* LoadedObj = LoadObject<UObject>(Owner, *ObjectPath);
	if (LoadedObj == nullptr)
	{
		lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
		FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
		lua_pop(InL, 1);
		FastLuaHelper::LuaLog(FString::Printf(TEXT("LoadObject failed: %s"), *ObjectPath), 1, LuaWrapper);
		return 0;
	}
	FastLuaHelper::PushObject(InL, LoadedObj);

	return 1;
}

//LuaLoadClass(Owner, ClassPath)
int FastLuaHelper::LuaLoadClass(lua_State* InL)
{
	int32 tp = lua_gettop(InL);
	if (tp < 2)
	{
		return 0;
	}

	FString ClassPath = UTF8_TO_TCHAR(lua_tostring(InL, 2));

	if (UClass * FoundClass = FindObject<UClass>(ANY_PACKAGE, *ClassPath))
	{
		FastLuaHelper::PushObject(InL, FoundClass);
		return 1;
	}

	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	UObject* Owner = (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Object) ? Wrapper->ObjInst.Get() : nullptr;
	UClass* LoadedClass = LoadObject<UClass>(Owner, *ClassPath);
	if (LoadedClass == nullptr)
	{
		lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
		FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
		lua_pop(InL, 1);
		FastLuaHelper::LuaLog(FString::Printf(TEXT("LoadClass failed: %s"), *ClassPath), 1, LuaWrapper);
		return 0;
	}

	FastLuaHelper::PushObject(InL, LoadedClass);
	return 1;
}

//LuaNewObject(Owner, ClassName, ObjectName)
int FastLuaHelper::LuaNewObject(lua_State* InL)
{
	int ParamNum = lua_gettop(InL);
	if (ParamNum < 3)
	{
		//error
		return 0;
	}

	UObject* ObjOuter = nullptr;
	FLuaObjectWrapper* OwnerWrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	if (OwnerWrapper && OwnerWrapper->WrapperType == ELuaUnrealWrapperType::Object)
	{
		ObjOuter = OwnerWrapper->ObjInst.Get();
	}
	else
	{
		return 0;
	}

	FString ClassName = UTF8_TO_TCHAR(lua_tostring(InL, 2));


	FString ObjName = UTF8_TO_TCHAR(lua_tostring(InL, 3));
	UClass* ObjClass = FindObject<UClass>(ANY_PACKAGE, *ClassName);
	if (ObjClass == nullptr)
	{
		return 0;
	}
	UObject* NewObj = NewObject<UObject>(ObjOuter, ObjClass, FName(*ObjName));
	FastLuaHelper::PushObject(InL, NewObj);
	return 1;
}


//Lua usage: GameSingleton:GetGameEvent():GetOnPostLoadMap():Call(0)
int FastLuaHelper::LuaCallUnrealDelegate(lua_State* InL)
{
	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaUnrealWrapperType::Delegate || Wrapper->DelegateInst == nullptr)
	{
		return 0;
	}

	UFunction* SignatureFunction = Wrapper->FunctionSignature;

	int32 StackTop = 2;

	int32 ReturnNum = 0;
	//Fill parameters
	FStructOnScope FuncParam(SignatureFunction);
	UProperty* ReturnProp = nullptr;

	for (TFieldIterator<UProperty> It(SignatureFunction); It; ++It)
	{
		UProperty* Prop = *It;
		if (Prop->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			ReturnProp = Prop;
		}
		else
		{
			//FastLuaHelper::GetFetchPropertyStr(Prop, TODO, StackTop++);
		}
	}

	if (Wrapper->bIsMulti)
	{
		FMulticastScriptDelegate* MultiDelegate = (FMulticastScriptDelegate*)Wrapper->DelegateInst;
		MultiDelegate->ProcessMulticastDelegate<UObject>(FuncParam.GetStructMemory());
	}
	else
	{
		FScriptDelegate* SingleDelegate = (FScriptDelegate*)Wrapper->DelegateInst;
		SingleDelegate->ProcessDelegate<UObject>(FuncParam.GetStructMemory());
	}

	if (ReturnProp)
	{
		FastLuaHelper::PushProperty(InL, ReturnProp, FuncParam.GetStructMemory(), true);
		++ReturnNum;
	}

	if (SignatureFunction->HasAnyFunctionFlags(FUNC_HasOutParms))
	{
		for (TFieldIterator<UProperty> It(SignatureFunction); It; ++It)
		{
			UProperty* Prop = *It;
			if (Prop->HasAnyPropertyFlags(CPF_OutParm) && !Prop->HasAnyPropertyFlags(CPF_ConstParm))
			{
				FastLuaHelper::PushProperty(InL, *It, FuncParam.GetStructMemory(), false);
				++ReturnNum;
			}
		}
	}

	return ReturnNum;
}

//usage: Unreal.LuaGetUnrealCDO("KismetSystemLibrary")
int FastLuaHelper::LuaGetUnrealCDO(lua_State* InL)
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
		FastLuaHelper::PushObject(InL, Class->GetDefaultObject());
		return 1;
	}

	return 0;
}

int FastLuaHelper::PrintLog(lua_State* L)
{
	FString StringToPrint;
	int Num = lua_gettop(L);
	for (int i = 1; i <= Num; ++i)
	{
		StringToPrint.Append(UTF8_TO_TCHAR(lua_tostring(L, i)));
		if (Num > 1 && i < Num)
		{
			StringToPrint.Append(FString(","));
		}
	}

	lua_rawgetp(L, LUA_REGISTRYINDEX, L);
	FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	FastLuaHelper::LuaLog(FString::Printf(TEXT("%s"), *StringToPrint), 0, LuaWrapper);
	return 0;
}
