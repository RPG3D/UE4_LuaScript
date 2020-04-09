// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaGeneratedStruct.h"
#include "FastLuaHelper.h"
#include "lua/lua.hpp"
#include "LuaObjectWrapper.h"

static int32 FillPropertyIntoField_Struct(UField* InOutField, TMap<FString, int32>& InKV)
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

int32 ULuaGeneratedStruct::GenerateStruct(lua_State* InL)
{
	FString InStructName = FString(UTF8_TO_TCHAR(lua_tostring(InL, 1)));

	ULuaGeneratedStruct* NewStruct = FindObject<ULuaGeneratedStruct>(ANY_PACKAGE, *InStructName);
	if (NewStruct)
	{
		NewStruct->RemoveFromRoot();
		NewStruct->MarkPendingKill();
		NewStruct = nullptr;
	}
	NewStruct = NewObject<ULuaGeneratedStruct>(GetTransientPackage(), *InStructName, RF_Public | RF_Standalone | RF_Transient);
	NewStruct->SetMetaData(TEXT("BlueprintType"), TEXT("true"));
	NewStruct->Bind();
	NewStruct->StaticLink(true);
	NewStruct->AddToRoot();



	TMap<FString, int32> TmpKV;
	if (lua_istable(InL, 2))
	{
		lua_getfield(InL, 2, "VariableList");
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

	}

	FillPropertyIntoField_Struct(NewStruct, TmpKV);

	/*if(bIsPOD)
	{
		NewStruct->StructFlags = (EStructFlags)(NewStruct->StructFlags | EStructFlags::STRUCT_IsPlainOldData);
	}*/
	NewStruct->Bind();
	NewStruct->StaticLink(true);

	FLuaObjectWrapper::PushObject(InL, NewStruct);
	return 1;
}
