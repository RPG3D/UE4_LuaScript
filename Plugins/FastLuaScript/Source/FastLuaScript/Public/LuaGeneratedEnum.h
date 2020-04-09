// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UObject/Class.h"
#include "LuaGeneratedEnum.generated.h"


struct lua_State;

/**
 * 
 */
UCLASS()
class FASTLUASCRIPT_API ULuaGeneratedEnum : public UEnum
{
	GENERATED_BODY()
public:

    static int32 GenerateEnum(struct lua_State* InL);

public:

    virtual FString GenerateFullEnumName(const TCHAR* InEnumName) const;

    virtual FText GetDisplayNameTextByIndex(int32 InIndex) const;
};
