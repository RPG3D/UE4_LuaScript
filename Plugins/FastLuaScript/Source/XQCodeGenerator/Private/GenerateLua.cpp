

#include "GenerateLua.h"
#include "UObject/MetaData.h"
#include "FileHelper.h"
#include "FastLuaHelper.h"



FString GenerateLua::GenerateFunctionBodyStr(const class UFunction* InFunction, const class UClass* InClass)
{
	if (InClass == nullptr || FastLuaHelper::IsScriptCallableFunction(InFunction) == false)
	{
		return FString("");
	}

	const FString ClassName = InClass->GetName();
	const FString ClassPrefix = InClass->GetPrefixCPP();
	
	const FString FuncName = InFunction->GetName();
	
	//first line: FLuaObjectWrapper* ThisObject_Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	FString BodyStr = FString("\t") + GenerateFetchPropertyStr(nullptr, FString("ThisObject"), 1, InClass) + FString("\n");
	BodyStr += FString("\tif(ThisObject == nullptr) { return 0; } \n");

	int32 LuaStackIndex = 2;

	TArray<FString> InputParamNames;

	//fill params
	UProperty* ReturnProp = nullptr;

	for (TFieldIterator<UProperty> It(InFunction); It; ++It)
	{
		UProperty* Prop = *It;
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
		FString PointerFlag;
		if (FastLuaHelper::IsPointerType(ReturnTypeName))
		{
			PointerFlag = FString("*");
		}
		ReturnPropStr = FString::Printf(TEXT("%s%s ReturnValue = "), *ReturnTypeName, *PointerFlag);
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
	for (TFieldIterator<UProperty> It(InFunction); It; ++It)
	{
		UProperty* Prop = *It;
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

FString GenerateLua::GenerateGetPropertyStr(const class UProperty* InProperty, const FString& InParamName, const class UStruct* InStruct)
{
	const FString ClassName = InStruct->GetName();
	const FString ClassPrefix = InStruct->GetPrefixCPP();

	FString BodyStr = GenerateFetchPropertyStr(nullptr, FString("ThisObject"), 1, InStruct) + FString("\n");

	if (FastLuaHelper::IsPointerType(ClassPrefix))
	{
		BodyStr += FString("\tif(ThisObject == nullptr) { return 0; } \n");
	}

	if (FastLuaHelper::IsPointerType(ClassPrefix))
	{
		BodyStr += FString("\t") + GeneratePushPropertyStr(InProperty, FString("ThisObject->") + InParamName);
	}
	else
	{
		BodyStr += FString("\t") + GeneratePushPropertyStr(InProperty, FString("ThisObject.") + InParamName);
	}

	BodyStr += FString("\n\treturn 1;");
	return BodyStr;
}

FString GenerateLua::GenerateSetPropertyStr(const class UProperty* InProperty, const FString& InParamName, const class UStruct* InStruct)
{
	const FString ClassName = InStruct->GetName();
	const FString ClassPrefix = InStruct->GetPrefixCPP();

	FString BodyStr = GenerateFetchPropertyStr(nullptr, FString("ThisObject"), 1, InStruct) + FString("\n");

	if (FastLuaHelper::IsPointerType(ClassPrefix))
	{
		BodyStr += FString("\tif(ThisObject == nullptr) { return 0; } \n");
	}

	BodyStr += FString("\t") + GenerateFetchPropertyStr(InProperty, FString("InParam_") + InParamName, 2) + FString("\n\t");

	if (FastLuaHelper::IsPointerType(ClassPrefix))
	{
		if (const UDelegateProperty* DelegateProp = Cast<UDelegateProperty>(InProperty))
		{
			BodyStr += FString::Printf(TEXT("\n\t ThisObject->%s.BindUFunction(%s.GetUObject(), %s.GetFunctionName());"), *InParamName, *FString::Printf(TEXT("InParam_%s"), *InParamName), *FString::Printf(TEXT("InParam_%s"), *InParamName));
		}
		//else if (const UMulticastDelegateProperty* MultiDelegateProp = Cast<UMulticastDelegateProperty>(InProperty))
		//{
			//BodyStr += FString::Printf(TEXT("\n\t ThisObject->%s.BindUFunction(%s.GetObject(), %s.GetFunctionName());"), *InParamName, *FString::Printf(TEXT("InParam_%s"), *InParamName), *FString::Printf(TEXT("InParam_%s"), *InParamName));
		//}
		else
		{
			BodyStr += FString::Printf(TEXT("\n\tThisObject->%s = InParam_%s;"), *InParamName, *InParamName);
		}

	}
	else
	{
		BodyStr += FString::Printf(TEXT("\n\tThisObject.%s = InParam_%s;"), *InParamName, *InParamName);
	}

	BodyStr += FString("\n\treturn 0;");
	return BodyStr;
}


int32 GenerateLua::InitConfig()
{
	if (FFileHelper::LoadFileToStringArray(ModulesShouldExport, *(FPaths::ProjectPluginsDir() / FString("FastLuaScript/Config/ModuleToExport.txt"))))
	{
		UE_LOG(LogTemp, Warning, TEXT("Read ModuleToExport.txt OK!"));
	}

	if (FFileHelper::LoadFileToStringArray(IgnoredClass, *(FPaths::ProjectPluginsDir() / FString("FastLuaScript/Config/IgnoredClass.txt"))))
	{
		UE_LOG(LogTemp, Warning, TEXT("Read IgnoredClass.txt OK!"));
	}

	if (FFileHelper::LoadFileToStringArray(IgnoredFunctions, *(FPaths::ProjectPluginsDir() / FString("FastLuaScript/Config/IgnoredFunction.txt"))))
	{
		UE_LOG(LogTemp, Warning, TEXT("Read IgnoredFunction.txt OK!"));
	}

	return 0;
}

int32 GenerateLua::GeneratedCode() const
{
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


	TMap<FString, const UScriptStruct*> AllGeneratedStruct;
	for (TObjectIterator<UScriptStruct>It; It; ++It)
	{
		const UScriptStruct* Struct = *It;
		if (GenerateCodeForStruct(Struct) == 0)
		{
			AllGeneratedStruct.Add(Struct->GetName(), Struct);
		}
	}

	//write API header file 
	{
		FString APIHeaderPath = CodeDirectory / FString("FastLuaAPI.h");
		FString HeaderStr = FString("\n// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved. \n\n#pragma once \n\n#include \"CoreMinimal.h\" \n\n");
		HeaderStr += FString::Printf(TEXT("struct FASTLUASCRIPT_API FastLuaAPI\n{\n"));
		HeaderStr += FString("\tstatic int32 RegisterUnrealClass(struct lua_State* InL);\n\n");
		HeaderStr += FString("\tstatic int32 RegisterUnrealStruct(struct lua_State* InL);\n\n");
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
			SourceStr += FString::Printf(TEXT("#include \"Generated/Class/Lua_%s.h\"\n"), *It.Key);
		}


		SourceStr += FString("\n\n");

		//include struct header file
		for (auto It : AllGeneratedStruct)
		{
			SourceStr += FString::Printf(TEXT("#include \"Generated/Struct/Lua_%s.h\"\n"), *It.Key);
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


		//struct
		SourceStr += FString("int32 FastLuaAPI::RegisterUnrealStruct(struct lua_State* InL)\n{\n");
		SourceStr += FString::Printf(TEXT("\tTArray<const UScriptStruct*> AllRegistedStruct; \n\tUScriptStruct* TempStruct = nullptr;\n\n\n"));
		for (auto It : AllGeneratedStruct)
		{
			//luaL_newlib(InL, Lua_Class::FuncsBindToLua);
			//lua_rawsetp(InL, LUA_REGISTRYINDEX, (void*)FindObject<UScriptStruct>(ANY_PACKAGE, *It.Key));
			SourceStr += FString::Printf(TEXT("\tluaL_newlib(InL, Lua_%s::FuncsBindToLua);\n"), *It.Key);
			SourceStr += FString::Printf(TEXT("\tTempStruct = FindObject<UScriptStruct>(ANY_PACKAGE, *FString(\"%s\"));\n"), *It.Key);
			SourceStr += FString::Printf(TEXT("\tAllRegistedStruct.Add(TempStruct);\n"));
			SourceStr += FString::Printf(TEXT("\tlua_rawsetp(InL, LUA_REGISTRYINDEX, (const void*)TempStruct);\n\n"));

		}
		SourceStr += FString("\tFastLuaHelper::FixStructMetatable(InL, AllRegistedStruct);\n");
		SourceStr += FString("\n\treturn 1;\n}\n\n");


		FFileHelper::SaveStringToFile(SourceStr, *APISourcePath);
	}

	return 0;
}

int32 GenerateLua::GenerateCodeForClass(const class UClass* InClass) const
{
	if (InClass == nullptr)
	{
		return -1;
	}

	UPackage* Pkg = InClass->GetOutermost();
	FString PkgName = Pkg->GetName();

	if (InClass->HasAnyClassFlags(EClassFlags::CLASS_RequiredAPI) == false)
	{
		return -1;
	}

	/*if (InClass->GetName() == FString("BlueprintMapLibrary"))
	{
		UE_LOG(LogTemp, Log, TEXT("breakpoint!"));
	}*/

	bool bShouldExportModule = false;
	for (int32 i = 0; i < ModulesShouldExport.Num(); ++i)
	{
		if (PkgName == ModulesShouldExport[i])
		{
			bShouldExportModule = true;
			break;
		}
	}

	if (bShouldExportModule == false)
	{
		return -1;
	}

	FString ClassName = InClass->GetName();

	FString RawHeaderPath = InClass->GetMetaData(*FString("IncludePath"));

	//UE_LOG(LogTemp, Log, TEXT("Exporting: %s    %s"), *ClassName, *RawHeaderPath);

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
			/*if (FuncName == FString("StackTrace"))
			{
				UE_LOG(LogTemp, Log, TEXT("breakpoint!"));
			}*/
			if (FastLuaHelper::IsScriptCallableFunction(*It) == false)
			{
				continue;
			}
			FString ClassFunctionName = ClassName + FString("::") + FuncName;
			if (IgnoredFunctions.Find(ClassFunctionName) > INDEX_NONE)
			{
				continue;
			}

			++ExportedFieldCount;
			FunctionList.Add(FuncName, FString("Lua_") + FuncName);

			FString FuncDeclareStr = FString::Printf(TEXT("\tstatic int32 Lua_%s(lua_State* InL);\n\n"), *FuncName);
			HeaderStr += FuncDeclareStr;
		}

		for (TFieldIterator<const UProperty> It(InClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			FString PropName = It->GetName();
			/*if (PropName == FString("AnimBlueprintGeneratedClass"))
			{
				UE_LOG(LogTemp, Log, TEXT("breakpoint!"));
			}*/

			if (FastLuaHelper::IsScriptReadableProperty(*It) == false)
			{
				continue;
			}

			if (InClass->FindFunctionByName(*FString(FString("Get") + PropName)) == nullptr)
			{
				FunctionList.Add(FString("Get") + PropName, FString("Lua_Get") + PropName);
				FString PropGetDeclareStr = FString::Printf(TEXT("\tstatic int32 Lua_Get%s(lua_State* InL);\n"), *PropName);
				HeaderStr += PropGetDeclareStr;
				++ExportedFieldCount;
			}

			if (InClass->FindFunctionByName(*FString(FString("Set") + PropName)) == nullptr)
			{
				FunctionList.Add(FString("Set") + PropName, FString("Lua_Set") + PropName);
				FString PropSetDeclareStr = FString::Printf(TEXT("\tstatic int32 Lua_Set%s(lua_State* InL);\n\n"), *PropName);
				HeaderStr += PropSetDeclareStr;
				++ExportedFieldCount;
			}

		}

		if (ExportedFieldCount < 1)
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
			if (FastLuaHelper::IsScriptCallableFunction(*It) == false)
			{
				continue;
			}
			FString ClassFunctionName = ClassName + FString("::") + FuncName;
			if (IgnoredFunctions.Find(ClassFunctionName) > INDEX_NONE)
			{
				continue;
			}

			FString FuncBeginStr = FString::Printf(TEXT("int32 Lua_%s::Lua_%s(lua_State* InL)\n{\n"), *ClassName, *FuncName);
			SourceStr += FuncBeginStr;

			FString FuncBodyStr = GenerateFunctionBodyStr(*It, InClass);

			SourceStr += FuncBodyStr;

			FString FuncEndStr = FString("\n\n}\n\n");
			SourceStr += FuncEndStr;
		}

		for (TFieldIterator<const UProperty> It(InClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			if (FastLuaHelper::IsScriptReadableProperty(*It) == false)
			{
				continue;
			}

			FString PropName = It->GetName();

			if (InClass->FindFunctionByName(*FString(FString("Get") + PropName)) == nullptr)
			{
				FString PropGetDefineBeginStr = FString::Printf(TEXT("int32 Lua_%s::Lua_Get%s(lua_State* InL)\n{\n"), *ClassName, *PropName);
				SourceStr += PropGetDefineBeginStr;

				FString GetBodyStr = FString("\t") + GenerateGetPropertyStr(*It, PropName, InClass);
				SourceStr += GetBodyStr;

				FString PropGetEndStr = FString("\n\n}\n\n");
				SourceStr += PropGetEndStr;
			}

			if (InClass->FindFunctionByName(*FString(FString("Set") + PropName)) == nullptr)
			{
				FString PropSetDefineBeginStr = FString::Printf(TEXT("int32 Lua_%s::Lua_Set%s(lua_State* InL)\n{\n"), *ClassName, *PropName);
				SourceStr += PropSetDefineBeginStr;

				FString SetBodyStr = FString("\t") + GenerateSetPropertyStr(*It, PropName, InClass);
				SourceStr += SetBodyStr;

				FString PropSetEndStr = FString("\n\n}\n\n");
				SourceStr += PropSetEndStr;
			}

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


int32 GenerateLua::GenerateCodeForStruct(const class UScriptStruct* InStruct) const
{
	if (InStruct == nullptr)
	{
		return -1;
	}

	UPackage* Pkg = InStruct->GetOutermost();
	FString PkgName = Pkg->GetName();
	FString StructName = InStruct->GetName();

	bool bShouldExportModule = false;
	for (int32 i = 0; i < ModulesShouldExport.Num(); ++i)
	{
		if (PkgName == ModulesShouldExport[i])
		{
			bShouldExportModule = true;
			break;
		}
	}

	bShouldExportModule = bShouldExportModule && (IgnoredClass.Find(StructName) < 0);

	if (bShouldExportModule == false)
	{
		return -1;
	}


	FString RawHeaderPath = InStruct->GetMetaData(*FString("ModuleRelativePath"));

	TMap<FString, FString> FunctionList;

	//first, generate header
	{
		int32 ExportedFieldCount = 0;

		FString HeaderFilePath = FString::Printf(TEXT("%s/Struct/Lua_%s.h"), *CodeDirectory, *StructName);

		FString HeaderStr = FString("\n// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved. \n\n#pragma once \n\n#include \"CoreMinimal.h\" \n\n#include \"lua/lua.hpp\" \n\n");

		FString ClassBeginStr = FString::Printf(TEXT("\nstruct Lua_%s\n{\n"), *StructName);
		HeaderStr += ClassBeginStr;

		for (TFieldIterator<const UProperty> It(InStruct, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			FString PropName = It->GetName();

			if (FastLuaHelper::IsScriptReadableProperty(*It) == false)
			{
				continue;
			}
			++ExportedFieldCount;

			FunctionList.Add(FString("Get") + PropName, FString("Lua_Get") + PropName);
			FunctionList.Add(FString("Set") + PropName, FString("Lua_Set") + PropName);


			FString PropGetDeclareStr = FString::Printf(TEXT("\tstatic int32 Lua_Get%s(lua_State* InL);\n"), *PropName);
			HeaderStr += PropGetDeclareStr;

			FString PropSetDeclareStr = FString::Printf(TEXT("\tstatic int32 Lua_Set%s(lua_State* InL);\n\n"), *PropName);
			HeaderStr += PropSetDeclareStr;
		}

		if (ExportedFieldCount < 1)
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
		FString SourceFilePath = FString::Printf(TEXT("%s/Struct/Lua_%s.cpp"), *CodeDirectory, *StructName);

		FString SourceStr = FString("\n// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved. \n\n");
		FString ClassBeginStr = FString::Printf(TEXT("#include \"Lua_%s.h\" \n\n#include \"FastLuaHelper.h\" \n\n"), *StructName);

		if (RawHeaderPath.IsEmpty() == false)
		{
			ClassBeginStr += FString::Printf(TEXT("#include \"%s\"\n\n"), *RawHeaderPath);
			TArray<FString> HeaderList = CollectHeaderFilesReferencedByClass(InStruct);
			for (int32 i = 0; i < HeaderList.Num(); ++i)
			{
				ClassBeginStr += FString::Printf(TEXT("#include \"%s\"\n\n"), *HeaderList[i]);
			}
		}

		SourceStr += ClassBeginStr;

		for (TFieldIterator<const UProperty> It(InStruct, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			if (FastLuaHelper::IsScriptReadableProperty(*It) == false)
			{
				continue;
			}

			FString PropName = It->GetName();

			FString PropGetDefineBeginStr = FString::Printf(TEXT("int32 Lua_%s::Lua_Get%s(lua_State* InL)\n{\n"), *StructName, *PropName);
			SourceStr += PropGetDefineBeginStr;

			FString GetBodyStr = FString("\t") + GenerateGetPropertyStr(*It, PropName, InStruct);
			SourceStr += GetBodyStr;

			FString PropGetEndStr = FString("\n\n}\n\n");
			SourceStr += PropGetEndStr;



			FString PropSetDefineBeginStr = FString::Printf(TEXT("int32 Lua_%s::Lua_Set%s(lua_State* InL)\n{\n"), *StructName, *PropName);
			SourceStr += PropSetDefineBeginStr;

			FString SetBodyStr = FString("\t") + GenerateSetPropertyStr(*It, PropName, InStruct);
			SourceStr += SetBodyStr;

			FString PropSetEndStr = FString("\n\n}\n\n");
			SourceStr += PropSetEndStr;
		}


		FString WrapperStructName = FString::Printf(TEXT("Lua_%s"), *StructName);
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

FString GenerateLua::GeneratePushPropertyStr(const UProperty* InProp, const FString& InParamName)
{
	FString BodyStr = FString::Printf(TEXT("lua_pushnil(InL);"));

	if (InProp == nullptr || InProp->GetClass() == nullptr)
	{
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
	else if (const UStructProperty* StructProp = Cast<UStructProperty>(InProp))
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

		BodyStr = FString::Printf(TEXT("static UScriptStruct* %s_MetatableIndex = FindObject<UScriptStruct>(ANY_PACKAGE, *FString(\"%s\")); \n\tFastLuaHelper::PushStruct(InL, %s_MetatableIndex, &%s);"), *MetatableStr, *StructName, *MetatableStr, *InParamName);
	}
	else if (const UClassProperty* ClassProp = Cast<UClassProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FastLuaHelper::PushObject(InL, (UObject*)%s);"), *InParamName);
	}
	else if (const UObjectProperty* ObjectProp = Cast<UObjectProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FastLuaHelper::PushObject(InL, (UObject*)%s);"), *InParamName);
	}
	else if (const UWeakObjectProperty* WeakObjectProp = Cast<UWeakObjectProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("FastLuaHelper::PushObject(InL, (UObject*)%s.Get());"), *InParamName);
	}
	else if (const UDelegateProperty* DelegateProp = Cast<UDelegateProperty>(InProp))
	{
		TArray<FString> ParamNames;
		InParamName.ParseIntoArray(ParamNames, *FString("->"));
		if (ParamNames.Num() == 2)
		{
			BodyStr = FString::Printf(TEXT("FastLuaHelper::PushDelegate(InL, %s->GetClass()->FindPropertyByName(\"%s\"), %s, false);"), *ParamNames[0], *ParamNames[1], *ParamNames[0]);
		}
	}
	else if (const UMulticastDelegateProperty* MultiDelegateProp = Cast<UMulticastDelegateProperty>(InProp))
	{
		TArray<FString> ParamNames;
		InParamName.ParseIntoArray(ParamNames, *FString("->"));
		if (ParamNames.Num() == 2)
		{
			BodyStr = FString::Printf(TEXT("FastLuaHelper::PushDelegate(InL, %s->GetClass()->FindPropertyByName(\"%s\"), %s, true);"), *ParamNames[0], *ParamNames[1], *ParamNames[0]);
		}
	}
	else if (const UArrayProperty* ArrayProp = Cast<UArrayProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_newtable(InL); \n\tfor(int32 i = 0; i < %s.Num(); ++i) \n\t{ \n\t\t%s \n\t\tlua_rawseti(InL, -2, i + 1); \n\t}"), *InParamName, *GeneratePushPropertyStr(ArrayProp->Inner, FString::Printf(TEXT("%s[i]"), *InParamName)));
	}
	else if (const USetProperty* SetProp = Cast<USetProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_newtable(InL); \t\tfor(int32 i = 0; i < %s.Num(); ++i) \n\t{ \n\t\t%s \n\t\tlua_rawseti(InL, -2, i + 1); \n\t}"), *InParamName, *GeneratePushPropertyStr(SetProp->ElementProp, FString::Printf(TEXT("%s[FSetElementId::FromInteger(i)]"), *InParamName)));
	}
	else if (const UMapProperty* MapProp = Cast<UMapProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("lua_newtable(InL);  \n\tfor(auto& It : %s) \n\t{ \n\t\t%s \n\t\t%s \n\t\tlua_rawset(InL, -3); \n\t}"), *InParamName, *GeneratePushPropertyStr(MapProp->KeyProp, FString("It.Key")), *GeneratePushPropertyStr(MapProp->ValueProp, FString("It.Value")));
	}

	return BodyStr;
}

FString GenerateLua::GenerateFetchPropertyStr(const UProperty* InProp, const FString& InParamName, int32 InStackIndex /*= -1*/, const UStruct* InSruct/*= nullptr*/)
{
	FString BodyStr;

	//this case is used for fetching lua's self!
	if (InProp == nullptr && InSruct != nullptr)
	{
		if (const UClass* Cls = Cast<UClass>(InSruct))
		{
			FString MetaClassName = Cls->GetName();
			FString MetaClassPrefix = Cls->GetPrefixCPP();
			BodyStr = FString::Printf(TEXT("%s%s* %s = Cast<%s%s>(FastLuaHelper::FetchObject(InL, %d));"), *MetaClassPrefix, *MetaClassName, *InParamName, *MetaClassPrefix, *MetaClassName, InStackIndex);
		}
		else if (const UScriptStruct* Struct = Cast<UScriptStruct>(InSruct))
		{
			FString MetaClassName = Struct->GetName();
			FString MetaClassPrefix = Struct->GetPrefixCPP();
			BodyStr = FString::Printf(TEXT("FLuaStructWrapper* %s_Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, %d); \n\tF%s %s_Fallback;  \n\tF%s& %s = %s_Wrapper ? *(F%s*)(&(%s_Wrapper->StructInst)) : %s_Fallback;"), *InParamName, InStackIndex, *MetaClassName, *InParamName, *MetaClassName, *InParamName, *InParamName, *MetaClassName, *InParamName, *InParamName);
		}
	}
	else if (const UIntProperty* IntProp = Cast<UIntProperty>(InProp))
	{
		BodyStr = FString::Printf(TEXT("int32 %s = lua_tointeger(InL, %d);"), *InParamName, InStackIndex);
	}
	else if (const UInt64Property* Int64Prop = Cast<UInt64Property>(InProp))
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
		BodyStr = FString::Printf(TEXT("FLuaStructWrapper* %s_Wrapper = (FLuaStructWrapper*)lua_touserdata(InL, %d); \n\tF%s %s_Fallback;  \n\tF%s& %s = %s_Wrapper ? *(F%s*)(&(%s_Wrapper->StructInst)) : %s_Fallback;"), *InParamName, InStackIndex, *MetaClassName, *InParamName, *MetaClassName, *InParamName, *InParamName, *MetaClassName, *InParamName, *InParamName);
	}
	else if (const UClassProperty * ClassProp = Cast<UClassProperty>(InProp))
	{
		FString MetaClassName = ClassProp->MetaClass->GetName();
		FString MetaClassPrefix = ClassProp->MetaClass->GetPrefixCPP();
		BodyStr = FString::Printf(TEXT("FLuaObjectWrapper* %s_Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, %d); \n\tTSubclassOf<class %s%s> %s = %s_Wrapper ? (UClass*)(%s_Wrapper->ObjInst.Get()) : nullptr;"), *InParamName, InStackIndex, *MetaClassPrefix, *MetaClassName, *InParamName, *InParamName, *InParamName);
	}
	else if (const UObjectProperty* ObjectProp = Cast<UObjectProperty>(InProp))
	{
		FString MetaClassName = ObjectProp->PropertyClass->GetName();
		FString MetaClassPrefix = ObjectProp->PropertyClass->GetPrefixCPP();
		BodyStr = FString::Printf(TEXT("FLuaObjectWrapper* %s_Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, %d); \n\t%s%s* %s = %s_Wrapper ? (%s%s*)(%s_Wrapper->ObjInst.Get()) : nullptr;"), *InParamName, InStackIndex, *MetaClassPrefix, *MetaClassName, *InParamName, *InParamName, *MetaClassPrefix, *MetaClassName, *InParamName);
	}
	else if (const USoftClassProperty* SoftClassProp = Cast<USoftClassProperty>(InProp))
	{
		FString MetaClassName = SoftClassProp->MetaClass->GetName();
		BodyStr = FString::Printf(TEXT("FLuaSoftClassWrapper* %s_Wrapper = (FLuaSoftClassWrapper*)lua_touserdata(InL, %d); \n\tTSoftClassPtr<U%s> %s = %s_Wrapper ? (TSoftClassPtr<U%s>)%s_Wrapper->SoftClassInst : nullptr;"), *InParamName, InStackIndex, *MetaClassName, *InParamName, *InParamName, *MetaClassName, *InParamName);
	}
	else if (const USoftObjectProperty* SoftObjectProp = Cast<USoftObjectProperty>(InProp))
	{
		FString MetaClassName = SoftObjectProp->PropertyClass->GetName();
		BodyStr = FString::Printf(TEXT("FLuaSoftObjectWrapper* %s_Wrapper = (FLuaSoftObjectWrapper*)lua_touserdata(InL, %d); \n\tTSoftObjectPtr<U%s> %s = *(TSoftObjectPtr<U%s>*)&%s_Wrapper->SoftObjInst;"), *InParamName, InStackIndex, *MetaClassName, *InParamName, *MetaClassName, *InParamName);
	}
	else if (const UWeakObjectProperty* WeakObjectProp = Cast<UWeakObjectProperty>(InProp))
	{
		FString MetaClassName = WeakObjectProp->PropertyClass->GetName();
		FString MetaClassPrefix = WeakObjectProp->PropertyClass->GetPrefixCPP();
		BodyStr = FString::Printf(TEXT("FLuaObjectWrapper* %s_Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, %d); \n\t%s%s* %s = %s_Wrapper ? (%s%s*)(%s_Wrapper->ObjInst.Get()) : nullptr;"), *InParamName, InStackIndex, *MetaClassPrefix, *MetaClassName, *InParamName, *InParamName, *MetaClassPrefix, *MetaClassName, *InParamName);
	}
	else if (const UInterfaceProperty* InterfaceProp = Cast<UInterfaceProperty>(InProp))
	{
		FString MetaClassName = InterfaceProp->InterfaceClass->GetName();
		FString InterfaceName = InterfaceProp->GetName();
		BodyStr = FString::Printf(TEXT("FLuaInterfaceWrapper* %s_Wrapper = (FLuaInterfaceWrapper*)lua_touserdata(InL, %d); \n\tTScriptInterface<I%s>& %s = *(TScriptInterface<I%s>*)%s_Wrapper->InterfaceInst;"), *InParamName, InStackIndex, *MetaClassName, *InParamName, *MetaClassName, *InParamName);
	}
	else if (const UDelegateProperty* DelegateProp = Cast<UDelegateProperty>(InProp))
	{
		FString FuncName = DelegateProp->SignatureFunction->GetName();
		int32 SplitIndex = INDEX_NONE;
		FuncName.FindChar('_', SplitIndex);
		FString DelegateName = FuncName.Left(SplitIndex);

		FString ScopePrefix = DelegateProp->SignatureFunction->GetOuter()->GetClass()->GetPrefixCPP() + DelegateProp->SignatureFunction->GetOuter()->GetName() + FString("::");
		if (ScopePrefix.Contains(FString("/")))
		{
			ScopePrefix = FString("");
		}

		BodyStr = FString::Printf(TEXT("FLuaDelegateWrapper* %s_Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, %d); \n\t%sF%s& %s = *(%sF%s*)%s_Wrapper->DelegateInst;"), *InParamName, InStackIndex, *ScopePrefix, *DelegateName, *InParamName, *ScopePrefix, *DelegateName, *InParamName);
	}
	else if (const UMulticastDelegateProperty* MultiDelegateProp = Cast<UMulticastDelegateProperty>(InProp))
	{
		FString FuncName = MultiDelegateProp->SignatureFunction->GetName();
		int32 SplitIndex = INDEX_NONE;
		FuncName.FindChar('_', SplitIndex);
		FString DelegateName = FuncName.Left(SplitIndex);

		FString ScopePrefix = MultiDelegateProp->SignatureFunction->GetOuter()->GetClass()->GetPrefixCPP() + MultiDelegateProp->SignatureFunction->GetOuter()->GetName() + FString("::");
		if (ScopePrefix.Contains(FString("/")))
		{
			ScopePrefix = FString("");
		}

		BodyStr = FString::Printf(TEXT("FLuaDelegateWrapper* %s_Wrapper = (FLuaDelegateWrapper*)lua_touserdata(InL, %d); \n\t%sF%s& %s = *(%sF%s*)%s_Wrapper->DelegateInst;"), *InParamName, InStackIndex, *ScopePrefix, *DelegateName, *InParamName, *ScopePrefix, *DelegateName, *InParamName);
	}
	else if (const UArrayProperty* ArrayProp = Cast<UArrayProperty>(InProp))
	{
		FString ElementTypeName = FastLuaHelper::GetPropertyTypeName(ArrayProp->Inner);

		FString PointerFlag;
		if (FastLuaHelper::IsPointerType(ElementTypeName))
		{
			PointerFlag = FString("*");
		}

		FString NewElement = FString::Printf(TEXT("%s"), *GenerateFetchPropertyStr(ArrayProp->Inner, FString::Printf(TEXT("Temp_NewElement")), -1));

		BodyStr = FString::Printf(TEXT("TArray<%s%s> %s;\n\t{\n\tif(lua_istable(InL, %d))\n\t{\n\tlua_pushnil(InL);\n\twhile(lua_next(InL, -2))\n\t{\n\t%s \n\t%s.Add(Temp_NewElement);\n\tlua_pop(InL, 1); \n\t}\n\t}  \n\t}"), *ElementTypeName, *PointerFlag, *InParamName, InStackIndex, *NewElement, *InParamName);
	}
	else if (const USetProperty * SetProp = Cast<USetProperty>(InProp))
	{
		FString ElementTypeName = FastLuaHelper::GetPropertyTypeName(SetProp->ElementProp);

		FString PointerFlag;
		if (FastLuaHelper::IsPointerType(ElementTypeName))
		{
			PointerFlag = FString("*");
		}

		FString NewElement = FString::Printf(TEXT("%s"), *GenerateFetchPropertyStr(SetProp->ElementProp, FString::Printf(TEXT("Temp_NewElement")), -1));

		BodyStr = FString::Printf(TEXT("TSet<%s%s> %s;\n\t{\n\tif(lua_istable(InL, %d))\n\t{\n\tlua_pushnil(InL);\n\twhile(lua_next(InL, -2))\n\t{\n\t%s \n\t%s.Add(Temp_NewElement);\n\tlua_pop(InL, 1); \n\t}\n\t}  \n\t}"), *ElementTypeName, *PointerFlag, *InParamName, InStackIndex, *NewElement, *InParamName);
	}
	else if (const UMapProperty* MapProp = Cast<UMapProperty>(InProp))
	{
		FString KeyTypeName = FastLuaHelper::GetPropertyTypeName(MapProp->KeyProp);

		FString KeyPointerFlag;
		if (FastLuaHelper::IsPointerType(KeyTypeName))
		{
			KeyPointerFlag = FString("*");
		}

		FString ValueTypeName = FastLuaHelper::GetPropertyTypeName(MapProp->ValueProp);

		FString ValuePointerFlag;
		if (FastLuaHelper::IsPointerType(ValueTypeName))
		{
			ValuePointerFlag = FString("*");
		}

		FString NewKeyElement = FString::Printf(TEXT("%s"), *GenerateFetchPropertyStr(MapProp->KeyProp, FString::Printf(TEXT("Temp_NewKeyElement")), -2));

		FString NewValueElement = FString::Printf(TEXT("%s"), *GenerateFetchPropertyStr(MapProp->ValueProp, FString::Printf(TEXT("Temp_NewValueElement")), -1));

		BodyStr = FString::Printf(TEXT("TMap<%s%s, %s%s> %s;\n\t{\n\tif(lua_istable(InL, %d))\n\t{\n\tlua_pushnil(InL);\n\twhile(lua_next(InL, -2))\n\t{\n\t%s\n\t%s \n\t%s.Add(Temp_NewKeyElement, Temp_NewValueElement);\n\tlua_pop(InL, 1); \n\t}\n\t}  \n\t}"), *KeyTypeName, *KeyPointerFlag, *ValueTypeName, *ValuePointerFlag, *InParamName, InStackIndex, *NewKeyElement, *NewValueElement, *InParamName);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("breakpoint!"));
	}

	return BodyStr;

}

TArray<FString> GenerateLua::CollectHeaderFilesReferencedByClass(const class UStruct* InClass)
{
	TArray<FString> HeaderList;

	for (TFieldIterator<const UFunction> ItFunc(InClass, EFieldIteratorFlags::ExcludeSuper); ItFunc; ++ItFunc)
	{
		if (FastLuaHelper::IsScriptCallableFunction(*ItFunc) == false)
		{
			continue;
		}
		
		for (TFieldIterator<const UProperty> ItProp(*ItFunc); ItProp; ++ItProp)
		{
			const UProperty* Prop = *ItProp;
			if (const UClassProperty* ClassProp = Cast<UClassProperty>(Prop))
			{
				FString RawHeaderPath = ClassProp->MetaClass->GetMetaData(*FString("IncludePath"));
				if (RawHeaderPath.IsEmpty() == false)
				{
					HeaderList.AddUnique(RawHeaderPath);
				}
			}
			else if (const UObjectProperty* ObjectProp = Cast<UObjectProperty>(Prop))
			{
				FString RawHeaderPath = ObjectProp->PropertyClass->GetMetaData(*FString("IncludePath"));
				if (RawHeaderPath.IsEmpty() == false)
				{
					HeaderList.AddUnique(RawHeaderPath);
				}
			}
		}
	}

	for (TFieldIterator<const UProperty> It(InClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		if (FastLuaHelper::IsScriptReadableProperty(*It) == false)
		{
			continue;
		}

		if (const UClassProperty * ClassProp = Cast<UClassProperty>(*It))
		{
			FString RawHeaderPath = ClassProp->MetaClass->GetMetaData(*FString("IncludePath"));
			if (RawHeaderPath.IsEmpty() == false)
			{
				HeaderList.AddUnique(RawHeaderPath);
			}
		}
		else if (const UObjectProperty * ObjectProp = Cast<UObjectProperty>(*It))
		{
			FString RawHeaderPath = ObjectProp->PropertyClass->GetMetaData(*FString("IncludePath"));
			if (RawHeaderPath.IsEmpty() == false)
			{
				HeaderList.AddUnique(RawHeaderPath);
			}
		}

	}

	return HeaderList;
}


