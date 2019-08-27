// Fill out your copyright notice in the Description page of Project Settings.


#include "FastLuaScriptCommandlet.h"
#include "FileHelper.h"
#include "FastLuaHelper.h"
#include "FastLuaUnrealWrapper.h"

int32 UFastLuaScriptCommandlet::Main(const FString& Params)
{
	InitConfig();
	return GeneratedCode();
}

int32 UFastLuaScriptCommandlet::InitConfig()
{
	if (FFileHelper::LoadFileToStringArray(ModulesShouldExport, *(FPaths::ProjectPluginsDir() / FString("FastLuaScript/Config/ModuleToExport.txt"))))
	{
		UE_LOG(LogTemp, Warning, TEXT("Read ModuleToExport.txt OK!"));
	}

	return 0;
}

int32 UFastLuaScriptCommandlet::GeneratedCode() const
{
	for (TObjectIterator<UClass>It; It; ++It)
	{
		if (const UClass* Class = Cast<const UClass>(*It))
		{
			GenerateCodeForClass(Class);
		}
	}

	return 0;
}

int32 UFastLuaScriptCommandlet::GenerateCodeForClass(const class UClass* InClass) const
{
	if (InClass == nullptr)
	{
		return -1;
	}

	UPackage* Pkg = InClass->GetOutermost();
	FString PkgName = Pkg->GetName();

	/*if (PkgName.EndsWith(FString("UnrealEd"), ESearchCase::IgnoreCase) || PkgName.EndsWith(FString("Editor"), ESearchCase::IgnoreCase))
	{
		UE_LOG(LogTemp, Warning, TEXT("Skip non runtime module: %s"), *PkgName);
		return 0;
	}*/

	bool bShouldExportModule = false;

	for (int32 i = 0; i < ModulesShouldExport.Num(); ++i)
	{
		if (PkgName.EndsWith(ModulesShouldExport[i], ESearchCase::IgnoreCase))
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
		int32 ExportedFunctionCount = 0;

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
			++ExportedFunctionCount;

			FString FuncName = It->GetName();
			FString FuncDeclareStr = FString::Printf(TEXT("\tstatic int32 Lua_%s(lua_State* InL);\n\n"), *FuncName);
			HeaderStr += FuncDeclareStr;
		}

		if (ExportedFunctionCount < 1)
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

			FString FuncBodyStr = FastLuaUnrealWrapper::GetFunctionBodyStr(*It, InClass);

			SourceStr += FuncBodyStr;

			FString FuncEndStr = FString("\n\n}\n\n");
			SourceStr += FuncEndStr;
		}


		FFileHelper::SaveStringToFile(SourceStr, *SourceFilePath);

	}

	return 0;
}
