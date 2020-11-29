// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "UObject/WeakObjectPtr.h"

struct lua_State;
class UGameInstance;

/**
 * this is the Entry class for the plugin
 */
class FASTLUASCRIPT_API FastLuaUnrealWrapper
{
public:

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnLuaEventNoParam, lua_State*);


	static FOnLuaEventNoParam OnLuaUnrealReset;

	static FOnLuaEventNoParam OnLuaLoadThirdPartyLib;

public:

	bool bStatMemory = true;
	int32 LuaMemory = 0;

	~FastLuaUnrealWrapper();

	//lua state run on server
	static TSharedPtr<FastLuaUnrealWrapper> GetDefault(const UGameInstance* InGameInstance = nullptr);

	//re-live
	void Init();
	void Reset();

	lua_State* GetLuaState() const { return L; }

	FString DoLuaCode(const FString& InCode);
	
	int32 RunMainFunction(class UGameInstance* InGameInstance);

protected:

	lua_State* L = nullptr;

	FTickerDelegate LuaTickerDelegate;
	FDelegateHandle LuaTickerHandle;

	bool bTickError = false;

	const char* ProgramTableName = "Program";

	FastLuaUnrealWrapper();
	FastLuaUnrealWrapper(const FastLuaUnrealWrapper&) = delete;
	FastLuaUnrealWrapper& operator=(const FastLuaUnrealWrapper&) = delete;

	bool HandleLuaTick(float InDelta);
};
