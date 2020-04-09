// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaGeneratedClass.h"
#include "lua/lua.hpp"
#include "FastLuaHelper.h"
#include "LuaFunction.h"
#include "LuaObjectWrapper.h"
#include "FastLuaUnrealWrapper.h"

static int32 FillPropertyIntoField_Class(UField* InOutField, TMap<FString, int32>& InKV)
{
	int32 PropCount = 0;
	int32 PropOffset = 0;
	EObjectFlags ObjectFlags = EObjectFlags::RF_Standalone;
	EPropertyFlags PropertyFlags = EPropertyFlags::CPF_None;
	bool bIsPOD = true;
	for (const TPair<FString, int32>& It : InKV)
	{
		FProperty* NewProp = nullptr;
		UE4CodeGen_Private::EPropertyGenFlags PropertyType = (UE4CodeGen_Private::EPropertyGenFlags)(It.Value);
		switch (PropertyType)
		{
		case UE4CodeGen_Private::EPropertyGenFlags::Int8:
			NewProp = new FInt8Property(InOutField, FName(*It.Key), ObjectFlags, PropOffset, PropertyFlags);
			NewProp->SetPropertyFlags(EPropertyFlags::CPF_IsPlainOldData);
			break;

		case UE4CodeGen_Private::EPropertyGenFlags::Byte:
			NewProp = new FByteProperty(InOutField, FName(*It.Key), ObjectFlags, PropOffset, PropertyFlags, nullptr);
			NewProp->SetPropertyFlags(EPropertyFlags::CPF_IsPlainOldData);
			break;

		case UE4CodeGen_Private::EPropertyGenFlags::Int16:
			NewProp = new FInt16Property(InOutField, FName(*It.Key), ObjectFlags, PropOffset, PropertyFlags);
			NewProp->SetPropertyFlags(EPropertyFlags::CPF_IsPlainOldData);
			break;

		case UE4CodeGen_Private::EPropertyGenFlags::UInt16:
			NewProp = new FUInt16Property(InOutField, FName(*It.Key), ObjectFlags, PropOffset, PropertyFlags);
			NewProp->SetPropertyFlags(EPropertyFlags::CPF_IsPlainOldData);
			break;

		case UE4CodeGen_Private::EPropertyGenFlags::Int:
			NewProp = new FIntProperty(InOutField, FName(*It.Key), ObjectFlags, PropOffset, PropertyFlags);
			NewProp->SetPropertyFlags(EPropertyFlags::CPF_IsPlainOldData);
			break;

		case UE4CodeGen_Private::EPropertyGenFlags::UInt32:
			NewProp = new FUInt32Property(InOutField, FName(*It.Key), ObjectFlags, PropOffset, PropertyFlags);
			NewProp->SetPropertyFlags(EPropertyFlags::CPF_IsPlainOldData);
			break;

		case UE4CodeGen_Private::EPropertyGenFlags::Int64:
			NewProp = new FInt64Property(InOutField, FName(*It.Key), ObjectFlags, PropOffset, PropertyFlags);
			NewProp->SetPropertyFlags(EPropertyFlags::CPF_IsPlainOldData);
			break;

		case UE4CodeGen_Private::EPropertyGenFlags::UInt64:
			NewProp = new FUInt64Property(InOutField, FName(*It.Key), ObjectFlags, PropOffset, PropertyFlags);
			NewProp->SetPropertyFlags(EPropertyFlags::CPF_IsPlainOldData);
			break;

		case UE4CodeGen_Private::EPropertyGenFlags::Float:
			NewProp = new FFloatProperty(InOutField, FName(*It.Key), ObjectFlags, PropOffset, PropertyFlags);
			NewProp->SetPropertyFlags(EPropertyFlags::CPF_IsPlainOldData);
			break;

		case UE4CodeGen_Private::EPropertyGenFlags::Str:
			NewProp = new FStrProperty(InOutField, FName(*It.Key), ObjectFlags, PropOffset, PropertyFlags);
			bIsPOD = false;
			break;

		case UE4CodeGen_Private::EPropertyGenFlags::Array:
			NewProp = new FArrayProperty(InOutField, FName(*It.Key), ObjectFlags, PropOffset, PropertyFlags, EArrayPropertyFlags::None);
			bIsPOD = false;
			break;

		case UE4CodeGen_Private::EPropertyGenFlags::Map:
			NewProp = new FMapProperty(InOutField, FName(*It.Key), ObjectFlags, PropOffset, PropertyFlags, EMapPropertyFlags::None);
			bIsPOD = false;
			break;
		}
		++PropCount;
		PropOffset += NewProp->GetSize();
	}

	return PropCount;
}

