// Fill out your copyright notice in the Description page of Project Settings.

#include "UnrealMisc.h"
#include "StructOnScope.h"
#include "CoreUObject.h"
#include "LuaUnrealWrapper.h"
#include "lua.hpp"
#include "LuaScript.h"
#include "LuaStat.h"
#include "DelegateCallLua.h"
#include "ObjectLuaReference.h"


bool FUnrealMisc::HasScriptAccessibleField(const UStruct* InStruct)
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

bool FUnrealMisc::IsScriptCallableFunction(const UFunction* InFunction)
{
	return InFunction && InFunction->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Public);
}

bool FUnrealMisc::IsScriptReadableProperty(const UProperty* InProperty)
{
	const uint64 ReadableFlags = CPF_BlueprintAssignable | CPF_BlueprintVisible | CPF_InstancedReference;
	return InProperty && InProperty->HasAnyPropertyFlags(ReadableFlags);
}

void FUnrealMisc::PushProperty(lua_State* InL, UProperty* InProp, void* InBuff, bool bRef /*= true*/)
{
	SCOPE_CYCLE_COUNTER(STAT_PushToLua);
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

void FUnrealMisc::FetchProperty(lua_State* InL, const UProperty* InProp, void* InBuff, int32 InStackIndex /*= -1*/, FName ErrorName /*= TEXT("")*/)
{
	SCOPE_CYCLE_COUNTER(STAT_FetchFromLua);
	//no enough params
	if (lua_gettop(InL) < lua_absindex(InL, InStackIndex))
	{
		return;
	}

	FString PropName = InProp->GetName();

	if (const UIntProperty* IntProp = Cast<UIntProperty>(InProp))
	{
		IntProp->SetPropertyValue_InContainer(InBuff, lua_tointeger(InL, InStackIndex));
	}
	else if (const UInt64Property * Int64Prop = Cast<UInt64Property>(InProp))
	{
		Int64Prop->SetPropertyValue_InContainer(InBuff, lua_tointeger(InL, InStackIndex));
	}
	else if (const UFloatProperty* FloatProp = Cast<UFloatProperty>(InProp))
	{
		FloatProp->SetPropertyValue_InContainer(InBuff, lua_tonumber(InL, InStackIndex));
	}
	else if (const UBoolProperty* BoolProp = Cast<UBoolProperty>(InProp))
	{
		BoolProp->SetPropertyValue_InContainer(InBuff, (bool)lua_toboolean(InL, InStackIndex));
	}
	else if (const UNameProperty* NameProp = Cast<UNameProperty>(InProp))
	{
		NameProp->SetPropertyValue_InContainer(InBuff, UTF8_TO_TCHAR(lua_tostring(InL, InStackIndex)));
	}
	else if (const UStrProperty* StrProp = Cast<UStrProperty>(InProp))
	{
		StrProp->SetPropertyValue_InContainer(InBuff, UTF8_TO_TCHAR(lua_tostring(InL, InStackIndex)));
	}
	else if (const UTextProperty* TextProp = Cast<UTextProperty>(InProp))
	{
		TextProp->SetPropertyValue_InContainer(InBuff, FText::FromString(UTF8_TO_TCHAR(lua_tostring(InL, InStackIndex))));
	}
	else if (const UByteProperty* ByteProp = Cast<UByteProperty>(InProp))
	{
		ByteProp->SetPropertyValue_InContainer(InBuff, lua_tointeger(InL, InStackIndex));
	}
	else if (const UEnumProperty* EnumProp = Cast<UEnumProperty>(InProp))
	{
		UNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
		UnderlyingProp->SetIntPropertyValue(EnumProp->ContainerPtrToValuePtr<void>(InBuff), lua_tointeger(InL, InStackIndex));
	}
	else if (const UClassProperty* ClassProp = Cast<UClassProperty>(InProp))
	{
		//TODO check property
		ClassProp->SetPropertyValue_InContainer(InBuff, FetchObject(InL, InStackIndex));
	}
	else if (const UStructProperty* StructProp = Cast<UStructProperty>(InProp))
	{
		void* Data = FetchStruct(InL, InStackIndex);
		if (Data)
		{
			StructProp->Struct->CopyScriptStruct(InBuff, Data);
		}
	}
	else if (const UObjectProperty* ObjectProp = Cast<UObjectProperty>(InProp))
	{
		//TODO, check property
		ObjectProp->SetObjectPropertyValue_InContainer(InBuff, FetchObject(InL, InStackIndex));
	}
	else if (const UDelegateProperty* DelegateProp = Cast<UDelegateProperty>(InProp))
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
	}
	else if (const UArrayProperty* ArrayProp = Cast<UArrayProperty>(InProp))
	{
		FScriptArrayHelper ArrayHelper(ArrayProp, InBuff);
		int32 i = 0;
		lua_pushnil(InL);
		while (lua_next(InL, -2))
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

	else if (const USetProperty* SetProp = Cast<USetProperty>(InProp))
	{
		FScriptSetHelper SetHelper(SetProp, InBuff);
		int32 i = 0;
		
		lua_pushnil(InL);
		while (lua_next(InL, -2))
		{
			if (SetHelper.Num() < i + 1)
			{
				SetHelper.AddDefaultValue_Invalid_NeedsRehash();
			}

			FetchProperty(InL, SetHelper.ElementProp, SetHelper.GetElementPtr(i), -1);
			++i;
			lua_pop(InL, 1);
		}

		SetHelper.Rehash();
	}
	else if (const UMapProperty* MapProp = Cast<UMapProperty>(InProp))
	{
		FScriptMapHelper MapHelper(MapProp, InBuff);
		int32 i = 0;
		
		lua_pushnil(InL);
		while (lua_next(InL, -2))
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

		MapHelper.Rehash();
	}

	return ;

}

UObject* FUnrealMisc::FetchObject(lua_State* InL, int32 InIndex)
{
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, InIndex);
	if (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Object)
	{
		return Wrapper->ObjInst.Get();
	}

	return nullptr;
}

