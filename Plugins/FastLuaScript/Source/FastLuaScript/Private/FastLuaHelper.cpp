// Fill out your copyright notice in the Description page of Project Settings.

#include "FastLuaHelper.h"
#include "StructOnScope.h"
#include "CoreUObject.h"
#include "lua/lua.hpp"
#include "FastLuaUnrealWrapper.h"
#include "FastLuaScript.h"
#include "FastLuaDelegate.h"
#include "FastLuaStat.h"

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
	bool bScriptCallable = InFunction && InFunction->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Public) && !InFunction->HasAnyFunctionFlags(FUNC_EditorOnly);
	if (bScriptCallable)
	{
		/*if (InFunction->GetName() == FString("IsHiddenEdAtStartup"))
		{
			UE_LOG(LogTemp, Log, TEXT("Breakpoint!"));
		}*/
		TMap<FName, FString> MetaDataList = *UMetaData::GetMapForObject(InFunction);
		if (MetaDataList.Find(FName("CustomThunk")) == nullptr)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}

bool FastLuaHelper::IsScriptReadableProperty(const UProperty* InProperty)
{
	const uint64 ReadableFlags = CPF_BlueprintAssignable | CPF_BlueprintVisible | CPF_InstancedReference;
	bool bScriptReadable = InProperty && InProperty->HasAnyPropertyFlags(ReadableFlags) && InProperty->HasAllPropertyFlags(CPF_NativeAccessSpecifierPublic) && !InProperty->HasAnyPropertyFlags(CPF_EditorOnly | CPF_Deprecated);
	if (bScriptReadable)
	{
		/*if (InProperty->GetName() == FString("NotifyColor"))
		{
			UE_LOG(LogTemp, Log, TEXT("Breakpoint!"));
		}*/
		TMap<FName, FString> MetaDataList = *UMetaData::GetMapForObject(InProperty);
		if (MetaDataList.Find(FName("DeprecationMessage")) == nullptr)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
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
	else if (const UInt64Property * Int64Prop = Cast<UInt64Property>(InProp))
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
		if (EnumProp->GetEnum())
		{
			TypeName = EnumProp->GetEnum()->CppType;
		}
	}
	else if (const UByteProperty* ByteProp = Cast<UByteProperty>(InProp))
	{
		if (ByteProp->Enum)
		{
			TypeName = FString::Printf(TEXT("TEnumAsByte<%s>"), *ByteProp->Enum->CppType);
		}
		else
		{
			TypeName = FString("uint8");
		}
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
	else if (const USoftClassProperty* SoftClassProp = Cast<USoftClassProperty>(InProp))
	{
		FString MetaClassName = SoftClassProp->MetaClass->GetName();
		TypeName = FString::Printf(TEXT("TSoftClassPtr<U%s>"), *MetaClassName);
	}
	else if (const USoftObjectProperty* SoftObjectProp = Cast<USoftObjectProperty>(InProp))
	{
		TypeName = FString::Printf(TEXT("TSoftObjectPtr<UObject>"));
	}
	else if (const UInterfaceProperty* InterfaceProp = Cast<UInterfaceProperty>(InProp))
	{
		FString MetaClassName = InterfaceProp->InterfaceClass->GetName();
		TypeName = FString::Printf(TEXT("TScriptInterface<I%s>"), *MetaClassName);
	}
	else if (const UArrayProperty* ArrayProp = Cast<UArrayProperty>(InProp))
	{
		FString ElementTypeName = GetPropertyTypeName(ArrayProp->Inner);
		FString PointerFlag;
		if (IsPointerType(ElementTypeName))
		{
			PointerFlag = FString("*");
		}
		TypeName = FString::Printf(TEXT("TArray<%s%s>"), *ElementTypeName, *PointerFlag);
	}
	else if (const UMapProperty* MapProp = Cast<UMapProperty>(InProp))
	{
		FString KeyTypeName = GetPropertyTypeName(MapProp->KeyProp);
		FString KeyPointerFlag;
		if (IsPointerType(KeyTypeName))
		{
			KeyPointerFlag = FString("*");
		}

		FString ValueTypeName = GetPropertyTypeName(MapProp->ValueProp);
		FString ValuePointerFlag;
		if (IsPointerType(ValueTypeName))
		{
			ValuePointerFlag = FString("*");
		}
		TypeName = FString::Printf(TEXT("TMap<%s%s, %s%s>"), *KeyTypeName, *KeyPointerFlag, *ValueTypeName, *ValuePointerFlag);
	}

	return TypeName;
}

bool FastLuaHelper::IsPointerType(const FString& InTypeName)
{
	return InTypeName.StartsWith(FString("U"), ESearchCase::CaseSensitive) || InTypeName.StartsWith(FString("A"), ESearchCase::CaseSensitive);
}


void FastLuaHelper::PushProperty(lua_State* InL, UProperty* InProp, void* InBuff, bool bRef /*= true*/)
{
	if (InProp == nullptr || InProp->GetClass() == nullptr)
	{
		LuaLog(FString("Property is nil"));
		return;
	}
	if (const UIntProperty * IntProp = Cast<UIntProperty>(InProp))
	{
		lua_pushinteger(InL, IntProp->GetPropertyValue_InContainer(InBuff));
	}
	else if (const UFloatProperty * FloatProp = Cast<UFloatProperty>(InProp))
	{
		lua_pushnumber(InL, FloatProp->GetPropertyValue_InContainer(InBuff));
	}
	else if (const UEnumProperty * EnumProp = Cast<UEnumProperty>(InProp))
	{
		const UNumericProperty* NumProp = EnumProp->GetUnderlyingProperty();
		lua_pushinteger(InL, NumProp ? NumProp->GetSignedIntPropertyValue(InBuff) : 0);
	}
	else if (const UBoolProperty * BoolProp = Cast<UBoolProperty>(InProp))
	{
		lua_pushboolean(InL, BoolProp->GetPropertyValue_InContainer(InBuff));
	}
	else if (const UNameProperty * NameProp = Cast<UNameProperty>(InProp))
	{
		FName name = NameProp->GetPropertyValue_InContainer(InBuff);
		lua_pushstring(InL, TCHAR_TO_UTF8(*name.ToString()));
	}
	else if (const UStrProperty * StrProp = Cast<UStrProperty>(InProp))
	{
		const FString& str = StrProp->GetPropertyValue_InContainer(InBuff);
		lua_pushstring(InL, TCHAR_TO_UTF8(*str));
	}
	else if (const UTextProperty * TextProp = Cast<UTextProperty>(InProp))
	{
		const FText& text = TextProp->GetPropertyValue_InContainer(InBuff);
		lua_pushstring(InL, TCHAR_TO_UTF8(*text.ToString()));
	}
	else if (const UClassProperty * ClassProp = Cast<UClassProperty>(InProp))
	{
		PushObject(InL, ClassProp->GetPropertyValue_InContainer(InBuff));
	}
	else if (const UStructProperty * StructProp = Cast<UStructProperty>(InProp))
	{
		if (const UScriptStruct * ScriptStruct = Cast<UScriptStruct>(StructProp->Struct))
		{
			PushStruct(InL, ScriptStruct, StructProp->ContainerPtrToValuePtr<void>(InBuff));
		}
	}
	else if (const UObjectProperty * ObjectProp = Cast<UObjectProperty>(InProp))
	{
		PushObject(InL, ObjectProp->GetObjectPropertyValue(ObjectProp->GetPropertyValuePtr_InContainer(InBuff)));
	}
	else if (UDelegateProperty * DelegateProp = Cast<UDelegateProperty>(InProp))
	{
		PushDelegate(InL, DelegateProp, InBuff, false);
	}
	else if (UMulticastDelegateProperty * MultiDelegateProp = Cast<UMulticastDelegateProperty>(InProp))
	{
		PushDelegate(InL, MultiDelegateProp, InBuff, true);
	}
	else if (const UArrayProperty * ArrayProp = Cast<UArrayProperty>(InProp))
	{
		FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(InBuff));
		lua_newtable(InL);
		for (int32 i = 0; i < ArrayHelper.Num(); ++i)
		{
			PushProperty(InL, ArrayProp->Inner, ArrayHelper.GetRawPtr(i), true);
			lua_rawseti(InL, -2, i + 1);
		}
	}
	else if (const USetProperty * SetProp = Cast<USetProperty>(InProp))
	{
		FScriptSetHelper SetHelper(SetProp, SetProp->ContainerPtrToValuePtr<void>(InBuff));
		lua_newtable(InL);
		for (int32 i = 0; i < SetHelper.Num(); ++i)
		{
			PushProperty(InL, SetHelper.GetElementProperty(), SetHelper.GetElementPtr(i), true);
			lua_rawseti(InL, -2, i);
		}
	}
	else if (const UMapProperty * MapProp = Cast<UMapProperty>(InProp))
	{
		FScriptMapHelper MapHelper(MapProp, MapProp->ContainerPtrToValuePtr<void>(InBuff));
		lua_newtable(InL);
		for (int32 i = 0; i < MapHelper.Num(); ++i)
		{
			uint8* PairPtr = MapHelper.GetPairPtr(i);
			PushProperty(InL, MapProp->KeyProp, PairPtr, true);
			PushProperty(InL, MapProp->ValueProp, PairPtr, true);
			lua_rawset(InL, -3);
		}
	}
}

