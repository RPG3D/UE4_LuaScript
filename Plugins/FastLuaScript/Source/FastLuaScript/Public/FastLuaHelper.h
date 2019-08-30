// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct lua_State;

//use magic number to detect if the force convert is valid
enum class ELuaUnrealWrapperType
{
	Object = 2019,
	Struct = 2020,
	Delegate = 2021,
	SoftClass,
	SoftObject,
	Interface
};

struct FLuaObjectWrapper
{
public:
	ELuaUnrealWrapperType WrapperType = ELuaUnrealWrapperType::Object;
	TWeakObjectPtr<class UObject> ObjInst = nullptr;
};


struct FLuaStructWrapper
{
public:
	ELuaUnrealWrapperType WrapperType = ELuaUnrealWrapperType::Struct;
	const UScriptStruct * StructType = nullptr;
	//just a memory address flag
	const void* StructInst = nullptr;
};

struct FLuaDelegateWrapper
{
public:
	ELuaUnrealWrapperType WrapperType = ELuaUnrealWrapperType::Delegate;
	//FMulticastScriptDelegate(Multi) or FScriptDelegate
	void* DelegateInst = nullptr;
	bool bIsMulti = false;
	class UFunction* FunctionSignature = nullptr;
};

struct FLuaSoftClassWrapper
{
public:
	ELuaUnrealWrapperType WrapperType = ELuaUnrealWrapperType::SoftClass;
	TSoftClassPtr<UObject> SoftClassInst = nullptr;
};

struct FLuaSoftObjectWrapper
{
public:
	ELuaUnrealWrapperType WrapperType = ELuaUnrealWrapperType::SoftObject;
	TSoftObjectPtr<UObject> SoftObjInst = nullptr;
};

struct FLuaInterfaceWrapper
{
public:
	ELuaUnrealWrapperType WrapperType = ELuaUnrealWrapperType::Interface;
	class FScriptInterface* InterfaceInst = nullptr;
};

/**
 * 
 */
class FASTLUASCRIPT_API FastLuaHelper
{
public:
	static bool HasScriptAccessibleField(const UStruct* InStruct);
	static bool IsScriptCallableFunction(const UFunction* InFunction);
	static bool IsScriptReadableProperty(const UProperty* InProperty);

	//get actual cpp type str. int32/float/TArray<AActor*>/TMap<int32, uint8>
	static FString GetPropertyTypeName(const UProperty* InProp);

	//start with U or A
	static bool IsPointerType(const FString& InTypeName);

	static UObject* FetchObject(lua_State* InL, int32 InIndex);
	static void PushObject(lua_State* InL, UObject* InObj);

	static void* FetchStruct(lua_State* InL, int32 InIndex);
	static void PushStruct(lua_State* InL, const UScriptStruct* InStruct, const void* InBuff);

	static void PushDelegate(lua_State* InL, void* InDelegateInst, bool InMulti = true);

	static int32 CallFunction(lua_State* L);

	static void* LuaAlloc(void* ud, void* ptr, size_t osize, size_t nsize);

	//0 log  1 warning 2 error
	static void LuaLog(const FString& InLog, int32 InLevel = 0, class FastLuaUnrealWrapper* InLuaWrapper = nullptr);

	static void FixClassMetatable(lua_State* InL, TArray<const UClass*> InRegistedClassList);
	static void FixStructMetatable(lua_State* InL, TArray<const UScriptStruct*> InRegistedStructList);

	static int LuaGetGameInstance(lua_State* L);
	static int LuaLoadObject(lua_State* Inl);
	static int LuaLoadClass(lua_State*);
	static int LuaGetUnrealCDO(lua_State* InL);//get unreal class default object
	static int LuaNewObject(lua_State* InL);
	static int LuaCallUnrealDelegate(lua_State* InL);
	static int PrintLog(lua_State* L);
};
