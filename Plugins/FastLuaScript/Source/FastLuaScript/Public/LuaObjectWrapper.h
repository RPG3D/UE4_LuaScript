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
	FLuaObjectWrapper(UObject* InObj);

	virtual ~FLuaObjectWrapper();

	UObject* GetObject()
	{
		return ObjectPtr;
	}

	static UObject* FetchObject(lua_State* InL, int32 InIndex, bool IsUClass = false);
	static void PushObject(lua_State* InL, UObject* InObj);

	static int32 ObjectToString(lua_State* InL);
	static int32 LuaGetUnrealCDO(lua_State* InL);//get unreal class default object

	static int32 LuaLoadObject(lua_State* Inl);

	static int32 ObjectGC(lua_State* InL);

	static bool RegisterClass(lua_State* InL, const UClass* InClass);

	static int ObjectIndex(lua_State* InL);
	static int ObjectNewIndex(lua_State* InL);


	const ELuaWrapperType WrapperType = ELuaWrapperType::Object;
protected:

	friend class FastLuaHelper;
	UObject* ObjectPtr = nullptr;

};