void FastLuaHelper::FetchProperty(lua_State* InL, const UProperty* InProp, void* InBuff, int32 InStackIndex /*= -1*/, FName ErrorName /*= TEXT("")*/)
{
	//no enough params
	if (lua_gettop(InL) < lua_absindex(InL, InStackIndex))
	{
		return;
	}

	FString PropName = InProp->GetName();

	if (const UIntProperty * IntProp = Cast<UIntProperty>(InProp))
	{
		IntProp->SetPropertyValue_InContainer(InBuff, lua_tointeger(InL, InStackIndex));
	}
	else if (const UInt64Property * Int64Prop = Cast<UInt64Property>(InProp))
	{
		Int64Prop->SetPropertyValue_InContainer(InBuff, lua_tointeger(InL, InStackIndex));
	}
	else if (const UFloatProperty * FloatProp = Cast<UFloatProperty>(InProp))
	{
		FloatProp->SetPropertyValue_InContainer(InBuff, lua_tonumber(InL, InStackIndex));
	}
	else if (const UBoolProperty * BoolProp = Cast<UBoolProperty>(InProp))
	{
		BoolProp->SetPropertyValue_InContainer(InBuff, (bool)lua_toboolean(InL, InStackIndex));
	}
	else if (const UNameProperty * NameProp = Cast<UNameProperty>(InProp))
	{
		NameProp->SetPropertyValue_InContainer(InBuff, UTF8_TO_TCHAR(lua_tostring(InL, InStackIndex)));
	}
	else if (const UStrProperty * StrProp = Cast<UStrProperty>(InProp))
	{
		StrProp->SetPropertyValue_InContainer(InBuff, UTF8_TO_TCHAR(lua_tostring(InL, InStackIndex)));
	}
	else if (const UTextProperty * TextProp = Cast<UTextProperty>(InProp))
	{
		TextProp->SetPropertyValue_InContainer(InBuff, FText::FromString(UTF8_TO_TCHAR(lua_tostring(InL, InStackIndex))));
	}
	else if (const UByteProperty * ByteProp = Cast<UByteProperty>(InProp))
	{
		ByteProp->SetPropertyValue_InContainer(InBuff, lua_tointeger(InL, InStackIndex));
	}
	else if (const UEnumProperty * EnumProp = Cast<UEnumProperty>(InProp))
	{
		UNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
		UnderlyingProp->SetIntPropertyValue(EnumProp->ContainerPtrToValuePtr<void>(InBuff), lua_tointeger(InL, InStackIndex));
	}
	else if (const UClassProperty * ClassProp = Cast<UClassProperty>(InProp))
	{
		//TODO check property
		ClassProp->SetPropertyValue_InContainer(InBuff, FetchObject(InL, InStackIndex));
	}
	else if (const UStructProperty * StructProp = Cast<UStructProperty>(InProp))
	{
		void* Data = FetchStruct(InL, InStackIndex, StructProp->Struct->GetStructureSize());
		if (Data)
		{
			StructProp->Struct->CopyScriptStruct(InBuff, Data);
		}
	}
	else if (const UObjectProperty * ObjectProp = Cast<UObjectProperty>(InProp))
	{
		//TODO, check property
		ObjectProp->SetObjectPropertyValue_InContainer(InBuff, FetchObject(InL, InStackIndex));
	}
	else if (const UDelegateProperty * DelegateProp = Cast<UDelegateProperty>(InProp))
	{
		FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, InStackIndex);
		if (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Delegate && Wrapper->DelegateInst && Wrapper->bIsMulti == false)
		{
			DelegateProp->SetPropertyValue_InContainer(InBuff, *(FScriptDelegate*)Wrapper->DelegateInst);
		}
	}
	else if (const UMulticastDelegateProperty * MultiDelegateProp = Cast<UMulticastDelegateProperty>(InProp))
	{
		FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, InStackIndex);
		if (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Delegate && Wrapper->DelegateInst && Wrapper->bIsMulti)
		{
			MultiDelegateProp->SetMulticastDelegate(InBuff, *(FMulticastScriptDelegate*)Wrapper->DelegateInst);
		}
	}
	else if (const UArrayProperty * ArrayProp = Cast<UArrayProperty>(InProp))
	{
		FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(InBuff));
		int32 i = 0;
		if (lua_istable(InL, -1))
		{
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
		
	}

	else if (const USetProperty * SetProp = Cast<USetProperty>(InProp))
	{
		FScriptSetHelper SetHelper(SetProp, SetProp->ContainerPtrToValuePtr<void>(InBuff));
		int32 i = 0;
		if (lua_istable(InL, -1))
		{
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

		}
		
		SetHelper.Rehash();
	}
	else if (const UMapProperty * MapProp = Cast<UMapProperty>(InProp))
	{
		FScriptMapHelper MapHelper(MapProp, MapProp->ContainerPtrToValuePtr<void>(InBuff));
		int32 i = 0;
		if (lua_istable(InL, -1))
		{
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
		}

		MapHelper.Rehash();
	}

	return;

}

