// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ILuaWrapper.h"


/**
 * 
 */
class FASTLUASCRIPT_API FLuaInterfaceWrapper : public ILuaWrapper
{
public:
	FLuaInterfaceWrapper()
	{

	}

	~FLuaInterfaceWrapper()
	{

	}

	static void InitWrapperMetatable(lua_State* InL);
	static int32 GetMetatableIndex()
	{
		return (int32)ELuaWrapperType::Interface + 1000;
	}

	void* InterfaceInst = nullptr;

	const ELuaWrapperType WrapperType = ELuaWrapperType::Interface;

protected:

	friend class FastLuaHelper;

};