void FUnrealMisc::PushObject(lua_State* InL, UObject* InObj)
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

		lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
		FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(InL, -1);
		lua_pop(InL, 1);
		lua_rawgeti(InL, LUA_REGISTRYINDEX, LuaWrapper->ClassMetatableIdx);
		if (lua_istable(InL, -1))
		{
			lua_setmetatable(InL, -2);
		}
		else
		{
			lua_pop(InL, 1);
		}

	}
}

void* FUnrealMisc::FetchStruct(lua_State* InL, int32 InIndex)
{
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, InIndex);
	if (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Struct)
	{
		return &(Wrapper->StructInst);
	}

	return nullptr;
}

void FUnrealMisc::PushStruct(lua_State* InL, const UScriptStruct* InStruct, const void* InBuff)
{
	const int32 StructSize = FMath::Max(InStruct->GetStructureSize(), 1);
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_newuserdata(InL, sizeof(FLuaStructWrapper) + StructSize);
	Wrapper->WrapperType = ELuaUnrealWrapperType::Struct;
	Wrapper->StructType = InStruct;

	InStruct->CopyScriptStruct(&(Wrapper->StructInst), InBuff);

	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(InL, -1);
	lua_pop(InL, 1);
	lua_rawgeti(InL, LUA_REGISTRYINDEX, LuaWrapper->StructMetatableIdx);
	if (lua_istable(InL, -1))
	{
		lua_setmetatable(InL, -2);
	}
	else
	{
		lua_pop(InL, 1);
	}
}

void FUnrealMisc::PushDelegate(lua_State* InL, void* InDelegateProperty, void* InBuff, bool InMulti)
{
	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_newuserdata(InL, sizeof(FLuaDelegateWrapper));
	Wrapper->WrapperType = ELuaUnrealWrapperType::Delegate;
	Wrapper->bIsMulti = InMulti;
	Wrapper->DelegateInst = InMulti ? (void*)((UMulticastDelegateProperty*)InDelegateProperty)->ContainerPtrToValuePtr<FMulticastScriptDelegate>(InBuff) : (void*)((UDelegateProperty*)InDelegateProperty)->GetPropertyValuePtr_InContainer(InBuff);
	Wrapper->FunctionSignature = InMulti ? ((UMulticastDelegateProperty*)InDelegateProperty)->SignatureFunction : ((UDelegateProperty*)InDelegateProperty)->SignatureFunction;


	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(InL, -1);
	lua_pop(InL, 1);
	lua_rawgeti(InL, LUA_REGISTRYINDEX, LuaWrapper->DelegateMetatableIndex);
	lua_setmetatable(InL, -2);
}

int32 FUnrealMisc::GetObjectProperty(lua_State* L)
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

int32 FUnrealMisc::SetObjectProperty(lua_State* L)
{
	UProperty* Prop = (UProperty*)lua_touserdata(L, lua_upvalueindex(1));
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(L, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaUnrealWrapperType::Object)
	{
		return 0;
	}

	FetchProperty(L, Prop, Wrapper->ObjInst.Get(), 2);

	return 0;
}

