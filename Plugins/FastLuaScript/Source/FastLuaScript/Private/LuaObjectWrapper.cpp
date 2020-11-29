// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaObjectWrapper.h"
#include "Engine/BlueprintCore.h"
#include "Engine/World.h"
#include "FastLuaUnrealWrapper.h"
#include "FastLuaHelper.h"
#include "LuaStructWrapper.h"

#include "lua.hpp"
#include "FastLuaStat.h"


class FLuaObjectRef : public FGCObject
{
public:
	FLuaObjectRef()
	{

	}

	virtual ~FLuaObjectRef()
	{

	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObjects(PendingRefObjects);
	}

	void AddRefObject(UObject* InObj)
	{
		PendingRefObjects.Add(InObj);
	}

	void RemoveRefObject(UObject* InObj)
	{
		PendingRefObjects.Remove(InObj);
	}

	static FLuaObjectRef* GetDefault()
	{
		static FLuaObjectRef* DefaultInst = nullptr;
		if (DefaultInst == nullptr)
		{
			DefaultInst = new FLuaObjectRef();
			FGCObject::GGCObjectReferencer->AddObject(DefaultInst);
		}

		return DefaultInst;
	}
protected:
	TSet<UObject*> PendingRefObjects;
};

FLuaObjectWrapper::FLuaObjectWrapper(UObject* InObj)
{
	FLuaObjectRef::GetDefault()->AddRefObject(InObj);
}

FLuaObjectWrapper::~FLuaObjectWrapper()
{
	FLuaObjectRef::GetDefault()->RemoveRefObject(ObjectPtr);
	ObjectPtr = nullptr;
}

UObject* FLuaObjectWrapper::FetchObject(lua_State* InL, int32 InIndex, bool IsUClass)
{
	UObject* RetObject = nullptr;
	UClass* RetClass = nullptr;
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, InIndex);
	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Object)
	{
		RetObject = Wrapper->ObjectPtr;
	}

	if (IsUClass && RetObject)
	{
		RetClass = Cast<UClass>(RetObject);
		if (!RetClass)
		{
			if (UBlueprintCore* BPC = Cast<UBlueprintCore>(RetObject))
			{
				RetClass = BPC->GeneratedClass;
			}
		}
	}

	return IsUClass ? RetClass : RetObject;
}

void FLuaObjectWrapper::PushObject(lua_State* InL, UObject* InObj)
{
	if (InObj == nullptr)
	{
		lua_pushnil(InL);
		return;
	}
	
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_newuserdata(InL, sizeof(FLuaObjectWrapper));
	new(Wrapper) FLuaObjectWrapper(InObj);

	const UClass* Class = InObj->GetClass();

	if (lua_rawgetp(InL, LUA_REGISTRYINDEX, Class) == LUA_TTABLE)
	{
		lua_setmetatable(InL, -2);
	}
	else
	{
		lua_pop(InL, 1);
		RegisterClass(InL, Class);
		if (lua_rawgetp(InL, LUA_REGISTRYINDEX, Class) == LUA_TTABLE)
		{
			lua_setmetatable(InL, -2);
		}
		else
		{
			lua_pop(InL, 1);
		}
	}
}

int32 FLuaObjectWrapper::ObjectToString(lua_State* InL)
{
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);
	if (Wrapper == nullptr || Wrapper->WrapperType != ELuaWrapperType::Object || !Wrapper->ObjectPtr)
	{
		lua_pushnil(InL);
		return 1;
	}

	FString ObjName = Wrapper->GetObject()->GetName();
	lua_pushstring(InL, TCHAR_TO_UTF8(*ObjName));
	return 1;
}

int32 FLuaObjectWrapper::LuaGetUnrealCDO(lua_State* InL)
{
	int32 tp = lua_gettop(InL);
	if (tp < 1)
	{
		return 0;
	}

	FString ClassName = UTF8_TO_TCHAR(lua_tostring(InL, 1));

	UClass* Class = FindObject<UClass>(ANY_PACKAGE, *ClassName);
	if (Class)
	{
		FLuaObjectWrapper::PushObject(InL, Class->GetDefaultObject());
		return 1;
	}

	return 0;
}


//local obj = Unreal.LuaLoadObject(Owner, ObjectPath)
int32 FLuaObjectWrapper::LuaLoadObject(lua_State* InL)
{
	UObject* Owner = FetchObject(InL, 1, false);
	
	FString ObjectPath = UTF8_TO_TCHAR(lua_tostring(InL, 2));

	UObject* LoadedObj = LoadObject<UObject>(Owner ? Owner : ANY_PACKAGE, *ObjectPath);

	PushObject(InL, LoadedObj);

	return 1;
}