static void CallLuaFunction(UObject* Context, FFrame& TheStack, RESULT_DECL)
{
	UClass* TmpClass = Context->GetClass();
	if (ULuaGeneratedClass* LuaClass = Cast<ULuaGeneratedClass>(TmpClass))
	{
		lua_State* LuaState = LuaClass->GetLuaState();
		int32 LuaTableIndex = LuaClass->GetLuaTableIndex();
		FString FuncName = TheStack.Node->GetName();
		lua_rawgeti(LuaState, LUA_REGISTRYINDEX, LuaTableIndex);
		int32 LuaType = lua_type(LuaState, -1);
		lua_getfield(LuaState, -1, TCHAR_TO_UTF8(*FuncName));
		if (lua_isfunction(LuaState, -1))
		{
			int32 ParamsNum = 0;
			FProperty* ReturnParam = nullptr;
			//store param from UE script VM stack
			FStructOnScope FuncTmpMem(TheStack.Node);
			//push self
			FLuaObjectWrapper::PushObject(LuaState, Context);
			++ParamsNum;

			for (TFieldIterator<FProperty> It(TheStack.Node); It; ++It)
			{
				//get function return Param
				FProperty* CurrentParam = *It;
				void* LocalValue = CurrentParam->ContainerPtrToValuePtr<void>(FuncTmpMem.GetStructMemory());
				TheStack.StepCompiledIn<FProperty>(LocalValue);
				if (CurrentParam->HasAnyPropertyFlags(CPF_ReturnParm))
				{
					ReturnParam = CurrentParam;
				}
				else
				{
					//set params for lua function
					FastLuaHelper::PushProperty(LuaState, CurrentParam, FuncTmpMem.GetStructMemory(), 0);
					++ParamsNum;
				}
			}

			//call lua function
			int32 CallRet = lua_pcall(LuaState, ParamsNum, ReturnParam ? 1 : 0, 0);
			if (CallRet)
			{
				UE_LOG(LogTemp, Warning, TEXT("%s"), UTF8_TO_TCHAR(lua_tostring(LuaState, -1)));
			}

			if (ReturnParam)
			{
				//get function return Value, in common
				FastLuaHelper::FetchProperty(LuaState, ReturnParam, FuncTmpMem.GetStructMemory(), -1);
			}
		}
	}
}

int32 ULuaGeneratedClass::GenerateClass(struct lua_State* InL)
{
	FString InClassName = FString(UTF8_TO_TCHAR(lua_tostring(InL, 1)));
	FString SuperClassName = FString(UTF8_TO_TCHAR(lua_tostring(InL, 2)));
	UClass* SuperClass = FindObject<UClass>(ANY_PACKAGE, *SuperClassName);
	if (!SuperClass)
	{
		SuperClass = UObject::StaticClass();
	}

	ULuaGeneratedClass* NewClass = FindObject<ULuaGeneratedClass>(ANY_PACKAGE, *InClassName);
	if (NewClass)
	{
		NewClass->RemoveFromRoot();
		NewClass->MarkPendingKill();
		NewClass = nullptr;
	}
	FastLuaUnrealWrapper::OnLuaUnrealReset.AddUObject(NewClass, &ULuaGeneratedClass::HandleLuaUnrealReset);

	NewClass = NewObject<ULuaGeneratedClass>(GetTransientPackage(), *InClassName, RF_Public | RF_Standalone | RF_Transient);
	NewClass->SetSuperStruct(SuperClass);
	NewClass->ClassFlags |= CLASS_Native;
	NewClass->AddToRoot();

	TMap<FString, int32> TmpKV;
	if (lua_istable(InL, 3))
	{
		{
			lua_pushvalue(InL, 3);
			NewClass->RegistryIndex = luaL_ref(InL, LUA_REGISTRYINDEX);
			NewClass->LuaState = InL;
		}

		lua_pushnil(InL);
		while (lua_next(InL, 3))
		{
			int32 KeyType = lua_type(InL, -2);
			int32 ValueType = lua_type(InL, -1);

			if (KeyType == LUA_TSTRING && ValueType == LUA_TFUNCTION)
			{
				FString TmpFunctionName = UTF8_TO_TCHAR(lua_tostring(InL, -2));
				UFunction* NewFunc = nullptr;
				UFunction* SuperFunction = SuperClass->FindFunctionByName(*TmpFunctionName);
				if (SuperFunction)
				{
					NewFunc = Cast<UFunction>(StaticDuplicateObject(SuperFunction, NewClass, *TmpFunctionName));
					if (NewFunc)
					{
						NewFunc->ParmsSize = SuperFunction->ParmsSize;
						NewFunc->PropertiesSize = SuperFunction->PropertiesSize;
						NewFunc->SetNativeFunc(CallLuaFunction);
						NewFunc->FunctionFlags |= FUNC_Native;
						NewClass->AddFunctionToFunctionMap(NewFunc, *TmpFunctionName);
					}
				}

			}
			lua_pop(InL, 1);
		}

		lua_getfield(InL, 3, "VariableList");
		if (lua_istable(InL, -1))
		{
			lua_pushnil(InL);
			while (lua_next(InL, -2))
			{
				TPair<FString, int32> TmpNew;
				TmpNew.Key = FString(UTF8_TO_TCHAR(lua_tostring(InL, -2)));
				TmpNew.Value = lua_tointeger(InL, -1);
				TmpKV.Add(TmpNew);

				lua_pop(InL, 1);
			}
		}
		lua_pop(InL, 1);
		FillPropertyIntoField_Class(NewClass, TmpKV);	
	}


	NewClass->Bind();
	NewClass->StaticLink(true);
	NewClass->AssembleReferenceTokenStream();
	// Ensure the CDO exists
	NewClass->GetDefaultObject();

	FLuaObjectWrapper::PushObject(InL, NewClass);
	return 1;
}

bool ULuaGeneratedClass::IsFunctionImplementedInScript(FName InFunctionName) const
{
	if (RegistryIndex > 0)
	{
		lua_rawgeti(LuaState, LUA_REGISTRYINDEX, RegistryIndex);
		lua_getfield(LuaState, -1, TCHAR_TO_UTF8(*InFunctionName.ToString()));
		return lua_isfunction(LuaState, -1);
	}
	return Super::IsFunctionImplementedInScript(InFunctionName);
}

