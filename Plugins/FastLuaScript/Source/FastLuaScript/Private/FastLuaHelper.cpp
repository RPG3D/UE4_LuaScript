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
		lua_rawgetp(InL, LUA_REGISTRYINDEX, Class);
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
	lua_rawgetp(InL, LUA_REGISTRYINDEX, InStruct);
	if (lua_istable(InL, -1))
	{
		lua_setmetatable(InL, -2);
	}
	else
	{
		lua_pop(InL, 1);
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
		/*lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
		FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
		lua_pop(InL, 1);
		lua_rawgeti(InL, LUA_REGISTRYINDEX, LuaWrapper->DelegateMetatableIndex);
		lua_setmetatable(InL, -2);*/
	}
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
		//if (Inst && Inst->bStatMemory && Inst->GetLuaSate())
		//{
			//int k = lua_gc(Inst->GetLuaSate(), LUA_GCCOUNT);
			//int b = lua_gc(Inst->GetLuaSate(), LUA_GCCOUNTB);
			//Inst->LuaMemory = (k << 10) + b;
			//SET_MEMORY_STAT(STAT_LuaMemory, Inst->LuaMemory);
		//}
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


int FastLuaHelper::LuaNewStruct(lua_State* InL)
{
	FString StructName = UTF8_TO_TCHAR(lua_tostring(InL, 1));
	UScriptStruct* StructClass = FindObject<UScriptStruct>(ANY_PACKAGE, *StructName);
	if (StructClass == nullptr)
	{
		lua_pushnil(InL);
	}
	else
	{
		FLuaStructWrapper* StructWrapper = (FLuaStructWrapper*)lua_newuserdata(InL, sizeof(FLuaStructWrapper) + StructClass->GetStructureSize());
		StructWrapper->StructType = StructClass;
		StructWrapper->WrapperType = ELuaUnrealWrapperType::Struct;
		StructClass->InitializeDefaultValue((uint8*)(&StructWrapper->StructInst));

		lua_rawgetp(InL, LUA_REGISTRYINDEX, StructClass);
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
