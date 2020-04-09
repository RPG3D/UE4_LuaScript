

#include "GenerateLua.h"
#include "UObject/MetaData.h"
#include "Misc/FileHelper.h"
#include "FastLuaHelper.h"
#include "HAL/PlatformFilemanager.h"



FString GenerateLua::GenerateFunctionBodyStr(const class UFunction* InFunction, const class UClass* InClass)
{
	const FString ClassName = InClass->GetName();
	const FString ClassPrefix = InClass->GetPrefixCPP();
	
	const FString FuncName = InFunction->GetName();
	
	//first line: FLuaObjectWrapper* ThisObject_Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	FString BodyStr = FString("\t") + GenerateFetchPropertyStr(nullptr, FString("ThisObject"), 1, InClass) + FString("\n");
	BodyStr += FString("\tif(ThisObject == nullptr) { return 0; } \n");

	int32 LuaStackIndex = 2;

	TArray<FString> InputParamNames;

	//fill params
	FProperty* ReturnProp = nullptr;

	for (TFieldIterator<FProperty> It(InFunction); It; ++It)
	{
		FProperty* Prop = *It;
		FString PropName = Prop->GetName();
		if (Prop->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			ReturnProp = Prop;
		}
		else
		{
			FString TmpParamName = FString::Printf(TEXT("Param_%d_%s"), LuaStackIndex, *PropName);
			InputParamNames.Add(TmpParamName);
			FString FetchPropStr = GenerateFetchPropertyStr(Prop, TmpParamName, LuaStackIndex, InClass);
			++LuaStackIndex;
			BodyStr += FString::Printf(TEXT("\t%s\n"), *FetchPropStr);
		}
	}

	FString FuncParamStr;
	for (int32 i = 0; i < InputParamNames.Num(); ++i)
	{
		FuncParamStr += InputParamNames[i];
		if (i != InputParamNames.Num() - 1)
		{
			FuncParamStr += FString(", ");
		}
	}
	
	FString ReturnPropStr;
	if (ReturnProp != nullptr)
	{
		FString ReturnTypeName = FastLuaHelper::GetPropertyTypeName(ReturnProp);
		ReturnPropStr = FString::Printf(TEXT("%s ReturnValue = "), *ReturnTypeName);
	}

	FString CallFuncStr = FString::Printf(TEXT("%sThisObject->%s(%s);"), *ReturnPropStr, *FuncName, *FuncParamStr);

	BodyStr += FString::Printf(TEXT("\n\t%s\n\t"), *CallFuncStr);

	//push return value
	if (ReturnProp)
	{
		FString PushReturnStr = FString::Printf(TEXT("%s"), *GeneratePushPropertyStr(ReturnProp, FString("ReturnValue")));
		BodyStr += PushReturnStr;
	}

	//return ref param
	int32 OutRefNum = 0;
	int32 OutRefIndex = 2;
	for (TFieldIterator<FProperty> It(InFunction); It; ++It)
	{
		FProperty* Prop = *It;
		FString PropName = Prop->GetName();
		if (Prop->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			continue;
		}

		if (Prop->HasAnyPropertyFlags(CPF_OutParm) && !Prop->HasAnyPropertyFlags(CPF_ConstParm | CPF_ReturnParm))
		{
			FString TmpParamName = FString::Printf(TEXT("Param_%d_%s"), OutRefIndex, *PropName);
			BodyStr += FString("\n\t") + GeneratePushPropertyStr(Prop, TmpParamName);

			++OutRefNum;
		}
		++OutRefIndex;
	}

	BodyStr += FString::Printf(TEXT("\n\treturn %d;"), !!ReturnProp + OutRefNum);

	return BodyStr;
}


int32 GenerateLua::InitConfig()
{
	if (FFileHelper::LoadFileToStringArray(ModulesShouldExport, *(FPaths::ProjectConfigDir() / FString("ModuleToExport.txt"))))
	{
		UE_LOG(LogTemp, Warning, TEXT("Read ModuleToExport.txt OK!"));
	}

	if (FFileHelper::LoadFileToStringArray(IgnoredClass, *(FPaths::ProjectConfigDir() / FString("/IgnoredClass.txt"))))
	{
		UE_LOG(LogTemp, Warning, TEXT("Read IgnoredClass.txt OK!"));
	}

	return 0;
}

