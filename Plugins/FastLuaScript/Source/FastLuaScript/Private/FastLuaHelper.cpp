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
	bool bBPCallable = InFunction && InFunction->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Public);
	if (bBPCallable)
	{
		for (TFieldIterator<UProperty> It(InFunction); It; ++It)
		{
			if (IsScriptReadableProperty(*It) == false)
			{
				return false;
			}
		}
	}

	return bBPCallable;
}

bool FastLuaHelper::IsScriptReadableProperty(const UProperty* InProperty)
{
	const uint64 ReadableFlags = CPF_BlueprintAssignable | CPF_BlueprintVisible | CPF_InstancedReference;

	if (const UInterfaceProperty* InterfaceProp = Cast<UInterfaceProperty>(InProperty))
	{
		return false;
	}
	else if (const UDelegateProperty* DelegateProp = Cast<UDelegateProperty>(InProperty))
	{
		return false;
	}
	else if (const UMulticastDelegateProperty* MultiDelegateProp = Cast<UMulticastDelegateProperty>(InProperty))
	{
		return false;
	}
	else if (const UClassProperty * ClassProp = Cast<UClassProperty>(InProperty))
	{
		return false;
	}
	else if (const USetProperty * SetProp = Cast<USetProperty>(InProperty))
	{
		return false;
	}

	return InProperty && InProperty->HasAnyPropertyFlags(ReadableFlags) && InProperty->HasAnyPropertyFlags(CPF_NativeAccessSpecifierPublic) && (InProperty->HasAnyPropertyFlags(CPF_Deprecated) == false);
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

FString FastLuaHelper::GeneratePushPropertyStr(const UProperty* InProp, const FString& InParamName)
{
	FString BodyStr = FString::Printf(TEXT("lua_pushnil(InL);"));

	if (InProp == nullptr || InProp->GetClass() == nullptr)
	{
		LuaLog(FString("Property is nil"));
		return BodyStr;
	}

	if (const UIntProperty* IntProp = Cast<UIntProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_pushinteger(InL, %s);"), *InParamName);
	}
	else if (const UFloatProperty* FloatProp = Cast<UFloatProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_pushnumber(InL, %s);"), *InParamName);
	}
	else if (const UEnumProperty* EnumProp = Cast<UEnumProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_pushinteger(InL, (uint8)%s);"), *InParamName);
	}
	else if (const UByteProperty* ByteProp = Cast<UByteProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_pushinteger(InL, (uint8)%s);"), *InParamName);
	}
	else if (const UBoolProperty* BoolProp = Cast<UBoolProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_pushboolean(InL, (bool)%s);"), *InParamName);
	}
	else if (const UNameProperty* NameProp = Cast<UNameProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_pushstring(InL, TCHAR_TO_UTF8(*%s.ToString()));"), *InParamName);
	}
	else if (const UStrProperty* StrProp = Cast<UStrProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_pushstring(InL, TCHAR_TO_UTF8(*%s));"), *InParamName);
	}
	else if (const UTextProperty* TextProp = Cast<UTextProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_pushstring(InL, TCHAR_TO_UTF8(*%s.ToString()));"), *InParamName);
	}
	else if (const UStructProperty * StructProp = Cast<UStructProperty>(InProp))
	{
		FString StructName = StructProp->Struct->GetName();
		BodyStr = FString::Printf(TEXT("FastLuaHelper::PushStruct(InL, (UScriptStruct*)FindObject<UScriptStruct>(ANY_PACKAGE, *FString(\"%s\")), &%s);"), *StructName, *InParamName);
	}
	else if (const UClassProperty* ClassProp = Cast<UClassProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FastLuaHelper::PushObject(InL, (UObject*)%s);"), *InParamName);
	}
	else if (const UObjectProperty* ObjectProp = Cast<UObjectProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FastLuaHelper::PushObject(InL, (UObject*)%s);"), *InParamName);
	}
	else if (const UWeakObjectProperty * WeakObjectProp = Cast<UWeakObjectProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FastLuaHelper::PushObject(InL, (UObject*)%s.Get());"), *InParamName);
	}
	else if (const UDelegateProperty* DelegateProp = Cast<UDelegateProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FastLuaHelper::PushDelegate(InL, (void*)&%s, false);"), *InParamName);
	}
	else if (const UMulticastDelegateProperty* MultiDelegateProp = Cast<UMulticastDelegateProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FastLuaHelper::PushDelegate(InL, (void*)&%s, true);"), *InParamName);
	}
	else if (const UArrayProperty* ArrayProp = Cast<UArrayProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_newtable(InL);\tfor(int32 i = 0; i < %s.Num(); ++i)\n\t{\n\t %s\t lua_rawseti(InL, -2, i + 1); \n\t}"), *InParamName, *FastLuaHelper::GeneratePushPropertyStr(ArrayProp->Inner, FString::Printf(TEXT("%s[i]"), *InParamName)));
	}
	else if (const USetProperty* SetProp = Cast<USetProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_newtable(InL);\tfor(int32 i = 0; i < %s.Num(); ++i)\n\t{\n\t %s\t lua_rawseti(InL, -2, i + 1); \n\t}"), *InParamName, *FastLuaHelper::GeneratePushPropertyStr(SetProp->ElementProp, FString::Printf(TEXT("%s[i]"), *InParamName)));
	}
	else if (const UMapProperty* MapProp = Cast<UMapProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_newtable(InL);  \n\t for(auto& It : %s) \n\t { \n\t %s \n\t %s \n\t lua_rawset(InL, -3); \n\t }"), *InParamName, *FastLuaHelper::GeneratePushPropertyStr(MapProp->KeyProp, FString("It->Key")), *FastLuaHelper::GeneratePushPropertyStr(MapProp->ValueProp, FString("It->Value")));
	}

	return BodyStr;
}

FString FastLuaHelper::GenerateFetchPropertyStr(const UProperty* InProp, const FString& InParamName, int32 InStackIndex /*= -1*/, const UStruct* InSruct/*= nullptr*/)
{
	FString BodyStr;

	//this case is used for fetching lua's self!
	if (InProp == nullptr && InSruct != nullptr)
	{
		if (const UClass* Cls = Cast<UClass>(InSruct))
		{
			FString MetaClassName = Cls->GetName();
			FString MetaClassPrefix = Cls->GetPrefixCPP();
			BodyStr = FString::Printf(TEXT("FLuaObjectWrapper* %s_Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, %d);\t%s%s* %s = (%s%s*)(%s_Wrapper->ObjInst.Get());"), *InParamName, InStackIndex, *MetaClassPrefix, *MetaClassName, *InParamName, *MetaClassPrefix, *MetaClassName, *InParamName);
		}
		else if (const UScriptStruct* Struct = Cast<UScriptStruct>(InSruct))
		{
			FString MetaClassName = Struct->GetName();
			FString MetaClassPrefix = Struct->GetPrefixCPP();

			BodyStr = FString::Printf(TEXT("FLuaStructWrapper* %s_Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, %d); \t%s%s %s; \t%s_Wrapper->StructType->CopyScriptStruct(&%s, &(%s_Wrapper->StructInst));"), *InParamName, InStackIndex, *MetaClassPrefix, *MetaClassName, *InParamName, *InParamName, *InParamName, *InParamName);
		}
	}
	else if (const UIntProperty* IntProp = Cast<UIntProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("int32 %s = lua_tointeger(InL, %d);"), *InParamName, InStackIndex);
	}
	else if (const UInt64Property * Int64Prop = Cast<UInt64Property>(InProp))
	{
		BodyStr = FString::Printf(TEXT("int64 %s = lua_tointeger(InL, %d);"), *InParamName, InStackIndex);
	}
	else if (const UFloatProperty* FloatProp = Cast<UFloatProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("float %s = lua_tonumber(InL, %d);"), *InParamName, InStackIndex);
	}
	else if (const UBoolProperty* BoolProp = Cast<UBoolProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("bool %s = (bool)lua_toboolean(InL, %d);"), *InParamName, InStackIndex);
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
		if (ByteProp->Enum)
		{
			FString EnumName = ByteProp->Enum->CppType;
			BodyStr = FString::Printf(TEXT("TEnumAsByte<%s> %s = (%s)lua_tointeger(InL, %d);"), *EnumName, *InParamName, *EnumName, InStackIndex);
		}
		else
		{
			BodyStr = FString::Printf(TEXT("uint8 %s = (uint8)lua_tointeger(InL, %d);"), *InParamName, InStackIndex);
		}
	}
	else if (const UEnumProperty* EnumProp = Cast<UEnumProperty>(InProp))
	{
		FString EnumName = EnumProp->GetEnum()->CppType;
		BodyStr = FString::Printf(TEXT("%s %s = (%s)lua_tointeger(InL, %d);"), *EnumName, *InParamName, *EnumName, InStackIndex);
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
		BodyStr = FString::Printf(TEXT("FLuaObjectWrapper* %s_Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, %d);\t%s%s* %s = (%s%s*)(%s_Wrapper->ObjInst.Get());"), *InParamName, InStackIndex, *MetaClassPrefix, *MetaClassName, *InParamName, *MetaClassPrefix, *MetaClassName, *InParamName);
	}
	else if (const USoftClassProperty* SoftClassProp = Cast<USoftClassProperty>(InProp))
	{
		FString MetaClassName = SoftClassProp->MetaClass->GetName();
		BodyStr = FString::Printf(TEXT("FLuaSoftClassWrapper* %s_Wrapper = (FLuaSoftClassWrapper*)lua_touserdata(InL, %d);\tTSoftClassPtr<U%s> %s = (TSoftClassPtr<U%s>)%s_Wrapper->SoftClassInst;"), *InParamName, InStackIndex, *MetaClassName, *InParamName, *MetaClassName, *InParamName);
	}
	else if (const USoftObjectProperty* SoftObjectProp = Cast<USoftObjectProperty>(InProp))
	{
		FString MetaClassName = SoftObjectProp->PropertyClass->GetName();
		BodyStr = FString::Printf(TEXT("FLuaSoftObjectWrapper* %s_Wrapper = (FLuaSoftObjectWrapper*)lua_touserdata(InL, %d);\tTSoftObjectPtr<U%s> %s = *(TSoftObjectPtr<U%s>*)&%s_Wrapper->SoftObjInst;"), *InParamName, InStackIndex, *MetaClassName, *InParamName, *MetaClassName, *InParamName);
	}
	else if (const UWeakObjectProperty* WeakObjectProp = Cast<UWeakObjectProperty>(InProp))
	{
		FString MetaClassName = WeakObjectProp->PropertyClass->GetName();
		FString MetaClassPrefix = WeakObjectProp->PropertyClass->GetPrefixCPP();
		BodyStr = FString::Printf(TEXT("FLuaObjectWrapper* %s_Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, %d);\t%s%s* %s = (%s%s*)(%s_Wrapper->ObjInst.Get());"), *InParamName, InStackIndex, *MetaClassPrefix, *MetaClassName, *InParamName, *MetaClassPrefix, *MetaClassName, *InParamName);
	}
	else if (const UInterfaceProperty* InterfaceProp = Cast<UInterfaceProperty>(InProp))
	{
		FString MetaClassName = InterfaceProp->InterfaceClass->GetName();
		FString InterfaceName = InterfaceProp->GetName();
		BodyStr = FString::Printf(TEXT("FLuaInterfaceWrapper* %s_Wrapper = (FLuaInterfaceWrapper*)lua_touserdata(InL, %d);\tTScriptInterface<I%s>& %s = *(TScriptInterface<I%s>*)%s_Wrapper->InterfaceInst;"), *InParamName, InStackIndex, *MetaClassName, *InParamName, *MetaClassName, *InParamName);
	}
	else if (const UDelegateProperty* DelegateProp = Cast<UDelegateProperty>(InProp))
	{
		FString FuncName = DelegateProp->SignatureFunction->GetName();
		int32 SplitIndex = INDEX_NONE;
		FuncName.FindChar('_', SplitIndex);
		FString DelegateName = FuncName.Left(SplitIndex);

		BodyStr = FString::Printf(TEXT("FLuaDelegateWrapper* %s_Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, %d);\tF%s& %s = *(F%s*)%s_Wrapper->DelegateInst;"), *InParamName, InStackIndex, *DelegateName, *InParamName, *DelegateName, *InParamName);
	}
	else if (const UMulticastDelegateProperty* MultiDelegateProp = Cast<UMulticastDelegateProperty>(InProp))
	{
		FString FuncName = MultiDelegateProp->SignatureFunction->GetName();
		int32 SplitIndex = INDEX_NONE;
		FuncName.FindChar('_', SplitIndex);
		FString DelegateName = FuncName.Left(SplitIndex);

		BodyStr = FString::Printf(TEXT("FLuaDelegateWrapper* %s_Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, %d);\tF%s& %s = *(F%s*)%s_Wrapper->DelegateInst;"), *InParamName, InStackIndex, *DelegateName, *InParamName, *DelegateName, *InParamName);
	}
	else if (const UArrayProperty* ArrayProp = Cast<UArrayProperty>(InProp))
	{
		FString ElementTypeName = GetPropertyTypeName(ArrayProp->Inner);

		FString PointerFlag;
		if (IsPointerType(ElementTypeName))
		{
			PointerFlag = FString("*");
		}

		FString NewElement = FString::Printf(TEXT("%s"), *FastLuaHelper::GenerateFetchPropertyStr(ArrayProp->Inner, FString::Printf(TEXT("Temp_NewElement")), -1));

		BodyStr = FString::Printf(TEXT("TArray<%s%s> %s;\n\t{\n\tif(lua_istable(InL, %d))\n\t{\n\tlua_pushnil(InL);\n\twhile(lua_next(InL, -2))\n\t{\n\t %s \n\t%s.Add(Temp_NewElement);\n\tlua_pop(InL, 1); \n\t}\n\t}  \n\t}"), *ElementTypeName, *PointerFlag, *InParamName, InStackIndex, *NewElement, *InParamName);
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

		FString NewKeyElement = FString::Printf(TEXT("%s"), *FastLuaHelper::GenerateFetchPropertyStr(MapProp->KeyProp, FString::Printf(TEXT("Temp_NewKeyElement")), -2));

		FString NewValueElement = FString::Printf(TEXT("%s"), *FastLuaHelper::GenerateFetchPropertyStr(MapProp->ValueProp, FString::Printf(TEXT("Temp_NewValueElement")), -1));

		BodyStr = FString::Printf(TEXT("TMap<%s%s, %s%s> %s;\n\t{\n\tif(lua_istable(InL, %d))\n\t{\n\tlua_pushnil(InL);\n\twhile(lua_next(InL, -2))\n\t{\n\t%s\n\t%s \n\t%s.Add(Temp_NewKeyElement, Temp_NewValueElement);\n\tlua_pop(InL, 1); \n\t}\n\t}  \n\t}"), *KeyTypeName, *KeyPointerFlag, *ValueTypeName, *ValuePointerFlag, *InParamName, InStackIndex, *NewKeyElement, *NewValueElement, *InParamName);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("breakpoint!"));
	}

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