int32 FUnrealMisc::GetStructProperty(lua_State* InL)
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

int32 FUnrealMisc::SetStructProperty(lua_State* InL)
{
	UProperty* Prop = (UProperty*)lua_touserdata(InL, lua_upvalueindex(1));
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaUnrealWrapperType::Struct)
	{
		return 0;
	}

	FetchProperty(InL, Prop, &(Wrapper->StructInst), 2);

	return 0;
}


int32 FUnrealMisc::CallFunction(lua_State* L)
{
	SCOPE_CYCLE_COUNTER(STAT_LuaCallBP);
	UFunction* Func = (UFunction*)lua_touserdata(L, lua_upvalueindex(1));
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(L, 1);
	UObject* Obj = nullptr;
	if (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Object)
	{
		Obj = Wrapper->ObjInst.Get();
	}
	int32 StackTop = 2;
	if (Func == nullptr || Obj == nullptr)
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
				FUnrealMisc::FetchProperty(L, Prop, FuncParam.GetStructMemory(), StackTop++, FName(*Func->GetName()));
			}
		}

		Obj->ProcessEvent(Func, FuncParam.GetStructMemory());

		int32 ReturnNum = 0;
		if (ReturnProp)
		{
			FUnrealMisc::PushProperty(L, ReturnProp, FuncParam.GetStructMemory(), false);
			++ReturnNum;
		}

		if (Func->HasAnyFunctionFlags(FUNC_HasOutParms))
		{
			for (TFieldIterator<UProperty> It(Func); It; ++It)
			{
				UProperty* Prop = *It;
				if (Prop->HasAnyPropertyFlags(CPF_OutParm) && !Prop->HasAnyPropertyFlags(CPF_ConstParm))
				{
					FUnrealMisc::PushProperty(L, *It, FuncParam.GetStructMemory(), false);
					++ReturnNum;
				}
			}
		}

		return ReturnNum;
	}

}

int32 FUnrealMisc::ObjectIndex(lua_State* InL)
{
	SCOPE_CYCLE_COUNTER(STAT_ObjectIndex);

	FLuaObjectWrapper* ObjectWrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	const char* FieldName = lua_tostring(InL, 2);
	if (ObjectWrapper && ObjectWrapper->WrapperType == ELuaUnrealWrapperType::Object)
	{
		UFunction* Func = ObjectWrapper->ObjInst->GetClass()->FindFunctionByName(FName(FieldName));
		UProperty* Prop = Func ? nullptr : ObjectWrapper->ObjInst->GetClass()->FindPropertyByName(FName(FieldName));
		if (Func)
		{
			lua_pushlightuserdata(InL, Func);
			lua_pushcclosure(InL, FUnrealMisc::CallFunction, 1);
		}
		else if (Prop)
		{
			PushProperty(InL, Prop, ObjectWrapper->ObjInst.Get(), !Prop->HasAnyPropertyFlags(CPF_BlueprintReadOnly));
		}
		else
		{
			lua_pushnil(InL);
		}
	}
	else
	{
		lua_pushnil(InL);
	}

	return 1;
}

int32 FUnrealMisc::ObjectNewIndex(lua_State* InL)
{
	SCOPE_CYCLE_COUNTER(STAT_ObjectNewIndex);
	FLuaObjectWrapper* ObjectWrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	const char* FieldName = lua_tostring(InL, 2);
	if (ObjectWrapper && ObjectWrapper->WrapperType == ELuaUnrealWrapperType::Object)
	{
		UProperty* Prop = ObjectWrapper->ObjInst->GetClass()->FindPropertyByName(FName(FieldName));
		if (Prop)
		{
			FUnrealMisc::FetchProperty(InL, Prop, ObjectWrapper->ObjInst.Get(), 3);
		}
	}

	return 0;
}

int32 FUnrealMisc::StructIndex(lua_State* InL)
{
	SCOPE_CYCLE_COUNTER(STAT_StructIndex);
	FLuaStructWrapper* StructWrapper = (FLuaStructWrapper*)lua_touserdata(InL, 1);
	const char* FieldName = lua_tostring(InL, 2);
	if (StructWrapper && StructWrapper->WrapperType == ELuaUnrealWrapperType::Struct)
	{
		UProperty* Prop = StructWrapper->StructType->FindPropertyByName(FName(FieldName));
		if (Prop)
		{
			FUnrealMisc::PushProperty(InL, Prop, &(StructWrapper->StructInst));
		}
		else
		{
			lua_pushnil(InL);
		}
	}
	else
	{
		lua_pushnil(InL);
	}

	return 1;
}

