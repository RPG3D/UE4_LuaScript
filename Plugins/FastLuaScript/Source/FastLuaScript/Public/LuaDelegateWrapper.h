// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ILuaWrapper.h"
#include "FastLuaDelegate.h"

class FDelegateProperty;

/**
 * 
 */
class FASTLUASCRIPT_API FLuaDelegateWrapper : public ILuaWrapper
{
public:
	FLuaDelegateWrapper(class UFunction* InFunction, void* InDelegateAddr, bool InMulti)
	{
		//we only allow one LUA function bound with UE delegate!
		//check whether the delegate already is bound with a proxy!
		if (InDelegateAddr)
		{
			if (InMulti)
			{
				FMulticastScriptDelegate* MultiDelegate = (FMulticastScriptDelegate*)InDelegateAddr;
				TArray<UObject*> ObjList = MultiDelegate->GetAllObjects();
				for (int32 Idx = 0; Idx < ObjList.Num(); ++Idx)
				{
					DelegateObjectProxy = Cast<UFastLuaDelegate>(ObjList[Idx]);
					if (DelegateObjectProxy)
					{
						break;
					}
				}
			}
			else
			{
				FScriptDelegate* SingleDelegate = (FScriptDelegate*)InDelegateAddr;
				DelegateObjectProxy = Cast<UFastLuaDelegate>(SingleDelegate->GetUObject());
			}
		}
		if (!DelegateObjectProxy)
		{
			DelegateObjectProxy = NewObject<UFastLuaDelegate>(GetTransientPackage());
			DelegateObjectProxy->AddToRoot();
			DelegateObjectProxy->FunctionSignature = InFunction;
			DelegateObjectProxy->DelegateAddr = InDelegateAddr;
			DelegateObjectProxy->bIsMulti = InMulti;

			if (!InDelegateAddr && InFunction)
			{
				DelegateObjectProxy->InitFromUFunction(InFunction);
			}
		}

	}

	~FLuaDelegateWrapper()
	{
		//only release if not bound! 
		//make sure DelegateObjectProxy alive after wrapper destroy
		if (!DelegateObjectProxy->IsBound())
		{
			DelegateObjectProxy->Unbind();
			DelegateObjectProxy->RemoveFromRoot();
			DelegateObjectProxy = nullptr;
		}
	}

	static void InitWrapperMetatable(lua_State* InL);

	static int32 GetMetatableIndex()
	{
		return (int32)ELuaWrapperType::Delegate + 1000;
	}

	void* GetDelegateValueAddr()
	{
		return DelegateObjectProxy->DelegateAddr;
	}

	bool IsMulti() const
	{
		return DelegateObjectProxy->bIsMulti;
	}

	static void PushDelegate(lua_State* InL, void* InValueAddr, bool InMulti, UFunction* InFunction);
	static void* FetchDelegate(lua_State* InL, int32 InIndex, bool InIsMulti = true);


	static int LuaNewDelegate(lua_State* InL);
	static int LuaBindDelegate(lua_State* InL);
	static int LuaUnbindDelegate(lua_State* InL);
	static int LuaCallUnrealDelegate(lua_State* InL);

	static int UserDelegateGC(lua_State* InL);

	const ELuaWrapperType WrapperType = ELuaWrapperType::Delegate;

protected:

	friend class FastLuaHelper;

	UFastLuaDelegate* DelegateObjectProxy = nullptr;

};
