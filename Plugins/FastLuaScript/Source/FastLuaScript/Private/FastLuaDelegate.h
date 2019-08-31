// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FastLuaDelegate.generated.h"

struct lua_State;

/**
 * a UObject wrapper for UE4 and Lua function
 */
UCLASS()
class UFastLuaDelegate : public UObject
{
	GENERATED_BODY()
public:

	//call this after hot update
	bool HotfixLuaFunction();

	//now, now conside One lua state for the plugin
	lua_State* LuaState = nullptr;

	//cache lua function name, used for hot update
	FString LuaFunctionName;

	//Lua function index in lua global registry
	int32 LuaFunctionID = 0;
	int32 LuaSelfID = 0;

	//single delegate or multi delegate
	bool bIsMulti;

	//raw delegate
	void* DelegateInst = nullptr;

	//the UFunction bound to this Delegate
	UFunction* FunctionSignature = nullptr;

	UFUNCTION()
		void TestFunction() {}

		static FName GetWrapperFunctionName() { return FName(TEXT("TestFunction")); }

	virtual void ProcessEvent(UFunction* InFunction, void* Parms) override;
};
