// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UObject/Class.h"
#include "LuaGeneratedStruct.generated.h"

/**
 * 
 */
UCLASS()
class FASTLUASCRIPT_API ULuaGeneratedStruct : public UScriptStruct
{
	GENERATED_BODY()
public:

    static int32 GenerateStruct(struct lua_State* InL);

};
