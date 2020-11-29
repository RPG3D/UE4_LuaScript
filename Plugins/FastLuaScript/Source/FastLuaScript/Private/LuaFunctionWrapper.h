// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/UObjectIterator.h"
#include "LuaFunctionWrapper.generated.h"

struct lua_State;

/**
 * a UObject wrapper for UE4 and Lua function
 */
UCLASS()
class ULuaFunctionWrapper : public UObject
{
	GENERATED_BODY()
public:

	static FName GetWrapperFunctionFName() { return FName(TEXT("TestFunction")); }

	bool BindLuaFunction(lua_State* InL, uint32 InStackIndex);

	int32 Unbind();

	bool IsBound() const
	{
		return LuaState && LuaFunctionID > 0;
	}

	const UFunction* GetUFunction()
	{
		return FunctionSignature;
	}

	void HandleLuaUnrealReset(lua_State* InL)
	{
		if (InL == LuaState)
		{
			for (TObjectIterator<ULuaFunctionWrapper>It; It; ++It)
			{
				It->Unbind();
				It->RemoveFromRoot();
				It->MarkPendingKill();
			}
		}
	}


protected:

	virtual void BeginDestroy() override;

	UFUNCTION()
		void TestFunction() {}

	virtual void ProcessEvent(UFunction* InFunction, void* Parms) override;

	//now, now conside One lua state for the plugin
	lua_State* LuaState = nullptr;

	//Lua function index in lua global registry
	int32 LuaFunctionID = 0;
	int32 LuaSelfID = 0;

	friend class FLuaDelegateWrapper;

	//the UFunction bound to this Delegate
	const UFunction* FunctionSignature = nullptr;

	static FDelegateHandle OnLuaResetHandle;

	int32 RefCount = 0;

};
