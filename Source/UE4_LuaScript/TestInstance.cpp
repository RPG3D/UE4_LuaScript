// Fill out your copyright notice in the Description page of Project Settings.

#include "TestInstance.h"
#include "Engine/Engine.h"
#include "UObjectGlobals.h"
#include "FastLuaUnrealWrapper.h"


void UTestInstance::Init()
{
	Super::Init();
}

void UTestInstance::Shutdown()
{
	Super::Shutdown();

	if (LuaWrapper.IsValid())
	{
		LuaWrapper->Reset();
		LuaWrapper = nullptr;
	}
}

void UTestInstance::OnStart()
{
	RunGameScript();
}

void UTestInstance::RunGameScript()
{
	if (LuaWrapper.IsValid() == false)
	{
		LuaWrapper = FastLuaUnrealWrapper::GetDefault(this);
	}

	LuaWrapper->RunMainFunction(this);
}

bool UTestInstance::RunLuaCode(const FString& InCode)
{
	FString Ret = LuaWrapper->DoLuaCode(InCode);
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Ret);
	return true;
}