// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ILuaWrapper.h"


/**
 * 
 */
class FASTLUASCRIPT_API FLuaStructWrapper : public ILuaWrapper
{
public:
	FLuaStructWrapper(const class UScriptStruct* InStructType)
	{
		StructType = InStructType;
	}

	~FLuaStructWrapper()
	{
		uint8* ValuePtr = ((uint8*)this) + sizeof(FLuaStructWrapper);
		StructType->DestroyStruct(ValuePtr);
		StructType = nullptr;
	}

	static void* FetchStruct(lua_State* InL, int32 InIndex, const UScriptStruct* InStruct);
	static void PushStruct(lua_State* InL, UScriptStruct* InStruct, const void* InBuff);

	static int32 StructNew(lua_State* InL);
	static int32 StructGC(lua_State* InL);

	static int32 StructToString(lua_State* InL);

	static bool RegisterStruct(lua_State* InL, UScriptStruct* InStruct);

	static int StructIndex(lua_State* InL);
	static int StructNewIndex(lua_State* InL);

	const ELuaWrapperType WrapperType = ELuaWrapperType::Struct;

protected:

	friend class FastLuaHelper;

	const UScriptStruct* StructType = nullptr;
};