int32 GenerateLua::GeneratedCode() const
{

	//delete old generated files
	FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*CodeDirectory);

	TMap<FString, const UClass*> AllGeneratedClass;

	//write class wrapper file
	for (TObjectIterator<UClass>It; It; ++It)
	{
		if (const UClass * Class = Cast<const UClass>(*It))
		{
			if (GenerateCodeForClass(Class) == 0)
			{
				AllGeneratedClass.Add(Class->GetName(), Class);
			}

		}
	}

	//write API header file 
	{
		FString APIHeaderPath = CodeDirectory / FString("FastLuaAPI.h");
		FString HeaderStr = FString("\n// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved. \n\n#pragma once \n\n#include \"CoreMinimal.h\" \n\n");
		HeaderStr += FString::Printf(TEXT("struct FastLuaAPI\n{\n"));
		HeaderStr += FString("\tstatic int32 RegisterUnrealClass(struct lua_State* InL);\n\n");
		HeaderStr += FString("};\n");

		FFileHelper::SaveStringToFile(HeaderStr, *APIHeaderPath);
	}

	//write API source file
	{
		FString APISourcePath = CodeDirectory / FString("FastLuaAPI.cpp");
		FString SourceStr = FString("\n// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved. \n\n#include \"FastLuaAPI.h\"  \n\n#include \"FastLuaHelper.h\"\n\n");

		//include class header file
		for (auto It : AllGeneratedClass)
		{
			SourceStr += FString::Printf(TEXT("#include \"GeneratedLua/Class/Lua_%s.h\"\n"), *It.Key);
		}

		SourceStr += FString("\n\n");

		//class
		SourceStr += FString("int32 FastLuaAPI::RegisterUnrealClass(struct lua_State* InL)\n{\n");
		SourceStr += FString::Printf(TEXT("\tTArray<const UClass*> AllRegistedClass; \n\tUClass* TempClass = nullptr;\n\n\n"));
		for (auto It : AllGeneratedClass)
		{
			//luaL_newlib(InL, Lua_Class::FuncsBindToLua);
			//lua_rawsetp(InL, LUA_REGISTRYINDEX, (void*)FindObject<UClass>(ANY_PACKAGE, *It.Key));
			SourceStr += FString::Printf(TEXT("\tluaL_newlib(InL, Lua_%s::FuncsBindToLua);\n"), *It.Key);
			SourceStr += FString::Printf(TEXT("\tTempClass = FindObject<UClass>(ANY_PACKAGE, *FString(\"%s\"));\n"), *It.Key);
			SourceStr += FString::Printf(TEXT("\tAllRegistedClass.Add(TempClass);\n"));
			SourceStr += FString::Printf(TEXT("\tlua_rawsetp(InL, LUA_REGISTRYINDEX, (const void*)TempClass);\n\n"));

		}
		SourceStr += FString("\tFastLuaHelper::FixClassMetatable(InL, AllRegistedClass);\n");
		SourceStr += FString("\n\treturn 1;\n}\n\n");

		FFileHelper::SaveStringToFile(SourceStr, *APISourcePath);
	}

	return 0;
}

bool GenerateLua::ShouldGenerateClass(const class UClass* InClass) const
{
	if (!InClass || !InClass->HasAnyClassFlags(EClassFlags::CLASS_RequiredAPI) || IgnoredClass.Find(InClass->GetName()) >= 0)
	{
		return false;
	}

	UPackage* Pkg = InClass->GetOutermost();
	FString PkgName = Pkg->GetName();

	bool bShouldExportModule = false;
	for (int32 i = 0; i < ModulesShouldExport.Num(); ++i)
	{
		if (PkgName == FString("/Script/") + ModulesShouldExport[i])
		{
			bShouldExportModule = true;
			break;
		}
	}
	
	return bShouldExportModule;
}

bool GenerateLua::ShouldGenerateStruct(const class UScriptStruct* InStruct) const
{
	if (InStruct->StructFlags & EStructFlags::STRUCT_IsPlainOldData)
	{
		return true;
	}

	if (InStruct->StructFlags & EStructFlags::STRUCT_ZeroConstructor)
	{
		return true;
	}

	if (InStruct->StructFlags & EStructFlags::STRUCT_RequiredAPI )
	{
		return true;
	}

	if (IgnoredClass.Find(InStruct->GetName()) >= 0)
	{
		return false;
	}

	return false;
}

bool GenerateLua::ShouldGenerateProperty(const class FProperty* InProperty) const
{
	uint64 ReadableFlags = CPF_NativeAccessSpecifierPublic;
	bool bScriptReadable = InProperty && InProperty->HasAnyPropertyFlags(ReadableFlags)
		&& !InProperty->IsEditorOnlyProperty()
		&& !InProperty->HasAnyPropertyFlags(CPF_Deprecated)
#if WITH_EDITOR
		&& !InProperty->HasMetaData("DeprecationMessage")
#endif
		;

	if (!bScriptReadable)
	{
		return false;
	}

	if (const FStructProperty* StructProp = CastField<FStructProperty>(InProperty))
	{
		if (!ShouldGenerateStruct(StructProp->Struct))
		{
			return false;
		}
	}

	if (const FFieldPathProperty* FieldPathProp = CastField<FFieldPathProperty>(InProperty))
	{
		return false;
	}

	return true;
}

