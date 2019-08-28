// Fill out your copyright notice in the Description page of Project Settings.

#include "FastLuaUnrealWrapper.h"
#include "Package.h"
#include "FastLuaScript.h"
#include "lua.hpp"
#include "Engine/GameInstance.h"
#include "UObjectIterator.h"
#include "FileHelper.h"
#include "Paths.h"
#include "PlatformFilemanager.h"
#include "UObject/StructOnScope.h"
#include "FastLuaHelper.h"


int InitUnrealLib(lua_State* L);
int RegisterRawAPI(lua_State* InL);
int RequireFromUFS(lua_State* InL);


FastLuaUnrealWrapper::FastLuaUnrealWrapper()
{
}

FastLuaUnrealWrapper::~FastLuaUnrealWrapper()
{
	Reset();
}

TSharedPtr<FastLuaUnrealWrapper> FastLuaUnrealWrapper::Create(class UGameInstance* InGameInstance, const FString& InName)
{
	TSharedPtr<FastLuaUnrealWrapper> Inst(new FastLuaUnrealWrapper());
	if (InName.Len())
	{
		Inst->LuaStateName = InName;
	}
	Inst->Init(InGameInstance);

	return Inst;
}

void FastLuaUnrealWrapper::Init(UGameInstance* InGameInstance)
{
	if (L == nullptr)
	{
		L = lua_newstate(FastLuaHelper::LuaAlloc, this);
		luaL_openlibs(L);

		lua_pushlightuserdata(L, this);
		lua_rawsetp(L, LUA_REGISTRYINDEX, L);
	}
}

void FastLuaUnrealWrapper::RegisterProperty(lua_State* InL, const UProperty* InProperty, bool InIsStruct)
{
	if (FastLuaHelper::IsScriptReadableProperty(InProperty) == false)
	{
		return;
	}

	{
		const FString PropName = "Get" + InProperty->GetName();

		lua_pushlightuserdata(InL, (void*)InProperty);
		if (InIsStruct)
		{
			lua_pushcclosure(InL, FastLuaHelper::GetStructProperty, 1);
		}
		else
		{
			lua_pushcclosure(InL, FastLuaHelper::GetObjectProperty, 1);
		}

		lua_setfield(InL, -2, TCHAR_TO_UTF8(*PropName));
	}

	{
		const FString PropName = "Set" + InProperty->GetName();

		lua_pushlightuserdata(InL, (void*)InProperty);
		if (InIsStruct)
		{
			lua_pushcclosure(InL, FastLuaHelper::SetStructProperty, 1);
		}
		else
		{
			lua_pushcclosure(InL, FastLuaHelper::SetObjectProperty, 1);
		}


		lua_setfield(InL, -2, TCHAR_TO_UTF8(*PropName));
	}
}

FString FastLuaUnrealWrapper::GenerateFunctionBodyStr(const class UFunction* InFunction, const class UClass* InClass)
{
	if (InClass == nullptr || FastLuaHelper::IsScriptCallableFunction(InFunction) == false)
	{
		return FString("");
	}

	const FString ClassName = InClass->GetName();
	const FString ClassPrefix = InClass->GetPrefixCPP();
	
	const FString FuncName = InFunction->GetName();
	
	//first line: FLuaObjectWrapper* ThisObject_Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	FString BodyStr = FString::Printf(TEXT("\tFLuaObjectWrapper* ThisObject_Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);\t%s%s* ThisObject = Cast<%s%s>(ThisObject_Wrapper->ObjInst.Get());\n"), *ClassPrefix, *ClassName, *ClassPrefix, *ClassName);

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
			FString FetchPropStr = FastLuaHelper::GenerateFetchPropertyStr(Prop, TmpParamName, LuaStackIndex, InClass);
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
		FString PushReturnStr = FString::Printf(TEXT("%s"), *FastLuaHelper::GeneratePushPropertyStr(ReturnProp, FString("ReturnValue")));
		BodyStr += PushReturnStr;
	}

	BodyStr += FString::Printf(TEXT("\n\treturn %d;"), ReturnProp != nullptr);

	return BodyStr;
}

