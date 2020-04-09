// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UObject/Class.h"
#include "LuaFunction.generated.h"

/**
 * 
 */
UCLASS()
class FASTLUASCRIPT_API ULuaFunction : public UFunction
{
	GENERATED_BODY()
public:

    UPROPERTY(VisibleAnywhere)
        int32 RegistryIndex = 0;
};