UObject* FastLuaHelper::FetchObject(lua_State* InL, int32 InIndex)
{
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

		SCOPE_CYCLE_COUNTER(STAT_FindClassMetatable);

		const UClass* Class = InObj->GetClass();

		lua_rawgetp(InL, LUA_REGISTRYINDEX, Class);
		if (lua_istable(InL, -1))
		{
			lua_setmetatable(InL, -2);
		}
		else
		{
			lua_pop(InL, 1);

			if (RegisterClassMetatable(InL, Class))
			{
				lua_rawgetp(InL, LUA_REGISTRYINDEX, Class);
				lua_setmetatable(InL, -2);
			}
		}
	}
}

void* FastLuaHelper::FetchStruct(lua_State* InL, int32 InIndex, int32 InDesiredSize)
{
	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, InIndex);
	if (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Struct && Wrapper->StructType->GetStructureSize() >= InDesiredSize)
	{
		return Wrapper->StructInst;
	}

	return nullptr;
}

void FastLuaHelper::PushStruct(lua_State* InL, const UScriptStruct* InStruct, const void* InBuff)
{
	if (InStruct == nullptr)
	{
		lua_pushnil(InL);
		return;
	}


	FLuaStructWrapper* Wrapper = (FLuaStructWrapper*)lua_newuserdata(InL, sizeof(FLuaStructWrapper));
	Wrapper->WrapperType = ELuaUnrealWrapperType::Struct;
	Wrapper->StructType = InStruct;
	Wrapper->StructInst = (uint8*)FMemory::Malloc(InStruct->GetStructureSize());
	InStruct->InitializeDefaultValue(Wrapper->StructInst);

	if (InBuff != nullptr)
	{
		InStruct->CopyScriptStruct(Wrapper->StructInst, InBuff);
	}

	SCOPE_CYCLE_COUNTER(STAT_FindStructMetatable);


	lua_rawgetp(InL, LUA_REGISTRYINDEX, InStruct);
	if (lua_istable(InL, -1))
	{
		lua_setmetatable(InL, -2);
	}
	else
	{
		lua_pop(InL, 1);

		if (RegisterStructMetatable(InL, InStruct))
		{
			lua_rawgetp(InL, LUA_REGISTRYINDEX, InStruct);
			lua_setmetatable(InL, -2);
		}
	}

}

