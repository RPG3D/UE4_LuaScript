// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaGeneratedEnum.h"
#include "lua/lua.hpp"
#include "FastLuaHelper.h"
#include "LuaObjectWrapper.h"



int32 ULuaGeneratedEnum::GenerateEnum(struct lua_State* InL)
{
	FString InEnumName = UTF8_TO_TCHAR(lua_tostring(InL, 1));

	ULuaGeneratedEnum* NewEnum = FindObject<ULuaGeneratedEnum>(ANY_PACKAGE, *InEnumName);
	if (NewEnum)
	{
		NewEnum->RemoveFromRoot();
		NewEnum->MarkPendingKill();
		NewEnum = nullptr;
	}

	NewEnum = NewObject<ULuaGeneratedEnum>(GetTransientPackage(), *InEnumName, RF_Public | RF_Standalone | RF_Transient);
	//NewEnum->SetMetaData(TEXT("BlueprintType"), TEXT("true"));
	NewEnum->AddToRoot();


	TArray<TPair<FName, int64>> TmpKV;
	if (lua_istable(InL, 2))
	{
		lua_getfield(InL, 2, "VariableList");
		if (lua_istable(InL, -1))
		{
			lua_pushnil(InL);
			while (lua_next(InL, -2))
			{
				TPair<FName, int64> TmpNew;
				TmpNew.Key = FName(UTF8_TO_TCHAR(lua_tostring(InL, -2)));
				TmpNew.Value = lua_tointeger(InL, -1);
				TmpKV.Add(TmpNew);

				lua_pop(InL, 1);
			}
		}
		
	}

	NewEnum->SetEnums(TmpKV, UEnum::ECppForm::Namespaced);

	FLuaObjectWrapper::PushObject(InL, NewEnum);
	return 1;
}

FString ULuaGeneratedEnum::GenerateFullEnumName(const TCHAR* InEnumName) const
{
	return Super::GenerateFullEnumName(InEnumName);
}

FText ULuaGeneratedEnum::GetDisplayNameTextByIndex(int32 InIndex) const
{
	FText Ret  = FText::FromString(GetName() + FString("_") + GetNameByIndex(InIndex).ToString());
	return Ret;
}