int32 FUnrealMisc::StructNewIndex(lua_State* InL)
{
	SCOPE_CYCLE_COUNTER(STAT_StructNewIndex);
	FLuaStructWrapper* StructWrapper = (FLuaStructWrapper*)lua_touserdata(InL, 1);
	const char* FieldName = lua_tostring(InL, 2);
	if (StructWrapper && StructWrapper->WrapperType == ELuaUnrealWrapperType::Struct)
	{
		UProperty* Prop = StructWrapper->StructType->FindPropertyByName(FName(FieldName));
		if (Prop)
		{
			FetchProperty(InL, Prop, &(StructWrapper->StructInst), 3);
		}
	}

	return 0;
}

void* FUnrealMisc::LuaAlloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	FLuaUnrealWrapper* Inst = (FLuaUnrealWrapper*)ud;
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
			SET_MEMORY_STAT(STAT_LuaMemory, Inst->LuaMemory);
		}
		return ptr;
	}
}

void FUnrealMisc::LuaLog(const FString& InLog, int32 InLevel, FLuaUnrealWrapper* InLuaWrapper)
{
	FString Str = InLog;
	if (InLuaWrapper)
	{
		Str = FString("[") + InLuaWrapper->LuaStateName + FString("]") + Str;
	}
	switch (InLevel)
	{
	case 0 :
		UE_LOG(LogLuaScript, Log, TEXT("%s"), *Str);
		break;
	case 1:
		UE_LOG(LogLuaScript, Warning, TEXT("%s"), *Str);
		break;
	case 2:
		UE_LOG(LogLuaScript, Error, TEXT("%s"), *Str);
		break;
	default:
		break;
	}

}


int FUnrealMisc::LuaGetGameInstance(lua_State* InL)
{
	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(InL, -1);
	lua_pop(InL, 1);
	FUnrealMisc::PushObject(InL, (UObject*)LuaWrapper->CachedGameInstance);
	return 1;
}


//Lua usage: LoadMapEndedEvent:Bind(LuaFunction, LuaObj, InWrapperObjectName[option])
int FUnrealMisc::LuaBindDelegate(lua_State* InL)
{
	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(InL, -1);
	lua_pop(InL, 1);

	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, 1);

	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaUnrealWrapperType::Delegate || !lua_isfunction(InL, 2))
	{
		luaL_traceback(InL, InL, "Error in BindDelegate", 1);
		FUnrealMisc::LuaLog(FString::Printf(TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(InL, -1))), 1, LuaWrapper);
		lua_pushnil(InL);
		return 1;
	}

	//FString WrapperObjectName = ;
	UDelegateCallLua* DelegateObject = NewObject<UDelegateCallLua>(GetTransientPackage(), FName(lua_tostring(InL, 4)));
	//ref function
	lua_pushvalue(InL, 2);
	DelegateObject->LuaFunctionID = luaL_ref(InL, LUA_REGISTRYINDEX);
	//ref self
	if (lua_istable(InL, 3))
	{
		lua_pushvalue(InL, 3);
		DelegateObject->LuaSelfID = luaL_ref(InL, LUA_REGISTRYINDEX);
	}

	DelegateObject->FunctionSignature = Wrapper->FunctionSignature;
	DelegateObject->bIsMulti = Wrapper->bIsMulti;

	if (Wrapper->bIsMulti)
	{
		FMulticastScriptDelegate* MultiDelegate = (FMulticastScriptDelegate*)Wrapper->DelegateInst;
		if (MultiDelegate)
		{
			DelegateObject->AddToRoot();
			FScriptDelegate ScriptDelegate;
			ScriptDelegate.BindUFunction(DelegateObject, UDelegateCallLua::GetWrapperFunctionName());
			MultiDelegate->Add(ScriptDelegate);
			DelegateObject->DelegateInst = MultiDelegate;
			LuaWrapper->DelegateCallLuaList.Add(DelegateObject);
		}
	}
	else
	{
		FScriptDelegate* SingleDelegate = (FScriptDelegate*)Wrapper->DelegateInst;
		if (SingleDelegate)
		{
			DelegateObject->AddToRoot();
			SingleDelegate->BindUFunction(DelegateObject, UDelegateCallLua::GetWrapperFunctionName());
			DelegateObject->DelegateInst = SingleDelegate;
			LuaWrapper->DelegateCallLuaList.Add(DelegateObject);
		}
	}

	DelegateObject->LuaState = InL;
	//return wrapper object
	FUnrealMisc::PushObject(InL, DelegateObject);
	return 1;
}

