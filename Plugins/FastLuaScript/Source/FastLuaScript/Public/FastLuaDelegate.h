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

	bool BindLuaFunction(lua_State* InL, uint32 InStackIndex);
	//int32 BindLuaFunctionByName(lua_State* InL, int32 InIndex);

	int32 Unbind();

	void InitFromUFunction(UFunction* InFunction);

	bool IsBound() const
	{
		return LuaState && LuaFunctionID > 0;
	}

	void HandleLuaUnrealReset(lua_State* InL)
	{
		if (InL == LuaState)
		{
			Unbind();
			RemoveFromRoot();
			MarkPendingKill();
		}
	}
protected:

	virtual void BeginDestroy() override;

	friend class FLuaDelegateWrapper;

	//now, now conside One lua state for the plugin
	lua_State* LuaState = nullptr;

	//Lua function index in lua global registry
	int32 LuaFunctionID = 0;
	int32 LuaSelfID = 0;

	//single delegate or multi delegate
	bool bIsMulti;
	//raw delegate
	void* DelegateAddr = nullptr;
	bool bIsUserDefined = false;
	//the UFunction bound to this Delegate
	const UFunction* FunctionSignature = nullptr;

	FDelegateHandle OnLuaResetHandle;

	UFUNCTION()
		void TestFunction() {}

		static FName GetWrapperFunctionName() { return FName(TEXT("TestFunction")); }

	virtual void ProcessEvent(UFunction* InFunction, void* Parms) override;
};
