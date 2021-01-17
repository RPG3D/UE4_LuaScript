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

	static void PushProperty(lua_State* InL, const FProperty* InProp, void* InContainer);
	static void FetchProperty(lua_State* InL, const FProperty* InProp, void* InContainer, int32 InStackIndex);

	static int32 CallUnrealFunction(lua_State* L);

	static int PrintLog(lua_State* L);

};
