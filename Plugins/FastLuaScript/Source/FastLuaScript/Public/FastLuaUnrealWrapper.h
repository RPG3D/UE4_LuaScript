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

	FString GetInstanceName() const
	{
		return LuaStateName;
	}

	class UGameInstance* GetGameInstance() const
	{
		return CachedGameInstance;
	}

protected:

	//lua has to hold a long life time Uobject, GameInstance is a right one 
	class UGameInstance* CachedGameInstance = nullptr;


	FastLuaUnrealWrapper();
	FastLuaUnrealWrapper(const FastLuaUnrealWrapper&) = delete;
	FastLuaUnrealWrapper& operator=(const FastLuaUnrealWrapper&) = delete;

	lua_State* L = nullptr;

	FTickerDelegate LuaTickerDelegate;
	FDelegateHandle LuaTickerHandle;

	bool HandleLuaTick(float InDelta);

	int32 ClassMetatableIdx = 0;
	int32 StructMetatableIdx = 0;
	int32 DelegateMetatableIndex = 0;

	//lua UE4's delegate proxy object
	//TArray<class UDelegateCallLua*> DelegateCallLuaList;

	FString LuaStateName = FString("Unknown");

	bool bStatMemory = false;
	size_t LuaMemory = 0;

};
