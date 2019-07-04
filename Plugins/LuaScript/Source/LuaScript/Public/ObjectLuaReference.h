// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ObjectLuaReference.generated.h"

/**
 * 
 */
UCLASS()
class LUASCRIPT_API UObjectLuaReference : public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite)
		TMap<UObject*, int32> ObjectRefMap;
};
