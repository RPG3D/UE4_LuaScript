// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class GenerateLua
{
public:

	static FString GenerateFunctionBodyStr(const class UFunction* InFunction, const class UClass* InClass);

	static FString GenerateGetPropertyStr(const class UProperty* InProperty, const FString& InParamName, const class UStruct* InStruct);

	static FString GenerateSetPropertyStr(const class UProperty* InProperty, const FString& InParamName, const class UStruct* InStruct);


	int32 InitConfig();

	int32 GeneratedCode() const;

	int32 GenerateCodeForClass(const class UClass* InClass) const;

	int32 GenerateCodeForStruct(const class UScriptStruct* InStruct) const;


	static FString GeneratePushPropertyStr(const UProperty* InProp, const FString& InParamName);
	static FString GenerateFetchPropertyStr(const UProperty* InProp, const FString& InParamName, int32 InStackIndex = -1, const class UStruct* InSruct = nullptr);

	static TArray<FString> CollectHeaderFilesReferencedByClass(const class UStruct* InClass);

protected:
	FString CodeDirectory = FPaths::ProjectPluginsDir() / FString("FastLuaScript/Source/FastLuaScript/Generated");

	TArray<FString> ModulesShouldExport;

	TArray<FString> IgnoredClass;

	TArray<FString> IgnoredFunctions;

};