bool GenerateLua::ShouldGenerateFunction(const class UFunction* InFunction) const
{
	bool bScriptCallable = InFunction
		&& InFunction->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure)
		&& InFunction->HasAnyFunctionFlags(FUNC_Public)
		&& !InFunction->IsEditorOnly()
#if WITH_EDITOR
		&& !InFunction->HasMetaData("DeprecationMessage")
		&& !InFunction->HasMetaData("CustomThunk")
#endif
		;

	if (!bScriptCallable)
	{
		return false;
	}

	for (TFieldIterator<const FProperty> It(InFunction, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		if (!ShouldGenerateProperty(*It))
		{
			return false;
		}
	}

	return true;
}

int32 GenerateLua::GenerateCodeForClass(const class UClass* InClass) const
{
	if (!ShouldGenerateClass(InClass))
	{
		return -1;
	}

	FString ClassName = InClass->GetName();

	TMap<FString, FString> FunctionList;

	//first, generate header
	{
		int32 ExportedFieldCount = 0;

		FString HeaderFilePath = FString::Printf(TEXT("%s/Class/Lua_%s.h"), *CodeDirectory, *ClassName);

		FString HeaderStr = FString("\n// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved. \n\n#pragma once \n\n#include \"CoreMinimal.h\" \n\n#include \"lua/lua.hpp\" \n\n");

		FString ClassBeginStr = FString::Printf(TEXT("\nstruct Lua_%s\n{\n"), *ClassName);
		HeaderStr += ClassBeginStr;


		for (TFieldIterator<const UFunction> It(InClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			FString FuncName = It->GetName();

			if (ShouldGenerateFunction(*It) == false)
			{
				continue;
			}
			FString ClassFunctionName = ClassName + FString("::") + FuncName;


			++ExportedFieldCount;
			FunctionList.Add(FuncName, FString("Lua_") + FuncName);

			FString FuncDeclareStr = FString::Printf(TEXT("\tstatic int32 Lua_%s(lua_State* InL);\n\n"), *FuncName);
			HeaderStr += FuncDeclareStr;
		}

		if (FunctionList.Num() < 1)
		{
			return -1;
		}

		//write function bind!
		FString FuncsBindStr = FString::Printf(TEXT("\tstatic luaL_Reg FuncsBindToLua[%d];\n\t"), FunctionList.Num() + 1);
		HeaderStr += FuncsBindStr;

		FString ClassEndStr = FString("\n};\n");
		HeaderStr += ClassEndStr;

		FFileHelper::SaveStringToFile(HeaderStr, *HeaderFilePath);

	}

	//second, generate source
	{
		FString SourceFilePath = FString::Printf(TEXT("%s/Class/Lua_%s.cpp"), *CodeDirectory, *ClassName);

		FString SourceStr = FString("\n// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved. \n\n");
		FString ClassBeginStr = FString::Printf(TEXT("#include \"Lua_%s.h\" \n\n#include \"FastLuaHelper.h\" \n\n"), *ClassName);

		FString RawHeaderPath = InClass->GetMetaData(*FString("IncludePath"));
		if (RawHeaderPath.IsEmpty() == false)
		{
			ClassBeginStr += FString::Printf(TEXT("#include \"%s\"\n\n"), *RawHeaderPath);
			TArray<FString> HeaderList = CollectHeaderFilesReferencedByClass(InClass);
			for (int32 i = 0; i < HeaderList.Num(); ++i)
			{
				ClassBeginStr += FString::Printf(TEXT("#include \"%s\"\n\n"), *HeaderList[i]);
			}
		}

		SourceStr += ClassBeginStr;


		for (TFieldIterator<const UFunction> It(InClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			FString FuncName = It->GetName();
			if (ShouldGenerateFunction(*It) == false)
			{
				continue;
			}
			FString ClassFunctionName = ClassName + FString("::") + FuncName;

			FString FuncBeginStr = FString::Printf(TEXT("int32 Lua_%s::Lua_%s(lua_State* InL)\n{\n"), *ClassName, *FuncName);
			SourceStr += FuncBeginStr;

			FString FuncBodyStr = GenerateFunctionBodyStr(*It, InClass);

			SourceStr += FuncBodyStr;

			FString FuncEndStr = FString("\n\n}\n\n");
			SourceStr += FuncEndStr;
		}

		FString WrapperStructName = FString::Printf(TEXT("Lua_%s"), *ClassName);
		//write function bind!
		FString FuncsBindStr = FString::Printf(TEXT("luaL_Reg %s::FuncsBindToLua[%d] =\n{\n"), *WrapperStructName, FunctionList.Num() + 1);
		for (auto It : FunctionList)
		{
			FuncsBindStr += FString::Printf(TEXT("\t{\"%s\", %s::%s},\n"), *It.Key, *WrapperStructName, *It.Value);
		}

		FuncsBindStr += FString("\t{nullptr, nullptr}\n};\n");

		SourceStr += FuncsBindStr;

		FFileHelper::SaveStringToFile(SourceStr, *SourceFilePath);

	}

	return 0;
}

FString GenerateLua::GeneratePushPropertyStr(const FProperty* InProp, const FString& InParamName)
{
	FString BodyStr = FString::Printf(TEXT("lua_pushnil(InL);"));

	if (InProp == nullptr || InProp->GetClass() == nullptr)
	{
		return BodyStr;
	}

	else if (const FNumericProperty* NumProp = CastField<FNumericProperty>(InProp))
	{
		if (NumProp->IsInteger())
		{
			BodyStr = FString::Printf(TEXT("lua_pushinteger(InL, %s);"), *InParamName);
		}
		else
		{
			BodyStr = FString::Printf(TEXT("lua_pushnumber(InL, %s);"), *InParamName);
		}
		
	}
	else if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_pushboolean(InL, (bool)%s);"), *InParamName);
	}
	else if (const FNameProperty* NameProp = CastField<FNameProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_pushstring(InL, TCHAR_TO_UTF8(*%s.ToString()));"), *InParamName);
	}
	else if (const FStrProperty* StrProp = CastField<FStrProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_pushstring(InL, TCHAR_TO_UTF8(*%s));"), *InParamName);
	}
	else if (const FTextProperty* TextProp = CastField<FTextProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_pushstring(InL, TCHAR_TO_UTF8(*%s.ToString()));"), *InParamName);
	}
	else if (const FStructProperty* StructProp = CastField<FStructProperty>(InProp))
	{
		FString StructName = StructProp->Struct->GetName();
		FString MetatableStr = InParamName;
		int32 SplitIndex = INDEX_NONE;
		InParamName.FindChar('.', SplitIndex);
		if (SplitIndex < 0)
		{
			InParamName.FindChar('>', SplitIndex);
		}
		if (SplitIndex > INDEX_NONE)
		{
			MetatableStr = InParamName.Mid(SplitIndex + 1);
		}
		
		MetatableStr.FindChar('[', SplitIndex);
		if (SplitIndex > INDEX_NONE)
		{
			MetatableStr = MetatableStr.Left(SplitIndex);
		}

		BodyStr = FString::Printf(TEXT("static UScriptStruct* %s_MetatableIndex = FindObject<UScriptStruct>(ANY_PACKAGE, *FString(\"%s\")); \n\tFLuaStructWrapper::PushStruct(InL, %s_MetatableIndex, &%s);"), *MetatableStr, *StructName, *MetatableStr, *InParamName);
	}
	else if (const FClassProperty* ClassProp = CastField<FClassProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FLuaObjectWrapper::PushObject(InL, (UObject*)%s);"), *InParamName);
	}
	else if (const FObjectProperty* ObjectProp = CastField<FObjectProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FLuaObjectWrapper::PushObject(InL, (UObject*)%s);"), *InParamName);
	}
	else if (const FWeakObjectProperty* WeakObjectProp = CastField<FWeakObjectProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FLuaObjectWrapper::PushObject(InL, (UObject*)%s.Get());"), *InParamName);
	}
	else if (const FDelegateProperty* DelegateProp = CastField<FDelegateProperty>(InProp))
	{
		TArray<FString> ParamNames;
		InParamName.ParseIntoArray(ParamNames, *FString("->"));
		if (ParamNames.Num() == 2)
		{
			FString DelegateTypeName = DelegateProp->SignatureFunction->GetName();
			BodyStr = FString::Printf(TEXT("UFunction* TmpFunc = FindObject<UFunction>(ANY_PACKAGE, %s); FLuaDelegateWrapper::PushDelegate(InL, &%s->%s, false, TmpFunc);"), *DelegateTypeName, *ParamNames[0], *ParamNames[1]);
		}
	}
	else if (const FMulticastDelegateProperty* MultiDelegateProp = CastField<FMulticastDelegateProperty>(InProp))
	{
		TArray<FString> ParamNames;
		InParamName.ParseIntoArray(ParamNames, *FString("->"));
		if (ParamNames.Num() == 2)
		{
			FString DelegateTypeName = MultiDelegateProp->SignatureFunction->GetName();
			BodyStr = FString::Printf(TEXT("UFunction* TmpFunc = FindObject<UFunction>(ANY_PACKAGE, %s); FLuaDelegateWrapper::PushDelegate(InL, &%s->%s, true, TmpFunc);"), *DelegateTypeName, *ParamNames[0], *ParamNames[1]);
		}
	}
	else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_newtable(InL); \n\tfor(int32 i = 0; i < %s.Num(); ++i) \n\t{ \n\t\t%s \n\t\tlua_rawseti(InL, -2, i + 1); \n\t}"), *InParamName, *GeneratePushPropertyStr(ArrayProp->Inner, FString::Printf(TEXT("%s[i]"), *InParamName)));
	}
	else if (const FSetProperty* SetProp = CastField<FSetProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_newtable(InL); \t\tfor(int32 i = 0; i < %s.Num(); ++i) \n\t{ \n\t\t%s \n\t\tlua_rawseti(InL, -2, i + 1); \n\t}"), *InParamName, *GeneratePushPropertyStr(SetProp->ElementProp, FString::Printf(TEXT("%s[FSetElementId::FromInteger(i)]"), *InParamName)));
	}
	else if (const FMapProperty* MapProp = CastField<FMapProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_newtable(InL);  \n\tfor(auto& It : %s) \n\t{ \n\t\t%s \n\t\t%s \n\t\tlua_rawset(InL, -3); \n\t}"), *InParamName, *GeneratePushPropertyStr(MapProp->KeyProp, FString("It.Key")), *GeneratePushPropertyStr(MapProp->ValueProp, FString("It.Value")));
	}

	return BodyStr;
}

