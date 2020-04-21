// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LuaLatentActionWrapper.generated.h"


struct lua_State;
class FastLuaUnrealWrapper;

/**
 * 
 */
UCLASS()
class FASTLUASCRIPT_API ULuaLatentActionWrapper : public UObject
{
	GENERATED_BODY()
public:

    UFUNCTION()
        void TestFunction(int32 InParam);

    static FName GetWrapperFunctionName() { return FName(TEXT("TestFunction")); }

    lua_State* MainThread = nullptr;
    lua_State* WorkerThread = nullptr;
};
