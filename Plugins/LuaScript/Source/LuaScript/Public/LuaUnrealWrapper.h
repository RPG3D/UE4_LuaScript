// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ticker.h"

struct lua_State;

/**
 * this is the Entry class for the plugin
 */
class LUASCRIPT_API FLuaUnrealWrapper
{
public:

	~FLuaUnrealWrapper();

	//lua state run on server
	static TSharedPtr<FLuaUnrealWrapper> Create(class UGameInstance* InGameInstance = nullptr, const FString& InName = FString(""));

	//re-live
	void Init(class UGameInstance* InGameInstance = nullptr);
	void Reset();

	lua_State* GetLuaSate() const { return L; }

	FString DoLuaCode(const FString& InCode);

	//register a UClass to lua
	bool RegisterClass(lua_State* InL, int32 InModuleIndex, const UClass* InClass);
	//register a FStruct to lu
	bool RegisterStruct(lua_State* InL, int32 InModuleIndex, const UScriptStruct* InScriptStruct);

	bool RegisterDelegate(lua_State* InL, int32 InModuleIndex);

	void RunMainFunction(const FString& InMainFile = FString("ApplicationMain"));

	//lua has to hold a long life time Uobject, GameInstance is a right one 
	class UGameInstance* CachedGameInstance = nullptr;
protected:
	FLuaUnrealWrapper();
	FLuaUnrealWrapper(const FLuaUnrealWrapper&) = delete;
	FLuaUnrealWrapper& operator=(const FLuaUnrealWrapper&) = delete;

	//export all UObject and FStruct to lua state, if InScriptPath is nul, use config path
	void RegisterUnreal(class UGameInstance* InGameInstance);

	void RegisterProperty(lua_State* InL, const class UProperty* InProperty, bool InIsStruct = false);
	void RegisterFunction(lua_State* InL, const class UFunction* InFunction, const class UClass* InClass = nullptr);

	lua_State * L = nullptr;

	FTickerDelegate LuaTickerDelegate;
	FDelegateHandle LuaTickerHandle;

	bool HandleLuaTick(float InDelta);

public:

	int32 ClassMetatableIdx = 0;
	int32 StructMetatableIdx = 0;
	int32 DelegateMetatableIndex = 0;

	//lua UE4's delegate proxy object
	TArray<class UDelegateCallLua*> DelegateCallLuaList;

	FString LuaStateName = FString("Unknown");

	bool bStatMemory = false;
	size_t LuaMemory = 0;

	class UObjectLuaReference* ObjectRef;
};