void FastLuaHelper::PushDelegate(lua_State* InL, UProperty* InDelegateProperty, void* InBuff, bool InMulti, UFunction* InFunction)
{

	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_newuserdata(InL, sizeof(FLuaDelegateWrapper));
	Wrapper->WrapperType = ELuaUnrealWrapperType::Delegate;
	Wrapper->bIsMulti = InMulti;

	if (InDelegateProperty != nullptr && InBuff != nullptr)
	{
		if (InMulti)
		{
			Wrapper->DelegateInst = (uint8*)((UMulticastDelegateProperty*)InDelegateProperty)->ContainerPtrToValuePtr<FMulticastScriptDelegate>(InBuff);
			Wrapper->FunctionSignature = ((UMulticastDelegateProperty*)InDelegateProperty)->SignatureFunction;
		}
		else
		{
			Wrapper->DelegateInst = (uint8*)((UDelegateProperty*)InDelegateProperty)->GetPropertyValuePtr_InContainer(InBuff);
			Wrapper->FunctionSignature = ((UDelegateProperty*)InDelegateProperty)->SignatureFunction;
		}

		Wrapper->bIsUserDefined = false;
	}
	else if (InFunction)
	{
		if (InMulti)
		{
			Wrapper->DelegateInst = (uint8*)FMemory::Malloc(sizeof(FMulticastScriptDelegate));
			new(Wrapper->DelegateInst) FMulticastScriptDelegate();
		}
		else
		{
			Wrapper->DelegateInst = (uint8*)FMemory::Malloc(sizeof(FScriptDelegate));
			new(Wrapper->DelegateInst) FScriptDelegate();
		}

		Wrapper->FunctionSignature = InFunction;
		Wrapper->bIsUserDefined = true;
	}
	else
	{
		//error!
		Wrapper->DelegateInst = nullptr;
		Wrapper->FunctionSignature = nullptr;
		Wrapper->bIsUserDefined = false;
	}
	
	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
	lua_pop(InL, 1);
	lua_rawgeti(InL, LUA_REGISTRYINDEX, LuaWrapper->GetDelegateMetatableIndex());
	lua_setmetatable(InL, -2);
}

