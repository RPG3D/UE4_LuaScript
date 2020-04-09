// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class GenerateLua
{
public:

	static FString GenerateFunctionBodyStr(const class UFunction* InFunction, const class UClass* InClass);

	int32 InitConfig();

	int32 GeneratedCode() const;

	bool ShouldGenerateClass(const class UClass* InClass) const;
	bool ShouldGenerateStruct(const class UScriptStruct* InStruct) const;
	bool ShouldGenerateProperty(const class FProperty* InProp) const;
	bool ShouldGenerateFunction(const class UFunction* InFunction) const;

	int32 GenerateCodeForClass(const class UClass* InClass) const;

	static FString GeneratePushPropertyStr(const FProperty* InProp, const FString& InParamName);
	static FString GenerateFetchPropertyStr(const FProperty* InProp, const FString& InParamName, int32 InStackIndex = -1, const class UStruct* InSruct = nullptr);

	TArray<FString> CollectHeaderFilesReferencedByClass(const class UStruct* InClass) const;

protected:
	FString CodeDirectory = FPaths::GameSourceDir() / FApp::GetProjectName() / FString("GeneratedLua");

	TArray<FString> ModulesShouldExport;

	TArray<FString> IgnoredClass;
};