// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UObject/Class.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "LuaGeneratedClass.generated.h"

struct lua_State;


/**
 * 
 */
UCLASS()
class FASTLUASCRIPT_API ULuaGeneratedClass : public UClass
{
	GENERATED_BODY()
public:

	static int32 GenerateClass(lua_State* InL);

	virtual bool IsFunctionImplementedInScript(FName InFunctionName) const override;

	int32 GetLuaTableIndex() const
	{
		return RegistryIndex;
	}

	lua_State* GetLuaState() const
	{
		return LuaState;
	}

	void HandleLuaUnrealReset(lua_State* InL)
	{
		if (InL == LuaState)
		{
			//avoid CallLuaFunction crash
			LuaState = nullptr;
		}
	}
protected:

	int32 RegistryIndex = 0;
	lua_State* LuaState = nullptr;
};