FString GenerateLua::GenerateFetchPropertyStr(const FProperty* InProp, const FString& InParamName, int32 InStackIndex /*= -1*/, const UStruct* InSruct/*= nullptr*/)
{
	FString BodyStr;

	//this case is used for fetching lua's self!
	if (InProp == nullptr && InSruct != nullptr)
	{
		if (const UClass* Cls = Cast<UClass>(InSruct))
		{
			FString MetaClassName = Cls->GetName();
			FString MetaClassPrefix = Cls->GetPrefixCPP();
			BodyStr = FString::Printf(TEXT("%s%s* %s = Cast<%s%s>(FLuaObjectWrapper::FetchObject(InL, %d));"), *MetaClassPrefix, *MetaClassName, *InParamName, *MetaClassPrefix, *MetaClassName, InStackIndex);
		}
		else if (const UScriptStruct* Struct = Cast<UScriptStruct>(InSruct))
		{
			FString MetaClassName = Struct->GetName();
			BodyStr = FString::Printf(TEXT("F%s %s_Fallback; \n\tvoid* %s_UDataPointer = FLuaStructWrapper::FetchStruct(InL, %d, %d);  \n\tF%s& %s = %s_UDataPointer ? *(F%s*)(%s_UDataPointer) : %s_Fallback;"),
				*MetaClassName , *InParamName, *InParamName, InStackIndex, Struct->GetStructureSize(), *MetaClassName, *InParamName, *InParamName, *MetaClassName, *InParamName, *InParamName);
		}
	}
	else if (const FNumericProperty* NumProp = CastField<FNumericProperty>(InProp))
	{
		FString NumType = FastLuaHelper::GetPropertyTypeName(InProp);
		if (NumProp->IsEnum())
		{
			FString EnumBaseName = NumProp->GetIntPropertyEnum()->CppType;
			BodyStr = FString::Printf(TEXT("%s %s = (%s)lua_tointeger(InL, %d);"), *NumType, *InParamName, *EnumBaseName, InStackIndex);
		}
		else if (NumProp->IsInteger())
		{
			BodyStr = FString::Printf(TEXT("%s %s = (%s)lua_tointeger(InL, %d);"), *NumType, *InParamName, *NumType, InStackIndex);
			
		}
		else
		{
			BodyStr = FString::Printf(TEXT("float %s = (float)lua_tonumber(InL, %d);"), *InParamName, InStackIndex);
		}
		
	}
	else if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("bool %s = (bool)lua_toboolean(InL, %d);"), *InParamName, InStackIndex);
	}
	else if (const FNameProperty* NameProp = CastField<FNameProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FName %s = FName(UTF8_TO_TCHAR(lua_tostring(InL, %d)));\n"), *InParamName, InStackIndex);
	}
	else if (const FStrProperty* StrProp = CastField<FStrProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FString %s = UTF8_TO_TCHAR(lua_tostring(InL, %d));\n"), *InParamName, InStackIndex);
	}
	else if (const FTextProperty* TextProp = CastField<FTextProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FText %s = FText::FromString(UTF8_TO_TCHAR(lua_tostring(InL, %d)));\n"), *InParamName, InStackIndex);
	}
	else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(InProp))
	{
		FString EnumName = EnumProp->GetEnum()->CppType;
		BodyStr = FString::Printf(TEXT("%s %s = (%s)lua_tointeger(InL, %d);"), *EnumName, *InParamName, *EnumName, InStackIndex);
	}
	else if (const FStructProperty* StructProp = CastField<FStructProperty>(InProp))
	{
		FString MetaClassName = StructProp->Struct->GetName();
		BodyStr = FString::Printf(TEXT("F%s %s_Fallback; \n\tvoid* %s_Pointer = FLuaStructWrapper::FetchStruct(InL, %d, %d);  \n\tF%s& %s = %s_Pointer ? *(F%s*)(%s_Pointer) : %s_Fallback;"),
			*MetaClassName, *InParamName, *InParamName, InStackIndex, StructProp->Struct->GetStructureSize(), *MetaClassName, *InParamName, *InParamName, *MetaClassName, *InParamName, *InParamName);
	}
	else if (const FClassProperty * ClassProp = CastField<FClassProperty>(InProp))
	{
		FString ClassName = InProp->GetCPPType();
		BodyStr = FString::Printf(TEXT("%s %s = Cast<UClass>(FLuaObjectWrapper::FetchObject(InL, %d, true));"), *ClassName, *InParamName, InStackIndex);
	}
	else if (const FObjectProperty* ObjectProp = CastField<FObjectProperty>(InProp))
	{
		FString MetaClassName = ObjectProp->PropertyClass->GetName();
		FString MetaClassPrefix = ObjectProp->PropertyClass->GetPrefixCPP();
		BodyStr = FString::Printf(TEXT("%s%s* %s = Cast<%s%s>(FLuaObjectWrapper::FetchObject(InL, %d));"), *MetaClassPrefix, *MetaClassName, *InParamName, *MetaClassPrefix, *MetaClassName, InStackIndex);
	}
	else if (const FSoftClassProperty* SoftClassProp = CastField<FSoftClassProperty>(InProp))
	{
		FString MetaClassName = SoftClassProp->MetaClass->GetName();
		BodyStr = FString::Printf(TEXT("FString %s_Path = UTF8_TO_TCHAR(lua_tostring(InL, %d)); \n\tTSoftClassPtr<U%s> %s(%s_Path);"), *InParamName, InStackIndex, *MetaClassName, *InParamName, *InParamName);
	}
	else if (const FSoftObjectProperty* SoftObjectProp = CastField<FSoftObjectProperty>(InProp))
	{
		FString MetaClassName = SoftObjectProp->PropertyClass->GetName();
		BodyStr = FString::Printf(TEXT("FString %s_Path = UTF8_TO_TCHAR(lua_tostring(InL, %d)); \n\tTSoftObjectPtr<U%s> %s(%s_Path);"), *InParamName, InStackIndex, *MetaClassName, *InParamName, *InParamName);
	}
	else if (const FWeakObjectProperty* WeakObjectProp = CastField<FWeakObjectProperty>(InProp))
	{
		FString MetaClassName = WeakObjectProp->PropertyClass->GetName();
		FString MetaClassPrefix = WeakObjectProp->PropertyClass->GetPrefixCPP();
		BodyStr = FString::Printf(TEXT("FLuaObjectWrapper* %s_Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, %d); \n\t%s%s* %s = %s_Wrapper ? Cast<%s%s>(%s_Wrapper->GetObject()) : nullptr;"), *InParamName, InStackIndex, *MetaClassPrefix, *MetaClassName, *InParamName, *InParamName, *MetaClassPrefix, *MetaClassName, *InParamName);
	}
	else if (const FInterfaceProperty* InterfaceProp = CastField<FInterfaceProperty>(InProp))
	{
		FString MetaClassName = InterfaceProp->InterfaceClass->GetName();
		FString InterfaceName = InterfaceProp->GetName();
		BodyStr = FString::Printf(TEXT("FLuaInterfaceWrapper* %s_Wrapper = (FLuaInterfaceWrapper*)lua_touserdata(InL, %d); \n\tTScriptInterface<I%s>& %s = *(TScriptInterface<I%s>*)%s_Wrapper->InterfaceInst;"), *InParamName, InStackIndex, *MetaClassName, *InParamName, *MetaClassName, *InParamName);
	}
	else if (const FDelegateProperty* DelegateProp = CastField<FDelegateProperty>(InProp))
	{
		FString FuncName = DelegateProp->SignatureFunction->GetName();
		int32 SplitIndex = INDEX_NONE;
		FuncName.FindChar('_', SplitIndex);
		FString DelegateName = FuncName.Left(SplitIndex);

		UObject* Outer = DelegateProp->SignatureFunction->GetOuter();
		UClass* OuterClass = Cast<UClass>(Outer);
		FString ScopePrefix = OuterClass ? (OuterClass->GetPrefixCPP() + Outer->GetName() + FString("::")) : FString("");

		BodyStr = FString::Printf(TEXT("%sF%s %s_Fallback;  \n\tvoid* %s_Pointer = FLuaDelegateWrapper::FetchDelegate(InL, %d, false); \n\t%sF%s& %s = %s_Pointer ? *(%sF%s*)%s_Pointer : %s_Fallback;"),
			*ScopePrefix, *DelegateName, *InParamName, *InParamName, InStackIndex, *ScopePrefix, *DelegateName, *InParamName, *InParamName, *ScopePrefix, *DelegateName, *InParamName, *InParamName);
	}
	else if (const FMulticastDelegateProperty* MultiDelegateProp = CastField<FMulticastDelegateProperty>(InProp))
	{
		FString FuncName = MultiDelegateProp->SignatureFunction->GetName();
		int32 SplitIndex = INDEX_NONE;
		FuncName.FindChar('_', SplitIndex);
		FString DelegateName = FuncName.Left(SplitIndex);

		UObject* Outer = MultiDelegateProp->SignatureFunction->GetOuter();
		UClass* OuterClass = Cast<UClass>(Outer);
		FString ScopePrefix = OuterClass ? (OuterClass->GetPrefixCPP() + Outer->GetName() + FString("::")) : FString("");

		BodyStr = FString::Printf(TEXT("%sF%s %s_Fallback;  \n\tvoid* %s_Pointer = FLuaDelegateWrapper::FetchDelegate(InL, %d, true); \n\t%sF%s& %s = %s_Pointer ? *(%sF%s*)%s_Pointer : %s_Fallback;"),
			*ScopePrefix, *DelegateName, *InParamName, *InParamName, InStackIndex, *ScopePrefix, *DelegateName, *InParamName, *InParamName, *ScopePrefix, *DelegateName, *InParamName, *InParamName);
	}
	else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(InProp))
	{
		FString ElementTypeName = FastLuaHelper::GetPropertyTypeName(ArrayProp->Inner);

		FString NewElement = FString::Printf(TEXT("%s"), *GenerateFetchPropertyStr(ArrayProp->Inner, FString::Printf(TEXT("Temp_NewElement")), -1));

		BodyStr = FString::Printf(TEXT("TArray<%s> %s;\n\t{\n\tif(lua_istable(InL, %d))\n\t{\n\tlua_pushnil(InL);\n\twhile(lua_next(InL, -2))\n\t{\n\t%s \n\t%s.Add(Temp_NewElement);\n\tlua_pop(InL, 1); \n\t}\n\t}  \n\t}"), *ElementTypeName, *InParamName, InStackIndex, *NewElement, *InParamName);
	}
	else if (const FSetProperty * SetProp = CastField<FSetProperty>(InProp))
	{
		FString ElementTypeName = FastLuaHelper::GetPropertyTypeName(SetProp->ElementProp);

		FString PointerFlag;
		if (ElementTypeName.EndsWith(FString("*")))
		{
			PointerFlag = FString("*");
		}

		FString NewElement = FString::Printf(TEXT("%s"), *GenerateFetchPropertyStr(SetProp->ElementProp, FString::Printf(TEXT("Temp_NewElement")), -1));

		BodyStr = FString::Printf(TEXT("TSet<%s%s> %s;\n\t{\n\tif(lua_istable(InL, %d))\n\t{\n\tlua_pushnil(InL);\n\twhile(lua_next(InL, -2))\n\t{\n\t%s \n\t%s.Add(Temp_NewElement);\n\tlua_pop(InL, 1); \n\t}\n\t}  \n\t}"), *ElementTypeName, *PointerFlag, *InParamName, InStackIndex, *NewElement, *InParamName);
	}
	else if (const FMapProperty* MapProp = CastField<FMapProperty>(InProp))
	{
		FString KeyTypeName = FastLuaHelper::GetPropertyTypeName(MapProp->KeyProp);

		FString KeyPointerFlag;
		if (KeyTypeName.EndsWith(FString("*")))
		{
			KeyPointerFlag = FString("*");
		}

		FString ValueTypeName = FastLuaHelper::GetPropertyTypeName(MapProp->ValueProp);

		FString ValuePointerFlag;
		if (ValueTypeName.EndsWith(FString("*")))
		{
			ValuePointerFlag = FString("*");
		}

		FString NewKeyElement = FString::Printf(TEXT("%s"), *GenerateFetchPropertyStr(MapProp->KeyProp, FString::Printf(TEXT("Temp_NewKeyElement")), -2));

		FString NewValueElement = FString::Printf(TEXT("%s"), *GenerateFetchPropertyStr(MapProp->ValueProp, FString::Printf(TEXT("Temp_NewValueElement")), -1));

		BodyStr = FString::Printf(TEXT("TMap<%s%s, %s%s> %s;\n\t{\n\tif(lua_istable(InL, %d))\n\t{\n\tlua_pushnil(InL);\n\twhile(lua_next(InL, -2))\n\t{\n\t%s\n\t%s \n\t%s.Add(Temp_NewKeyElement, Temp_NewValueElement);\n\tlua_pop(InL, 1); \n\t}\n\t}  \n\t}"), *KeyTypeName, *KeyPointerFlag, *ValueTypeName, *ValuePointerFlag, *InParamName, InStackIndex, *NewKeyElement, *NewValueElement, *InParamName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("breakpoint!"));
	}

	return BodyStr;

}

