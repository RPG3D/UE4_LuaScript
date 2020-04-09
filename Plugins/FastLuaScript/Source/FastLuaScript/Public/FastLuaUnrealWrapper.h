// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"

struct lua_State;

/**
 * this is the Entry class for the plugin
 */
class FASTLUASCRIPT_API FastLuaUnrealWrapper
{
public:

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnLuaUnrealReset, lua_State*);

	static FOnLuaUnrealReset OnLuaUnrealReset;

public:

	FString LuaStateName = FString("Unknown");

	bool bStatMemory = true;
	int32 LuaMemory = 0;

	~FastLuaUnrealWrapper();

	//lua state run on server
	static TSharedPtr<FastLuaUnrealWrapper> Create(class UGameInstance* InGameInstance = nullptr, const FString& InName = FString(""));

	//re-live
	void Init(class UGameInstance* InGameInstance = nullptr);
	void Reset();

	lua_State* GetLuaSate() const { return L; }

	FString DoLuaCode(const FString& InCode);

	
	int32 RunMainFunction(const FString& InMainFile = FString("ApplicationMain"));

	FString GetInstanceName() const
	{
		return LuaStateName;
	}

	class UGameInstance* GetGameInstance() const
	{
		return CachedGameInstance;
	}

	void SetGameInstance(class UGameInstance* InGameInstance)
	{
		CachedGameInstance = InGameInstance;
	}

	//never call me from C++!
	//call Unreal.RegisterTickFunction(function() print(1) end) in Lua code!
	void SetLuaTickFunction(int32 InFunctionIndex);

	int32 GetLuaTickFunction() const
	{
		return LuaTickFunctionIndex;
	}

protected:

	//LUA needs to hold a long life time UObject, GameInstance may be a right one 
	class UGameInstance* CachedGameInstance = nullptr;

	lua_State* L = nullptr;

	FTickerDelegate LuaTickerDelegate;
	FDelegateHandle LuaTickerHandle;

	//reference in registry table
	int32 LuaTickFunctionIndex = 0;

	bool bTickError = false;

	FastLuaUnrealWrapper();
	FastLuaUnrealWrapper(const FastLuaUnrealWrapper&) = delete;
	FastLuaUnrealWrapper& operator=(const FastLuaUnrealWrapper&) = delete;

	bool HandleLuaTick(float InDelta);
};
