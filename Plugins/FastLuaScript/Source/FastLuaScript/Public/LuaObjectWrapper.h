// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ILuaWrapper.h"
#include "UObject/WeakObjectPtr.h"


/**
 * 
 */
class FASTLUASCRIPT_API FLuaObjectWrapper : public ILuaWrapper
{
public:
	FLuaObjectWrapper(UObject* InObj)
	{
		ObjectWeakPtr = InObj;
	}

	~FLuaObjectWrapper()
	{
		ObjectWeakPtr = nullptr;
	}

	UObject* GetObject()
	{
		return ObjectWeakPtr.Get();
	}

	static void InitWrapperMetatable(lua_State* InL);

	static char* GetMetatableName()
	{
		return "ObjectWrapper";
	}

	static UObject* FetchObject(lua_State* InL, int32 InIndex, bool IsUClass = false);
	static void PushObject(lua_State* InL, UObject* InObj);


	static int32 IndexInClass(lua_State* InL);
	static int32 NewIndexInClass(lua_State* InL);
	static int32 ObjectToString(lua_State* InL);
	static int LuaGetUnrealCDO(lua_State* InL);//get unreal class default object
	static int LuaNewObject(lua_State* InL);

	static int LuaLoadObject(lua_State* Inl);
	static int LuaLoadClass(lua_State*);

	static int ObjectGC(lua_State* InL);

	const ELuaWrapperType WrapperType = ELuaWrapperType::Object;
protected:

	friend class FastLuaHelper;
	FWeakObjectPtr ObjectWeakPtr = nullptr;
};