void* FastLuaHelper::FetchDelegate(lua_State* InL, int32 InIndex, bool InIsMulti)
{
	FLuaDelegateWrapper* DelegateWrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, InIndex);
	if (DelegateWrapper && DelegateWrapper->WrapperType == ELuaUnrealWrapperType::Delegate && DelegateWrapper->bIsMulti == InIsMulti)
	{
		return DelegateWrapper->DelegateInst;
	}
	
	return nullptr;
}

int32 FastLuaHelper::CallUnrealFunction(lua_State* InL)
{
	//SCOPE_CYCLE_COUNTER(STAT_LuaCallBP);
	UFunction* Func = (UFunction*)lua_touserdata(InL, lua_upvalueindex(1));
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	UObject* Obj = nullptr;
	if (Wrapper && Wrapper->WrapperType == ELuaUnrealWrapperType::Object)
	{
		Obj = Wrapper->ObjInst.Get();
	}
	int32 StackTop = 2;
	if (Obj == nullptr)
	{
		lua_pushnil(InL);
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
			for (TFieldIterator<UProperty> It(Func); It; ++It)
			{
				UProperty* Prop = *It;
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

void* FastLuaHelper::LuaAlloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	FastLuaUnrealWrapper* Inst = (FastLuaUnrealWrapper*)ud;
	if (nsize == 0)
	{
		FMemory::Free(ptr);
		ptr = nullptr;
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

void FastLuaHelper::LuaLog(const FString& InLog, int32 InLevel, FastLuaUnrealWrapper* InLuaWrapper)
{
	FString Str = InLog;
	if (InLuaWrapper)
	{
		Str = FString("[") + InLuaWrapper->GetInstanceName() + FString("]") + Str;
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
			}
		}

		lua_pop(InL, 1);
	}
}

void FastLuaHelper::FixStructMetatable(lua_State* InL, TArray<const UScriptStruct*> InRegistedStructList)
{
	for (int32 i = 0; i < InRegistedStructList.Num(); ++i)
	{
		//fix metatable.__index = metatable
		lua_rawgetp(InL, LUA_REGISTRYINDEX, (const void*)InRegistedStructList[i]);
		if (lua_istable(InL, -1))
		{
			lua_pushvalue(InL, -1);
			lua_setfield(InL, -2, "__index");

			lua_pushcfunction(InL, FastLuaHelper::StructGC);
			lua_setfield(InL, -2, "__gc");
		}
		else
		{
			lua_pop(InL, 1);
			continue;
		}

		UScriptStruct* SuperStruct = Cast<UScriptStruct>(InRegistedStructList[i]->GetSuperStruct());
		if (SuperStruct)
		{
			lua_rawgetp(InL, LUA_REGISTRYINDEX, (const void*)SuperStruct);
			if (lua_istable(InL, -1))
			{
				lua_setmetatable(InL, -2);
			}
			else
			{
				lua_pop(InL, 1);
			}
		}

		lua_pop(InL, 1);
	}
}

int FastLuaHelper::LuaGetGameInstance(lua_State* InL)
{
	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
	lua_pop(InL, 1);
	FastLuaHelper::PushObject(InL, (UObject*)LuaWrapper->GetGameInstance());
	return 1;
}

//local obj = Unreal.LuaLoadObject(Owner, ObjectPath)
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

//local cls = Unreal.LuaLoadClass(Owner, ClassPath)
int FastLuaHelper::LuaLoadClass(lua_State* InL)
{
	int32 tp = lua_gettop(InL);
	if (tp < 2)
	{
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

	UClass* ObjClass = FindObject<UClass>(ANY_PACKAGE, *ClassName);
	if (ObjClass == nullptr)
	{
		LoadObject<UClass>(ObjOuter, *ClassName);
	}
	if (ObjClass == nullptr && ClassName.Contains(FString("_C")) == false)
	{
		int32 Pos = ClassName.Find(FString("'/"));
		ClassName = ClassName.Mid(Pos);
		ClassName.ReplaceInline(*FString("'"), *FString(""));
		ClassName += FString("_C");
		ObjClass = LoadObject<UClass>(ObjOuter, *ClassName);
	}

	if (ObjClass)
	{
		FastLuaHelper::PushObject(InL, ObjClass);
		return 1;
	}
	else
	{
		return 0;
	}
}

//local obj = Unreal.LuaNewObject(Owner, ClassName, ObjectName[option])
int FastLuaHelper::LuaNewObject(lua_State* InL)
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
		LoadObject<UClass>(ObjOuter, *ClassName);
	}

	if (ObjClass == nullptr && ClassName.Contains(FString("_C")) == false)
	{
		int32 Pos = ClassName.Find(FString("'/"));
		if (Pos > 0)
		{
			ClassName = ClassName.Mid(Pos);
			ClassName.ReplaceInline(*FString("'"), *FString(""));
			ClassName += FString("_C");
			ObjClass = LoadObject<UClass>(ObjOuter, *ClassName);
		}
	}

	if (ObjClass == nullptr)
	{
		return 0;
	}

	UObject* NewObj = NewObject<UObject>(ObjOuter, ObjClass, FName(*ObjName));
	if (NewObj == nullptr)
	{
		return 0;
	}

	FastLuaHelper::PushObject(InL, NewObj);
	return 1;
}

//local VectorInstance = Unreal.LuaNewStruct('Vector')
int FastLuaHelper::LuaNewStruct(lua_State* InL)
{
	FString StructName = UTF8_TO_TCHAR(lua_tostring(InL, 1));
	UScriptStruct* StructClass = FindObject<UScriptStruct>(ANY_PACKAGE, *StructName);
	PushStruct(InL, StructClass, nullptr);

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
			FastLuaHelper::FetchProperty(InL, Prop, FuncParam.GetStructMemory(), StackTop++);
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

int FastLuaHelper::LuaNewDelegate(lua_State* InL)
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
		PushDelegate(InL, nullptr, nullptr, bIsMulti, Func);
	}

	return 1;
}

