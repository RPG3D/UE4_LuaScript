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
	uint8* StructInst = nullptr;
};

struct FLuaDelegateWrapper
{
public:
	ELuaUnrealWrapperType WrapperType = ELuaUnrealWrapperType::Delegate;
	//FMulticastScriptDelegate(Multi) or FScriptDelegate
	bool bIsMulti = false;
	bool bIsUserDefined = false;
	class UFunction* FunctionSignature = nullptr;
	void* DelegateInst = nullptr;
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

	static void PushProperty(lua_State* InL, UProperty* InProp, void* InBuff, bool bRef = true);
	static void FetchProperty(lua_State* InL, const UProperty* InProp, void* InBuff, int32 InStackIndex = -1, FName ErrorName = TEXT(""));

	static UObject* FetchObject(lua_State* InL, int32 InIndex);
	static void PushObject(lua_State* InL, UObject* InObj);

	static void* FetchStruct(lua_State* InL, int32 InIndex, int32 InDesiredSize);
	static void PushStruct(lua_State* InL, const UScriptStruct* InStruct, const void* InBuff);

	static void PushDelegate(lua_State* InL, class UProperty* InDelegateProperty, void* InBuff, bool InMulti);
	static void* FetchDelegate(lua_State* InL, int32 InIndex, bool InIsMulti = true);

	static int32 CallUnrealFunction(lua_State* L);

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
	static int LuaNewStruct(lua_State* InL);
	static int LuaCallUnrealDelegate(lua_State* InL);
	static int PrintLog(lua_State* L);

	static int LuaNewDelegate(lua_State* InL);
	static int LuaBindDelegate(lua_State* InL);
	static int LuaUnbindDelegate(lua_State* InL);

	static int RegisterTickFunction(lua_State* InL);

	static int UserDelegateGC(lua_State* InL);
	static int StructGC(lua_State* InL);

	static int32 GetObjectProperty(lua_State* L);
	static int32 SetObjectProperty(lua_State* L);

	static int32 GetStructProperty(lua_State* InL);
	static int32 SetStructProperty(lua_State* InL);

	static bool RegisterClassMetatable(lua_State* InL, const UClass* InClass);
	static bool RegisterStructMetatable(lua_State* InL, const UScriptStruct* InStruct);
};