void FastLuaHelper::PushDelegate(lua_State* InL, void* InDelegateInst, bool InMulti)
{
	bool bValid = false;
	if (InMulti == false)
	{
		/*FScriptDelegate* Delegate = ((UDelegateProperty*)InDelegateProperty)->GetPropertyValuePtr_InContainer(InBuff);
		if (Delegate)
		{
			FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_newuserdata(InL, sizeof(FLuaDelegateWrapper));
			Wrapper->WrapperType = ELuaUnrealWrapperType::Delegate;
			Wrapper->bIsMulti = false;
			Wrapper->DelegateInst = Delegate;
			Wrapper->FunctionSignature = ((UDelegateProperty*)InDelegateProperty)->SignatureFunction;
			bValid = true;
		}*/
	}
	else
	{
		/*FMulticastScriptDelegate* Delegate = ((UMulticastDelegateProperty*)InDelegateProperty)->ContainerPtrToValuePtr<FMulticastScriptDelegate>(InBuff);
		if (Delegate)
		{
			FLuaDelegateWrapper* Wrapper = (FLuaDelegateWrapper*)lua_newuserdata(InL, sizeof(FLuaDelegateWrapper));
			Wrapper->WrapperType = ELuaUnrealWrapperType::Delegate;
			Wrapper->bIsMulti = true;
			Wrapper->DelegateInst = Delegate;
			Wrapper->FunctionSignature = ((UMulticastDelegateProperty*)InDelegateProperty)->SignatureFunction;
			bValid = true;
		}*/
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

	//GeneratePushPropertyStr(L, Prop, Wrapper->ObjInst.Get(), !Prop->HasAnyPropertyFlags(CPF_BlueprintReadOnly));

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

	//GeneratePushPropertyStr(InL, Prop, &(Wrapper->StructInst), !Prop->HasAnyPropertyFlags(CPF_BlueprintReadOnly));
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
			//FastLuaHelper::GeneratePushPropertyStr(L, ReturnProp, FuncParam.GetStructMemory(), false);
			++ReturnNum;
		}

		if (Func->HasAnyFunctionFlags(FUNC_HasOutParms))
		{
			for (TFieldIterator<UProperty> It(Func); It; ++It)
			{
				UProperty* Prop = *It;
				if (Prop->HasAnyPropertyFlags(CPF_OutParm) && !Prop->HasAnyPropertyFlags(CPF_ConstParm))
				{
					//FastLuaHelper::GeneratePushPropertyStr(L, *It, FuncParam.GetStructMemory(), false);
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
		//FastLuaHelper::GeneratePushPropertyStr(InL, ReturnProp, FuncParam.GetStructMemory(), true);
		++ReturnNum;
	}

	if (SignatureFunction->HasAnyFunctionFlags(FUNC_HasOutParms))
	{
		for (TFieldIterator<UProperty> It(SignatureFunction); It; ++It)
		{
			UProperty* Prop = *It;
			if (Prop->HasAnyPropertyFlags(CPF_OutParm) && !Prop->HasAnyPropertyFlags(CPF_ConstParm))
			{
				//FastLuaHelper::GeneratePushPropertyStr(InL, *It, FuncParam.GetStructMemory(), false);
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
