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
		if (StructInst)
		{
			StructType->DestroyStruct(StructInst);
			FMemory::Free(StructInst);
			StructInst = nullptr;
			StructType = nullptr;
		}
	}

	static void InitWrapperMetatable(lua_State* InL);

	static char* GetMetatableName()
	{
		return "StructWrapper";
	}

	static void* FetchStruct(lua_State* InL, int32 InIndex, int32 InDesiredSize);
	static void PushStruct(lua_State* InL, const UScriptStruct* InStruct, const void* InBuff);

	static int32 IndexInStruct(lua_State* InL);
	static int32 NewIndexInStruct(lua_State* InL);
	static int LuaNewStruct(lua_State* InL);
	static int StructGC(lua_State* InL);

	static int32 StructToString(lua_State* InL);

	const ELuaWrapperType WrapperType = ELuaWrapperType::Struct;

protected:

	friend class FastLuaHelper;

	const UScriptStruct* StructType = nullptr;
	//just a memory address flag
	uint8* StructInst = nullptr;
};
