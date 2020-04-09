// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"

struct lua_State;



/**
 * 
 */
class FASTLUASCRIPT_API FastLuaHelper
{
public:

	//get actual cpp type str. int32/float/TArray<AActor*>/TMap<int32, uint8>
	static FString GetPropertyTypeName(const FProperty* InProp);

	static void PushProperty(lua_State* InL, const FProperty* InProp, void* InBuff, int32 InArrayElementIndex = 0);
	static void FetchProperty(lua_State* InL, const FProperty* InProp, void* InBuff, int32 InStackIndex, int32 InArrayElementIndex = 0);

	static int32 CallUnrealFunction(lua_State* L);

	static void FixClassMetatable(lua_State* InL, TArray<const UClass*> InRegistedClassList);

	static int LuaGetGameInstance(lua_State* L);
	static int LuaLoadObject(lua_State* Inl);
	static int LuaLoadClass(lua_State*);


	static int PrintLog(lua_State* L);


	static int RegisterTickFunction(lua_State* InL);

};