//Lua usage: LoadMapEndedEvent:Bind(LuaFunction, LuaObj)
int FastLuaHelper::LuaBindDelegate(lua_State* InL)
{
	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
	lua_pop(InL, 1);

	FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, 1);

	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaUnrealWrapperType::Delegate || !lua_isfunction(InL, 2))
	{
		luaL_traceback(InL, InL, "Error in BindDelegate", 1);
		FastLuaHelper::LuaLog(FString::Printf(TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(InL, -1))), 1, LuaWrapper);
		lua_pushnil(InL);
		return 1;
	}

	UFastLuaDelegate* DelegateObject = NewObject<UFastLuaDelegate>(GetTransientPackage(), FName(lua_tostring(InL, 4)));
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
			ScriptDelegate.BindUFunction(DelegateObject, UFastLuaDelegate::GetWrapperFunctionName());
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
			SingleDelegate->BindUFunction(DelegateObject, UFastLuaDelegate::GetWrapperFunctionName());
			DelegateObject->DelegateInst = SingleDelegate;
			LuaWrapper->DelegateCallLuaList.Add(DelegateObject);
		}
	}

	DelegateObject->LuaState = InL;
	//return wrapper object
	FastLuaHelper::PushObject(InL, DelegateObject);
	return 1;
}

//Lua usage: LoadMapEndedEvent:Unbind(WrapperObject[option, remove single])
int FastLuaHelper::LuaUnbindDelegate(lua_State* InL)
{
	bool bIsMulti = false;
	UFastLuaDelegate* FuncObj = nullptr;
	const void* DelegateInst = nullptr;

	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
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

	FuncObj = Cast<UFastLuaDelegate>(FetchObject(InL, 2));
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
				if (UFastLuaDelegate * LuaObj = Cast<UFastLuaDelegate>(It))
				{
					LuaObj->Unbind();
				}
			}
		}
		else
		{
			FScriptDelegate* SingleDelegate = (FScriptDelegate*)DelegateInst;
			if (UFastLuaDelegate * LuaObj = Cast<UFastLuaDelegate>(SingleDelegate->GetUObject()))
			{
				LuaObj->Unbind();
			}
		}
	}

	lua_pushboolean(InL, true);
	return 1;
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
		LuaWrapper->SetLuaTickFunction(0);
	}

	lua_pushboolean(InL, true);
	return 1;
}

