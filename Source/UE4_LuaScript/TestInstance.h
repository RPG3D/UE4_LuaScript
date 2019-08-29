// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TestInstance.generated.h"


class UGameDatabase;


/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class UE4_LUASCRIPT_API UTestInstance : public UGameInstance
{
	GENERATED_BODY()
public:

	virtual void Init() override;

	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable, Exec)
		bool RunLuaCode(const FString& InCode);

protected:

	virtual void OnStart() override;


	void RunGameScript();

	TSharedPtr<class FastLuaUnrealWrapper> LuaWrapper = nullptr;
};
