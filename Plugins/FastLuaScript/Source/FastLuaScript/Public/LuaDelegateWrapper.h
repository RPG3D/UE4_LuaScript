// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ILuaWrapper.h"


class FDelegateProperty;

/**
 * 
 */
class FASTLUASCRIPT_API FLuaDelegateWrapper : public ILuaWrapper
{
public:
	FLuaDelegateWrapper(class UFunction* InFunction, void* InDelegateAddr, bool InMulti);

	~FLuaDelegateWrapper();

	static void InitWrapperMetatable(lua_State* InL);

	static char* GetMetatableName()
	{
		static char DelegateWrapper[] = "DelegateWrapper";
		return DelegateWrapper;
	}

	void* GetDelegateValueAddr();

	bool IsMulti() const;

	static void PushDelegate(lua_State* InL, void* InValueAddr, bool InMulti, UFunction* InFunction);
	static void* FetchDelegate(lua_State* InL, int32 InIndex, bool InIsMulti = true);


	static int32 LuaNewDelegate(lua_State* InL);
	static int32 LuaBindDelegate(lua_State* InL);
	static int32 LuaUnbindDelegate(lua_State* InL);
	static int32 LuaCallUnrealDelegate(lua_State* InL);

	static int32 UserDelegateGC(lua_State* InL);

	const ELuaWrapperType WrapperType = ELuaWrapperType::Delegate;

protected:

	//the UFunction bound to this Delegate
	const UFunction* FunctionSignature = nullptr;

	//single delegate or multi delegate
	bool bIsMulti = true;
	//raw delegate
	void* DelegateAddr = nullptr;
	bool bIsUserCreated = false;

};