int FastLuaHelper::UserDelegateGC(lua_State* InL)
{
	FLuaDelegateWrapper* DelegateWrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, -1);

	if (DelegateWrapper && DelegateWrapper->DelegateInst && DelegateWrapper->WrapperType == ELuaUnrealWrapperType::Delegate)
	{
		if (DelegateWrapper->bIsUserDefined)
		{
			if (DelegateWrapper->bIsMulti)
			{
				((FMulticastScriptDelegate*)DelegateWrapper->DelegateInst)->Clear();
				((FMulticastScriptDelegate*)DelegateWrapper->DelegateInst)->~FMulticastScriptDelegate();
			}
			else
			{
				((FScriptDelegate*)DelegateWrapper->DelegateInst)->Clear();
				((FScriptDelegate*)DelegateWrapper->DelegateInst)->~FScriptDelegate();
			}

			FMemory::Free(DelegateWrapper->DelegateInst);
			DelegateWrapper->DelegateInst = nullptr;
		}
	}

	return 0;
}

int FastLuaHelper::StructGC(lua_State* InL)
{
	FLuaStructWrapper* StructWrapper = (FLuaStructWrapper*)lua_touserdata(InL, -1);

	if (StructWrapper && StructWrapper->StructInst && StructWrapper->WrapperType == ELuaUnrealWrapperType::Struct)
	{
		StructWrapper->StructType->DestroyStruct(StructWrapper->StructInst);
		FMemory::Free(StructWrapper->StructInst);
		StructWrapper->StructInst = nullptr;
	}

	return 0;
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

	FetchProperty(L, Prop, Wrapper->ObjInst.Get(), 2);

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

	PushProperty(InL, Prop, Wrapper->StructInst, !Prop->HasAnyPropertyFlags(CPF_BlueprintReadOnly));
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

	FetchProperty(InL, Prop, Wrapper->StructInst, 2);

	return 0;
}

