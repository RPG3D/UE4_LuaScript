// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ILuaWrapper.h"

class FArrayProperty;
class FScriptArray;
struct lua_State;

/**
 * 
 */
class FASTLUASCRIPT_API FLuaArrayWrapper : ILuaWrapper
{
public:

    FLuaArrayWrapper(const FProperty* InElementProp, FScriptArray* InArrayData);

    ~FLuaArrayWrapper(); 

    static void InitMetatableWrapper(lua_State* InL);

    static int32 GetMetatableIndex()
    {
        return (int32)ELuaWrapperType::Array + 1000;
    }

    int32 GetElementNum() const
    {
        return ArrayData->Num();
    }
    uint8* GetElementAddr(int32 InIndex = 0)
    {
        return (uint8*)ArrayData->GetData() + ElementSize * InIndex;
    }

	FScriptArray* GetScriptArray()
	{
        return ArrayData;
	}

    static int32 PushScriptArray(lua_State* InL, const class FProperty* InElementProp, FScriptArray* InScriptArray);
    static FLuaArrayWrapper* FetchArrayWrapper(lua_State* InL, int32 InIndex);

    static int32 LuaArrayLen(lua_State* InL);
    static int32 LuaArrayIndex(lua_State* InL);
    static int32 LuaArrayNewIndex(lua_State* InL);
    static int32 LuaArrayGC(lua_State* InL);
    static int32 LuaArrayNew(lua_State* InL);


    const ELuaWrapperType WrapperType = ELuaWrapperType::Array;

protected:

    const FProperty* ElementProp = nullptr;
    FScriptArray* ArrayData = nullptr;
    int32 ElementSize = 0;
    bool bRefElementProp = false;
};
