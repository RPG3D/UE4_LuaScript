// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "UObject/WeakObjectPtr.h"

struct lua_State;

/**
 * this is the Entry class for the plugin
 */
class FASTLUASCRIPT_API FastLuaUnrealWrapper
{
public:

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnLuaUnrealReset, lua_State*);

	static FOnLuaUnrealReset OnLuaUnrealReset;

	static TMap<UObject*, FastLuaUnrealWrapper*> LuaStateMap;

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

protected:

	lua_State* L = nullptr;

	int32 InstanceIndex = 0;

	FWeakObjectPtr CachedGameInstance = nullptr;

	FTickerDelegate LuaTickerDelegate;
	FDelegateHandle LuaTickerHandle;

	bool bTickError = false;

	FastLuaUnrealWrapper();
	FastLuaUnrealWrapper(const FastLuaUnrealWrapper&) = delete;
	FastLuaUnrealWrapper& operator=(const FastLuaUnrealWrapper&) = delete;

	bool HandleLuaTick(float InDelta);
};