bool FastLuaHelper::RegisterClassMetatable(lua_State* InL, const UClass* InClass)
{
	if (InClass == nullptr || InL == nullptr)
	{
		return false;
	}

	int tp = lua_gettop(InL);

	//try find metatable
	lua_rawgetp(InL, LUA_REGISTRYINDEX, InClass);
	if (lua_istable(InL, -1))
	{
		//already exist!
		lua_pop(InL, 1);
	}
	else
	{
		lua_pop(InL, 1);

		//create new table
		lua_newtable(InL);
		{
			lua_pushvalue(InL, -1);
			lua_rawsetp(InL, LUA_REGISTRYINDEX, InClass);

			lua_pushvalue(InL, -1);
			lua_setfield(InL, -2, "__index");

			UClass* SuperClass = InClass->GetSuperClass();
			while (SuperClass)
			{
				if (RegisterClassMetatable(InL, SuperClass))
				{
					lua_rawgetp(InL, LUA_REGISTRYINDEX, SuperClass);
					lua_setmetatable(InL, -2);
					break;
				}
				SuperClass = SuperClass->GetSuperClass();
			}
		}

		for (TFieldIterator<UProperty>It(InClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			lua_pushlightuserdata(InL, *It);
			lua_pushcclosure(InL, GetObjectProperty, 1);
			FString GetPropName = FString("Get") + It->GetName();
			lua_setfield(InL, -2, TCHAR_TO_UTF8(*GetPropName));

			lua_pushlightuserdata(InL, *It);
			lua_pushcclosure(InL, SetObjectProperty, 1);
			FString SetPropName = FString("Set") + It->GetName();
			lua_setfield(InL, -2, TCHAR_TO_UTF8(*SetPropName));
		}

		for (TFieldIterator<UFunction>It(InClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			lua_pushlightuserdata(InL, *It);
			lua_pushcclosure(InL, CallUnrealFunction, 1);
			lua_setfield(InL, -2, TCHAR_TO_UTF8(*It->GetName()));
		}
	}

	lua_settop(InL, tp);

	return true;
}

bool FastLuaHelper::RegisterStructMetatable(lua_State* InL, const UScriptStruct* InStruct)
{
	if (InStruct == nullptr || InL == nullptr)
	{
		return false;
	}

	int tp = lua_gettop(InL);

	//try find metatable
	lua_rawgetp(InL, LUA_REGISTRYINDEX, InStruct);
	if (lua_istable(InL, -1))
	{
		//already exist!
		lua_pop(InL, 1);
	}
	else
	{
		lua_pop(InL, 1);

		//create new table
		lua_newtable(InL);
		{
			lua_pushvalue(InL, -1);
			lua_rawsetp(InL, LUA_REGISTRYINDEX, InStruct);

			lua_pushvalue(InL, -1);
			lua_setfield(InL, -2, "__index");

			lua_pushcfunction(InL, FastLuaHelper::StructGC);
			lua_setfield(InL, -2, "__gc");

			UScriptStruct* SuperStruct = Cast<UScriptStruct>(InStruct->GetSuperStruct());
			while (SuperStruct)
			{
				if (RegisterStructMetatable(InL, SuperStruct))
				{
					lua_rawgetp(InL, LUA_REGISTRYINDEX, SuperStruct);
					lua_setmetatable(InL, -2);
					break;
				}
				SuperStruct = Cast<UScriptStruct>(SuperStruct->GetSuperStruct());
			}
		}

		for (TFieldIterator<UProperty>It(InStruct, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			lua_pushlightuserdata(InL, *It);
			lua_pushcclosure(InL, GetStructProperty, 1);
			FString GetPropName = FString("Get") + It->GetName();
			lua_setfield(InL, -2, TCHAR_TO_UTF8(*GetPropName));

			lua_pushlightuserdata(InL, *It);
			lua_pushcclosure(InL, SetStructProperty, 1);
			FString SetPropName = FString("Set") + It->GetName();
			lua_setfield(InL, -2, TCHAR_TO_UTF8(*SetPropName));
		}

	}

	lua_settop(InL, tp);

	return true;
}