//Lua usage: LoadMapEndedEvent:Unbind(WrapperObject[option, remove single])
int FUnrealMisc::LuaUnbindDelegate(lua_State* InL)
{
	bool bIsMulti = false;
	UDelegateCallLua* FuncObj = nullptr;
	const void* DelegateInst = nullptr;

	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(InL, -1);
	lua_pop(InL, 1);

	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, 1);
	if (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Delegate)
	{
		DelegateInst = Wrapper->DelegateInst;
		bIsMulti = Wrapper->bIsMulti;
	}

	if (DelegateInst == nullptr)
	{
		lua_pushnil(InL);
		return 1;
	}

	FuncObj = Cast<UDelegateCallLua>(FetchObject(InL, 2));
	if (FuncObj)
	{
		FuncObj->Unbind();
		lua_pushboolean(InL, true);
		return 1;
	}
	else
	{
		if (bIsMulti)
		{
			FMulticastScriptDelegate* MultiDelegate = (FMulticastScriptDelegate*)DelegateInst;
			TArray<UObject*> ObjList = MultiDelegate->GetAllObjects();
			for (auto It : ObjList)
			{
				if (UDelegateCallLua* LuaObj = Cast<UDelegateCallLua>(It))
				{
					LuaObj->Unbind();
				}
			}
		}
		else
		{
			FScriptDelegate* SingleDelegate = (FScriptDelegate*)DelegateInst;
			if (UDelegateCallLua* LuaObj = Cast<UDelegateCallLua>(SingleDelegate->GetUObject()))
			{
				LuaObj->Unbind();
			}
		}
	}

	lua_pushboolean(InL, true);
	return 1;
}

//LuaLoadObject(Owner, ObjectPath)
int FUnrealMisc::LuaLoadObject(lua_State* InL)
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
		FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(InL, -1);
		lua_pop(InL, 1);
		FUnrealMisc::LuaLog(FString::Printf(TEXT("LoadObject failed: %s"), *ObjectPath), 1, LuaWrapper);
		return 0;
	}
	FUnrealMisc::PushObject(InL, LoadedObj);

	return 1;
}

//LuaLoadClass(Owner, ClassPath)
int FUnrealMisc::LuaLoadClass(lua_State* InL)
{
	int32 tp = lua_gettop(InL);
	if (tp < 2)
	{
		return 0;
	}

	FString ClassPath = UTF8_TO_TCHAR(lua_tostring(InL, 2));

	if (UClass * FoundClass = FindObject<UClass>(ANY_PACKAGE, *ClassPath))
	{
		FUnrealMisc::PushObject(InL, FoundClass);
		return 1;
	}

	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	UObject* Owner = (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Object) ? Wrapper->ObjInst.Get() : nullptr;
	UClass* LoadedClass = LoadObject<UClass>(Owner, *ClassPath);
	if (LoadedClass == nullptr)
	{
		lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
		FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(InL, -1);
		lua_pop(InL, 1);
		FUnrealMisc::LuaLog(FString::Printf(TEXT("LoadClass failed: %s"), *ClassPath), 1, LuaWrapper);
		return 0;
	}

	FUnrealMisc::PushObject(InL, LoadedClass);
	return 1;
}

//LuaNewObject(Owner, ClassName, ObjectName)
int FUnrealMisc::LuaNewObject(lua_State* InL)
{
	int ParamNum = lua_gettop(InL);
	if (ParamNum < 2)
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
	FUnrealMisc::PushObject(InL, NewObj);
	return 1;
}


int FUnrealMisc::LuaNewStruct(lua_State* InL)
{
	int ParamNum = lua_gettop(InL);
	if (ParamNum < 1)
	{
		//error
		return 0;
	}


	UScriptStruct* StructType = FindObject<UScriptStruct>(ANY_PACKAGE, UTF8_TO_TCHAR(lua_tostring(InL, 1)));
	if (StructType == nullptr)
	{
		lua_pushnil(InL);
		return 1;
	}
	else
	{
		FLuaStructWrapper* StructWrapper = (FLuaStructWrapper*)lua_newuserdata(InL, sizeof(FLuaStructWrapper) + StructType->GetStructureSize());
		StructWrapper->WrapperType = ELuaUnrealWrapperType::Struct;
		StructWrapper->StructType = StructType;
		StructType->InitializeDefaultValue((uint8*)&(StructWrapper->StructInst));

		lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
		FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(InL, -1);
		lua_pop(InL, 1);
		lua_rawgeti(InL, LUA_REGISTRYINDEX, LuaWrapper->StructMetatableIdx);
		if (lua_istable(InL, -1))
		{
			lua_setmetatable(InL, -2);
		}
		else
		{
			lua_pop(InL, 1);
		}
	}

	return 1;
}