FString FastLuaUnrealWrapper::GenerateGetPropertyStr(const class UProperty* InProperty, const FString& InParamName, const class UStruct* InStruct)
{
	const FString ClassName = InStruct->GetName();
	const FString ClassPrefix = InStruct->GetPrefixCPP();

	FString BodyStr = FastLuaHelper::GenerateFetchPropertyStr(nullptr, FString("ThisObject"), 1, InStruct);

	if (FastLuaHelper::IsPointerType(ClassPrefix))
	{
		BodyStr += FastLuaHelper::GeneratePushPropertyStr(InProperty, FString("ThisObject->") + InParamName);
	}
	else
	{
		BodyStr += FastLuaHelper::GeneratePushPropertyStr(InProperty, FString("ThisObject.") + InParamName);
	}

	BodyStr += FString("\n\treturn 1;");
	return BodyStr;
}

FString FastLuaUnrealWrapper::GenerateSetPropertyStr(const class UProperty* InProperty, const FString& InParamName, const class UStruct* InStruct)
{
	const FString ClassName = InStruct->GetName();
	const FString ClassPrefix = InStruct->GetPrefixCPP();

	FString BodyStr = FastLuaHelper::GenerateFetchPropertyStr(nullptr, FString("ThisObject"), 1, InStruct);
	BodyStr += FastLuaHelper::GenerateFetchPropertyStr(InProperty, FString("InParam_") + InParamName, 2);

	if (FastLuaHelper::IsPointerType(ClassPrefix))
	{
		BodyStr += FString::Printf(TEXT("\n\tThisObject->%s = InParam_%s;"), *InParamName, *InParamName);
	}
	else
	{
		BodyStr += FString::Printf(TEXT("\n\tThisObject.%s = InParam_%s;"), *InParamName, *InParamName);
	}

	BodyStr += FString("\n\treturn 0;");
	return BodyStr;
}

void FastLuaUnrealWrapper::Reset()
{
	if (L)
	{
		lua_close(L);
		L = nullptr;
	}

	LuaMemory = 0;

	if (LuaTickerHandle.IsValid())
	{
		FTicker::GetCoreTicker().RemoveTicker(LuaTickerHandle);
		LuaTickerHandle.Reset();
	}

}

bool FastLuaUnrealWrapper::HandleLuaTick(float InDeltaTime)
{
	static float Count = 0.f;

	if (L)
	{
		int32 tp = lua_gettop(L);
		lua_getglobal(L, "Unreal");
		if (lua_istable(L, -1))
		{
			lua_getfield(L, -1, "Ticker");
			if (lua_isfunction(L, -1))
			{
				//SCOPE_CYCLE_COUNTER(STAT_LuaTick);
				lua_pushnumber(L, InDeltaTime);
				int32 Ret = lua_pcall(L, 1, 0, 0);
				if (Ret)
				{
					Count += InDeltaTime;
					if (Count > 2.f)
					{
						Count = 0.f;
						FastLuaHelper::LuaLog(FString::Printf(TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(L, -1))), 1, this);
					}

				}
				
			}
		}

		lua_settop(L, tp);
	}

	return true;
}

void FastLuaUnrealWrapper::RunMainFunction(const FString& InMainFile /*=FString("ApplicationMain")*/)
{
	//load entry lua
	FString LoadMainScript = FString::Printf(TEXT("require('%s')"), *InMainFile);
	int32 Ret = luaL_dostring(GetLuaSate(), TCHAR_TO_UTF8(*LoadMainScript));
	if (Ret > 0)
	{
		FString RetString = UTF8_TO_TCHAR(lua_tostring(L, -1));
		FastLuaHelper::LuaLog(FString::Printf(TEXT("%s"), *RetString), 1, this);
	}
	//run Main
	Ret = lua_getglobal(L, "Main");
	Ret = lua_pcall(L, 0, 0, 0);
	if (Ret > 0)
	{
		FString RetString = UTF8_TO_TCHAR(lua_tostring(L, -1));
		FastLuaHelper::LuaLog(FString::Printf(TEXT("%s"), *RetString), 1, this);
	}

	if (LuaTickerHandle.IsValid() == false)
	{
		LuaTickerHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FastLuaUnrealWrapper::HandleLuaTick));
	}

}

FString FastLuaUnrealWrapper::DoLuaCode(const FString& InCode)
{
	luaL_dostring(GetLuaSate(), TCHAR_TO_UTF8(*InCode));
	return UTF8_TO_TCHAR(lua_tostring(L, -1));
}

int InitUnrealLib(lua_State* L)
{
	const static luaL_Reg funcs[] =
	{
		{"LuaGetGameInstance", FastLuaHelper::LuaGetGameInstance},
		{"LuaLoadObject", FastLuaHelper::LuaLoadObject},
		{"LuaLoadClass", FastLuaHelper::LuaLoadClass},
		{"LuaGetUnrealCDO", FastLuaHelper::LuaGetUnrealCDO},
		{"LuaNewObject", FastLuaHelper::LuaNewObject},
		{nullptr, nullptr}
	};

	luaL_newlib(L, funcs);

	return 1;
}

