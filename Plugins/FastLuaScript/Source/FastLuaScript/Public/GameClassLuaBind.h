// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class FASTLUASCRIPT_API GameClassLuaBind
{
public:
	GameClassLuaBind();
	~GameClassLuaBind();

	int32 InitLuaBind(class UObject* InObject);
	int32 ReleaseLuaBind();

	struct lua_State* GetLua();

	void AddFunction(const FString& InFuncName, const void* InFuncAddr)
	{
		FunctionNameToAddress.Add(InFuncName, InFuncAddr);
	}

	int32 GetFunctionIndex(const void* InFunctionAddress) const
	{
		const int32* FuncPtr = FunctionAddressToIndex.Find(InFunctionAddress);
		if (FuncPtr)
		{
			return *FuncPtr;
		}
		else
		{
			return 0;
		}
	}

	int32 GetLuaSelfIndex() const
	{
		return LuaSelf;
	}

	void SetLuaTableName(const FString& InTableName)
	{
		LuaTableName = InTableName;
	}

	void SetLuaWrapper(class FastLuaUnrealWrapper* InLuaWrapper)
	{
		LuaWrapper = InLuaWrapper;
	}

protected:
	TMap<FString, const void*> FunctionNameToAddress;
	TMap<const void*, int32> FunctionAddressToIndex;

	class FastLuaUnrealWrapper* LuaWrapper = nullptr;

	int32 LuaSelf = 0;
	FString LuaTableName;
};