//Lua usage: GameSingleton:GetGameEvent():GetOnPostLoadMap():Call(0)
int FUnrealMisc::LuaCallUnrealDelegate(lua_State* InL)
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
			FUnrealMisc::FetchProperty(InL, Prop, FuncParam.GetStructMemory(), StackTop++, FName(*SignatureFunction->GetName()));
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
		FUnrealMisc::PushProperty(InL, ReturnProp, FuncParam.GetStructMemory(), true);
		++ReturnNum;
	}

	if (SignatureFunction->HasAnyFunctionFlags(FUNC_HasOutParms))
	{
		for (TFieldIterator<UProperty> It(SignatureFunction); It; ++It)
		{
			UProperty* Prop = *It;
			if (Prop->HasAnyPropertyFlags(CPF_OutParm) && !Prop->HasAnyPropertyFlags(CPF_ConstParm))
			{
				FUnrealMisc::PushProperty(InL, *It, FuncParam.GetStructMemory(), false);
				++ReturnNum;
			}
		}
	}

	return ReturnNum;
}

int FUnrealMisc::LuaDumpObject(lua_State* InL)
{
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	if (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Object)
	{
		for (TFieldIterator<const UField> It(Wrapper->ObjInst->GetClass()); It; ++It)
		{
			lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
			FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(InL, -1);
			lua_pop(InL, 1);
			FUnrealMisc::LuaLog(FString::Printf(TEXT("%s"), *It->GetName()), 1, LuaWrapper);
		}
	}
	return 0;
}

//usage: Unreal.LuaGetUnrealCDO("KismetSystemLibrary")
int FUnrealMisc::LuaGetUnrealCDO(lua_State* InL)
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
		FUnrealMisc::PushObject(InL, Class->GetDefaultObject());
		return 1;
	}

	return 0;
}

int FUnrealMisc::LuaAddObjectRef(lua_State* InL)
{
	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(InL, -1);
	lua_pop(InL, 1);
	if (LuaWrapper == nullptr || LuaWrapper->ObjectRef == nullptr)
	{
		lua_pushinteger(InL, 0);
		return 1;
	}

	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaUnrealWrapperType::Object)
	{
		lua_pushinteger(InL, 0);
		return 1;
	}

	int32 NumToAdd = lua_tointeger(InL, 2);
	UObject* Obj = Wrapper->ObjInst.Get();
	int32* NumPtr = LuaWrapper->ObjectRef->ObjectRefMap.Find(Obj);
	int32 NewNum = NumPtr ? *NumPtr + NumToAdd : NumToAdd;
	NewNum = FMath::Clamp(NewNum, 0, 99999);
	if (NewNum > 0)
	{
		LuaWrapper->ObjectRef->ObjectRefMap.Emplace(Obj, NewNum);
	}
	else if (NumPtr)
	{
		LuaWrapper->ObjectRef->ObjectRefMap.Remove(Obj);
	}

	lua_pushinteger(InL, NewNum);
	return 1;

}

int32 FUnrealMisc::RegisterTickFunction(lua_State* InL)
{
	if (lua_isfunction(InL, 1))
	{
		lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
		FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(InL, -1);
		lua_pop(InL, 1);

		LuaWrapper->LuaTickFunctionIndex = luaL_ref(InL, LUA_REGISTRYINDEX);
	}
	else
	{
		bool bUnRegister = !lua_toboolean(InL, 1);
		if (bUnRegister)
		{
			lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
			FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(InL, -1);
			lua_pop(InL, 1);

			luaL_unref(InL, LUA_REGISTRYINDEX, LuaWrapper->LuaTickFunctionIndex);
			LuaWrapper->LuaTickFunctionIndex = 0;
		}
	}

	return 0;
}

int FUnrealMisc::PrintLog(lua_State* L)
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
	FLuaUnrealWrapper* LuaWrapper = (FLuaUnrealWrapper*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	FUnrealMisc::LuaLog(FString::Printf(TEXT("%s"), *StringToPrint), 0, LuaWrapper);
	return 0;
}