int RegisterRawAPI(lua_State* InL)
{
	lua_getglobal(InL, "_G");
	const static luaL_Reg funcs[] =
	{
		{ "print", FastLuaHelper::PrintLog },
		{ nullptr, nullptr }
	};

	luaL_setfuncs(InL, funcs, 0);

	lua_pop(InL, 1);

	return 0;
}


int RequireFromUFS(lua_State* InL)
{
	const char* RawFilwName = lua_tostring(InL, -1);
	FString FileName = UTF8_TO_TCHAR(RawFilwName);

	FileName.ReplaceInline(*FString("."), *FString("/"));

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	lua_getglobal(InL, "package");
	lua_getfield(InL, -1, "path");
	FString LuaSearchPath = UTF8_TO_TCHAR(lua_tostring(InL, -1));
	TArray<FString> PathList;
	LuaSearchPath.ParseIntoArray(PathList, TEXT(";"));

	for (int32 i = 0; i < PathList.Num(); ++i)
	{
		FString FullFilePath = PathList[i].Replace(TEXT("?"), *FileName);
		FullFilePath = FullFilePath.Replace(TEXT("\\"), TEXT("/"));

		FPaths::NormalizeFilename(FullFilePath);

		IFileHandle* LuaFile = PlatformFile.OpenRead(*FullFilePath);
		if (LuaFile)
		{
			TArray<uint8> FileData;
			FileData.Init(0, LuaFile->Size());
			if (FileData.Num() < 1)
			{
				continue;
			}
			LuaFile->Read(FileData.GetData(), FileData.Num());

			int32 BomLen = 0;
			if (FileData.Num() > 2 &&
				FileData[0] == 0xEF &&
				FileData[1] == 0xBB &&
				FileData[2] == 0xBF)
			{
				BomLen = 3;
			}

			FString RetPath = FString("@") + FullFilePath;
			int ret = luaL_loadbuffer(InL, (const char*)FileData.GetData() + BomLen, FileData.Num() - BomLen, TCHAR_TO_UTF8(*RetPath));
			//return full file path as 2nd value, useful for some debug tool 
			lua_pushstring(InL, TCHAR_TO_UTF8(*FullFilePath));
			if (ret != LUA_OK)
			{
				lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
				FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
				lua_pop(InL, 1);
				FastLuaHelper::LuaLog(FString::Printf(TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(InL, -1))), 1, LuaWrapper);
			}

			delete LuaFile;
			return 2;
		}
	}

	FString MessageToPush = FString("RequireFromUFS failed: ") + FileName;
	luaL_traceback(InL, InL, TCHAR_TO_UTF8(*MessageToPush), 1);

	lua_rawgetp(InL, LUA_REGISTRYINDEX, InL);
	FastLuaUnrealWrapper* LuaWrapper = (FastLuaUnrealWrapper*)lua_touserdata(InL, -1);
	lua_pop(InL, 1);
	FastLuaHelper::LuaLog(FString::Printf(TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(InL, -1))), 1, LuaWrapper);
	return 0;
}



int32 FastLuaUnrealWrapper::InitConfig()
{
	if (FFileHelper::LoadFileToStringArray(ModulesShouldExport, *(FPaths::ProjectPluginsDir() / FString("FastLuaScript/Config/ModuleToExport.txt"))))
	{
		UE_LOG(LogTemp, Warning, TEXT("Read ModuleToExport.txt OK!"));
	}

	return 0;
}

int32 FastLuaUnrealWrapper::GeneratedCode() const
{
	for (TObjectIterator<UClass>It; It; ++It)
	{
		if (const UClass * Class = Cast<const UClass>(*It))
		{
			GenerateCodeForClass(Class);
		}
	}

	return 0;
}