int32 FLuaObjectWrapper::ObjectGC(lua_State* InL)
{
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, -1);
	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Object)
	{
		Wrapper->~FLuaObjectWrapper();
	}

	return 0;
}

bool FLuaObjectWrapper::RegisterClass(lua_State* InL, const UClass* InClass)
{
	bool bResult = false;
	if (InL == nullptr || InClass == nullptr)
	{
		return bResult;
	}

	int32 ValueType = lua_rawgetp(InL, LUA_REGISTRYINDEX, InClass);
	lua_pop(InL, 1);
	if (ValueType == LUA_TTABLE)
	{

		return true;
	}

	int32 tp = lua_gettop(InL);

	UClass* SuperClass = InClass->GetSuperClass();
	if (SuperClass)
	{
		RegisterClass(InL, SuperClass);
	}

	lua_newtable(InL);
	{
		lua_pushvalue(InL, -1);
		lua_rawsetp(InL, LUA_REGISTRYINDEX, InClass);

		lua_pushvalue(InL, -1);
		lua_setfield(InL, -2, "__index");

		lua_pushcfunction(InL, FLuaObjectWrapper::ObjectToString);
		lua_setfield(InL, -2, "__tostring");

		lua_pushcfunction(InL, FLuaObjectWrapper::ObjectGC);
		lua_setfield(InL, -2, "__gc");

		if (lua_rawgetp(InL, LUA_REGISTRYINDEX, SuperClass) == LUA_TTABLE)
		{
			lua_setmetatable(InL, -2);
		}
		else
		{
			lua_pop(InL, 1);
		}
	}
	for (TFieldIterator<UFunction>It(InClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		FString FuncName = It->GetName();
		lua_pushlightuserdata(InL, *It);
		lua_pushcclosure(InL, FastLuaHelper::CallUnrealFunction, 1);

		lua_setfield(InL, -2, TCHAR_TO_UTF8(*FuncName));
	}

	for (TFieldIterator<FProperty>It(InClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		FString PropName = It->GetName();
			
		FString GetPropName = FString("Get") + PropName;
		if (!InClass->FindFunctionByName(FName(*GetPropName)))
		{
			lua_pushlightuserdata(InL, *It);
			lua_pushcclosure(InL, FLuaObjectWrapper::ObjectIndex, 1);
			lua_setfield(InL, -2, TCHAR_TO_UTF8(*GetPropName));
		}

		FString SetPropName = FString("Set") + PropName;
		if (!InClass->FindFunctionByName(FName(*SetPropName)))
		{
			lua_pushlightuserdata(InL, *It);
			lua_pushcclosure(InL, FLuaObjectWrapper::ObjectNewIndex, 1);
			lua_setfield(InL, -2, TCHAR_TO_UTF8(*SetPropName));
		}
	}

	lua_settop(InL, tp);

	return bResult;
}



int FLuaObjectWrapper::ObjectIndex(lua_State* InL)
{
	SCOPE_CYCLE_COUNTER(STAT_PushToLua);
	FProperty* Prop = (FProperty*)lua_touserdata(InL, lua_upvalueindex(1));
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);

	void* ValueAddr = nullptr;
	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Object)
	{
		ValueAddr = Wrapper->GetObject();
	}

	if (ValueAddr)
	{
		FastLuaHelper::PushProperty(InL, Prop, ValueAddr);
	}
	else
	{
		lua_pushnil(InL);
	}

	return 1;
}

int FLuaObjectWrapper::ObjectNewIndex(lua_State* InL)
{
	SCOPE_CYCLE_COUNTER(STAT_FetchFromLua);
	FProperty* Prop = (FProperty*)lua_touserdata(InL, lua_upvalueindex(1));
	FLuaObjectWrapper* Wrapper = (FLuaObjectWrapper*)lua_touserdata(InL, 1);

	void* ValueAddr = nullptr;
	if (Wrapper && Wrapper->WrapperType == ELuaWrapperType::Object)
	{
		ValueAddr = Wrapper->GetObject();
	}

	if (ValueAddr)
	{
		FastLuaHelper::FetchProperty(InL, Prop, ValueAddr, 2);
	}

	return 0;
}