static FString FixupHeaderPath(const FString& InPath)
{
	FString RawHeaderPath = InPath;
	if (RawHeaderPath.Len() > 7)
	{
		if (RawHeaderPath.StartsWith(FString("Public/")))
		{
			RawHeaderPath = RawHeaderPath.Mid(7);
		}
		if (RawHeaderPath.StartsWith(FString("Classes/")))
		{
			RawHeaderPath = RawHeaderPath.Mid(8);
		}
	}

	return RawHeaderPath;
}

static FString GetHeaderFileForProperty(const FProperty* InProperty)
{
	const FProperty* Prop = InProperty;
	FString RawHeaderPath;
	if (const FClassProperty* ClassProp = CastField<FClassProperty>(Prop))
	{
		RawHeaderPath = ClassProp->MetaClass->GetMetaData(*FString("IncludePath"));
	}
	else if (const FObjectProperty* ObjectProp = CastField<FObjectProperty>(Prop))
	{
		RawHeaderPath = ObjectProp->PropertyClass->GetMetaData(*FString("IncludePath"));
	}
	else if (const FWeakObjectProperty* WeakObjectProp = CastField<FWeakObjectProperty>(Prop))
	{
		RawHeaderPath = WeakObjectProp->PropertyClass->GetMetaData(*FString("IncludePath"));
	}
	else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
	{
		if (const FClassProperty* ClassEle = CastField<FClassProperty>(ArrayProp->Inner))
		{
			RawHeaderPath = ClassEle->MetaClass->GetMetaData(*FString("IncludePath"));
		}
		else if (const FObjectProperty* ObjectEle = CastField<FObjectProperty>(ArrayProp->Inner))
		{
			RawHeaderPath = ObjectEle->PropertyClass->GetMetaData(*FString("IncludePath"));
		}
		else if (const FStructProperty* StructEle = CastField<FStructProperty>(ArrayProp->Inner))
		{
			RawHeaderPath = StructEle->GetMetaData(*FString("IncludePath"));
		}
	}
	else if (const FSetProperty* SetProp = CastField<FSetProperty>(Prop))
	{
		if (const FClassProperty* ClassEle = CastField<FClassProperty>(SetProp->ElementProp))
		{
			RawHeaderPath = ClassEle->MetaClass->GetMetaData(*FString("IncludePath"));
		}
		else if (const FObjectProperty* ObjectEle = CastField<FObjectProperty>(SetProp->ElementProp))
		{
			RawHeaderPath = ObjectEle->PropertyClass->GetMetaData(*FString("IncludePath"));
		}
	}

	RawHeaderPath = FixupHeaderPath(RawHeaderPath);
	return RawHeaderPath;
}

TArray<FString> GenerateLua::CollectHeaderFilesReferencedByClass(const class UStruct* InClass) const
{
	TArray<FString> HeaderList;

	HeaderList.Add(FString("LuaObjectWrapper.h"));
	HeaderList.Add(FString("LuaStructWrapper.h"));
	HeaderList.Add(FString("LuaDelegateWrapper.h"));
	HeaderList.Add(FString("LuaInterfaceWrapper.h"));

	for (TFieldIterator<const UFunction> ItFunc(InClass, EFieldIteratorFlags::ExcludeSuper); ItFunc; ++ItFunc)
	{
		if (ShouldGenerateFunction(*ItFunc) == false)
		{
			continue;
		}
		
		for (TFieldIterator<const FProperty> ItProp(*ItFunc); ItProp; ++ItProp)
		{
			FString HeaderPath = GetHeaderFileForProperty(*ItProp);
			if (HeaderPath.Len())
			{
				HeaderList.AddUnique(HeaderPath);
			}
		}
	}

	return HeaderList;
}