int32 FastLuaUnrealWrapper::GenerateCodeForClass(const class UClass* InClass) const
{
	if (InClass == nullptr)
	{
		return -1;
	}

	UPackage* Pkg = InClass->GetOutermost();
	FString PkgName = Pkg->GetName();

	if (InClass->HasAnyClassFlags(EClassFlags::CLASS_RequiredAPI) == false)
	{
		return 0;
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
		return 0;
	}

	FString ClassName = InClass->GetName();

	FString RawHeaderPath = InClass->GetMetaData(*FString("IncludePath"));

	UE_LOG(LogTemp, Log, TEXT("Exporting: %s    %s"), *ClassName, *RawHeaderPath);

	//first, generate header
	{
		int32 ExportedFieldCount = 0;

		FString HeaderFilePath = FString::Printf(TEXT("%s/Lua_%s.h"), *CodeDirectory, *ClassName);

		FString HeaderStr = FString("\n// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved. \n\n#pragma once \n\n#include \"CoreMinimal.h\" \n\n");

		FString ClassBeginStr = FString::Printf(TEXT("struct lua_State; \n\nstruct Lua_%s\n{\n"), *ClassName);
		HeaderStr += ClassBeginStr;


		for (TFieldIterator<const UFunction> It(InClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			if (FastLuaHelper::IsScriptCallableFunction(*It) == false)
			{
				continue;
			}
			++ExportedFieldCount;

			FString FuncName = It->GetName();
			FString FuncDeclareStr = FString::Printf(TEXT("\tstatic int32 Lua_%s(lua_State* InL);\n\n"), *FuncName);
			HeaderStr += FuncDeclareStr;
		}

		for (TFieldIterator<const UProperty> It(InClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			if (FastLuaHelper::IsScriptReadableProperty(*It) == false)
			{
				continue;
			}
			++ExportedFieldCount;

			FString PropName = It->GetName();

			FString PropGetDeclareStr = FString::Printf(TEXT("\tstatic int32 Lua_Get_%s(lua_State* InL);\n"), *PropName);
			HeaderStr += PropGetDeclareStr;

			FString PropSetDeclareStr = FString::Printf(TEXT("\tstatic int32 Lua_Set_%s(lua_State* InL);\n\n"), *PropName);
			HeaderStr += PropSetDeclareStr;
		}

		if (ExportedFieldCount < 1)
		{
			return 0;
		}

		FString ClassEndStr = FString("\n};\n");
		HeaderStr += ClassEndStr;

		FFileHelper::SaveStringToFile(HeaderStr, *HeaderFilePath);

	}

	//second, generate source
	{
		FString SourceFilePath = FString::Printf(TEXT("%s/Lua_%s.cpp"), *CodeDirectory, *ClassName);

		FString SourceStr = FString("\n// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved. \n\n");
		FString ClassBeginStr = FString::Printf(TEXT("#include \"Lua_%s.h\" \n\n#include \"lua/lua.hpp\" \n\n#include \"FastLuaHelper.h\"  \n\n#include \"CoreMinimal.h\" \n\n"), *ClassName);

		if (RawHeaderPath.IsEmpty() == false)
		{
			ClassBeginStr += FString::Printf(TEXT("#include \"%s\"\n\n"), *RawHeaderPath);
		}

		SourceStr += ClassBeginStr;


		for (TFieldIterator<const UFunction> It(InClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			if (FastLuaHelper::IsScriptCallableFunction(*It) == false)
			{
				continue;
			}


			FString FuncName = It->GetName();
			FString FuncBeginStr = FString::Printf(TEXT("int32 Lua_%s::Lua_%s(lua_State* InL)\n{\n"), *ClassName, *FuncName);
			SourceStr += FuncBeginStr;

			FString FuncBodyStr = FastLuaUnrealWrapper::GenerateFunctionBodyStr(*It, InClass);

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

			FString PropGetDefineBeginStr = FString::Printf(TEXT("int32 Lua_%s::Lua_Get_%s(lua_State* InL)\n{\n"), *ClassName, *PropName);
			SourceStr += PropGetDefineBeginStr;

			FString GetBodyStr = FastLuaUnrealWrapper::GenerateGetPropertyStr(*It, PropName, InClass);
			SourceStr += GetBodyStr;

			FString PropGetEndStr = FString("\n\n}\n\n");
			SourceStr += PropGetEndStr;



			FString PropSetDefineBeginStr = FString::Printf(TEXT("int32 Lua_%s::Lua_Set_%s(lua_State* InL)\n{\n"), *ClassName, *PropName);
			SourceStr += PropSetDefineBeginStr;

			FString SetBodyStr = FastLuaUnrealWrapper::GenerateSetPropertyStr(*It, PropName, InClass);
			SourceStr += SetBodyStr;

			FString PropSetEndStr = FString("\n\n}\n\n");
			SourceStr += PropSetEndStr;
		}

		FFileHelper::SaveStringToFile(SourceStr, *SourceFilePath);

	}

	return 0;
}
