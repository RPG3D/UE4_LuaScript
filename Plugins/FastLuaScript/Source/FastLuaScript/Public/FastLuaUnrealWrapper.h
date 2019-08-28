// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ticker.h"

struct lua_State;

/**
 * this is the Entry class for the plugin
 */
class FASTLUASCRIPT_API FastLuaUnrealWrapper
{
public:

	~FastLuaUnrealWrapper();

	//lua state run on server
	static TSharedPtr<FastLuaUnrealWrapper> Create(class UGameInstance* InGameInstance = nullptr, const FString& InName = FString(""));

	//re-live
	void Init(class UGameInstance* InGameInstance = nullptr);
	void Reset();

	lua_State* GetLuaSate() const { return L; }

	FString DoLuaCode(const FString& InCode);

	
	void RunMainFunction(const FString& InMainFile = FString("ApplicationMain"));

	//lua has to hold a long life time Uobject, GameInstance is a right one 
	class UGameInstance* CachedGameInstance = nullptr;


	FastLuaUnrealWrapper();
	FastLuaUnrealWrapper(const FastLuaUnrealWrapper&) = delete;
	FastLuaUnrealWrapper& operator=(const FastLuaUnrealWrapper&) = delete;

	void RegisterProperty(lua_State* InL, const class UProperty* InProperty, bool InIsStruct = false);

	static FString GenerateFunctionBodyStr(const class UFunction* InFunction, const class UClass* InClass);

	static FString GenerateGetPropertyStr(const class UProperty* InProperty, const FString& InParamName, const class UStruct* InStruct);

	static FString GenerateSetPropertyStr(const class UProperty* InProperty, const FString& InParamName, const class UStruct* InStruct);


	lua_State * L = nullptr;

	FTickerDelegate LuaTickerDelegate;
	FDelegateHandle LuaTickerHandle;

	bool HandleLuaTick(float InDelta);


	int32 InitConfig();

	int32 GeneratedCode() const;

	int32 GenerateCodeForClass(const class UClass* InClass) const;

	//int32 GenerateCodeForStruct(const class UScriptStruct* InClass);

	FString CodeDirectory = FPaths::ProjectPluginsDir() / FString("FastLuaScript/Source/FastLuaScript/Generated");

	TArray<FString> ModulesShouldExport;


public:

	int32 ClassMetatableIdx = 0;
	int32 StructMetatableIdx = 0;
	int32 DelegateMetatableIndex = 0;

	//lua UE4's delegate proxy object
	//TArray<class UDelegateCallLua*> DelegateCallLuaList;

	FString LuaStateName = FString("Unknown");

	bool bStatMemory = false;
	size_t LuaMemory = 0;

};
