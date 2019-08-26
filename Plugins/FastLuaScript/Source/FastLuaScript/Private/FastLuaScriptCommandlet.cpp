// Fill out your copyright notice in the Description page of Project Settings.


#include "FastLuaScriptCommandlet.h"
#include "FileHelper.h"

int32 UFastLuaScriptCommandlet::Main(const FString& Params)
{
	return GeneratedCode();
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

	FString ClassName = InClass->GetName();
	int32 ExportedFunctionCount = 0;

	FString RawHeaderPath = InClass->GetMetaData(*FString("IncludePath"));

	UE_LOG(LogTemp, Log, TEXT("Exporting: %s    %s"), *ClassName, *RawHeaderPath);


	//first, generate header
	{

		FString HeaderFilePath = FString::Printf(TEXT("%s/Lua_%s.h"), *CodeDirectory, *ClassName);

		FString HeaderStr = FString("\n// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved. \n\n#pragma once \n\n#include \"CoreMinimal.h\"\n\n");
		FString ClassBeginStr = FString::Printf(TEXT("struct lua_State; \n\nstruct Lua_%s\n{\n"), *ClassName);
		HeaderStr += ClassBeginStr;


		for (TFieldIterator<const UFunction> It(InClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
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

		FString SourceStr = FString("\n// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved. \n\n#pragma once \n\n");
		FString ClassBeginStr = FString::Printf(TEXT("#include \"Lua_%s.h\" \n\n#include \"lua/lua.hpp\" \n\n#include \"CoreMinimal.h\" \n\n"), *ClassName);
		SourceStr += ClassBeginStr;


		for (TFieldIterator<const UFunction> It(InClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			FString FuncName = It->GetName();
			FString FuncBeginStr = FString::Printf(TEXT("int32 Lua_%s::Lua_%s(lua_State* InL)\n{\n"), *ClassName, *FuncName);
			SourceStr += FuncBeginStr;

			FString FuncEndStr = FString("\treturn 1;\n}\n\n");
			SourceStr += FuncEndStr;
		}


		FFileHelper::SaveStringToFile(SourceStr, *SourceFilePath);

	}

	return 0;
}
